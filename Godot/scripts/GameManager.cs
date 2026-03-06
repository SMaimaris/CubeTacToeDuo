using Godot;

public partial class GameManager : Node
{
    public enum GameState { Playing, Paused, GameOver }

    public GameState State { get; private set; } = GameState.Playing;
    public int CurrentPlayer { get; private set; } = 1;

    [Export]
    public GameConfig Config { get; set; }

    private int _movesThisTurn = 0;
    private int _maxMovesThisTurn = 1;
    private bool _firstTurnDone = false;
    private float _turnTimer;
    private float TurnDuration => Config?.TurnDuration ?? 15f;
    private int _firstMoveFaceIndex = -1; // enforces second move must be on a different face

    private GameData.WinCondition _winCondition;
    private int[] _faceWinner = new int[6];

    // HUD references
    private Label _turnLabel;
    private ProgressBar _timerBar;
    private Label _p1ScoreLabel;
    private Label _p2ScoreLabel;
    private Panel _p1Panel;
    private Panel _p2Panel;
    private Label _p1NameLabel;
    private Label _p2NameLabel;
    private float _turnLabelNaturalY;
    private bool _panelScalesReady = false;
    private Control _winScreen;
    private Label _winLabel;
    private Control _pauseMenu;
    private Control _optionsMenu;
    private Control _disconnectScreen;

    // Face references
    private CubeFace[] _faces = new CubeFace[6];

    // AI
    private AIController _ai;

    // Online helpers
    private bool IsOnline => GameData.Mode == GameData.GameMode.Online;
    private bool IsHost => NetworkManager.Instance?.IsHost ?? false;
    // True when this peer is the authority for game logic (always true offline, host-only online)
    private bool IsAuthority => !IsOnline || IsHost;

    public override void _Ready()
    {
        Config ??= GD.Load<GameConfig>("res://game_config.tres");
        _winCondition = GameData.SelectedWinCondition;

        // Get HUD nodes
        var hud = GetTree().Root.GetNode("Game/HUD");
        _turnLabel = hud.GetNode<Label>("TurnLabel");
        _timerBar = hud.GetNode<ProgressBar>("TimerBar");
        _p1ScoreLabel = hud.GetNode<Label>("Player1Panel/ScoreLabel");
        _p2ScoreLabel = hud.GetNode<Label>("Player2Panel/ScoreLabel");
        _p1Panel = hud.GetNode<Panel>("Player1Panel");
        _p2Panel = hud.GetNode<Panel>("Player2Panel");
        _p1NameLabel = _p1Panel.GetNode<Label>("NameLabel");
        _p2NameLabel = _p2Panel.GetNode<Label>("NameLabel");
        _p1NameLabel.Text = GameData.Player1Name;
        _p2NameLabel.Text = GameData.Player2Name;
        _winScreen = hud.GetNode<Control>("WinScreen");
        _winLabel = hud.GetNode<Label>("WinScreen/WinVBox/WinLabel");
        _pauseMenu = hud.GetNode<Control>("PauseMenu");
        _optionsMenu = hud.GetNode<Control>("PauseMenu/OptionsMenu");

        _turnTimer = TurnDuration;
        _winScreen.Visible = false;
        _pauseMenu.Visible = false;

        // Player name labels are set after panel refs are obtained below

        // Pause buttons
        hud.GetNode<Button>("PauseMenu/PauseVBox/ResumeButton").Pressed += ResumeGame;
        hud.GetNode<Button>("PauseMenu/PauseVBox/OptionsButton").Pressed += () => _optionsMenu.Visible = true;
        hud.GetNode<Button>("PauseMenu/PauseVBox/MainMenuButton").Pressed += GoToMainMenu;
        hud.GetNode<Button>("WinScreen/WinVBox/MainMenuButton").Pressed += GoToMainMenu;

        // Disconnect overlay
        _disconnectScreen = hud.GetNodeOrNull<Control>("DisconnectScreen");
        if (_disconnectScreen != null)
        {
            _disconnectScreen.Visible = false;
            _disconnectScreen.GetNode<Button>("DisconnectVBox/MainMenuButton").Pressed += GoToMainMenu;
        }

        // Connect cube faces
        var cubeContainer = GetTree().Root.GetNode<Node3D>("Game/CubeContainer");
        string[] faceNames = { "FaceFront", "FaceBack", "FaceRight", "FaceLeft", "FaceTop", "FaceBottom" };
        for (int i = 0; i < 6; i++)
        {
            _faces[i] = cubeContainer.GetNode<CubeFace>(faceNames[i]);
            _faces[i].FaceIndex = i;
            int captured = i;
            _faces[i].CellClaimedOnFace += OnCellClaimedOnFace;
        }

        if (GameData.Mode == GameData.GameMode.VsAI)
            _ai = new AIController(GameData.Difficulty);

        // Online: listen for disconnection
        if (IsOnline)
        {
            NetworkManager.Instance.PlayerDisconnected += OnPeerDisconnected;
            NetworkManager.Instance.ServerDisconnected += OnServerLost;
        }

        UpdateHUD();
        _p1Panel.Modulate = new Color(1.2f, 1.1f, 0.7f, 1f);
        _p2Panel.Modulate = new Color(0.5f, 0.5f, 0.5f, 1f);
        CallDeferred(nameof(SetInitialPanelScales));
        AudioManager.Instance?.PlayMusic(Config?.MusicBattle, loop: true);
    }

    public override void _ExitTree()
    {
        if (IsOnline && NetworkManager.Instance != null)
        {
            NetworkManager.Instance.PlayerDisconnected -= OnPeerDisconnected;
            NetworkManager.Instance.ServerDisconnected -= OnServerLost;
        }
    }

    // Timer sync: host sends timer value at this interval
    private const float TimerSyncInterval = 0.5f;
    private float _timerSyncAccum = 0f;

    public override void _Process(double delta)
    {
        if (State != GameState.Playing)
            return;

        if (IsOnline && !IsAuthority)
        {
            // Client: just update visuals from the last synced _turnTimer
            UpdateTimerVisuals();
            return;
        }

        _turnTimer -= (float)delta;
        UpdateTimerVisuals();

        // Host: periodically broadcast timer to client
        if (IsOnline)
        {
            _timerSyncAccum += (float)delta;
            if (_timerSyncAccum >= TimerSyncInterval)
            {
                _timerSyncAccum = 0f;
                Rpc(nameof(NetSyncTimer), _turnTimer);
            }
        }

        if (_turnTimer <= 0f)
        {
            if (IsOnline)
                Rpc(nameof(NetEndTurn));
            else
                EndTurn();
        }
    }

    private void UpdateTimerVisuals()
    {
        _timerBar.Value = (_turnTimer / TurnDuration) * 100.0;
        if (_turnTimer <= 3f)
            _timerBar.Modulate = new Color(0.8f, 0.2f, 0.1f, 1f);
        else
            _timerBar.Modulate = Colors.White;
    }

    public override void _UnhandledInput(InputEvent @event)
    {
        if (@event.IsActionPressed("open_menu"))
        {
            if (State == GameState.Playing)
                PauseGame();
            else if (State == GameState.Paused)
                ResumeGame();
        }
    }

    public bool CanClaimOnFace(int faceIndex)
    {
        return _movesThisTurn == 0 || faceIndex != _firstMoveFaceIndex;
    }

    // Client requests a move, server validates & broadcasts

    [Rpc(MultiplayerApi.RpcMode.AnyPeer, CallLocal = true,
         TransferMode = MultiplayerPeer.TransferModeEnum.Reliable)]
    public void ServerRequestMove(int faceIndex, int cellIndex)
    {
        // only host processes
        if (!IsAuthority) return;

        int senderPeer = Multiplayer.GetRemoteSenderId();
        // If CallLocal, sender is 0 for the local call on the host
        if (senderPeer == 0) senderPeer = 1;
        int senderPlayer = NetworkManager.Instance.PeerToPlayer(senderPeer);

        // Validate it's the sender's turn
        if (senderPlayer != CurrentPlayer) return;
        if (State != GameState.Playing) return;
        if (!CanClaimOnFace(faceIndex)) return;
        if (_faces[faceIndex].GetCell(cellIndex) != 0) return;
        if (_faces[faceIndex].Winner != 0) return;

        // If valid broadcast to all
        Rpc(nameof(NetApplyMove), faceIndex, cellIndex, senderPlayer);
    }

    [Rpc(MultiplayerApi.RpcMode.Authority, CallLocal = true,
         TransferMode = MultiplayerPeer.TransferModeEnum.Reliable)]
    public void NetApplyMove(int faceIndex, int cellIndex, int player)
    {
        _faces[faceIndex].ClaimCell(cellIndex, player);
        OnCellClaimedOnFace(faceIndex, cellIndex, player);
    }

    [Rpc(MultiplayerApi.RpcMode.Authority, CallLocal = true,
         TransferMode = MultiplayerPeer.TransferModeEnum.Reliable)]
    public void NetEndTurn()
    {
        EndTurn();
    }

    [Rpc(MultiplayerApi.RpcMode.Authority, CallLocal = true,
         TransferMode = MultiplayerPeer.TransferModeEnum.Reliable)]
    public void NetSyncTimer(float timerValue)
    {
        _turnTimer = timerValue;
        _timerBar.Value = (_turnTimer / TurnDuration) * 100.0;
    }

    private void OnCellClaimedOnFace(int faceIndex, int cellIndex, int player)
    {
        if (State != GameState.Playing || player != CurrentPlayer)
            return;

        AudioManager.Instance?.PlaySfx(player == 1 ? "place_sword" : "place_shield");

        if (_movesThisTurn == 0)
            _firstMoveFaceIndex = faceIndex;

        _movesThisTurn++;
        _faceWinner[faceIndex] = _faces[faceIndex].Winner;

        if (CheckGameWin(out int winner))
        {
            TriggerGameWon(winner);
            return;
        }

        if (CheckDraw())
        {
            TriggerDraw();
            return;
        }

        UpdateScoreLabels();

        if (_movesThisTurn >= _maxMovesThisTurn)
        {
            if (IsOnline && IsAuthority)
                Rpc(nameof(NetEndTurn));
            else if (!IsOnline)
                EndTurn();
            // Client waits for host's NetEndTurn
        }
        else
        {
            _faces[faceIndex].SetLocked(true);
            UpdateHUD();
        }
    }

    private void EndTurn()
    {
        if (!_firstTurnDone)
            _firstTurnDone = true;

        CurrentPlayer = CurrentPlayer == 1 ? 2 : 1;
        _movesThisTurn = 0;
        _firstMoveFaceIndex = -1;
        // both players get 2 after opening turn
        _maxMovesThisTurn = 2;
        _turnTimer = TurnDuration;

        foreach (var face in _faces)
            face.SetLocked(false);

        if (CheckDraw())
        {
            TriggerDraw();
            return;
        }

        UpdateHUD();
        AnimateActivePanels(CurrentPlayer);
        AudioManager.Instance?.PlaySfx("turn");

        if (GameData.Mode == GameData.GameMode.VsAI && CurrentPlayer == 2 && State == GameState.Playing)
            TriggerAITurn();
    }

    // Called when it becomes the AI's turn, handles up to 2 moves with thinking delays
    private async void TriggerAITurn()
    {
        var cubeController = GetTree().Root.GetNodeOrNull<CubeController>("Game/CubeContainer");

        while (State == GameState.Playing && CurrentPlayer == 2)
        {
            // Brief pause before rotating so the turn change doesn't feel instant
            await ToSignal(GetTree().CreateTimer(0.35f), SceneTreeTimer.SignalName.Timeout);
            if (State != GameState.Playing || CurrentPlayer != 2) break;

            // Run search off the main thread so animations stay smooth
            var (fi, ci) = await _ai.PickMoveAsync(_faces, _firstMoveFaceIndex, _winCondition);
            if (fi == -1) break; // no valid moves available
            cubeController?.SnapToFace(fi);

            // Let the snap animation play and give the player a moment to see it
            float postSnapDelay = (float)GD.RandRange(0.5, 0.9);
            await ToSignal(GetTree().CreateTimer(postSnapDelay), SceneTreeTimer.SignalName.Timeout);
            if (State != GameState.Playing || CurrentPlayer != 2) break;

            ApplyMove(fi, ci);
            // If EndTurn() ran inside ApplyMove, CurrentPlayer switched to 1 then loop exits
        }
    }

    // Applies a move directly, used by AI to bypass the CubeFace input signal.
    public void ApplyMove(int faceIndex, int cellIndex)
    {
        _faces[faceIndex].ClaimCell(cellIndex, CurrentPlayer);
        OnCellClaimedOnFace(faceIndex, cellIndex, CurrentPlayer);
    }

    private bool CheckGameWin(out int winner)
    {
        winner = 0;

        if (_winCondition == GameData.WinCondition.AnySide)
        {
            foreach (var face in _faces)
            {
                if (face.Winner != 0)
                {
                    winner = face.Winner;
                    return true;
                }
            }
        }
        else // Majority
        {
            int p1 = 0, p2 = 0;
            foreach (var face in _faces)
            {
                if (face.Winner == 1) p1++;
                else if (face.Winner == 2) p2++;
            }
            if (p1 >= 4) { winner = 1; return true; }
            if (p2 >= 4) { winner = 2; return true; }
        }
        return false;
    }

    private bool CheckDraw()
    {
        // Check if any valid moves remain on any face
        bool anyMovesLeft = false;
        for (int fi = 0; fi < 6; fi++)
        {
            if (_faces[fi].Winner != 0) continue;
            for (int ci = 0; ci < 9; ci++)
            {
                if (_faces[fi].GetCell(ci) == 0)
                {
                    anyMovesLeft = true;
                    break;
                }
            }
            if (anyMovesLeft) break;
        }

        if (!anyMovesLeft)
            return true;

        // Draw if neither player can possibly reach 4 face wins
        if (_winCondition == GameData.WinCondition.Majority)
        {
            int p1Won = 0, p2Won = 0, undecided = 0;
            for (int fi = 0; fi < 6; fi++)
            {
                if (_faces[fi].Winner == 1) p1Won++;
                else if (_faces[fi].Winner == 2) p2Won++;
                else undecided++;
            }
            if (p1Won + undecided < 4 && p2Won + undecided < 4)
                return true;
        }

        return false;
    }

    private void TriggerGameWon(int winner)
    {
        State = GameState.GameOver;
        foreach (var face in _faces)
            face.SetLocked(false);
        string name = winner == 1 ? GameData.Player1Name : GameData.Player2Name;
        _winLabel.Text = $"{name}\nVICTORIOUS!";
        _winScreen.Visible = true;
        AudioManager.Instance?.PlaySfx("win");
        AudioManager.Instance?.PlayMusic(Config?.MusicVictory, loop: false);
    }

    private void TriggerDraw()
    {
        State = GameState.GameOver;
        foreach (var face in _faces)
            face.SetLocked(false);
        _winLabel.Text = "DRAW";
        _winScreen.Visible = true;
        AudioManager.Instance?.PlayMusic(Config?.MusicVictory, loop: false);
    }

    private void PauseGame()
    {
        State = GameState.Paused;
        _pauseMenu.Visible = true;
    }

    private void ResumeGame()
    {
        State = GameState.Playing;
        _pauseMenu.Visible = false;
        if (_optionsMenu != null) _optionsMenu.Visible = false;
    }

    private void GoToMainMenu()
    {
        if (IsOnline)
            NetworkManager.Instance?.Disconnect();
        GetTree().ChangeSceneToFile("res://scenes/MainMenu.tscn");
    }

    private void UpdateHUD()
    {
        string playerName = CurrentPlayer == 1 ? GameData.Player1Name : GameData.Player2Name;
        int movesLeft = _maxMovesThisTurn - _movesThisTurn;
        _turnLabel.Text = $"{playerName}  -  {movesLeft} move{(movesLeft != 1 ? "s" : "")} left";
        _timerBar.Value = (_turnTimer / TurnDuration) * 100.0;

        UpdateScoreLabels();
    }

    private void UpdateScoreLabels()
    {
        int p1Faces = 0, p2Faces = 0;
        foreach (var face in _faces)
        {
            if (face.Winner == 1) p1Faces++;
            else if (face.Winner == 2) p2Faces++;
        }
        _p1ScoreLabel.Text = $"Sides: {p1Faces}";
        _p2ScoreLabel.Text = $"Sides: {p2Faces}";
    }

    // Disconnection handling

    private void OnPeerDisconnected(long id)
    {
        ShowDisconnectOverlay("Opponent disconnected.");
    }

    private void OnServerLost()
    {
        ShowDisconnectOverlay("Host disconnected.");
    }

    private void ShowDisconnectOverlay(string message)
    {
        State = GameState.GameOver;
        if (_disconnectScreen == null) return;
        _disconnectScreen.GetNode<Label>("DisconnectVBox/DisconnectLabel").Text = message;
        _disconnectScreen.Visible = true;
    }

    private void SetInitialPanelScales()
    {
        _p1Panel.PivotOffset = new Vector2(0f, _p1Panel.Size.Y / 2f);
        _p2Panel.PivotOffset = new Vector2(_p2Panel.Size.X, _p2Panel.Size.Y / 2f);

        _turnLabelNaturalY = _turnLabel.Position.Y;

        _p2Panel.Scale = new Vector2(0.72f, 0.72f);

        _panelScalesReady = true;
    }

    private void AnimateActivePanels(int player)
    {
        if (!_panelScalesReady) return;

        var activePanel   = player == 1 ? _p1Panel     : _p2Panel;
        var inactivePanel = player == 1 ? _p2Panel     : _p1Panel;
        var activeName    = player == 1 ? _p1NameLabel : _p2NameLabel;

        activePanel.Scale = new Vector2(0.82f, 0.82f);
        var scaleTween = CreateTween().SetParallel();
        scaleTween.TweenProperty(activePanel,   "scale", Vector2.One,                 0.28f)
            .SetEase(Tween.EaseType.Out).SetTrans(Tween.TransitionType.Back);
        scaleTween.TweenProperty(inactivePanel, "scale", new Vector2(0.72f, 0.72f),   0.22f)
            .SetEase(Tween.EaseType.Out).SetTrans(Tween.TransitionType.Cubic);
        scaleTween.TweenProperty(activePanel,   "modulate", new Color(1.2f, 1.1f, 0.7f, 1f), 0.2f)
            .SetEase(Tween.EaseType.Out).SetTrans(Tween.TransitionType.Cubic);
        scaleTween.TweenProperty(inactivePanel, "modulate", new Color(0.5f, 0.5f, 0.5f, 1f), 0.2f)
            .SetEase(Tween.EaseType.Out).SetTrans(Tween.TransitionType.Cubic);

        activeName.PivotOffset = activeName.Size / 2f;
        var punchTween = CreateTween();
        punchTween.TweenProperty(activeName, "scale", new Vector2(1.3f, 1.3f), 0.09f)
            .SetEase(Tween.EaseType.Out).SetTrans(Tween.TransitionType.Back);
        punchTween.TweenProperty(activeName, "scale", Vector2.One, 0.22f)
            .SetEase(Tween.EaseType.Out).SetTrans(Tween.TransitionType.Elastic);

        _turnLabel.Modulate = new Color(1f, 1f, 1f, 0f);
        _turnLabel.Position = new Vector2(_turnLabel.Position.X, _turnLabelNaturalY - 20f);
        var labelTween = CreateTween().SetParallel();
        labelTween.TweenProperty(_turnLabel, "modulate:a", 1f, 0.28f)
            .SetEase(Tween.EaseType.Out).SetTrans(Tween.TransitionType.Cubic);
        labelTween.TweenProperty(_turnLabel, "position:y", _turnLabelNaturalY, 0.28f)
            .SetEase(Tween.EaseType.Out).SetTrans(Tween.TransitionType.Back);
    }
}

using Godot;

public partial class GameSetup : Control
{
    private OptionButton _winConditionOption;
    private OptionButton _gameModeOption;
    private OptionButton _difficultyOption;
    private Label _diffLabel;
    private Label _p2Label;
    private LineEdit _player1Name;
    private LineEdit _player2Name;
    private Button _startButton;
    private Button _backButton;

    public override void _Ready()
    {
        _winConditionOption = GetNode<OptionButton>("%WinConditionOption");
        _gameModeOption = GetNode<OptionButton>("%GameModeOption");
        _difficultyOption = GetNode<OptionButton>("%DifficultyOption");
        _diffLabel = GetNode<Label>("CenterContainer/PanelContent/VBoxContainer/DiffLabel");
        _p2Label = GetNode<Label>("CenterContainer/PanelContent/VBoxContainer/P2Label");
        _player1Name = GetNode<LineEdit>("%Player1Name");
        _player2Name = GetNode<LineEdit>("%Player2Name");
        _startButton = GetNode<Button>("%StartButton");
        _backButton = GetNode<Button>("%BackButton");

        _player1Name.Text = GameData.Player1Name;
        _player2Name.Text = GameData.Player2Name;
        _winConditionOption.Selected = (int)GameData.SelectedWinCondition;
        _gameModeOption.Selected = (int)GameData.Mode;
        _difficultyOption.Selected = (int)GameData.Difficulty;

        RefreshModeUI();

        _gameModeOption.ItemSelected += _ => RefreshModeUI();
        _startButton.Pressed += OnStartPressed;
        _backButton.Pressed += OnBackPressed;
    }

    private void RefreshModeUI()
    {
        bool vsAI = _gameModeOption.Selected == (int)GameData.GameMode.VsAI;
        _diffLabel.Visible = vsAI;
        _difficultyOption.Visible = vsAI;
        _p2Label.Visible = !vsAI;
        _player2Name.Visible = !vsAI;
    }

    private void OnStartPressed()
    {
        AudioManager.Instance?.PlaySfx("click");

        GameData.SelectedWinCondition = (GameData.WinCondition)_winConditionOption.Selected;
        GameData.Mode = (GameData.GameMode)_gameModeOption.Selected;
        GameData.Difficulty = (GameData.AIDifficulty)_difficultyOption.Selected;

        string p1 = _player1Name.Text.Trim();
        GameData.Player1Name = p1.Length > 0 ? p1 : "Player 1";

        if (GameData.Mode == GameData.GameMode.VsAI)
        {
            GameData.Player2Name = $"AI ({GameData.Difficulty})";
        }
        else
        {
            string p2 = _player2Name.Text.Trim();
            GameData.Player2Name = p2.Length > 0 ? p2 : "Player 2";
        }

        GetTree().ChangeSceneToFile("res://scenes/Game.tscn");
    }

    private void OnBackPressed()
    {
        GetTree().ChangeSceneToFile("res://scenes/MainMenu.tscn");
    }
}

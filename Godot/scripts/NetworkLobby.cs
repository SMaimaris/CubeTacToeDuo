using Godot;

public partial class NetworkLobby : Control
{
    // Host section
    private LineEdit _hostPortEdit;
    private LineEdit _hostNameEdit;
    private Button _hostButton;
    private Label _hostStatusLabel;
    private Button _startGameButton;

    // Join section
    private LineEdit _joinIpEdit;
    private LineEdit _joinPortEdit;
    private LineEdit _joinNameEdit;
    private Button _joinButton;
    private Label _joinStatusLabel;

    // Common
    private Button _backButton;
    private OptionButton _winConditionOption;

    private bool _peerConnected = false;

    public override void _Ready()
    {
        // Host controls
        _hostNameEdit = GetNode<LineEdit>("%HostNameEdit");
        _hostPortEdit = GetNode<LineEdit>("%HostPortEdit");
        _hostButton = GetNode<Button>("%HostButton");
        _hostStatusLabel = GetNode<Label>("%HostStatusLabel");
        _startGameButton = GetNode<Button>("%StartGameButton");

        // Join controls
        _joinIpEdit = GetNode<LineEdit>("%JoinIpEdit");
        _joinPortEdit = GetNode<LineEdit>("%JoinPortEdit");
        _joinNameEdit = GetNode<LineEdit>("%JoinNameEdit");
        _joinButton = GetNode<Button>("%JoinButton");
        _joinStatusLabel = GetNode<Label>("%JoinStatusLabel");

        // Common
        _backButton = GetNode<Button>("%BackButton");
        _winConditionOption = GetNode<OptionButton>("%WinConditionOption");

        // Defaults
        _hostNameEdit.Text = "Player 1";
        _hostPortEdit.Text = NetworkManager.DefaultPort.ToString();
        _joinIpEdit.Text = "127.0.0.1";
        _joinPortEdit.Text = NetworkManager.DefaultPort.ToString();
        _joinNameEdit.Text = "Player 2";
        _winConditionOption.Selected = (int)GameData.SelectedWinCondition;
        _startGameButton.Disabled = true;

        // Signals
        _hostButton.Pressed += OnHostPressed;
        _joinButton.Pressed += OnJoinPressed;
        _startGameButton.Pressed += OnStartGamePressed;
        _backButton.Pressed += OnBackPressed;

        // Network events
        var net = NetworkManager.Instance;
        net.PlayerConnected += OnPlayerConnected;
        net.PlayerDisconnected += OnPlayerDisconnected;
        net.ConnectionSucceeded += OnConnectionSucceeded;
        net.ConnectionFailed += OnConnectionFailed;
        net.ServerDisconnected += OnServerDisconnected;
    }

    public override void _ExitTree()
    {
        var net = NetworkManager.Instance;
        if (net == null) return;
        net.PlayerConnected -= OnPlayerConnected;
        net.PlayerDisconnected -= OnPlayerDisconnected;
        net.ConnectionSucceeded -= OnConnectionSucceeded;
        net.ConnectionFailed -= OnConnectionFailed;
        net.ServerDisconnected -= OnServerDisconnected;
    }

    // Button handlers

    private void OnHostPressed()
    {
        int port = _hostPortEdit.Text.IsValidInt()
            ? _hostPortEdit.Text.ToInt()
            : NetworkManager.DefaultPort;

        var err = NetworkManager.Instance.HostGame(port);
        if (err != Error.Ok)
        {
            _hostStatusLabel.Text = $"Failed to host: {err}";
            return;
        }

        _hostStatusLabel.Text = $"Hosting on port {port}… waiting for opponent";
        _hostButton.Disabled = true;
        _joinButton.Disabled = true;
    }

    private void OnJoinPressed()
    {
        string ip = _joinIpEdit.Text.Trim();
        int port = _joinPortEdit.Text.IsValidInt()
            ? _joinPortEdit.Text.ToInt()
            : NetworkManager.DefaultPort;

        if (ip.Length == 0) ip = "127.0.0.1";

        var err = NetworkManager.Instance.JoinGame(ip, port);
        if (err != Error.Ok)
        {
            _joinStatusLabel.Text = $"Failed to connect: {err}";
            return;
        }

        _joinStatusLabel.Text = $"Connecting to {ip}:{port}…";
        _joinButton.Disabled = true;
        _hostButton.Disabled = true;
    }

    private void OnStartGamePressed()
    {
        if (!_peerConnected) return;

        string hostName = _hostNameEdit.Text.Trim();
        if (hostName.Length == 0) hostName = "Player 1";

        string clientName = NetworkManager.Instance.RemotePlayerName;
        if (clientName.Length == 0) clientName = "Player 2";

        int winCond = _winConditionOption.Selected;

        // Broadcast to all peers (including self via CallLocal)
        NetworkManager.Instance.Rpc(
            nameof(NetworkManager.StartOnlineGame),
            hostName, clientName, winCond);
    }

    private void OnBackPressed()
    {
        NetworkManager.Instance.Disconnect();
        GetTree().ChangeSceneToFile("res://scenes/MainMenu.tscn");
    }

    // Network callbacks

    private void OnPlayerConnected(long id)
    {
        _peerConnected = true;

        if (NetworkManager.Instance.IsHost)
        {
            _hostStatusLabel.Text = "Opponent connected!";
            _startGameButton.Disabled = false;
            // Send our name to the client
            NetworkManager.Instance.Rpc(
                nameof(NetworkManager.SendPlayerName),
                _hostNameEdit.Text.Trim());
        }
    }

    private void OnPlayerDisconnected(long id)
    {
        _peerConnected = false;
        _startGameButton.Disabled = true;

        if (NetworkManager.Instance.IsHost)
            _hostStatusLabel.Text = "Opponent disconnected. Waiting…";
    }

    private void OnConnectionSucceeded()
    {
        _joinStatusLabel.Text = "Connected! Waiting for host to start…";
        // Send our name to the host
        NetworkManager.Instance.Rpc(
            nameof(NetworkManager.SendPlayerName),
            _joinNameEdit.Text.Trim());
    }

    private void OnConnectionFailed()
    {
        _joinStatusLabel.Text = "Connection failed.";
        _joinButton.Disabled = false;
        _hostButton.Disabled = false;
    }

    private void OnServerDisconnected()
    {
        _joinStatusLabel.Text = "Host disconnected.";
        _joinButton.Disabled = false;
        _hostButton.Disabled = false;
        _peerConnected = false;
    }
}

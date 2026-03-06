using Godot;

public partial class NetworkManager : Node
{
    public static NetworkManager Instance { get; private set; }

    public const int DefaultPort = 7000;
    public const int MaxClients = 1;

    private ENetMultiplayerPeer _peer;

    // True when a multiplayer peer is active and connected
    public bool IsOnline => Multiplayer.HasMultiplayerPeer()
        && Multiplayer.MultiplayerPeer.GetConnectionStatus()
           == MultiplayerPeer.ConnectionStatus.Connected;

    // True when this instance is the server (host = Player 1)
    public bool IsHost => Multiplayer.IsServer();

    // 1 for host, 2 for client
    public int LocalPlayerNumber => IsHost ? 1 : 2;

    // Signals for lobby UI
    [Signal]
    public delegate void PlayerConnectedEventHandler(long id);
    [Signal]
    public delegate void PlayerDisconnectedEventHandler(long id);
    [Signal]
    public delegate void ConnectionSucceededEventHandler();
    [Signal]
    public delegate void ConnectionFailedEventHandler();
    [Signal]
    public delegate void ServerDisconnectedEventHandler();

    // Remote player name received during lobby handshake
    public string RemotePlayerName { get; set; } = "";

    public override void _Ready()
    {
        Instance = this;

        // Wire SceneTree multiplayer signals
        Multiplayer.PeerConnected += OnPeerConnected;
        Multiplayer.PeerDisconnected += OnPeerDisconnected;
        Multiplayer.ConnectedToServer += OnConnectedToServer;
        Multiplayer.ConnectionFailed += OnConnectionFailed;
        Multiplayer.ServerDisconnected += OnServerDisconnected;
    }

    // Host / Join / Disconnect

    public Error HostGame(int port = DefaultPort)
    {
        _peer = new ENetMultiplayerPeer();
        var err = _peer.CreateServer(port, MaxClients);
        if (err != Error.Ok)
        {
            GD.PrintErr($"[Net] Failed to create server on port {port}: {err}");
            return err;
        }
        Multiplayer.MultiplayerPeer = _peer;
        GD.Print($"[Net] Hosting on port {port}");
        return Error.Ok;
    }

    public Error JoinGame(string address, int port = DefaultPort)
    {
        _peer = new ENetMultiplayerPeer();
        var err = _peer.CreateClient(address, port);
        if (err != Error.Ok)
        {
            GD.PrintErr($"[Net] Failed to join {address}:{port}: {err}");
            return err;
        }
        Multiplayer.MultiplayerPeer = _peer;
        GD.Print($"[Net] Connecting to {address}:{port}…");
        return Error.Ok;
    }

    public void Disconnect()
    {
        if (Multiplayer.MultiplayerPeer != null)
        {
            Multiplayer.MultiplayerPeer.Close();
            Multiplayer.MultiplayerPeer = null;
        }
        RemotePlayerName = "";
        GD.Print("[Net] Disconnected");
    }

    // Maps a peer ID to player number (server=1, any client=2)
    public int PeerToPlayer(int peerId) => peerId == 1 ? 1 : 2;

    // Name exchange RPCs

    [Rpc(MultiplayerApi.RpcMode.AnyPeer, CallLocal = false,
         TransferMode = MultiplayerPeer.TransferModeEnum.Reliable)]
    public void SendPlayerName(string playerName)
    {
        RemotePlayerName = playerName;
        GD.Print($"[Net] Remote player name: {playerName}");
    }

    // Scene transition (host only)

    [Rpc(MultiplayerApi.RpcMode.Authority, CallLocal = true,
         TransferMode = MultiplayerPeer.TransferModeEnum.Reliable)]
    public void StartOnlineGame(string hostName, string clientName, int winCondition)
    {
        GameData.Mode = GameData.GameMode.Online;
        GameData.SelectedWinCondition = (GameData.WinCondition)winCondition;
        GameData.Player1Name = hostName;
        GameData.Player2Name = clientName;
        GetTree().ChangeSceneToFile("res://scenes/Game.tscn");
    }

    // Callbacks

    private void OnPeerConnected(long id)
    {
        GD.Print($"[Net] Peer connected: {id}");
        EmitSignal(SignalName.PlayerConnected, id);
    }

    private void OnPeerDisconnected(long id)
    {
        GD.Print($"[Net] Peer disconnected: {id}");
        EmitSignal(SignalName.PlayerDisconnected, id);
    }

    private void OnConnectedToServer()
    {
        GD.Print("[Net] Connected to server");
        EmitSignal(SignalName.ConnectionSucceeded);
    }

    private void OnConnectionFailed()
    {
        GD.Print("[Net] Connection failed");
        Disconnect();
        EmitSignal(SignalName.ConnectionFailed);
    }

    private void OnServerDisconnected()
    {
        GD.Print("[Net] Server disconnected");
        Disconnect();
        EmitSignal(SignalName.ServerDisconnected);
    }
}

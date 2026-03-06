using Godot;

public partial class CubeController : Node3D
{
    [Export]
    public Camera3D Camera { get; set; }
    [Export]
    public GameConfig Config { get; set; }

    private bool _isRotating = false;
    private bool _isSnapRotating = false;
    private const float RotationSensitivity = 0.4f;
    private const float SnapAngle = 90f;
    private const float SnapRotationSpeed = 16f;
    private const float SnapFinishThreshold = 0.02f;

    private Quaternion _targetRotation;
    private Quaternion _originalRotation;

    private Vector3 _cameraRestPosition;

    // Face definitions: name, position, euler rotation in degrees
    private static readonly (string Name, Vector3 Pos, Vector3 RotDeg)[] FaceConfigs =
    {
        ("FaceFront",  new Vector3(0,      0,      0.5f),  new Vector3( 90, 0,   0)),
        ("FaceBack",   new Vector3(0,      0,     -0.5f),  new Vector3(-90, 0,   0)),
        ("FaceRight",  new Vector3( 0.5f,  0,      0),     new Vector3(  0, 0, -90)),
        ("FaceLeft",   new Vector3(-0.5f,  0,      0),     new Vector3(  0, 0,  90)),
        ("FaceTop",    new Vector3(0,      0.5f,   0),     Vector3.Zero),
        ("FaceBottom", new Vector3(0,     -0.5f,   0),     new Vector3(180, 0,   0)),
    };

    public override void _Ready()
    {
        Config ??= GD.Load<GameConfig>("res://game_config.tres");
        _originalRotation = Quaternion;
        _targetRotation = Quaternion;
        if (Camera != null)
            _cameraRestPosition = Camera.Position;
        for (int i = 0; i < FaceConfigs.Length; i++)
        {
            var (name, pos, rotDeg) = FaceConfigs[i];
            var face = new CubeFace
            {
                Name = name,
                FaceIndex = i,
                Position = pos,
                RotationDegrees = rotDeg,
                Config = Config
            };
            AddChild(face);
        }
    }

    public override void _Process(double delta)
    {
        if (_isSnapRotating)
        {
            float t = Mathf.Clamp((float)(SnapRotationSpeed * delta), 0f, 1f);
            Quaternion = Quaternion.Slerp(_targetRotation, t);
            if (Quaternion.AngleTo(_targetRotation) < SnapFinishThreshold)
            {
                Quaternion = _targetRotation;
                _isSnapRotating = false;
            }
        }

    }

    // Order matches FaceConfigs: Front, Back, Right, Left, Top, Bottom.
    private static readonly Quaternion[] FaceViewRotations =
    {
        Quaternion.Identity, // Front  (no rotation)
        new Quaternion(Vector3.Up,    Mathf.Pi), // Back   (180° Y)
        new Quaternion(Vector3.Up,   -Mathf.Pi / 2f), // Right  (-90° Y)
        new Quaternion(Vector3.Up,    Mathf.Pi / 2f), // Left   (+90° Y)
        new Quaternion(Vector3.Right, Mathf.Pi / 2f), // Top    (+90° X)
        new Quaternion(Vector3.Right,-Mathf.Pi / 2f), // Bottom (-90° X)
    };

    public void SnapToFace(int faceIndex)
    {
        if (faceIndex < 0 || faceIndex >= FaceViewRotations.Length) return;
        _targetRotation = FaceViewRotations[faceIndex];
        _isSnapRotating = true;
    }

    // In online mode, only the active player controls rotation.
    // Returns true if this peer is allowed to rotate right now.
    private bool CanRotate()
    {
        if (GameData.Mode != GameData.GameMode.Online)
            return true;
        var gm = GetTree().Root.GetNodeOrNull<GameManager>("Game/GameManager");
        if (gm == null) return true;
        return NetworkManager.Instance.LocalPlayerNumber == gm.CurrentPlayer;
    }

    public override void _UnhandledInput(InputEvent @event)
    {
        if (!CanRotate())
        {
            // Still stop drag if we were rotating and turn changed
            if (@event is InputEventMouseButton mb2 && mb2.ButtonIndex == MouseButton.Right && !mb2.Pressed)
                _isRotating = false;
            return;
        }

        if (@event is InputEventMouseButton mb)
        {
            if (mb.ButtonIndex == MouseButton.Right)
                _isRotating = mb.Pressed;
        }
        else if (@event is InputEventMouseMotion mm && _isRotating)
        {
            float dx = mm.Relative.X * RotationSensitivity;
            float dy = mm.Relative.Y * RotationSensitivity;
            RotateY(Mathf.DegToRad(dx));
            GlobalRotate(Camera.GlobalBasis.X, Mathf.DegToRad(dy));

            // Replicate free-drag rotation to the other peer
            if (GameData.Mode == GameData.GameMode.Online)
                Rpc(nameof(NetSetRotation), Quaternion.X, Quaternion.Y, Quaternion.Z, Quaternion.W);
        }

        if (Input.IsActionJustPressed("rotate_up"))
            SnapRotateAndSync(Camera.GlobalBasis.X, -SnapAngle);
        else if (Input.IsActionJustPressed("rotate_down"))
            SnapRotateAndSync(Camera.GlobalBasis.X, SnapAngle);
        else if (Input.IsActionJustPressed("rotate_left"))
            SnapRotateAndSync(Vector3.Up, -SnapAngle);
        else if (Input.IsActionJustPressed("rotate_right"))
            SnapRotateAndSync(Vector3.Up, SnapAngle);
        else if (Input.IsActionJustPressed("rotate_reset"))
            ResetRotationAndSync();
    }

    private void SnapRotateAndSync(Vector3 axis, float angleDeg)
    {
        SnapRotate(axis, angleDeg);
        if (GameData.Mode == GameData.GameMode.Online)
            Rpc(nameof(NetSetSnapTarget), _targetRotation.X, _targetRotation.Y, _targetRotation.Z, _targetRotation.W);
    }

    private void ResetRotationAndSync()
    {
        ResetRotation();
        if (GameData.Mode == GameData.GameMode.Online)
            Rpc(nameof(NetSetSnapTarget), _targetRotation.X, _targetRotation.Y, _targetRotation.Z, _targetRotation.W);
    }

    // Rotation replication RPCs

    [Rpc(MultiplayerApi.RpcMode.AnyPeer, CallLocal = false,
         TransferMode = MultiplayerPeer.TransferModeEnum.UnreliableOrdered)]
    public void NetSetRotation(float qx, float qy, float qz, float qw)
    {
        Quaternion = new Quaternion(qx, qy, qz, qw);
        _targetRotation = Quaternion;
        _isSnapRotating = false;
    }

    [Rpc(MultiplayerApi.RpcMode.AnyPeer, CallLocal = false,
         TransferMode = MultiplayerPeer.TransferModeEnum.Reliable)]
    public void NetSetSnapTarget(float qx, float qy, float qz, float qw)
    {
        _targetRotation = new Quaternion(qx, qy, qz, qw);
        _isSnapRotating = true;
    }

    private void SnapRotate(Vector3 axis, float angleDeg)
    {
        var rot = new Quaternion(axis, Mathf.DegToRad(angleDeg));
        _targetRotation = rot * _targetRotation;
        _isSnapRotating = true;
    }

    private void ResetRotation()
    {
        _targetRotation = _originalRotation;
        _isSnapRotating = true;
    }
}

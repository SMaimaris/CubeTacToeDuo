using Godot;

public partial class CubeFace : Node3D
{
    [Export]
    public int FaceIndex { get; set; } = 0;

    public GameConfig Config { get; set; }

    public int Winner { get; private set; } = 0;

    private int[] _board = new int[9];
    private CubeCell[] _cells = new CubeCell[9];
    private MeshInstance3D _wonOverlay;
    private StandardMaterial3D _wonMaterial;
    private MeshInstance3D _lockedOverlay;

    private static readonly int[][] WinPatterns = new int[][]
    {
        new[] { 0, 1, 2 }, new[] { 3, 4, 5 }, new[] { 6, 7, 8 },
        new[] { 0, 3, 6 }, new[] { 1, 4, 7 }, new[] { 2, 5, 8 },
        new[] { 0, 4, 8 }, new[] { 2, 4, 6 }
    };

    [Signal]
    public delegate void FaceWonEventHandler(int faceIndex, int player);

    [Signal]
    public delegate void CellClaimedOnFaceEventHandler(int faceIndex, int cellIndex, int player);

    public override void _Ready()
    {
        Color faceColor = Config?.FaceColor ?? new Color(0.42f, 0.34f, 0.26f);
        Color gridColor = Config?.GridColor ?? new Color(0.784f, 0.588f, 0.047f);

        // Face base
        var faceBase = new MeshInstance3D { Name = "FaceBase" };
        var planeMesh = new PlaneMesh { Size = new Vector2(1.0f, 1.0f) };
        var faceMat = new StandardMaterial3D
        {
            AlbedoColor = faceColor,
            AlbedoTexture = Config?.FaceTexture,
            CullMode = BaseMaterial3D.CullModeEnum.Disabled
        };
        planeMesh.SurfaceSetMaterial(0, faceMat);
        faceBase.Mesh = planeMesh;
        AddChild(faceBase);

        // Grid lines
        var gridMat = new StandardMaterial3D { AlbedoColor = gridColor };
        AddGridLine(new Vector3(0, 0.008f, 0.167f), new Vector3(0.99f, 0.0001f, 0.012f), gridMat);
        AddGridLine(new Vector3(0, 0.008f, -0.167f), new Vector3(0.99f, 0.0001f, 0.012f), gridMat);
        AddGridLine(new Vector3(0.167f, 0.008f, 0), new Vector3(0.012f, 0.0001f, 0.99f), gridMat);
        AddGridLine(new Vector3(-0.167f, 0.008f, 0), new Vector3(0.012f, 0.0001f, 0.99f), gridMat);

        // 3x3 cells
        for (int i = 0; i < 9; i++)
        {
            int col = i % 3;
            int row = i / 3;
            var cell = new CubeCell
            {
                Name = $"Cell_{i}",
                CellIndex = i,
                Position = new Vector3((col - 1) * 0.333f, 0.006f, (row - 1) * 0.333f),
                Config = Config
            };
            AddChild(cell);
            _cells[i] = cell;
            cell.CellClicked += OnCellClicked;
        }

        // Won overlay
        _wonOverlay = new MeshInstance3D { Name = "WonOverlay" };
        var overlayMesh = new PlaneMesh { Size = new Vector2(0.98f, 0.98f) };
        _wonMaterial = new StandardMaterial3D
        {
            AlbedoColor = Config?.P1WonOverlayColor ?? new Color(0.784f, 0.588f, 0.047f, 0.35f),
            Transparency = BaseMaterial3D.TransparencyEnum.Alpha,
            CullMode = BaseMaterial3D.CullModeEnum.Disabled
        };
        overlayMesh.SurfaceSetMaterial(0, _wonMaterial);
        _wonOverlay.Mesh = overlayMesh;
        _wonOverlay.Position = new Vector3(0, 0.012f, 0);
        _wonOverlay.Visible = false;
        AddChild(_wonOverlay);

        // Locked overlay (shown when this face can't be used for the second move)
        _lockedOverlay = new MeshInstance3D { Name = "LockedOverlay" };
        var lockedMesh = new PlaneMesh { Size = new Vector2(0.98f, 0.98f) };
        var lockedMat = new StandardMaterial3D
        {
            AlbedoColor = Config?.FaceLockedOverlayColor ?? new Color(0.05f, 0.05f, 0.05f, 0.6f),
            Transparency = BaseMaterial3D.TransparencyEnum.Alpha,
            CullMode = BaseMaterial3D.CullModeEnum.Disabled
        };
        lockedMesh.SurfaceSetMaterial(0, lockedMat);
        _lockedOverlay.Mesh = lockedMesh;
        _lockedOverlay.Position = new Vector3(0, 0.008f, 0);
        _lockedOverlay.Visible = false;
        AddChild(_lockedOverlay);
    }

    public override void _Process(double delta)
    {
        // Dynamically orient all markers
        var basis = GlobalBasis.Orthonormalized();
        var faceNormal = basis.Y;
        var baseUp = -basis.Z;

        var worldUp = Vector3.Up;
        float nDotUp = faceNormal.Dot(worldUp);
        var upProj = worldUp - nDotUp * faceNormal;

        float pivotYDeg;
        if (upProj.LengthSquared() < 0.001f)
        {
            pivotYDeg = 0f;
        }
        else
        {
            upProj = upProj.Normalized();
            var baseUpProj = (baseUp - baseUp.Dot(faceNormal) * faceNormal).Normalized();
            float cosA = Mathf.Clamp(baseUpProj.Dot(upProj), -1f, 1f);
            float sinA = faceNormal.Dot(baseUpProj.Cross(upProj));
            pivotYDeg = Mathf.RadToDeg(Mathf.Atan2(sinA, cosA));
        }

        foreach (var cell in _cells)
            cell.SetMarkerPivotYDeg(pivotYDeg);
    }

    public int GetCell(int index) => _board[index];

    public void SetLocked(bool locked)
    {
        _lockedOverlay.Visible = locked;
        foreach (var cell in _cells)
            cell.SetInteractable(!locked);
    }

    private void AddGridLine(Vector3 pos, Vector3 size, StandardMaterial3D mat)
    {
        var line = new MeshInstance3D();
        var boxMesh = new BoxMesh { Size = size };
        boxMesh.SurfaceSetMaterial(0, mat);
        line.Mesh = boxMesh;
        line.Position = pos;
        AddChild(line);
    }

    private void OnCellClicked(int cellIndex)
    {
        var gm = GetTree().Root.GetNodeOrNull<GameManager>("Game/GameManager");
        if (gm == null || gm.State != GameManager.GameState.Playing)
            return;

        if (!gm.CanClaimOnFace(FaceIndex))
            return;

        // Online: only allow if it's our turn, and send via RPC
        if (GameData.Mode == GameData.GameMode.Online)
        {
            if (NetworkManager.Instance.LocalPlayerNumber != gm.CurrentPlayer)
                return;
            gm.Rpc(nameof(GameManager.ServerRequestMove), FaceIndex, cellIndex);
            return;
        }

        // Local / AI: apply directly
        ClaimCell(cellIndex, gm.CurrentPlayer);
        EmitSignal(SignalName.CellClaimedOnFace, FaceIndex, cellIndex, gm.CurrentPlayer);
    }

    public void ClaimCell(int cellIndex, int player)
    {
        if (_board[cellIndex] != 0 || Winner != 0)
            return;

        _board[cellIndex] = player;
        _cells[cellIndex].ShowMarker(player);

        if (CheckWin(player))
        {
            Winner = player;
            ShowWonOverlay(player);
            EmitSignal(SignalName.FaceWon, FaceIndex, player);
        }
    }

    private bool CheckWin(int player)
    {
        foreach (var pattern in WinPatterns)
        {
            if (_board[pattern[0]] == player &&
                _board[pattern[1]] == player &&
                _board[pattern[2]] == player)
                return true;
        }
        return false;
    }

    private void ShowWonOverlay(int player)
    {
        _wonMaterial.AlbedoColor = player == 1
            ? (Config?.P1WonOverlayColor ?? new Color(0.784f, 0.588f, 0.047f, 0.4f))
            : (Config?.P2WonOverlayColor ?? new Color(0.545f, 0.125f, 0.125f, 0.4f));
        _wonOverlay.Visible = true;
    }
}

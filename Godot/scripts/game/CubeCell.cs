using Godot;

public partial class CubeCell : Area3D
{
    public int CellIndex { get; set; }
    public bool IsOccupied { get; private set; } = false;
    public GameConfig Config { get; set; }

    private MeshInstance3D _hoverHighlight;
    private Node3D _markerPivot;
    private Node3D _marker;

    [Signal]
    public delegate void CellClickedEventHandler(int cellIndex);

    public override void _Ready()
    {
        Color p1Color = Config?.Player1Color ?? new Color(0.784f, 0.588f, 0.047f, 1f);
        Color p2Color = Config?.Player2Color ?? new Color(0.545f, 0.125f, 0.125f, 1f);
        Color hoverColor = Config?.HoverHighlightColor ?? new Color(0.784f, 0.588f, 0.047f, 0.35f);

        InputRayPickable = true;
        Monitoring = false;

        // Collision shape, thin slab along face normal (Y)
        var colShape = new CollisionShape3D();
        var box = new BoxShape3D { Size = new Vector3(0.333f, 0.01f, 0.333f) };
        colShape.Shape = box;
        AddChild(colShape);

        // Hover highlight
        _hoverHighlight = new MeshInstance3D();
        var hMesh = new PlaneMesh { Size = new Vector2(0.28f, 0.28f) };
        var hMat = new StandardMaterial3D
        {
            AlbedoColor = hoverColor,
            Transparency = BaseMaterial3D.TransparencyEnum.Alpha,
            CullMode = BaseMaterial3D.CullModeEnum.Disabled
        };
        hMesh.SurfaceSetMaterial(0, hMat);
        _hoverHighlight.Mesh = hMesh;
        _hoverHighlight.Position = new Vector3(0, 0.001f, 0);
        _hoverHighlight.Visible = false;
        AddChild(_hoverHighlight);

        // Pivot rotated each frame by CubeFace to keep marker up aligned with world +Y
        _markerPivot = new Node3D();
        AddChild(_markerPivot);

        // Store colors for use in ShowMarker
        _p1Color = p1Color;
        _p2Color = p2Color;

        MouseEntered += OnMouseEntered;
        MouseExited += OnMouseExited;
    }

    private Color _p1Color;
    private Color _p2Color;
    private bool _isInteractable = true;

    public void SetMarkerPivotYDeg(float deg)
    {
        _markerPivot.RotationDegrees = new Vector3(0f, deg, 0f);
    }

    public void SetInteractable(bool interactable)
    {
        _isInteractable = interactable;
        if (!interactable)
            _hoverHighlight.Visible = false;
    }

    private void OnMouseEntered()
    {
        if (!IsOccupied && _isInteractable)
            _hoverHighlight.Visible = true;
    }

    private void OnMouseExited()
    {
        _hoverHighlight.Visible = false;
    }

    public override void _InputEvent(Camera3D camera, InputEvent @event, Vector3 eventPosition, Vector3 normal, int shapeIdx)
    {
        if (@event is InputEventMouseButton mb && mb.ButtonIndex == MouseButton.Left && mb.Pressed)
        {
            if (!IsOccupied)
                EmitSignal(SignalName.CellClicked, CellIndex);
        }
    }

    public void ShowMarker(int player)
    {
        IsOccupied = true;
        _hoverHighlight.Visible = false;

        var scene = player == 1 ? Config?.CrossedSwordsScene : Config?.ShieldScene;
        if (scene == null)
            return;

        _marker = scene.Instantiate<Node3D>();
        _marker.Position = new Vector3(0, 0.006f, 0);
        _marker.Scale = Vector3.Zero;
        _markerPivot.AddChild(_marker);

        ApplyOutline(_marker, player == 1 ? _p1Color : _p2Color);

        var tween = CreateTween();
        tween.TweenProperty(_marker, "scale", Vector3.One * 1.35f, 0.13f)
            .SetEase(Tween.EaseType.Out).SetTrans(Tween.TransitionType.Cubic);
        tween.TweenProperty(_marker, "scale", Vector3.One, 0.1f)
            .SetEase(Tween.EaseType.In).SetTrans(Tween.TransitionType.Cubic);
    }

    private static void ApplyOutline(Node3D root, Color color)
    {
        var shader = new Shader();
        shader.Code = """
            shader_type spatial;
            render_mode cull_front, unshaded, depth_draw_opaque;
            uniform float outline_width : hint_range(0.0, 0.1) = 0.025;
            uniform vec4 outline_color : source_color = vec4(1.0, 1.0, 1.0, 1.0);
            uniform float emission_strength : hint_range(0.0, 8.0) = 3.0;
            void vertex() {
                VERTEX += NORMAL * outline_width;
            }
            void fragment() {
                ALBEDO = outline_color.rgb;
                EMISSION = outline_color.rgb * emission_strength;
            }
            """;

        // Brighten the player color toward white for high contrast
        var bright = color.Lightened(0.55f);
        bright.A = 1f;

        var mat = new ShaderMaterial();
        mat.Shader = shader;
        mat.SetShaderParameter("outline_color", bright);

        foreach (var node in root.FindChildren("*", "MeshInstance3D", true, false))
        {
            if (node is MeshInstance3D mi)
                mi.MaterialOverlay = mat;
        }
    }
}

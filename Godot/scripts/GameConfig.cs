using Godot;

[GlobalClass]
public partial class GameConfig : Resource
{
    [ExportGroup("Timing")]
    [Export]
    public float TurnDuration { get; set; } = 15f;

    [ExportGroup("Textures")]
    [Export]
    public Texture2D FaceTexture { get; set; }

    [ExportGroup("Scenes")]
    [Export]
    public PackedScene CrossedSwordsScene { get; set; }
    [Export]
    public PackedScene ShieldScene { get; set; }

    [ExportGroup("Colors")]
    [Export]
    public Color Player1Color { get; set; } = new Color(0.784f, 0.588f, 0.047f, 1f);
    [Export]
    public Color Player2Color { get; set; } = new Color(0.545f, 0.125f, 0.125f, 1f);
    [Export]
    public Color FaceColor { get; set; } = new Color(0.42f, 0.34f, 0.26f, 1f);
    [Export]
    public Color GridColor { get; set; } = new Color(0.784f, 0.588f, 0.047f, 1f);
    [Export]
    public Color HoverHighlightColor { get; set; } = new Color(0.784f, 0.588f, 0.047f, 0.35f);
    [Export]
    public Color P1WonOverlayColor { get; set; } = new Color(0.784f, 0.588f, 0.047f, 0.4f);
    [Export]
    public Color P2WonOverlayColor { get; set; } = new Color(0.545f, 0.125f, 0.125f, 0.4f);
    [Export]
    public Color FaceLockedOverlayColor { get; set; } = new Color(0.05f, 0.05f, 0.05f, 0.6f);

    [ExportGroup("Music")]
    [Export]
    public AudioStream MusicMenu { get; set; }
    [Export]
    public AudioStream MusicBattle { get; set; }
    [Export]
    public AudioStream MusicVictory { get; set; }

    [ExportGroup("SFX")]
    [Export]
    public AudioStream SfxPlaceSword { get; set; }
    [Export]
    public AudioStream SfxPlaceShield { get; set; }
    [Export]
    public AudioStream SfxTurnEnd { get; set; }
    [Export]
    public AudioStream SfxWin { get; set; }
}

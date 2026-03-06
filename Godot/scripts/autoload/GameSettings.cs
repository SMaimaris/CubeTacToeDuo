using Godot;

public partial class GameSettings : Node
{
    public static GameSettings Instance { get; private set; }

    public float MasterVolume { get; set; } = 1.0f;
    public float SfxVolume { get; set; } = 0.5f;
    public float MusicVolume { get; set; } = 0.6f;
    public bool IsMuted { get; set; } = false;
    public Vector2I Resolution { get; set; } = new Vector2I(1920, 1080);
    public bool IsVSync { get; set; } = false;
    public bool IsFullscreen { get; set; } = true;

    private const string SavePath = "user://settings.cfg";

    public override void _Ready()
    {
        Instance = this;
        Load();
        ApplySettings();
    }

    public void Save()
    {
        var cfg = new ConfigFile();
        cfg.SetValue("audio", "master_volume", MasterVolume);
        cfg.SetValue("audio", "sfx_volume", SfxVolume);
        cfg.SetValue("audio", "music_volume", MusicVolume);
        cfg.SetValue("audio", "muted", IsMuted);
        cfg.SetValue("display", "resolution_x", Resolution.X);
        cfg.SetValue("display", "resolution_y", Resolution.Y);
        cfg.SetValue("display", "vsync", IsVSync);
        cfg.SetValue("display", "fullscreen", IsFullscreen);
        cfg.Save(SavePath);
    }

    public void Load()
    {
        var cfg = new ConfigFile();
        if (cfg.Load(SavePath) != Error.Ok)
            return;

        MasterVolume = (float)cfg.GetValue("audio", "master_volume", 1.0f);
        SfxVolume = (float)cfg.GetValue("audio", "sfx_volume", 0.5f);
        MusicVolume = (float)cfg.GetValue("audio", "music_volume", 0.6f);
        IsMuted = (bool)cfg.GetValue("audio", "muted", false);
        int rx = (int)cfg.GetValue("display", "resolution_x", 1920);
        int ry = (int)cfg.GetValue("display", "resolution_y", 1080);
        Resolution = new Vector2I(rx, ry);
        IsVSync = (bool)cfg.GetValue("display", "vsync", false);
        IsFullscreen = (bool)cfg.GetValue("display", "fullscreen", true);
    }

    public void ApplySettings()
    {
        // Apply audio bus volumes
        float effectiveMaster = IsMuted ? 0f : MasterVolume;
        AudioServer.SetBusVolumeDb(AudioServer.GetBusIndex("Master"),
            Mathf.LinearToDb(effectiveMaster));

        int sfxBus = AudioServer.GetBusIndex("SFX");
        if (sfxBus >= 0)
            AudioServer.SetBusVolumeDb(sfxBus, Mathf.LinearToDb(SfxVolume));

        int musicBus = AudioServer.GetBusIndex("Music");
        if (musicBus >= 0)
            AudioServer.SetBusVolumeDb(musicBus, Mathf.LinearToDb(MusicVolume));

        // Apply display settings
        DisplayServer.VSyncMode vsyncMode = IsVSync
            ? DisplayServer.VSyncMode.Enabled
            : DisplayServer.VSyncMode.Disabled;
        DisplayServer.WindowSetVsyncMode(vsyncMode);

        // Defer window mode change to avoid conflicts during initialization
        Callable.From(ApplyWindowMode).CallDeferred();
    }

    private void ApplyWindowMode()
    {
        if (IsFullscreen)
        {
            DisplayServer.WindowSetMode(DisplayServer.WindowMode.ExclusiveFullscreen);
        }
        else
        {
            DisplayServer.WindowSetMode(DisplayServer.WindowMode.Windowed);
            DisplayServer.WindowSetSize(Resolution);
            var screenSize = DisplayServer.ScreenGetSize();
            var winSize = DisplayServer.WindowGetSize();
            DisplayServer.WindowSetPosition((screenSize - winSize) / 2);
        }
    }
}

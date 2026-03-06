using Godot;

public partial class OptionsMenu : Panel
{
    private HSlider _masterSlider;
    private HSlider _sfxSlider;
    private HSlider _musicSlider;
    private CheckButton _muteCheck;
    private OptionButton _resolutionOption;
    private CheckButton _vsyncCheck;
    private CheckButton _fullscreenCheck;
    private Button _backButton;

    private static readonly Vector2I[] Resolutions =
    {
        new Vector2I(1280, 720),
        new Vector2I(1920, 1080),
        new Vector2I(2560, 1440),
        new Vector2I(3840, 2160)
    };

    public override void _Ready()
    {
        _masterSlider = GetNode<HSlider>("%MasterSlider");
        _sfxSlider = GetNode<HSlider>("%SfxSlider");
        _musicSlider = GetNode<HSlider>("%MusicSlider");
        _muteCheck = GetNode<CheckButton>("%MuteCheck");
        _resolutionOption = GetNode<OptionButton>("%ResolutionOption");
        _vsyncCheck = GetNode<CheckButton>("%VsyncCheck");
        _fullscreenCheck = GetNode<CheckButton>("%FullscreenCheck");
        _backButton = GetNode<Button>("%BackButton");

        LoadFromSettings();

        _masterSlider.ValueChanged += v => { GameSettings.Instance.MasterVolume = (float)v; GameSettings.Instance.ApplySettings(); };
        _sfxSlider.ValueChanged += v => AudioManager.Instance?.SetSfxVolume((float)v);
        _musicSlider.ValueChanged += v => AudioManager.Instance?.SetMusicVolume((float)v);
        _muteCheck.Toggled += v => { GameSettings.Instance.IsMuted = v; GameSettings.Instance.ApplySettings(); };
        _resolutionOption.ItemSelected += OnResolutionSelected;
        _vsyncCheck.Toggled += v => { GameSettings.Instance.IsVSync = v; GameSettings.Instance.ApplySettings(); };
        _fullscreenCheck.Toggled += v => { GameSettings.Instance.IsFullscreen = v; GameSettings.Instance.ApplySettings(); };
        _backButton.Pressed += OnBackPressed;
    }

    private void LoadFromSettings()
    {
        var s = GameSettings.Instance;
        if (s == null) return;

        _masterSlider.Value = s.MasterVolume;
        _sfxSlider.Value = s.SfxVolume;
        _musicSlider.Value = s.MusicVolume;
        _muteCheck.ButtonPressed = s.IsMuted;
        _vsyncCheck.ButtonPressed = s.IsVSync;
        _fullscreenCheck.ButtonPressed = s.IsFullscreen;

        for (int i = 0; i < Resolutions.Length; i++)
        {
            if (Resolutions[i] == s.Resolution)
            {
                _resolutionOption.Selected = i;
                break;
            }
        }
    }

    private void OnResolutionSelected(long index)
    {
        if (index >= 0 && index < Resolutions.Length)
        {
            GameSettings.Instance.Resolution = Resolutions[index];
            GameSettings.Instance.ApplySettings();
        }
    }

    private void OnBackPressed()
    {
        GameSettings.Instance?.Save();
        Visible = false;
    }
}

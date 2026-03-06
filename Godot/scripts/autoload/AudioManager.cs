using Godot;

public partial class AudioManager : Node
{
    public static AudioManager Instance { get; private set; }

    private AudioStreamPlayer _sfxPlayer;
    private AudioStreamPlayer _musicPlayer;
    private GameConfig _config;
    private bool _loopMusic = false;

    public override void _Ready()
    {
        Instance = this;
        _config = GD.Load<GameConfig>("res://game_config.tres");

        _sfxPlayer = new AudioStreamPlayer();
        AddChild(_sfxPlayer);

        _musicPlayer = new AudioStreamPlayer();
        AddChild(_musicPlayer);
        _musicPlayer.Finished += () => { if (_loopMusic && _musicPlayer.Stream != null) _musicPlayer.Play(); };
    }

    public void PlaySfx(string name)
    {
        if (GameSettings.Instance?.IsMuted == true || _config == null)
            return;

        AudioStream stream = name switch
        {
            "place_sword" => _config.SfxPlaceSword,
            "place_shield" => _config.SfxPlaceShield,
            "turn" => _config.SfxTurnEnd,
            "win" => _config.SfxWin,
            _ => null
        };

        if (stream == null)
            return;

        _sfxPlayer.Stream = stream;
        _sfxPlayer.VolumeDb = Mathf.LinearToDb(GameSettings.Instance?.SfxVolume ?? 0.8f);
        _sfxPlayer.Play();
    }

    public void PlayMusic(AudioStream stream, bool loop = true)
    {
        if (stream == null || (_musicPlayer.Stream == stream && _musicPlayer.Playing))
            return;

        _loopMusic = loop;
        _musicPlayer.Stream = stream;
        _musicPlayer.VolumeDb = Mathf.LinearToDb(GameSettings.Instance?.MusicVolume ?? 0.6f);
        _musicPlayer.Play();
    }

    public void StopMusic() => _musicPlayer.Stop();

    public GameConfig Config => _config;

    public void SetSfxVolume(float volume)
    {
        if (GameSettings.Instance != null)
            GameSettings.Instance.SfxVolume = volume;
        _sfxPlayer.VolumeDb = Mathf.LinearToDb(volume);
    }

    public void SetMusicVolume(float volume)
    {
        if (GameSettings.Instance != null)
            GameSettings.Instance.MusicVolume = volume;
        _musicPlayer.VolumeDb = Mathf.LinearToDb(volume);
    }
}

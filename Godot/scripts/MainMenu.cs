using Godot;

public partial class MainMenu : Control
{
    private Button _playButton;
    private Button _onlineButton;
    private Button _optionsButton;
    private Button _quitButton;
    private Control _optionsMenu;

    public override void _Ready()
    {
        _playButton = GetNode<Button>("%PlayButton");
        _onlineButton = GetNode<Button>("%OnlineButton");
        _optionsButton = GetNode<Button>("%OptionsButton");
        _quitButton = GetNode<Button>("%QuitButton");
        _optionsMenu = GetNode<Control>("%OptionsMenu");

        _playButton.Pressed += OnPlayPressed;
        _onlineButton.Pressed += OnOnlinePressed;
        _optionsButton.Pressed += OnOptionsPressed;
        _quitButton.Pressed += OnQuitPressed;

        _optionsMenu.Visible = false;
        AudioManager.Instance?.PlayMusic(AudioManager.Instance?.Config?.MusicMenu, loop: true);
    }

    private void OnPlayPressed()
    {
        AudioManager.Instance?.PlaySfx("click");
        GetTree().ChangeSceneToFile("res://scenes/GameSetup.tscn");
    }

    private void OnOnlinePressed()
    {
        AudioManager.Instance?.PlaySfx("click");
        GetTree().ChangeSceneToFile("res://scenes/NetworkLobby.tscn");
    }

    private void OnOptionsPressed()
    {
        AudioManager.Instance?.PlaySfx("click");
        _optionsMenu.Visible = true;
    }

    private void OnQuitPressed()
    {
        GetTree().Quit();
    }
}

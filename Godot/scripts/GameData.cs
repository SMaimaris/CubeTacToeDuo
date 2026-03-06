// Static data carrier passed between scenes
public static class GameData
{
    public enum WinCondition { AnySide, Majority }
    public enum GameMode { Local2P, VsAI, Online }
    public enum AIDifficulty { Easy, Medium, Hard }

    public static WinCondition SelectedWinCondition { get; set; } = WinCondition.AnySide;
    public static string Player1Name { get; set; } = "Player 1";
    public static string Player2Name { get; set; } = "Player 2";
    public static GameMode Mode { get; set; } = GameMode.Local2P;
    public static AIDifficulty Difficulty { get; set; } = AIDifficulty.Medium;
}

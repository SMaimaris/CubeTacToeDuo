using Godot;

/// <summary>
/// Simulates torch-like flickering on an OmniLight3D
/// </summary>
[GlobalClass]
public partial class TorchFlicker : OmniLight3D
{
    [Export]
    public float BaseEnergy { get; set; } = 2.0f;
    [Export]
    public float FlickerAmount { get; set; } = 0.3f;
    [Export]
    public float FlickerSpeed { get; set; } = 8.0f;

    private float _time;
    private float _seed;

    public override void _Ready()
    {
        _seed = (float)GD.RandRange(0.0, 100.0);
        BaseEnergy = LightEnergy;
    }

    public override void _Process(double delta)
    {
        _time += (float)delta * FlickerSpeed;

        float flicker = Mathf.Sin(_time + _seed) * 0.5f
                      + Mathf.Sin(_time * 2.3f + _seed * 1.7f) * 0.3f
                      + Mathf.Sin(_time * 5.7f + _seed * 0.3f) * 0.2f;

        LightEnergy = BaseEnergy + flicker * FlickerAmount;
    }
}

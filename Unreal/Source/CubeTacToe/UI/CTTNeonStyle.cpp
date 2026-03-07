#include "UI/CTTNeonStyle.h"
#include "Styling/CoreStyle.h"

// ---- Palette ----

FLinearColor FCTTNeonStyle::UltraPink()    { return FLinearColor::FromSRGBColor(FColor(0xFF, 0x5B, 0xFF)); }
FLinearColor FCTTNeonStyle::TyrianPurple() { return FLinearColor::FromSRGBColor(FColor(0xD3, 0x4D, 0xEE)); }
FLinearColor FCTTNeonStyle::Phlox()        { return FLinearColor::FromSRGBColor(FColor(0xBF, 0x4B, 0xF8)); }
FLinearColor FCTTNeonStyle::VividSkyBlue() { return FLinearColor::FromSRGBColor(FColor(0x2A, 0xC9, 0xF9)); }
FLinearColor FCTTNeonStyle::ElectricBlue() { return FLinearColor::FromSRGBColor(FColor(0x56, 0xF1, 0xFF)); }
FLinearColor FCTTNeonStyle::PanelBg()      { return FLinearColor::FromSRGBColor(FColor(0x0A, 0x00, 0x10)); }

// ---- Brush builders ----

FSlateBrush FCTTNeonStyle::RoundedBox(FLinearColor TintColor, float TintAlpha,
                                      FLinearColor OutlineColor, float OutlineWidth,
                                      float CornerRadius)
{
	FSlateBrush Brush;
	Brush.DrawAs   = ESlateBrushDrawType::RoundedBox;
	Brush.TintColor = FSlateColor(FLinearColor(TintColor.R, TintColor.G, TintColor.B, TintAlpha));
	Brush.OutlineSettings.CornerRadii    = FVector4(CornerRadius, CornerRadius, CornerRadius, CornerRadius);
	Brush.OutlineSettings.Color          = FSlateColor(OutlineColor);
	Brush.OutlineSettings.Width          = OutlineWidth;
	Brush.OutlineSettings.RoundingType   = ESlateBrushRoundingType::FixedRadius;
	Brush.OutlineSettings.bUseBrushTransparency = true;
	return Brush;
}

// ---- Style factories ----

FButtonStyle FCTTNeonStyle::ButtonStyle(float CornerRadius /* = 8.f */)
{
	FButtonStyle Style;
	// Thick neon border + matching inner tint simulates an inner glow without post-process bloom.
	Style.Normal   = RoundedBox(Phlox(),        0.12f, Phlox(),        4.0f, CornerRadius);
	Style.Hovered  = RoundedBox(UltraPink(),    0.20f, UltraPink(),    6.0f, CornerRadius);
	Style.Pressed  = RoundedBox(ElectricBlue(), 0.30f, ElectricBlue(), 5.0f, CornerRadius);
	Style.Disabled = RoundedBox(PanelBg(),      0.40f, PanelBg(),      2.0f, CornerRadius);

	return Style;
}

FComboBoxStyle FCTTNeonStyle::ComboBoxStyle(float CornerRadius)
{
	FComboBoxStyle Style = FCoreStyle::Get().GetWidgetStyle<FComboBoxStyle>("ComboBox");

	FComboButtonStyle& Btn = Style.ComboButtonStyle;
	Btn.ButtonStyle       = ButtonStyle(CornerRadius);
	Btn.MenuBorderBrush   = RoundedBox(PanelBg(), 0.95f, Phlox(), 2.f, CornerRadius);
	Btn.MenuBorderPadding = FMargin(2.f);

	return Style;
}

FTableRowStyle FCTTNeonStyle::ComboItemStyle(float CornerRadius)
{
	FTableRowStyle Style = FCoreStyle::Get().GetWidgetStyle<FTableRowStyle>("TableView.Row");

	const FSlateBrush NormalBg   = RoundedBox(PanelBg(),    0.90f, PanelBg(),    0.f,  CornerRadius);
	const FSlateBrush HoveredBg  = RoundedBox(Phlox(),      0.20f, Phlox(),      1.5f, CornerRadius);
	const FSlateBrush SelectedBg = RoundedBox(UltraPink(),  0.25f, UltraPink(),  2.f,  CornerRadius);

	Style.EvenRowBackgroundBrush        = NormalBg;
	Style.OddRowBackgroundBrush         = NormalBg;
	Style.EvenRowBackgroundHoveredBrush = HoveredBg;
	Style.OddRowBackgroundHoveredBrush  = HoveredBg;
	Style.ActiveBrush                   = SelectedBg;
	Style.InactiveBrush                 = SelectedBg;
	Style.ActiveHighlightedBrush        = SelectedBg;
	Style.InactiveHighlightedBrush      = SelectedBg;
	Style.TextColor                     = FSlateColor(Phlox());
	Style.SelectedTextColor             = FSlateColor(FLinearColor::White);

	return Style;
}

FProgressBarStyle FCTTNeonStyle::TimerBarStyle(float CornerRadius)
{
	FProgressBarStyle Style;
	Style.BackgroundImage = RoundedBox(PanelBg(), 0.80f, Phlox(), 2.5f, CornerRadius);

	// White fill — tinted at runtime via SetFillColorAndOpacity for the urgency gradient
	FSlateBrush Fill;
	Fill.DrawAs = ESlateBrushDrawType::RoundedBox;
	Fill.TintColor = FSlateColor(FLinearColor::White);
	Fill.OutlineSettings.CornerRadii  = FVector4(CornerRadius, CornerRadius, CornerRadius, CornerRadius);
	Fill.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
	Fill.OutlineSettings.bUseBrushTransparency = true;
	Style.FillImage = Fill;

	return Style;
}

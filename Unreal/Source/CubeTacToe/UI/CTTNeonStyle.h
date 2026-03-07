#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "Styling/CoreStyle.h"

/**
 * Industrial Glow neon palette and Slate style builders.
 * Pure-static helpers — no UObject overhead, safe to include anywhere.
 *
 * Palette:
 *   UltraPink    #FF5BFF  — Player X accent, hot hover
 *   TyrianPurple #D34DEE  — Button hover fill
 *   Phlox        #BF4BF8  — UI chrome, normal borders
 *   VividSkyBlue #2AC9F9  — Secondary highlight
 *   ElectricBlue #56F1FF  — Player O accent, timer full
 *   PanelBg      #0A0010  — Near-black panel background
 */
struct CUBETACTOE_API FCTTNeonStyle
{
	// ---- Palette (sRGB hex → linear via FromSRGBColor) ----

	static FLinearColor UltraPink();     // #FF5BFF
	static FLinearColor TyrianPurple();  // #D34DEE
	static FLinearColor Phlox();         // #BF4BF8
	static FLinearColor VividSkyBlue();  // #2AC9F9
	static FLinearColor ElectricBlue();  // #56F1FF
	static FLinearColor PanelBg();       // #0A0010

	// Semantic shortcuts
	static FLinearColor PlayerX() { return UltraPink(); }
	static FLinearColor PlayerO() { return ElectricBlue(); }

	// ---- Brush builders ----

	/** Rounded box with a translucent tint fill and a solid neon outline. */
	static FSlateBrush RoundedBox(FLinearColor TintColor, float TintAlpha,
	                              FLinearColor OutlineColor, float OutlineWidth,
	                              float CornerRadius = 4.f);

	// ---- Compound style factories ----

	/** Standard neon button: dark fill / pink hover / blue press. */
	static FButtonStyle ButtonStyle(float CornerRadius = 8.f);

	/**
	 * Timer progress bar: dark rounded background, white fill image.
	 * Drive the fill color at runtime via UProgressBar::SetFillColorAndOpacity.
	 */
	static FProgressBarStyle TimerBarStyle(float CornerRadius = 4.f);

	/** Combo box whose button part matches ButtonStyle; dark neon panel for the dropdown. */
	static FComboBoxStyle ComboBoxStyle(float CornerRadius = 8.f);

	/** Row style for combo dropdown items: dark bg, Phlox hover, pink selected. */
	static FTableRowStyle ComboItemStyle(float CornerRadius = 4.f);
};

#include "UI/CTTNeonButton.h"
#include "UI/CTTNeonStyle.h"
#include "Components/TextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"

UCTTNeonButton::UCTTNeonButton()
{
	SetStyle(FCTTNeonStyle::ButtonStyle());
}

// ---------------------------------------------------------------------------
// RebuildWidget — builds:
//
//   SOverlay  (size = button + 2 * GlowSpread)
//     [0] GlowOuter   — fills full overlay,        faintest halo
//     [1] GlowInner   — inset GlowSpread*0.5,      brighter inner ring
//     [2] SButton     — inset GlowSpread,           the neon tube + content
//
// GlowOuter and GlowInner are plain SBorder widgets whose FSlateBrush members
// are stored here; UpdateGlowBrushes() swaps the color and Invalidate(Paint)
// triggers a repaint — no widget rebuild required.
// ---------------------------------------------------------------------------
TSharedRef<SWidget> UCTTNeonButton::RebuildWidget()
{
	TSharedRef<SWidget> Btn = Super::RebuildWidget();

	UpdateGlowBrushes(FCTTNeonStyle::Phlox());

	// Bind hover delegates (Remove first to avoid duplicates on repeated builds)
	OnHovered.RemoveDynamic(this, &UCTTNeonButton::OnGlowHovered);
	OnHovered.AddDynamic(this, &UCTTNeonButton::OnGlowHovered);
	OnUnhovered.RemoveDynamic(this, &UCTTNeonButton::OnGlowUnhovered);
	OnUnhovered.AddDynamic(this, &UCTTNeonButton::OnGlowUnhovered);

	const float Half = GlowSpread * 0.5f;

	return SNew(SOverlay)

		// ---- Outer halo (largest, faintest) ----
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SAssignNew(GlowOuter, SBorder)
			.BorderImage(&GlowBrushOuter)
			.Padding(0.f)
		]

		// ---- Inner halo (medium) ----
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.Padding(Half)
		[
			SAssignNew(GlowInner, SBorder)
			.BorderImage(&GlowBrushInner)
			.Padding(0.f)
		]

		// ---- Actual button (neon tube) ----
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.Padding(GlowSpread)
		[
			Btn
		];
}

void UCTTNeonButton::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (!InternalTextBlock)
	{
		InternalTextBlock = NewObject<UTextBlock>(this);
		InternalTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		SetContent(InternalTextBlock);
	}

	InternalTextBlock->SetText(ButtonText);
}

// ---------------------------------------------------------------------------
// Glow helpers
// ---------------------------------------------------------------------------

void UCTTNeonButton::UpdateGlowBrushes(FLinearColor GlowColor)
{
	const float OuterRadius = 8.f + GlowSpread;       // matches button corner + halo spread
	const float InnerRadius = 8.f + GlowSpread * 0.5f;

	// No outline on glow rings — just a soft semi-transparent fill
	GlowBrushOuter = FCTTNeonStyle::RoundedBox(GlowColor, 0.15f, FLinearColor(0, 0, 0, 0), 0.f, OuterRadius);
	GlowBrushInner = FCTTNeonStyle::RoundedBox(GlowColor, 0.32f, FLinearColor(0, 0, 0, 0), 0.f, InnerRadius);
}

void UCTTNeonButton::OnGlowHovered()
{
	UpdateGlowBrushes(FCTTNeonStyle::UltraPink());
	if (GlowOuter.IsValid()) GlowOuter->Invalidate(EInvalidateWidgetReason::Paint);
	if (GlowInner.IsValid()) GlowInner->Invalidate(EInvalidateWidgetReason::Paint);
}

void UCTTNeonButton::OnGlowUnhovered()
{
	UpdateGlowBrushes(FCTTNeonStyle::Phlox());
	if (GlowOuter.IsValid()) GlowOuter->Invalidate(EInvalidateWidgetReason::Paint);
	if (GlowInner.IsValid()) GlowInner->Invalidate(EInvalidateWidgetReason::Paint);
}

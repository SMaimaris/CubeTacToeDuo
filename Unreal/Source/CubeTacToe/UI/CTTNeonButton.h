#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "Styling/SlateBrush.h"
#include "CTTNeonButton.generated.h"

class UTextBlock;
class SBorder;

/**
 * UButton pre-styled with the Industrial Glow neon theme.
 *
 * Set ButtonText in the Details panel — a styled UTextBlock is created
 * automatically as the button's content.
 *
 * Two concentric glow-ring SBorder layers are built behind the button in
 * RebuildWidget(), extending GlowSpread pixels beyond the button boundary to
 * simulate a neon halo without a post-process pass. Glow color updates on
 * hover/unhover via the button's own delegate callbacks.
 */
UCLASS(meta = (DisplayName = "Neon Button"))
class CUBETACTOE_API UCTTNeonButton : public UButton
{
	GENERATED_BODY()

public:
	UCTTNeonButton();

	/** Label displayed on the button. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	FText ButtonText;

	/** Extra pixels that the glow halo extends beyond the button edge (each side). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance", meta = (ClampMin = "0", ClampMax = "40"))
	float GlowSpread = 14.f;

	virtual void SynchronizeProperties() override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	/** Owned text block; created on first SynchronizeProperties if no content exists. */
	UPROPERTY()
	TObjectPtr<UTextBlock> InternalTextBlock;

	// Slate glow ring layers (rebuilt in RebuildWidget)
	TSharedPtr<SBorder> GlowOuter;
	TSharedPtr<SBorder> GlowInner;

	// Brushes are member variables — SBorder holds a raw pointer to them
	FSlateBrush GlowBrushOuter;
	FSlateBrush GlowBrushInner;

	/** Rebuild glow brushes with the given hue. */
	void UpdateGlowBrushes(FLinearColor GlowColor);

	UFUNCTION()
	void OnGlowHovered();

	UFUNCTION()
	void OnGlowUnhovered();
};

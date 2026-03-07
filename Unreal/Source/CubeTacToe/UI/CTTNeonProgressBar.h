#pragma once

#include "CoreMinimal.h"
#include "Components/ProgressBar.h"
#include "CTTNeonProgressBar.generated.h"

/**
 * UProgressBar pre-styled for the neon timer bar.
 * Dark rounded background with a white fill image.
 * Call SetFillColorAndOpacity at runtime to drive the urgency gradient
 * (ElectricBlue at full time → UltraPink at low time).
 */
UCLASS(meta = (DisplayName = "Neon Progress Bar"))
class CUBETACTOE_API UCTTNeonProgressBar : public UProgressBar
{
	GENERATED_BODY()

public:
	UCTTNeonProgressBar();
};

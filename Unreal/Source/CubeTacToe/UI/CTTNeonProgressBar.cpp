#include "UI/CTTNeonProgressBar.h"
#include "UI/CTTNeonStyle.h"

UCTTNeonProgressBar::UCTTNeonProgressBar()
{
	SetWidgetStyle(FCTTNeonStyle::TimerBarStyle());
	SetFillColorAndOpacity(FCTTNeonStyle::ElectricBlue());
}

#include "Data/ManhuntGameSettings.h"

UManhuntGameSettings::UManhuntGameSettings()
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("Manhunt Game Settings");
}

const UManhuntGameSettings* UManhuntGameSettings::Get()
{
	return GetDefault<UManhuntGameSettings>();
}

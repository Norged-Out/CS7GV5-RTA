// Copyright Epic Games, Inc. All Rights Reserved.

#include "demo_ikGameMode.h"
#include "demo_ikCharacter.h"
#include "UObject/ConstructorHelpers.h"

Ademo_ikGameMode::Ademo_ikGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

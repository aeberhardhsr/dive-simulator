#pragma once
#include "CoreMinimal.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

inline FString DiveShoreSurfaceFixture()
{
    FString name;FParse::Value(FCommandLine::Get(),TEXT("DiveCausticsCase="),name);
    return FParse::Param(FCommandLine::Get(),TEXT("DiveSmokeTest"))&&
        (name==TEXT("shore-top")||name==TEXT("shore-under"))?name:FString();
}

inline bool DiveShoreCausticsFixture()
{
    FString name;FParse::Value(FCommandLine::Get(),TEXT("DiveCausticsCase="),name);
    return (name==TEXT("shore")&&FParse::Param(FCommandLine::Get(),TEXT("DiveSmokeTest")))||!DiveShoreSurfaceFixture().IsEmpty();
}

inline FVector DiveCausticsFixtureCenter(const FVector& Default)
{
    // Point on the pinned swissTLM3D outline near Weesen: E2726254.938, N1221730.728.
    // A diagnostic cutaway receiver straddles this contour; this is not a dive entry.
    return DiveShoreCausticsFixture()?FVector(273072.8,25493.8,0):Default;
}

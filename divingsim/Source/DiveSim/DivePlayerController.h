#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "DivePlayerController.generated.h"

UCLASS()
class DIVESIM_API ADivePlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    virtual bool InputKey(const FInputKeyEventArgs& Params) override;
};

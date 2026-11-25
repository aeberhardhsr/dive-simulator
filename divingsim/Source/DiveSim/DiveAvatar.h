#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DivePose.h"
#include "DiveContacts.h"
#include "DiveAvatar.generated.h"

class ADivePawn;
class UPoseableMeshComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class USkeletalMesh;
class UProceduralMeshComponent;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class UPointLightComponent;

// World-space body, independent of the first-person camera pitch. The camera's
// wrist targets drive two-bone arms; they do not drag the torso through space.
UCLASS()
class DIVESIM_API ADiveAvatar : public AActor
{
    GENERATED_BODY()
public:
    ADiveAvatar();
    void Initialize(ADivePawn* Pawn);
    void UpdatePose(float DeltaSeconds,USceneComponent* Left,USceneComponent* Right);
    bool Validate(FString& Error) const;
    void ContactPose(FDivePoseState& Pose,const FVector& Eyes,TArray<FDiveContactShape>& Shapes,FTransform& Left,FTransform& Right);
    void SetInspection(bool Enabled);
    bool IsInspection() const { return bInspection; }
    UTextureRenderTarget2D* InspectionImage();
    float InspectionYaw=35;
    bool bInspectionClose=false,bPreviewSwimming=false;
    FVector InspectionTarget() const;
    float SwimBlend=0,MotionPhase=0;
    int32 PoseUpdates=0;
    double MaxArmLengthError=0;
    UPROPERTY() TObjectPtr<UPoseableMeshComponent> Suit;
private:
    UPROPERTY() TObjectPtr<ADivePawn> Diver;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Parts;
    UPROPERTY() TArray<TObjectPtr<USkeletalMesh>> Suits;
    UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> FinMaterials;
    UPROPERTY() TObjectPtr<UProceduralMeshComponent> RegulatorHose;
    UPROPERTY() TObjectPtr<USceneCaptureComponent2D> InspectionCapture;
    UPROPERTY() TObjectPtr<UTextureRenderTarget2D> InspectionTexture;
    UPROPERTY() TArray<TObjectPtr<UPointLightComponent>> InspectionLights;
    double LastInspectionCapture=-1;
    TMap<FName,FTransform> Reference;
    TArray<FName> BoneOrder;
    int32 SelectedBCD=INDEX_NONE,SelectedMaterial=INDEX_NONE,SelectedSize=INDEX_NONE;
    bool bDry=true,bConfigured=false,bInspection=false;
    float MotionBlend=0,RegulatorBlend=1,BodyPitch=0;
    FVector PoseEyes=FVector::ZeroVector,PoseGaze=FVector::ForwardVector;
    void Configure();
    void AimBone(FName Name,const FVector& RestEnd,const FVector& Start,const FVector& End);
    FVector Arm(const TCHAR* Side,const FTransform& Wrist);
    void Leg(const TCHAR* Side,float Sign,float Phase);
    FDiveLegPose LegPose(const TCHAR* Side,float Sign,const FDivePoseState& Pose,const FTransform& Body) const;
    void UpdateHose();
};

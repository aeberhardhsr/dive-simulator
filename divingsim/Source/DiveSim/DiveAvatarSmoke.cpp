#include "DiveHUD.h"
#include "DivePawn.h"
#include "DiveEquipment.h"
#include "DiveAvatar.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "GameFramework/PlayerController.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "StaticMeshResources.h"

bool ADiveHUD::AvatarSmokeTest(int phase)
{
    Diver=Cast<ADivePawn>(GetOwningPawn());
    if(!Diver||!Diver->Equipment||!Diver->Equipment->Avatar) return false;
    auto* avatar=Diver->Equipment->Avatar.Get();
    const int test=Diver->LoadoutTestCase;
    auto shot=[&](const TCHAR* stage) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/
        FString::Printf(TEXT("Screenshots/Avatar_%d_%s.png"),test,stage),true,false); };
    if(phase==0)
    {
        for(int i=0;i<test%3;++i) NotifyHitBoxClick(TEXT("tank"));
        if(test>=3) NotifyHitBoxClick(TEXT("cylinder_material"));
        for(int i=0;i<test%3;++i) NotifyHitBoxClick(TEXT("bcd"));
        if(test%2) NotifyHitBoxClick(TEXT("suit"));
        if(test==5) NotifyHitBoxClick(TEXT("shore"));
        Diver->bEnglish=(test%2)!=0;
        NotifyHitBoxClick(TEXT("loadout"));NotifyHitBoxClick(TEXT("avatar_view"));
        return bLoadoutPage&&bAvatarView;
    }
    FString error;if(!avatar->Validate(error)) { UE_LOG(LogTemp,Error,TEXT("DIVE_AVATAR_FAIL %s"),*error);return false; }
    if(phase==1)
    {
        const auto& c=Diver->PendingConfig;
        const FString body=c.DrySuit?TEXT("SK_DiverSuit"):TEXT("SK_DiverWet");
        const TCHAR* bcd[]={TEXT("SM_BCDJacket"),TEXT("SM_BCDTravel"),TEXT("SM_BCDWing")};
        const FString cylinder=FString::Printf(TEXT("SM_Cylinder%s%d"),c.CylinderMaterial?TEXT("Aluminium"):TEXT("Steel"),FMath::RoundToInt(c.CylinderLitres));
        bool foundBCD=false,foundCylinder=false,hasColors=true;TArray<UStaticMeshComponent*> meshes;avatar->GetComponents(meshes);
        for(auto* mesh:meshes) if(mesh->GetStaticMesh())
        {
            UStaticMesh* asset=mesh->GetStaticMesh();
            if(!asset->GetPathName().StartsWith(TEXT("/Game/Diving/Avatar/"))) continue;
            foundBCD|=asset->GetName()==bcd[c.BCDModel];foundCylinder|=asset->GetName()==cylinder;
            const auto& colors=asset->GetRenderData()->LODResources[0].VertexBuffers.ColorVertexBuffer;
            hasColors&=colors.GetNumVertices()>0;
            UE_LOG(LogTemp,Display,TEXT("DIVE_AVATAR_MESH name=%s bounds=%s colors=%u"),*asset->GetName(),*asset->GetBoundingBox().ToString(),colors.GetNumVertices());
#if WITH_EDITOR
            if(colors.GetNumVertices()) UE_LOG(LogTemp,Display,TEXT("DIVE_AVATAR_COLOR %s %s"),*asset->GetName(),*colors.VertexColor(0).ToString());
#endif
        }
        shot(TEXT("Front"));
        const auto* skin=Cast<USkeletalMesh>(avatar->Suit->GetSkinnedAsset());
        return avatar->IsInspection()&&avatar->InspectionImage()&&skin&&skin->GetName()==body&&skin->GetHasVertexColors()&&hasColors&&foundBCD&&foundCylinder;
    }
    if(phase==2) for(int i=0;i<4;++i) NotifyHitBoxClick(TEXT("avatar_right"));
    if(phase==3) shot(TEXT("Back"));
    if(phase==4) { for(int i=0;i<4;++i) NotifyHitBoxClick(TEXT("avatar_left"));NotifyHitBoxClick(TEXT("avatar_pose")); }
    if(phase==5) { shot(TEXT("Swimming"));return avatar->bPreviewSwimming&&avatar->SwimBlend>.95; }
    if(phase==6) { NotifyHitBoxClick(TEXT("avatar_zoom"));NotifyHitBoxClick(TEXT("avatar_pose")); }
    if(phase==7) { shot(TEXT("Detail"));return avatar->bInspectionClose&&avatar->SwimBlend<.05; }
    if(phase==8) { NotifyHitBoxClick(TEXT("start"));return Diver->bDiveActive; }
    if(phase==9) { shot(TEXT("FirstPerson"));return !avatar->IsInspection()&&avatar->MotionPhase>0; }
    if(phase==10) { Diver->bShowComputer=true; }
    if(phase==11) shot(TEXT("Computer"));
    if(phase==12)
    {
        Diver->SetMenu(true);avatar->bInspectionClose=false;
        AvatarPausedUpdates=avatar->PoseUpdates;AvatarPausedPhase=avatar->MotionPhase;
    }
    if(phase==13)
    {
        shot(TEXT("Paused"));
        return avatar->IsInspection()&&avatar->PoseUpdates==AvatarPausedUpdates&&avatar->MotionPhase==AvatarPausedPhase;
    }
    if(phase==14)
    {
        NotifyHitBoxClick(TEXT("avatar_view"));
        Diver->ResumeDive();return !bAvatarView;
    }
    if(phase==15)
    {
        Diver->bShowComputer=false;Diver->bShowInstruments=false;
        if(auto* pc=Cast<APlayerController>(Diver->GetController()))
            pc->SetControlRotation(FRotator(-65,pc->GetControlRotation().Yaw,0));
    }
    if(phase==16) { shot(TEXT("LookDown"));return !avatar->IsInspection()&&avatar->PoseUpdates>AvatarPausedUpdates; }
    return true;
}

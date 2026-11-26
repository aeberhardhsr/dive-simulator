#include "DiveWorld.h"
#include "DivePawn.h"
#include "DiveEquipment.h"
#include "DiveAvatar.h"
#include "Camera/CameraComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Simulation/BedDistribution.h"
#include "Simulation/Ecology.h"
#include "Materials/MaterialInterface.h"

// Fixed comparison scene in surveyed water. Variant zero is the previous mesh;
// the three new forms use the same position, scale, material and camera.
bool ADiveWorld::SetRockInspection(int32 Variant,int32 View,FVector& Eye,FRotator& Rotation)
{
    const bool plantView=FParse::Param(FCommandLine::Get(),TEXT("DivePlantView"));
    if(!bOpticsSmoke||!Diver||Variant<0||Variant>(plantView?5:3)) return false;
    const bool gravelView=FParse::Param(FCommandLine::Get(),TEXT("DiveGravelView"));
    FVector centre=Diver->EntryLocation;bool found=false;
    for(int radius=5;radius<100&&!found;radius+=5)
        for(int angle=0;angle<360&&!found;angle+=30)
        {
            const FVector p=Diver->EntryLocation+FRotator(0,angle,0).Vector()*radius*100;
            float ground=0,north=0,east=0;
            if(Terrain.Sample(p.X,p.Y,ground)&&Terrain.Sample(p.X+200,p.Y,north)&&Terrain.Sample(p.X,p.Y+200,east)&&
                ground<-450&&ground>-750&&FMath::Abs(ground-north)<50&&FMath::Abs(ground-east)<50)
            {
                if(gravelView)
                {
                    if(Dive::BedDistribution::Gravel(p.X*.01,p.Y*.01,-ground*.01)<.65) continue;
                    bool blocked=false;for(const auto& rock:RockState)
                        if(FVector::DistSquared2D(rock.Position,p)<FMath::Square(rock.Radius+150)) {blocked=true;break;}
                    if(blocked) continue;
                }
                centre=p;centre.Z=ground+15;found=true;
            }
        }
    if(!found) return false;
    if(gravelView)
    {
        StreamPopulation(centre);
        StreamGravel(centre,true);
        for(const auto& pair:GravelCells)
            for(const auto& component:pair.Value.Components) component->SetVisibility(Variant!=0);
        Eye=centre+(View?FVector(-250,-500,220):FVector(0,-180,120));
        Rotation=(centre-Eye).Rotation();return true;
    }
    for(const auto& pair:PopulationCells)
        for(const auto& component:pair.Value.Components) component->SetVisibility(false);
    for(const auto& pair:GravelCells)
        for(const auto& component:pair.Value.Components) component->SetVisibility(false);
    for(const auto& component:Fish) component->SetVisibility(false);
    if(plantView)
    {
        for(const auto& plant:Plants) plant->ClearInstances();
        const int species=Variant/2;const bool revised=Variant%2!=0;
        const FString name=UTF8_TO_TCHAR(Dive::PlantProfiles[species].Mesh);
        const FString folder=revised?TEXT("Plants"):TEXT("Meshes");
        auto* mesh=LoadObject<UStaticMesh>(nullptr,*(TEXT("/Game/Diving/")+folder+TEXT("/")+name+TEXT(".")+name));
        if(!mesh) return false;
        auto* plant=Plants[species].Get();plant->SetStaticMesh(mesh);
        const FString materialName=revised?TEXT("M_AquaticPlant"):TEXT("M_Plant");
        auto* material=LoadObject<UMaterialInterface>(nullptr,*(TEXT("/Game/Diving/Materials/")+materialName+TEXT(".")+materialName));
        if(!material) return false;
        plant->SetMaterial(0,material);
        const FVector root=centre-FVector(0,0,13);
        plant->AddInstance(FTransform(FRotator(0,35,0),root));
        const double h=Dive::PlantProfiles[species].MaxHeight*100;
        Eye=root+(View?FVector(190,-320,h*.8):FVector(0,-150,h*.55));
        Rotation=(root+FVector(0,0,h*.45)-Eye).Rotation();return true;
    }
    auto* component=Rocks[0].Get();
    const FString path=Variant?FString::Printf(TEXT("/Game/Diving/Rocks/SM_LakeRock%d.SM_LakeRock%d"),Variant,Variant):TEXT("/Game/Diving/Meshes/SM_Rock.SM_Rock");
    auto* mesh=LoadObject<UStaticMesh>(nullptr,*path);
    if(!mesh||!mesh->GetBodySetup()||mesh->GetBodySetup()->AggGeom.GetElementCount()<1) return false;
    component->SetStaticMesh(mesh);component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    const FTransform transform(FRotator(0,35,0),centre,FVector(1,.85,.7));
    if(component->GetInstanceCount()==0) component->AddInstance(transform);
    else component->UpdateInstanceTransform(0,transform,false,true,true);
    Eye=centre+(View?FVector(240,-340,150):FVector(0,-380,110));
    Rotation=(centre+FVector(0,0,5)-Eye).Rotation();
    return true;
}

void ADivePawn::TickRockInspection(float Dt)
{
    const bool gravelView=FParse::Param(FCommandLine::Get(),TEXT("DiveGravelView"));
    const bool plantView=FParse::Param(FCommandLine::Get(),TEXT("DivePlantView"));
    ControlsSmokeClock+=Dt;
    if(ControlsSmokePhase==0)
    {
        if(ControlsSmokeClock<4) return;
        Environment->SetActorTickEnabled(false);bLamp=false;bShowInstruments=false;bShowComputer=false;bShowMap=false;NoticeUntil=0;
        Equipment->SetActorHiddenInGame(true);if(Equipment->Avatar) Equipment->Avatar->SetActorHiddenInGame(true);
        if(auto* pc=Cast<APlayerController>(GetController())) if(pc->GetHUD()) pc->GetHUD()->bShowHUD=false;
        ControlsSmokePhase=1;ControlsSmokeClock=0;TestLessonStage=0;
    }
    if(ControlsSmokePhase>(plantView?12:gravelView?4:8))
    {
        if(plantView) {UE_LOG(LogTemp,Display,TEXT("DIVE_PLANT_INSPECTION_PASS groups=3 states=2 views=2 captures=12"));}
        else if(gravelView) {UE_LOG(LogTemp,Display,TEXT("DIVE_GRAVEL_INSPECTION_PASS states=2 views=2 captures=4"));}
        else {UE_LOG(LogTemp,Display,TEXT("DIVE_ROCK_INSPECTION_PASS forms=4 views=2 captures=8"));}
        UE_LOG(LogTemp,Display,TEXT("DIVE_SMOKE_PASS rocks=1"));FPlatformMisc::RequestExitWithStatus(false,0);return;
    }
    const int variant=(ControlsSmokePhase-1)/2,view=(ControlsSmokePhase-1)%2;
    FVector eye;FRotator rotation;
    if(!Environment->SetRockInspection(variant,view,eye,rotation))
    {UE_LOG(LogTemp,Error,TEXT("DIVE_ROCK_INSPECTION_FAIL phase=%d"),ControlsSmokePhase);FPlatformMisc::RequestExitWithStatus(false,1);return;}
    SetActorLocation(eye-FVector(0,0,CameraBaseHeight));ResetContactPose();
    if(auto* pc=Cast<APlayerController>(GetController())) pc->SetControlRotation(rotation);
    Camera->SetWorldLocationAndRotation(eye,rotation);Camera->SetFieldOfView(50);
    if(ControlsSmokeClock>3&&!TestLessonStage)
    {
        TestLessonStage=1;
        const FString name=FString::Printf(TEXT("%s_%d_%d.png"),plantView?TEXT("Plant"):gravelView?TEXT("Gravel"):TEXT("Rock"),variant,view);
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots")/name,true,false);
        UE_LOG(LogTemp,Display,TEXT("DIVE_ROCK_CAPTURE variant=%d view=%d file=%s"),variant,view,*name);
    }
    if(ControlsSmokeClock>4){++ControlsSmokePhase;ControlsSmokeClock=0;TestLessonStage=0;}
}

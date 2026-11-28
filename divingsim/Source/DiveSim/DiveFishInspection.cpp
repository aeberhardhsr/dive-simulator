#include "DiveWorld.h"
#include "DivePawn.h"
#include "DiveEquipment.h"
#include "DiveAvatar.h"
#include "Simulation/Ecology.h"
#include "Camera/CameraComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

// Controlled close-up fixture using the ordinary population components, meshes,
// per-instance animation data and materials. It does not exercise natural AI.
bool ADiveWorld::SetFishInspection(int32 Species,int32 View,float Cycle,FVector& Eye,FRotator& Rotation,bool FinOnly)
{
    if(!bOpticsSmoke||!Diver||!Fish.IsValidIndex(Species)) return false;
    FVector centre=Diver->EntryLocation;bool found=false;
    for(int radius=0;radius<=800&&!found;radius+=40)
        for(int a=0;a<360&&!found;a+=30)
        {
            const FVector p=centre+FRotator(0,a,0).Vector()*radius*100;float ground=0;
            if(Terrain.Sample(p.X,p.Y,ground)&&ground<-750) { centre=p;centre.Z=-450;found=true; }
        }
    if(!found) return false;
    const auto& profile=Dive::FishProfiles[Species];
    const FRotator bodyRotation=FinOnly&&View>=2?FRotator(25,60,0):FRotator::ZeroRotator;
    Eye=centre+bodyRotation.RotateVector(FVector(View%2?.6:0,View%2?-.8:-1,.04))*profile.LengthMetres*160;
    Rotation=(centre-Eye).Rotation();
    if(Cycle<.15) StreamTerrain(Eye,true);
    for(int32 i=0;i<Fish.Num();++i) Fish[i]->SetVisibility(i==Species);
    auto* component=Fish[Species].Get();
    component->SetForcedLodModel(View+1);
    for(int32 i=1;i<component->GetInstanceCount();++i)
        component->UpdateInstanceTransform(i,FTransform(FRotator::ZeroRotator,centre,FVector::ZeroVector),false,false,true);
    component->UpdateInstanceTransform(0,FTransform(bodyRotation,centre,FVector::OneVector),false,false,true);
    const auto right=FRotationMatrix(bodyRotation).GetUnitAxis(EAxis::Y),up=FRotationMatrix(bodyRotation).GetUnitAxis(EAxis::Z);
    const float values[]={FinOnly?0:Cycle,float(right.X),float(right.Y),0,float(profile.LengthMetres*7),float(right.Z),float(up.X),float(up.Y),float(up.Z),FinOnly?(Cycle<4?float(-PI/2):float(PI/2)):Cycle*.7f,1,FinOnly&&View==3?.7f:0,float(profile.LengthMetres*100)};
    for(int i=0;i<13;++i) component->SetCustomDataValue(0,i,values[i],false);
    component->MarkRenderStateDirty();
    const auto mesh=component->GetStaticMesh();const auto* material=component->GetMaterial(0);
#if WITH_EDITOR
    if(Cycle<1&&mesh&&mesh->GetRenderData())
    {
        const auto& lod=mesh->GetRenderData()->LODResources[0];const auto& colors=lod.VertexBuffers.ColorVertexBuffer;
        int tinted=0,low=255,high=0;
        for(uint32 i=0;i<colors.GetNumVertices();++i) { const auto c=colors.VertexColor(i);tinted+=c.R!=c.G||c.G!=c.B;low=FMath::Min(low,int(c.A));high=FMath::Max(high,int(c.A)); }
        UE_LOG(LogTemp,Display,TEXT("DIVE_FISH_MESH species=%d lods=%d triangles=%u colors=%u tinted=%d alpha=%d..%d"),Species,mesh->GetNumLODs(),lod.GetNumTriangles(),colors.GetNumVertices(),tinted,low,high);
        if(tinted<100||low!=0||high<180) return false;
        const auto& buffer=lod.VertexBuffers.StaticMeshVertexBuffer;
        UE_LOG(LogTemp,Display,TEXT("DIVE_FIN_CHANNELS count=%u lightmap=%d"),buffer.GetNumTexCoords(),mesh->GetLightMapCoordinateIndex());
        if(buffer.GetNumTexCoords()<3) return false;
        int roots=0,tips=0,left=0,rightCount=0;
        for(uint32 i=0;i<buffer.GetNumVertices();++i)
        {
            const auto offset=buffer.GetVertexUV(i,1),rig=buffer.GetVertexUV(i,2);const int kind=FMath::RoundToInt(rig.X*8);
            if(kind<0||kind>6) { UE_LOG(LogTemp,Error,TEXT("DIVE_FIN_UV_FAIL index=%u offset=%s rig=%s kind=%d"),i,*offset.ToString(),*rig.ToString(),kind);return false; }
            if(kind) { const double d=FMath::Sqrt(FMath::Square(offset.X-.5)+FMath::Square(offset.Y-.5));roots+=d<.00001;tips+=d>.015; }
            left+=kind==3;rightCount+=kind==4;
        }
        UE_LOG(LogTemp,Display,TEXT("DIVE_FIN_RIG species=%d uv=%u roots=%d tips=%d paired=%d/%d"),Species,buffer.GetNumTexCoords(),roots,tips,left,rightCount);
        if(roots<100||tips<100||left<100||rightCount<100) return false;
    }
#endif
    return mesh&&mesh->GetNumLODs()==4&&material&&material->GetName()==TEXT("MI_Fish_")+mesh->GetName().RightChop(3);
}

void ADivePawn::TickFishInspection(float Dt)
{
    const bool finOnly=FParse::Param(FCommandLine::Get(),TEXT("DiveFinInspection"));
    ControlsSmokeClock+=Dt;
    auto fail=[&]() { UE_LOG(LogTemp,Error,TEXT("DIVE_FISH_INSPECTION_FAIL phase=%d"),ControlsSmokePhase);FPlatformMisc::RequestExitWithStatus(false,1); };
    if(ControlsSmokePhase==0)
    {
        if(ControlsSmokeClock<4) return;
        Environment->SetActorTickEnabled(false);bLamp=false;bShowInstruments=false;bShowComputer=false;bShowMap=false;NoticeUntil=0;
        Equipment->SetActorHiddenInGame(true);if(Equipment->Avatar) Equipment->Avatar->SetActorHiddenInGame(true);
        if(auto* pc=Cast<APlayerController>(GetController())) if(pc->GetHUD()) pc->GetHUD()->bShowHUD=false;
        ControlsSmokePhase=1;ControlsSmokeClock=0;TestLessonStage=0;
    }
    if(ControlsSmokePhase>20)
    {
        UE_LOG(LogTemp,Display,TEXT("DIVE_FISH_INSPECTION_PASS species=5 views=20 captures=40 lods=4"));
        if(finOnly) UE_LOG(LogTemp,Display,TEXT("DIVE_FIN_INSPECTION_PASS species=5 views=20 captures=40 rotated=10 fixed_tail=1"));
        UE_LOG(LogTemp,Display,TEXT("DIVE_SMOKE_PASS fish=1"));FPlatformMisc::RequestExitWithStatus(false,0);return;
    }
    const int species=(ControlsSmokePhase-1)/4,view=(ControlsSmokePhase-1)%4;
    FVector eye;FRotator rotation;
    if(!Environment->SetFishInspection(species,view,finOnly?ControlsSmokeClock:ControlsSmokeClock*float(Dive::FishProfiles[species].TailHz)*2*PI,eye,rotation,finOnly)) { fail();return; }
    SetActorLocation(eye-FVector(0,0,CameraBaseHeight));ResetContactPose();
    if(auto* pc=Cast<APlayerController>(GetController())) pc->SetControlRotation(rotation);
    Camera->SetWorldLocationAndRotation(eye,rotation);Camera->SetFieldOfView(50);
    const int capture=ControlsSmokeClock>4.3?2:ControlsSmokeClock>3?1:0;
    if(capture>TestLessonStage)
    {
        TestLessonStage=capture;
        const FString name=FString::Printf(TEXT("%s_%s_%d_%d.png"),finOnly?TEXT("Fin"):TEXT("Fish"),UTF8_TO_TCHAR(Dive::FishProfiles[species].Mesh+3),view,capture);
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots")/name,true,false);
        UE_LOG(LogTemp,Display,TEXT("DIVE_FISH_CAPTURE species=%d view=%d frame=%d file=%s"),species,view,capture,*name);
    }
    if(ControlsSmokeClock>5) { ++ControlsSmokePhase;ControlsSmokeClock=0;TestLessonStage=0; }
}

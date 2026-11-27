#include "DiveWorld.h"
#include "DivePawn.h"
#include "DiveEquipment.h"
#include "DiveAvatar.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "PhysicsEngine/BodySetup.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "String/LexFromString.h"
#include "Simulation/BedDistribution.h"
#include "Simulation/Ecology.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "ProceduralMeshComponent.h"

// Fixed comparison scene in surveyed water. Variant zero is the previous mesh;
// the three new forms use the same position, scale and camera.
UMaterialInterface* ADiveWorld::RockSurfaceMaterial(int32 Variant) const
{
    const bool legacy=Variant<0||(bOpticsSmoke&&FParse::Param(FCommandLine::Get(),TEXT("DiveLegacyRockSurface")));
    const FString name=legacy?TEXT("M_RockSurface"):FString::Printf(TEXT("M_RockScan%d"),Variant+1);
    return LoadObject<UMaterialInterface>(nullptr,*(TEXT("/Game/Diving/Materials/")+name+TEXT(".")+name));
}

void ADiveWorld::RefreshInspectionMedium()
{
    if(bOpticsSmoke)
    {
        if(!FParse::Param(FCommandLine::Get(),TEXT("DiveWaterView"))) UpdateAtmosphere();
        UpdateCaustics();
        if(bBoundedWater&&Diver) UpdateWaterVolume(Diver->GetPawnViewLocation());
    }
}

bool ADiveWorld::InspectionChecksReady() const
{
    return bVegetationChecked&&(!FParse::Param(FCommandLine::Get(),TEXT("DivePopulationChecks"))||bPopulationChecked);
}

bool ADiveWorld::SetRockInspection(int32 Variant,int32 View,FVector& Eye,FRotator& Rotation)
{
    const bool plantView=FParse::Param(FCommandLine::Get(),TEXT("DivePlantView"));
    const bool waterView=FParse::Param(FCommandLine::Get(),TEXT("DiveWaterView"));
    if(!bOpticsSmoke||!Diver||Variant<0||Variant>(waterView?9:plantView?5:Rocks.Num())) return false;
    if(FParse::Param(FCommandLine::Get(),TEXT("DiveHabitatView")))
    {
        FString requestedPose;
        if(FParse::Value(FCommandLine::Get(),TEXT("DiveHabitatPose="),requestedPose,false))
        {
            TArray<FString> fields;requestedPose.ParseIntoArray(fields,TEXT(","),false);
            double values[5];
            if(fields.Num()!=5) return false;
            for(int i=0;i<5;++i)
                if(!LexTryParseString(values[i],*fields[i])||!FMath::IsFinite(values[i])) return false;
            if(FMath::Abs(values[0])>2.e7||FMath::Abs(values[1])>2.e7||
                values[2]>-100||values[2]<-6000||FMath::Abs(values[3])>360||FMath::Abs(values[4])>75) return false;
            Eye=FVector(values[0],values[1],values[2]);
            float measured=0;
            if(!Terrain.SampleMeasured(Eye.X,Eye.Y,measured)||Eye.Z-measured<100||Eye.Z-measured>600) return false;
            Rotation=FRotator(values[4],values[3]+(View-1)*35,0);
            // Inspection ticking is frozen. Load the actual terrain/population
            // at the requested survey point and recenter the resolved waves.
            StreamTerrain(Eye,true);StreamPopulation(Eye,true);UpdateWater(Eye);UpdateAtmosphere();
            return true;
        }
        if(FParse::Param(FCommandLine::Get(),TEXT("DiveHabitatClose")))
        {
            // Stable close views of actual outcrop feet. The comparator changes
            // gravel dressing only; rock layout and camera selection are shared.
            auto ordered=RockState;
            ordered.Sort([](const FDiveRock& A,const FDiveRock& B)
            {return A.Position.X==B.Position.X?A.Position.Y<B.Position.Y:A.Position.X<B.Position.X;});
            const FVector direction=FRotator(0,View*120+30,0).Vector();
            for(const auto& rock:ordered)
            {
                if(rock.Radius<180||FVector::DistSquared2D(rock.Position,Diver->EntryLocation)>FMath::Square(3000.)) continue;
                const FVector proposed=rock.Position+direction*(rock.Radius+350);
                const FVector foot=rock.Position+direction*(rock.Radius*.75);
                float bed,footBed;
                if(!Terrain.Sample(proposed.X,proposed.Y,bed)||!Terrain.Sample(foot.X,foot.Y,footBed)||bed>-350||bed<-1600) continue;
                bool blocked=false;
                for(const auto& other:ordered)
                    if(FVector::DistSquared2D(proposed,other.Position)<FMath::Square(other.Radius+75)) {blocked=true;break;}
                if(blocked) continue;
                Eye=FVector(proposed.X,proposed.Y,bed+130);
                Rotation=(FVector(foot.X,foot.Y,footBed+15)-Eye).Rotation();
                StreamPopulation(Eye);StreamGravel(Eye,true);UpdateAtmosphere();return true;
            }
            return false;
        }
        // Camera selection depends only on surveyed terrain, never population.
        // Retain exactly the same viewpoints across distribution revisions.
        // Murg has no shallow flat patch within this radius. A second pass
        // permits the surveyed bank instead of silently omitting that entry.
        for(int pass=0;pass<2;++pass)
        for(int radius=pass?5:20;radius<=80;radius+=5) for(int angle=0;angle<360;angle+=30)
        {
            const FVector p=Diver->EntryLocation+FRotator(0,angle,0).Vector()*radius*100;
            float bed=0,north=0,east=0;
            if(!Terrain.SampleMeasured(p.X,p.Y,bed)||!Terrain.SampleMeasured(p.X+200,p.Y,north)||!Terrain.SampleMeasured(p.X,p.Y+200,east)||
                bed>-550||bed<(pass?-2200:-1100)||FMath::Abs(north-bed)>(pass?300:50)||FMath::Abs(east-bed)>(pass?300:50)) continue;
            const bool low=FParse::Param(FCommandLine::Get(),TEXT("DiveHabitatLowView"));
            Eye=FVector(p.X,p.Y,bed+(low?130:300));
            Rotation=FRotator(low?-12:-18,angle+(View-1)*50,0);
            if(pass)
            {
                const FVector ahead=p+FRotator(0,Rotation.Yaw,0).Vector()*800;
                float aheadBed=0;
                if(Terrain.SampleMeasured(ahead.X,ahead.Y,aheadBed))
                    Rotation.Pitch=FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan2(aheadBed+60-Eye.Z,800.)),-65.,low?-12.:-18.);
            }
            if(FParse::Param(FCommandLine::Get(),TEXT("DiveHabitatSunView"))&&Sun)
            {
                const FRotator towardSun=(-Sun->GetForwardVector()).Rotation();
                Rotation=FRotator(View==1?FMath::Clamp(towardSun.Pitch,-75.,75.):View==0?12:0,towardSun.Yaw+(View==2?180:0),0);
            }
            StreamPopulation(Eye);StreamGravel(Eye,true);UpdateAtmosphere();
            return true;
        }
        return false;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("DiveWaterView")))
    {
        if(!WaterMaterial||Variant>9||View>2) return false;
        // Diagnostic only. Bits 0/1/2 disable the surface water volume, use air
        // fog, and omit camera attenuation. States 8/9 are clear-air controls.
        // The old entry marker occupied the exact spawn XY; an eye inside its
        // two-sided rope sees an opaque solid colour rather than the surface.
        // Pick surveyed water away from every rope before testing the surface.
        bool clear=false;
        for(int angle=0;angle<360&&!clear;angle+=30)
        {
            Eye=Diver->EntryLocation+FRotator(0,angle,0).Vector()*500;
            float bed=0;if(!Terrain.Sample(Eye.X,Eye.Y,bed)||bed>-350) continue;
            clear=true;
            for(const auto& rope:BuoyLines)
                if(FVector::DistSquared2D(Eye,rope->GetComponentLocation())<FMath::Square(200.)) {clear=false;break;}
        }
        if(!clear) return false;
        Eye.Z=View==2?150:-150;
        Rotation=FRotator(View==0?15:View==1?65:-15,180,0);
        UpdateAtmosphere();
        Fog->SetVisibility(Variant<8);
        Water->SetVisibility(Variant!=8);
        WaterMaterial->SetScalarParameterValue(TEXT("OpticalScale"),
            (Variant&1)&&View!=2?0.f:14.f/VisibleDistance);
        if((Variant&2)&&View!=2)
        {
            // Isolate the global underwater fog that also covers the sky in
            // the scene-color buffer behind the surface. Post-process stays on.
            Fog->SetFogDensity(Dive::WeatherScenario::At(WeatherSettings.Weather).AirFog);
            Fog->SetFogHeightFalloff(.2f);
            Fog->SetFogInscatteringColor(FLinearColor::Black);
            Fog->SetSkyAtmosphereAmbientContributionColorScale(FLinearColor::White);
        }
        return true;
    }
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
    auto* material=RockSurfaceMaterial(Variant-1);
    if(!material) return false;
    component->SetMaterial(0,material);
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
    const bool waterView=FParse::Param(FCommandLine::Get(),TEXT("DiveWaterView"));
    const bool habitatView=FParse::Param(FCommandLine::Get(),TEXT("DiveHabitatView"));
    const bool formationView=FParse::Param(FCommandLine::Get(),TEXT("DiveFormationView"));
    const int views=(waterView||habitatView)?3:2;
    int end=9;if(waterView) FParse::Value(FCommandLine::Get(),TEXT("DiveWaterEnd="),end);
    end=FMath::Clamp(end,0,9);
    ControlsSmokeClock+=Dt;
    if(ControlsSmokePhase==0)
    {
        if(ControlsSmokeClock<4) return;
        // World time clamps long frames; wall-clock warmup alone can finish
        // before its checks on slow raster starts. Never freeze them out.
        if(!Environment->InspectionChecksReady())
        {
            if(ControlsSmokeClock>30)
            {UE_LOG(LogTemp,Error,TEXT("DIVE_ROCK_INSPECTION_FAIL world checks did not complete"));FPlatformMisc::RequestExitWithStatus(false,1);}
            return;
        }
        Environment->SetActorTickEnabled(false);bLamp=false;bShowInstruments=false;bShowComputer=false;bShowMap=false;NoticeUntil=0;
        Equipment->SetActorHiddenInGame(true);if(Equipment->Avatar) Equipment->Avatar->SetActorHiddenInGame(true);
        if(auto* pc=Cast<APlayerController>(GetController())) if(pc->GetHUD()) pc->GetHUD()->bShowHUD=false;
        int start=0;if(waterView) FParse::Value(FCommandLine::Get(),TEXT("DiveWaterStart="),start);
        if(formationView) start=4;
        ControlsSmokePhase=1+FMath::Clamp(start,0,9)*views;ControlsSmokeClock=0;TestLessonStage=0;
    }
    if(ControlsSmokePhase>(habitatView?3:waterView?(end+1)*views:plantView||formationView?12:gravelView?4:8))
    {
        if(habitatView) {UE_LOG(LogTemp,Display,TEXT("DIVE_HABITAT_INSPECTION_PASS views=3"));}
        else if(waterView) {UE_LOG(LogTemp,Display,TEXT("DIVE_WATER_INSPECTION_PASS final_state=%d views=3 diagnostic=1"),end);}
        else if(formationView) {UE_LOG(LogTemp,Display,TEXT("DIVE_FORMATION_INSPECTION_PASS variants=2 views=2 captures=4"));}
        else if(plantView) {UE_LOG(LogTemp,Display,TEXT("DIVE_PLANT_INSPECTION_PASS groups=3 states=2 views=2 captures=12"));}
        else if(gravelView) {UE_LOG(LogTemp,Display,TEXT("DIVE_GRAVEL_INSPECTION_PASS states=2 views=2 captures=4"));}
        else {UE_LOG(LogTemp,Display,TEXT("DIVE_ROCK_INSPECTION_PASS forms=4 views=2 captures=8"));}
        UE_LOG(LogTemp,Display,TEXT("DIVE_SMOKE_PASS rocks=1"));FPlatformMisc::RequestExitWithStatus(false,0);return;
    }
    const int variant=(ControlsSmokePhase-1)/views,view=(ControlsSmokePhase-1)%views;
    FVector eye;FRotator rotation;
    if(!Environment->SetRockInspection(variant,view,eye,rotation))
    {UE_LOG(LogTemp,Error,TEXT("DIVE_ROCK_INSPECTION_FAIL phase=%d"),ControlsSmokePhase);FPlatformMisc::RequestExitWithStatus(false,1);return;}
    SetActorLocation(eye-FVector(0,0,CameraBaseHeight));ResetContactPose();
    if(auto* pc=Cast<APlayerController>(GetController())) pc->SetControlRotation(rotation);
    Camera->SetWorldLocationAndRotation(eye,rotation);Camera->SetFieldOfView(habitatView?75:waterView?70:50);
    // The fixture freezes world ticking. Refresh the medium after moving the
    // camera, including plant/rock close-ups that previously retained entry fog.
    // WaterView manages deliberate fog overrides inside SetRockInspection.
    Environment->RefreshInspectionMedium();
    if(waterView) Camera->PostProcessSettings.AutoExposureBias=Environment->ExposureBiasAt(eye);
    if(waterView&&((variant&4)||variant>=8)) Camera->PostProcessSettings.WeightedBlendables.Array.Reset();
    if(waterView&&variant>=8)
    {
        Camera->PostProcessSettings.WeightedBlendables.Array.Reset();
        Camera->PostProcessSettings.SceneColorTint=FLinearColor::White;
        Camera->PostProcessSettings.ColorSaturation=FVector4(1,1,1,1);
        Camera->PostProcessSettings.VignetteIntensity=0;
    }
    Equipment->SetActorHiddenInGame(true);if(Equipment->Avatar) Equipment->Avatar->SetActorHiddenInGame(true);
    if(ControlsSmokeClock>3&&!TestLessonStage)
    {
        TestLessonStage=1;
        const FString name=FString::Printf(TEXT("%s_%d_%d.png"),habitatView?TEXT("Habitat"):waterView?TEXT("Water"):plantView?TEXT("Plant"):gravelView?TEXT("Gravel"):TEXT("Rock"),variant,view);
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots")/name,true,false);
        UE_LOG(LogTemp,Display,TEXT("DIVE_ROCK_CAPTURE variant=%d view=%d file=%s"),variant,view,*name);
        if(habitatView) UE_LOG(LogTemp,Display,TEXT("DIVE_HABITAT_VIEW view=%d eye=%s rotation=%s"),view,*eye.ToString(),*rotation.ToString());
        if(habitatView)
        {
            FString requestedPose;
            if(FParse::Value(FCommandLine::Get(),TEXT("DiveHabitatPose="),requestedPose,false))
            {
                float measured=0;const bool sampled=Environment->Terrain.SampleMeasured(eye.X,eye.Y,measured);
                FCollisionQueryParams query(SCENE_QUERY_STAT(HabitatPose),false,this);
                if(Equipment) query.AddIgnoredActor(Equipment);
                const bool blocked=GetWorld()->OverlapBlockingTestByChannel(eye,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(20),query);
                UE_LOG(LogTemp,Display,TEXT("DIVE_HABITAT_POSE view=%d measured=%d bed_cm=%.3f clearance_cm=%.3f camera_sphere_clear=%d radius_cm=20"),
                    view,sampled,measured,eye.Z-measured,!blocked);
            }
        }
        if(waterView) UE_LOG(LogTemp,Display,TEXT("DIVE_WATER_VIEW state=%d view=%d eye=%s rotation=%s exposure=%.6f visibility=%.3f"),
            variant,view,*eye.ToString(),*rotation.ToString(),Camera->PostProcessSettings.AutoExposureBias,Simulation.Settings.VisibilityMetres);
        if(waterView)
        {
            float bed=0;const bool sampled=Environment->Terrain.Sample(eye.X,eye.Y,bed);
            auto* pc=Cast<APlayerController>(GetController());
            if(pc)
            {
                const auto& pov=pc->PlayerCameraManager->GetCameraCacheView();
                UE_LOG(LogTemp,Display,TEXT("DIVE_WATER_CACHED_PP weight=%.3f materials=%d vignette=%.3f tint=%s"),
                    pov.PostProcessBlendWeight,pov.PostProcessSettings.WeightedBlendables.Array.Num(),pov.PostProcessSettings.VignetteIntensity,*pov.PostProcessSettings.SceneColorTint.ToString());
            }
            UE_LOG(LogTemp,Display,TEXT("DIVE_WATER_CAMERA actual=%s rotation=%s bed=%.3f sampled=%d pp_weight=%.3f materials=%d"),
                pc?*pc->PlayerCameraManager->GetCameraLocation().ToString():TEXT("missing"),
                pc?*pc->PlayerCameraManager->GetCameraRotation().ToString():TEXT("missing"),bed,sampled,Camera->PostProcessBlendWeight,
                Camera->PostProcessSettings.WeightedBlendables.Array.Num());
        }
    }
    if(ControlsSmokeClock>4){++ControlsSmokePhase;ControlsSmokeClock=0;TestLessonStage=0;}
}

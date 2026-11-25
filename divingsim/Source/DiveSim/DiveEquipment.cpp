#include "DiveEquipment.h"
#include "DivePawn.h"
#include "DiveAvatar.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "ProceduralMeshComponent.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Engine/StaticMesh.h"
#include "CanvasItem.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetRenderingLibrary.h"

namespace
{
const TCHAR* EquipmentNames[]={TEXT("SM_LeftSleeve"),TEXT("SM_LeftGlove"),TEXT("SM_WristComputer"),
    TEXT("SM_RightSleeve"),TEXT("SM_RightGlove"),TEXT("SM_DiveTorch")};
const TCHAR* TorchMeshes[]={TEXT("SM_DiveTorch"),TEXT("SM_TorchSpot"),TEXT("SM_TorchWide")};
FString Clock(double Seconds)
{ const int32 s=FMath::Max(0,FMath::FloorToInt(Seconds)); return FString::Printf(TEXT("%02d:%02d"),s/60,s%60); }
}

ADiveEquipment::ADiveEquipment()
{
    PrimaryActorTick.bCanEverTick=false;
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("EquipmentRoot"));
    LeftWrist=CreateDefaultSubobject<USceneComponent>(TEXT("LeftWrist")); LeftWrist->SetupAttachment(RootComponent);
    RightHand=CreateDefaultSubobject<USceneComponent>(TEXT("RightHand")); RightHand->SetupAttachment(RootComponent);
    for(int32 i=0;i<6;++i)
    {
        auto* mesh=CreateDefaultSubobject<UStaticMeshComponent>(EquipmentNames[i]);
        mesh->SetupAttachment(i<3?LeftWrist:RightHand);
        mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        mesh->SetCastShadow(false); Meshes.Add(mesh);
    }
    Screen=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("WristDisplay"));
    Screen->SetupAttachment(LeftWrist); Screen->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Screen->SetCastShadow(false);
}

void ADiveEquipment::Initialize(ADivePawn* Pawn)
{
    Diver=Pawn;
    AttachToComponent(Pawn->Camera,FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    for(int32 i=0;i<Meshes.Num();++i)
    {
        const FString name=EquipmentNames[i];
        Meshes[i]->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*(TEXT("/Game/Diving/Meshes/")+name+TEXT(".")+name)));
        const TCHAR* material=(i==0||i==3)?TEXT("M_Suit"):TEXT("M_Equipment");
        Meshes[i]->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,*(FString(TEXT("/Game/Diving/Materials/"))+material+TEXT(".")+material)));
    }
    SuitMaterial=UMaterialInstanceDynamic::Create(Meshes[0]->GetMaterial(0),this);
    Meshes[0]->SetMaterial(0,SuitMaterial); Meshes[3]->SetMaterial(0,SuitMaterial);
    // The continuous articulated suit supplies both full arms. Keep the old
    // assets loaded for compatibility checks, but never draw duplicate sleeves.
    Meshes[0]->SetVisibility(false);Meshes[3]->SetVisibility(false);
    Avatar=GetWorld()->SpawnActor<ADiveAvatar>();Avatar->Initialize(Pawn);
    for(int index:{1,2,4,5}) Meshes[index]->SetCastShadow(true);
    TArray<FVector> vertices{{2.5,-4.15,5.57},{2.5,4.15,5.57},{-3.5,4.15,5.57},{-3.5,-4.15,5.57}};
    TArray<int32> triangles{0,1,2,0,2,3};
    TArray<FVector> normals{{0,0,1},{0,0,1},{0,0,1},{0,0,1}};
    TArray<FVector2D> uv{{0,0},{1,0},{1,1},{0,1}};
    TArray<FLinearColor> colors{FLinearColor::White,FLinearColor::White,FLinearColor::White,FLinearColor::White};
    TArray<FProcMeshTangent> tangents;
    Screen->CreateMeshSection_LinearColor(0,vertices,triangles,normals,uv,colors,tangents,false);
    Display=UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(this,UCanvasRenderTarget2D::StaticClass(),768,512);
    Display->RenderTargetFormat=RTF_RGBA8;
    Display->Filter=TF_Trilinear;
    Display->bAutoGenerateMips=true;
    Display->InitAutoFormat(768,512);
    Display->ClearColor=FLinearColor::Black;
    Display->OnCanvasRenderTargetUpdate.AddDynamic(this,&ADiveEquipment::DrawDisplay);
    Font=LoadObject<UFont>(nullptr,TEXT("/Engine/EngineFonts/Roboto.Roboto"));
    if(auto* material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Diving/Materials/M_ComputerScreen.M_ComputerScreen")))
    {
        ScreenMaterial=UMaterialInstanceDynamic::Create(material,this);
        ScreenMaterial->SetTextureParameterValue(TEXT("DisplayTexture"),Display);
        Screen->SetMaterial(0,ScreenMaterial);
    }
    Pawn->Lamp->AttachToComponent(RightHand,FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    Pawn->Lamp->SetRelativeLocation(FVector(23,0,-3.8));
    Pawn->Lamp->SetRelativeRotation(FRotator::ZeroRotator);
    Update(0);
}

void ADiveEquipment::Update(float DeltaSeconds)
{
    if(!Diver) return;
    SetActorHiddenInGame((!Diver->bDiveActive||Diver->bMenu)&&!(Avatar&&Avatar->IsInspection()));
    if(TorchModel!=Diver->PendingConfig.TorchModel)
    {
        TorchModel=Diver->PendingConfig.TorchModel;
        const FString name=TorchMeshes[TorchModel];
        Meshes[5]->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*(TEXT("/Game/Diving/Meshes/")+name+TEXT(".")+name)));
    }
    if(!Diver->bDiveActive||Diver->bMenu)
    {
        if(Avatar) Avatar->UpdatePose(DeltaSeconds,LeftWrist,RightHand);
        if(!Diver->bDiveActive&&Avatar&&Avatar->IsInspection())
        {
            DisplayTimer+=DeltaSeconds;
            if(DisplayUpdates==0||DisplayTimer>=.1f)
            {
                DisplayTimer=0;DisplayState=Dive::Model(Diver->PendingConfig).Data;
                bEnglish=Diver->bEnglish;bUnlimited=Diver->PendingConfig.GameMode==Dive::Mode::Exploration;
                Page=0;Heading=0;Display->UpdateResource();
            }
        }
        return;
    }
    if(SuitMaterial) SuitMaterial->SetScalarParameterValue(TEXT("DrySuit"),Diver->PendingConfig.DrySuit?1.f:0.f);
    const float sleeveScale=Diver->PendingConfig.DrySuit?1.f:.9f;
    Meshes[0]->SetRelativeScale3D(FVector(1,sleeveScale,sleeveScale));
    Meshes[3]->SetRelativeScale3D(FVector(1,sleeveScale,sleeveScale));
    WristBlend=FMath::FInterpTo(WristBlend,Diver->bShowComputer?1.f:0.f,DeltaSeconds,8.f);
    const double phase=Diver->Simulation.Data.ElapsedSeconds;
    const float movement=Diver->VelocityMetres.Size();
    const float sway=Diver->bCameraMotion?FMath::Sin(phase*2.5)*FMath::Min(movement,1.f)*.8f:0;
    const float readingSwim=Avatar?FMath::Clamp(Avatar->SwimBlend/.45f,0.f,1.f):0;
    LeftWrist->SetRelativeLocation(FMath::Lerp(FVector(35,-24,-18),FVector(FMath::Lerp(28.f,26.f,readingSwim),-6,-6),WristBlend)+FVector(0,0,sway));
    LeftWrist->SetRelativeRotation(FMath::Lerp(FRotator(18,-8,0),FRotator(FMath::Lerp(75.f,60.f,readingSwim),-8,0),WristBlend));
    RightHand->SetRelativeLocation(FVector(29,24,-20+sway));
    RightHand->SetRelativeRotation(FRotator(8,-5,0));
    if(Avatar) Avatar->UpdatePose(DeltaSeconds,LeftWrist,RightHand);
    DisplayTimer+=DeltaSeconds;
    if(DisplayUpdates==0||DisplayTimer>=.1f||Page!=Diver->ComputerPage||bEnglish!=Diver->bEnglish)
    {
        DisplayTimer=0; DisplayState=Diver->Simulation.Data;
        bEnglish=Diver->bEnglish; bUnlimited=Diver->PendingConfig.GameMode==Dive::Mode::Exploration;
        Page=Diver->ComputerPage;
        Heading=FMath::Fmod(Diver->GetControlRotation().Yaw+360,360);
        Display->UpdateResource();
    }
}

bool ADiveEquipment::Validate(FString& Error) const
{
    if(!Avatar||!Avatar->Validate(Error)) { if(Error.IsEmpty()) Error=TEXT("Missing world-space diver");return false; }
    if(!Display||!ScreenMaterial||!Font) { Error=TEXT("Missing wrist display resources"); return false; }
    for(int32 i=0;i<Meshes.Num();++i)
        if(!Meshes[i]->GetStaticMesh()||Meshes[i]->GetStaticMesh()->GetName()!=(i==5&&TorchModel>=0?TorchMeshes[TorchModel]:EquipmentNames[i]))
        { Error=TEXT("Missing equipment mesh"); return false; }
    if(Diver&&Diver->Simulation.Data.ElapsedSeconds>1&&
       (DisplayUpdates<1||Diver->Simulation.Data.ElapsedSeconds-DisplayState.ElapsedSeconds>.5))
    { Error=TEXT("Wrist display not updating from simulation"); return false; }
    return true;
}

void ADiveEquipment::ExportDisplayForTest(const FString& Directory,const FString& Filename) const
{
    if(Display) UKismetRenderingLibrary::ExportRenderTarget(Diver.Get(),Display,Directory,Filename);
}

void ADiveEquipment::DrawDisplay(UCanvas* Canvas,int32 Width,int32 Height)
{
    if(!Canvas||!Font) return;
    ++DisplayUpdates;
    DisplayedPages|=1u<<Page;
    const auto& s=DisplayState;
    const FLinearColor white(.80f,1,.91f,1),dim(.33f,.61f,.54f,1),amber(1,.58f,.12f,1);
    auto text=[&](const FString& value,float x,float y,float size,FLinearColor color)
    {
        const FSlateFontInfo info(Font,FMath::RoundToInt(size*.75f),TEXT("Regular"));
        FCanvasTextItem item(FVector2D(x,y),FText::FromString(value),info,color); Canvas->DrawItem(item);
    };
    auto rect=[&](float x,float y,float w,float h,FLinearColor color)
    { FCanvasTileItem item(FVector2D(x,y),FVector2D(w,h),color); item.BlendMode=SE_BLEND_Opaque; Canvas->DrawItem(item); };
    auto L=[&](const TCHAR* de,const TCHAR* en) {return FString(bEnglish?en:de);};
    rect(0,0,Width,Height,FLinearColor(.005f,.015f,.012f,1));
    text(FString::Printf(TEXT("AIR 21%%   %03.0f deg"),Heading),24,12,32,dim);
    text(Clock(s.ElapsedSeconds),540,12,40,white);
    rect(24,58,720,2,dim);
    text(L(TEXT("TIEFE / m"),TEXT("DEPTH / m")),24,76,32,dim);
    text(L(TEXT("DRUCK / bar"),TEXT("GAS / bar")),410,76,32,dim);
    text(FString::Printf(TEXT("%.1f"),s.DepthMetres),22,113,104,white);
    text(bUnlimited?TEXT("INF"):FString::Printf(TEXT("%.0f"),s.CylinderBar),408,113,104,s.CylinderBar<50?amber:white);
    rect(24,225,720,2,dim);
    auto value=[&](const FString& label,const FString& number,int column,int row)
    { const float x=24+column*386,y=244+row*97; text(label,x,y,28,dim); text(number,x,y+31,44,white); };
    if(Page==0)
    {
        value(L(TEXT("NULLZEIT"),TEXT("NDL")),FString::Printf(TEXT("%.0f min"),s.NDLMinutes),0,0);
        value(L(TEXT("AUFSTIEG"),TEXT("ASCENT")),FString::Printf(TEXT("%+.1f m/min"),s.AscentMetresPerMinute),1,0);
        value(L(TEXT("MAX / TEMP"),TEXT("MAX / TEMP")),FString::Printf(TEXT("%.1f m / %.1f C"),s.MaxDepthMetres,s.WaterTemperatureC),0,1);
        value(L(TEXT("STOPP 3-6 m"),TEXT("STOP 3-6 m")),s.MaxDepthMetres>10?Clock(s.StopRemainingSeconds):TEXT("--:--"),1,1);
    }
    else if(Page==1)
    {
        value(L(TEXT("PULS / SpO2"),TEXT("PULSE / SpO2")),FString::Printf(TEXT("%.0f / %.0f%%"),s.HeartBPM,s.SpO2),0,0);
        value(L(TEXT("ATMUNG / KERN"),TEXT("BREATH / CORE")),FString::Printf(TEXT("%.0f / %.1f C"),s.RespiratoryBPM,s.CoreTemperatureC),1,0);
        value(TEXT("STRESS"),FString::Printf(TEXT("%.0f%%"),s.Stress*100),0,1);
        value(L(TEXT("ERMUEDUNG"),TEXT("FATIGUE")),FString::Printf(TEXT("%.0f%%"),s.Fatigue*100),1,1);
    }
    else
    {
        value(TEXT("ppO2 / bar"),FString::Printf(TEXT("%.2f"),s.PPO2),0,0);
        value(L(TEXT("UMGEBUNG / bar"),TEXT("AMBIENT / bar")),FString::Printf(TEXT("%.2f"),s.AmbientBar),1,0);
        value(L(TEXT("GAS BIS 50 bar"),TEXT("GAS TO 50 bar")),bUnlimited?TEXT("INF"):FString::Printf(TEXT("%.0f min"),s.RemainingGasMinutes),0,1);
        value(L(TEXT("DEKO / LAMPE"),TEXT("CEILING / TORCH")),FString::Printf(TEXT("%.1f m / %.0f%%"),s.CeilingMetres,s.LampBattery),1,1);
    }
    rect(24,447,720,2,dim);
    FString warning;
    if(s.Warnings&Dive::NoBreathingSupply) warning=L(TEXT("! KEINE ATEMLUFT  Reserve "),TEXT("! NO SUPPLY  Reserve "))+FString::Printf(TEXT("%.0f%%"),s.BreathReserve*100);
    else if(s.Warnings&Dive::OutOfGas) warning=L(TEXT("! FLASCHE LEER"),TEXT("! CYLINDER EMPTY"));
    else if(s.Warnings&Dive::EquipmentFailure) warning=L(TEXT("! REGLERDEFEKT / GASFLUSS"),TEXT("! REGULATOR FAULT / FREEFLOW"));
    else if(s.Warnings&Dive::CeilingViolation) warning=L(TEXT("! DEKOTIEFE UNTERSCHRITTEN"),TEXT("! ABOVE CEILING"));
    else if(s.Warnings&Dive::FastAscent) warning=L(TEXT("! HOHE AUFSTIEGSRATE"),TEXT("! HIGH ASCENT RATE"));
    else if(s.Warnings&Dive::LowGas) warning=L(TEXT("! NIEDRIGER DRUCK"),TEXT("! LOW GAS"));
    else if(s.Warnings&Dive::HighPO2) warning=L(TEXT("! HOHER ppO2"),TEXT("! HIGH ppO2"));
    else if(s.Warnings&Dive::Cold) warning=L(TEXT("! KAELTEBELASTUNG"),TEXT("! COLD STRAIN"));
    else if(s.Warnings&Dive::DepthPlan) warning=L(TEXT("! GEPLANTE TIEFE UEBERSCHRITTEN"),TEXT("! BEYOND PLANNED DEPTH"));
    else if(s.Warnings&Dive::TimePlan) warning=L(TEXT("! GEPLANTE ZEIT UEBERSCHRITTEN"),TEXT("! BEYOND PLANNED TIME"));
    text(warning.IsEmpty()?FString::Printf(TEXT("%d/3    R %s    C %s"),Page+1,*L(TEXT("Seite"),TEXT("Page")),*L(TEXT("senken"),TEXT("lower"))):warning,24,465,30,warning.IsEmpty()?dim:amber);
}

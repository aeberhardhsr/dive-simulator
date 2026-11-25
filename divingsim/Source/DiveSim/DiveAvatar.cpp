#include "DiveAvatar.h"
#include "DiveBodyFrame.h"
#include "DivePawn.h"
#include "DiveWorld.h"
#include "Camera/CameraComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"

namespace
{
enum EPart { Hood,Mask,Regulator,BCD,Cylinder,Weights,Hose,Compass,BootL,BootR,FinL,FinR,PartCount };
const TCHAR* PartNames[]={TEXT("SM_DiverHood"),TEXT("SM_DiveMask"),TEXT("SM_SecondStage"),TEXT("SM_BCDJacket"),
    TEXT("SM_CylinderSteel12"),TEXT("SM_WeightPockets"),TEXT("SM_RegulatorHose"),TEXT("SM_DiveCompass"),
    TEXT("SM_DiveBoot"),TEXT("SM_DiveBoot"),TEXT("SM_DiveFin"),TEXT("SM_DiveFin")};
const TCHAR* BCDNames[]={TEXT("SM_BCDJacket"),TEXT("SM_BCDTravel"),TEXT("SM_BCDWing")};
UStaticMesh* Mesh(const FString& Name)
{ return LoadObject<UStaticMesh>(nullptr,*(TEXT("/Game/Diving/Avatar/")+Name+TEXT(".")+Name)); }
FName Bone(const TCHAR* Prefix,const TCHAR* Side) { return FName(FString(Prefix)+TEXT("_")+Side); }
}

ADiveAvatar::ADiveAvatar()
{
    PrimaryActorTick.bCanEverTick=false;
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("AvatarRoot"));
    Suit=CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("ArticulatedSuit"));
    Suit->SetupAttachment(RootComponent);
    Suit->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Suit->SetBoundsScale(2.f);
    for(int32 i=0;i<PartCount;++i)
    {
        auto* part=CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Equipment_%d"),i));
        part->SetupAttachment(RootComponent);part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        if(i<=Regulator) { part->SetOwnerNoSee(true);part->bCastHiddenShadow=true; }
        Parts.Add(part);
    }
    RegulatorHose=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("FlexibleRegulatorHose"));
    RegulatorHose->SetupAttachment(RootComponent);RegulatorHose->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ADiveAvatar::Initialize(ADivePawn* Pawn)
{
    Diver=Pawn;SetOwner(Pawn);
    for(const TCHAR* name:{TEXT("SK_DiverSuit"),TEXT("SK_DiverWet")})
        Suits.Add(LoadObject<USkeletalMesh>(nullptr,*(FString(TEXT("/Game/Diving/Avatar/"))+name+TEXT(".")+name)));
    for(int32 i=0;i<PartCount;++i) Parts[i]->SetStaticMesh(Mesh(PartNames[i]));
    Parts[Hose]->SetVisibility(false);
    // FBX reflects Y. The asymmetric inflator and chest instrument stay on
    // their authored anatomical sides in Unreal's coordinate system.
    Parts[BCD]->SetRelativeScale3D(FVector(1,-1,1));
    Parts[Compass]->SetRelativeScale3D(FVector(1,-1,1));
    RegulatorHose->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Diving/Avatar/M_DiverRubber.M_DiverRubber")));
    for(int32 i=FinL;i<=FinR;++i)
    {
        auto* mat=UMaterialInstanceDynamic::Create(Parts[i]->GetMaterial(0),this);
        Parts[i]->SetMaterial(0,mat);FinMaterials.Add(mat);
    }
    Configure();
    for(const auto& pair:Reference)
        UE_LOG(LogTemp,Display,TEXT("DIVE_AVATAR_REFERENCE bone=%s position=%s scale=%s"),
            *pair.Key.ToString(),*pair.Value.GetLocation().ToString(),*pair.Value.GetScale3D().ToString());
}

void ADiveAvatar::Configure()
{
    const auto& c=Diver->bDiveActive?Diver->Simulation.Settings:Diver->PendingConfig;
    if(!bConfigured||bDry!=c.DrySuit)
    {
        bDry=c.DrySuit;Suit->SetSkinnedAssetAndUpdate(Suits[bDry?0:1]);Reference.Reset();BoneOrder.Reset();
        if(auto* mesh=Cast<USkeletalMesh>(Suit->GetSkinnedAsset()))
        {
            const auto& ref=mesh->GetRefSkeleton();TArray<FTransform> component;
            for(int32 i=0;i<ref.GetNum();++i)
            {
                const int32 parent=ref.GetParentIndex(i);
                component.Add(parent>=0?ref.GetRefBonePose()[i]*component[parent]:ref.GetRefBonePose()[i]);
                Reference.Add(ref.GetBoneName(i),component.Last());
                BoneOrder.Add(ref.GetBoneName(i));
            }
        }
    }
    if(SelectedBCD!=c.BCDModel) { SelectedBCD=c.BCDModel;Parts[BCD]->SetStaticMesh(Mesh(BCDNames[SelectedBCD])); }
    const int size=c.CylinderLitres<11?10:c.CylinderLitres<13.5?12:15;
    if(SelectedMaterial!=c.CylinderMaterial||SelectedSize!=size)
    {
        SelectedMaterial=c.CylinderMaterial;SelectedSize=size;
        Parts[Cylinder]->SetStaticMesh(Mesh(FString::Printf(TEXT("SM_Cylinder%s%d"),SelectedMaterial?TEXT("Aluminium"):TEXT("Steel"),size)));
    }
    const double ratio=Dive::Loadout::Cylinder(c.CylinderLitres,c.CylinderMaterial).DisplacedLitres/
        Dive::Loadout::Cylinder(size,c.CylinderMaterial).DisplacedLitres;
    Parts[Cylinder]->SetRelativeScale3D(FVector(FMath::Pow(ratio,1./3)));
    const double cylinderTop=Parts[Cylinder]->GetStaticMesh()?Parts[Cylinder]->GetStaticMesh()->GetBoundingBox().Max.Z*Parts[Cylinder]->GetRelativeScale3D().Z:35;
    // Seat every cylinder below the neck, regardless of its selected length.
    // The former fixed centre put tall valves through the extended head.
    Parts[Cylinder]->SetRelativeLocation(FVector(-24,0,140-cylinderTop));
    Parts[Weights]->SetVisibility(c.WeightKg>.01);
    Parts[Weights]->SetRelativeScale3D(FVector(1,1,FMath::Clamp(c.WeightKg/6,.15,3.)));
    // Scale pockets about the belt, not about the ground-level rig origin.
    Parts[Weights]->SetRelativeLocation(FVector(0,0,99*(1-Parts[Weights]->GetRelativeScale3D().Z)));
    bConfigured=true;
}

void ADiveAvatar::AimBone(FName Name,const FVector& RestEnd,const FVector& Start,const FVector& End)
{
    const auto* ref=Reference.Find(Name);if(!ref) return;
    FTransform pose=*ref;
    pose.SetLocation(Start);
    pose.SetRotation((FQuat::FindBetweenVectors(RestEnd-ref->GetLocation(),End-Start)*ref->GetRotation()).GetNormalized());
    Suit->SetBoneTransformByName(Name,pose,EBoneSpaces::ComponentSpace);
}

FVector ADiveAvatar::Arm(const TCHAR* Side,const FTransform& Wrist)
{
    const FName upper=Bone(TEXT("upper"),Side),lower=Bone(TEXT("lower"),Side);
    if(!Reference.Contains(upper)||!Reference.Contains(lower)) return Wrist.GetLocation();
    const FVector shoulder=Reference[upper].GetLocation(),restElbow=Reference[lower].GetLocation();
    const float sign=restElbow.Y<0?-1.f:1.f;
    const FVector restWrist(0,sign*30,80.4);
    const double a=FVector::Distance(shoulder,restElbow),b=FVector::Distance(restElbow,restWrist);
    const FVector requested=GetActorTransform().InverseTransformPosition(Wrist.GetLocation());
    FVector axis=requested-shoulder;
    const double distance=FMath::Clamp(axis.Size(),FMath::Abs(a-b)+.01,a+b-.01);axis.Normalize();
    const FVector target=shoulder+axis*distance;
    const FVector forearm=GetActorTransform().InverseTransformVectorNoScale(Wrist.GetUnitAxis(EAxis::X));
    FVector pole=target-forearm*b-shoulder;pole-=axis*FVector::DotProduct(pole,axis);
    if(!pole.Normalize()) pole=FVector(0,sign,-.3).GetSafeNormal();
    const double along=(a*a-b*b+distance*distance)/(2*distance);
    const FVector elbow=shoulder+axis*along+pole*FMath::Sqrt(FMath::Max(0.,a*a-along*along));
    AimBone(upper,restElbow,shoulder,elbow);AimBone(lower,restWrist,elbow,target);
    MaxArmLengthError=FMath::Max(MaxArmLengthError,FMath::Max(FMath::Abs(FVector::Distance(shoulder,elbow)-a),FMath::Abs(FVector::Distance(elbow,target)-b)));
    return GetActorTransform().TransformPosition(target);
}

void ADiveAvatar::Leg(const TCHAR* Side,float Sign,float Phase)
{
    const FName thigh=Bone(TEXT("thigh"),Side),shin=Bone(TEXT("shin"),Side);
    if(!Reference.Contains(thigh)||!Reference.Contains(shin)) return;
    const FVector hip=Reference[thigh].GetLocation(),restKnee=Reference[shin].GetLocation(),restAnkle(0,Sign*11,10);
    const double upper=FVector::Distance(hip,restKnee),lower=FVector::Distance(restKnee,restAnkle);
    const double wave=FMath::Sin(Phase),walk=1-SwimBlend;
    const double angle=FMath::DegreesToRadians((walk*17+SwimBlend*12)*wave*MotionBlend);
    const double bend=FMath::DegreesToRadians((walk*32+SwimBlend*22)*FMath::Max(0.,-wave)*MotionBlend+SwimBlend*10);
    const FVector knee=hip+FVector(FMath::Sin(angle)*upper,0,-FMath::Cos(angle)*upper);
    const FVector ankle=knee+FVector(FMath::Sin(angle-bend)*lower,0,-FMath::Cos(angle-bend)*lower);
    AimBone(thigh,restKnee,hip,knee);AimBone(shin,restAnkle,knee,ankle);
    const int boot=Sign<0?BootL:BootR,fin=Sign<0?FinL:FinR;
    // Plantar flexion aligns the fin with the extended swimming leg. Keeping
    // the standing ankle angle would make both blades hang below the diver.
    const FRotator foot(FMath::RadiansToDegrees(-angle+bend*.6)-SwimBlend*80,Sign*7,0);
    for(int index:{boot,fin}) { Parts[index]->SetRelativeLocation(ankle-FVector(0,0,2.6));Parts[index]->SetRelativeRotation(foot); }
    const FVector direction=Parts[fin]->GetUpVector()*(SwimBlend*MotionBlend*3*FMath::Sin(Phase+.5));
    if(FinMaterials[Sign<0?0:1]) FinMaterials[Sign<0?0:1]->SetVectorParameterValue(TEXT("BladeBend"),FLinearColor(direction.X,direction.Y,direction.Z));
}

void ADiveAvatar::UpdatePose(float DeltaSeconds,USceneComponent* Left,USceneComponent* Right)
{
    if(!Diver) return;
    Configure();SetActorHiddenInGame(!Diver->bDiveActive&&!bInspection);
    if(!Diver->bDiveActive&&!bInspection) return;
    if(Diver->bMenu&&Diver->bDiveActive) return; // Pose and animation clock pause together.
    const float speed=Diver->VelocityMetres.Size();
    const bool moving=Diver->bDiveActive?(Diver->bWalking?speed>.05:Diver->Simulation.Data.Exertion>.04):(bInspection&&bPreviewSwimming);
    const FVector eyes=Diver->Camera->GetComponentLocation();float floor=0;
    const bool nearBed=Diver->Environment&&Diver->Environment->SupportZ(eyes.X,eyes.Y,floor,eyes.Z)&&eyes.Z-floor<170;
    const bool submerged=Diver->bDiveActive&&Diver->Simulation.Data.DepthMetres>.6;
    const float swimTarget=!Diver->bDiveActive?(bPreviewSwimming?1.f:0.f):Diver->bWalking?0.f:(moving||nearBed||submerged)?1.f:0.f;
    SwimBlend=FMath::FInterpTo(SwimBlend,swimTarget,DeltaSeconds,3);
    MotionBlend=FMath::FInterpTo(MotionBlend,moving?1.f:0.f,DeltaSeconds,5);
    MotionPhase=Diver->bDiveActive||bPreviewSwimming?FMath::Fmod(MotionPhase+DeltaSeconds*(Diver->bWalking?speed*7:4+4*Diver->Simulation.Data.Exertion),2*PI):0;
    const auto gaze=Diver->GetControlRotation();
    const float viewPitch=Diver->bDiveActive?FRotator::NormalizeAxis(gaze.Pitch):0;
    BodyPitch=FMath::FInterpTo(BodyPitch,FMath::Max(-80*SwimBlend,viewPitch-90),DeltaSeconds,5);
    const auto frame=FDiveBodyFrame::FromEyes(eyes,BodyPitch,gaze.Yaw,viewPitch);
    BodyPitch=frame.BodyPitch;
    PoseEyes=eyes;PoseGaze=FDiveBodyFrame::View(viewPitch,gaze.Yaw).Vector();
    SetActorTransform(frame.BodyWorld);
    for(int index:{Hood,Mask})
    {
        // Preserve the known single-axis Euler rotation. Converting the local
        // quaternion back to a rotator snaps near 90 degrees in Unreal, moving
        // these off-origin meshes despite a mathematically exact eye anchor.
        FDiveBodyFrame::ApplyLocalPitch(Parts[index],frame.HeadLocal.GetLocation(),frame.HeadPitch);
    }
    for(FName name:BoneOrder) Suit->SetBoneTransformByName(name,Reference[name],EBoneSpaces::ComponentSpace);
    // FBX handedness is established by imported rest coordinates, not a label assumption.
    const bool leftIsL=Reference.Contains(TEXT("upper_l"))&&Reference[TEXT("upper_l")].GetLocation().Y<0;
    if(!Diver->bDiveActive)
    {
        Left->SetWorldLocation(GetActorTransform().TransformPosition(bPreviewSwimming?FVector(10,-25,177):FVector(0,-30,80.4)));
        Right->SetWorldLocation(GetActorTransform().TransformPosition(bPreviewSwimming?FVector(10,25,177):FVector(0,30,80.4)));
        const FQuat handRotation=GetActorQuat()*FRotator(bPreviewSwimming?90:-90,0,0).Quaternion();
        Left->SetWorldRotation(handRotation);Right->SetWorldRotation(handRotation);
    }
    if(Diver->bDiveActive||bPreviewSwimming)
    {
        Left->SetWorldLocation(Arm(leftIsL?TEXT("l"):TEXT("r"),Left->GetComponentTransform()));
        Right->SetWorldLocation(Arm(leftIsL?TEXT("r"):TEXT("l"),Right->GetComponentTransform()));
    }
    Leg(leftIsL?TEXT("l"):TEXT("r"),-1,MotionPhase);
    Leg(leftIsL?TEXT("r"):TEXT("l"),1,MotionPhase+PI);
    RegulatorBlend=FMath::FInterpTo(RegulatorBlend,Diver->bRegulatorInMouth?1.f:0.f,DeltaSeconds,8);
    FDiveBodyFrame::ApplyLocalPitch(Parts[Regulator],frame.RegulatorLocal(RegulatorBlend).GetLocation(),frame.HeadPitch*RegulatorBlend);
    Parts[Regulator]->SetOwnerNoSee(RegulatorBlend>.7);
    const FRotator heading(0,0,-Diver->GetControlRotation().Yaw);
    const FVector compassCenter(14,8.5,131);
    Parts[Compass]->SetRelativeRotation(heading);
    Parts[Compass]->SetRelativeLocation(compassCenter-heading.RotateVector(compassCenter));
    UpdateHose();
    Suit->RefreshBoneTransforms();++PoseUpdates;
}

void ADiveAvatar::UpdateHose()
{
    const double top=Parts[Cylinder]->GetStaticMesh()?Parts[Cylinder]->GetStaticMesh()->GetBoundingBox().Max.Z*Parts[Cylinder]->GetRelativeScale3D().Z:35;
    const FVector end=Parts[Regulator]->GetRelativeTransform().TransformPosition(FVector(13,4.5,158.5));
    const TArray<FVector> points={Parts[Cylinder]->GetRelativeLocation()+FVector(0,4,top-1),FVector(-12,23,148),FVector(3,29,140),FVector(19,21,143),end+FVector(8,3,-1),end};
    TArray<FVector> centers;
    for(int i=0;i<points.Num()-1;++i)
    {
        const FVector p0=points[FMath::Max(0,i-1)],p1=points[i],p2=points[i+1],p3=points[FMath::Min(points.Num()-1,i+2)];
        for(int k=0;k<6;++k)
        { const double t=k/6.;centers.Add(.5*((2*p1)+(-p0+p2)*t+(2*p0-5*p1+4*p2-p3)*t*t+(-p0+3*p1-3*p2+p3)*t*t*t)); }
    }
    centers.Add(end);TArray<FVector> vertices,normals;TArray<FVector2D> uv;TArray<FLinearColor> colors;TArray<int32> indices;TArray<FProcMeshTangent> tangents;
    for(int i=0;i<centers.Num();++i)
    {
        const FVector axis=(centers[FMath::Min(i+1,centers.Num()-1)]-centers[FMath::Max(i-1,0)]).GetSafeNormal();
        FVector right=FVector::CrossProduct(axis,FVector::UpVector).GetSafeNormal();if(right.IsNearlyZero()) right=FVector::RightVector;
        const FVector up=FVector::CrossProduct(axis,right).GetSafeNormal();
        for(int j=0;j<12;++j)
        {
            const double angle=j*2*PI/12;const FVector normal=right*FMath::Cos(angle)+up*FMath::Sin(angle);
            vertices.Add(centers[i]+normal*.8);normals.Add(normal);uv.Add(FVector2D(j/12.,i/6.));colors.Add(FLinearColor(.028f,.037f,.043f,1));
            if(i) { const int v=i*12+j,n=i*12+(j+1)%12;indices.Append({v,v-12,n-12,v,n-12,n}); }
        }
    }
    if(RegulatorHose->GetNumSections()==0) RegulatorHose->CreateMeshSection_LinearColor(0,vertices,indices,normals,uv,colors,tangents,false);
    else RegulatorHose->UpdateMeshSection_LinearColor(0,vertices,normals,uv,colors,tangents);
}

FVector ADiveAvatar::InspectionTarget() const
{ return GetActorTransform().TransformPosition(FVector(0,0,90)); }

bool ADiveAvatar::Validate(FString& Error) const
{
    if(!Suit->GetSkinnedAsset()||Reference.Num()<12) { Error=TEXT("Missing articulated suit or bones");return false; }
    if(!Suit->GetMaterial(0)||Suit->GetMaterial(0)->GetName()!=TEXT("M_DiverSuit"))
    { Error=TEXT("Articulated suit material missing or replaced");return false; }
    for(FName bone:BoneOrder) if(Suit->GetBoneTransformByName(bone,EBoneSpaces::ComponentSpace).ContainsNaN())
    { Error=TEXT("Non-finite articulated pose");return false; }
    for(const TCHAR* side:{TEXT("l"),TEXT("r")})
    {
        for(const auto& pair:{TPair<FName,FName>(Bone(TEXT("upper"),side),Bone(TEXT("lower"),side)),
                             TPair<FName,FName>(Bone(TEXT("thigh"),side),Bone(TEXT("shin"),side))})
        {
            const double measured=FVector::Distance(Suit->GetBoneTransformByName(pair.Key,EBoneSpaces::ComponentSpace).GetLocation(),
                Suit->GetBoneTransformByName(pair.Value,EBoneSpaces::ComponentSpace).GetLocation());
            const double expected=FVector::Distance(Reference[pair.Key].GetLocation(),Reference[pair.Value].GetLocation());
            if(FMath::Abs(measured-expected)>.05) { Error=TEXT("Rendered bone hierarchy changes limb length");return false; }
        }
    }
    for(const auto& part:Parts) if(!part||!part->GetStaticMesh()) { Error=TEXT("Missing avatar equipment");return false; }
    if(PoseUpdates>0)
    {
        const FTransform head=Parts[Hood]->GetComponentTransform();
        const double eyeError=FVector::Distance(head.TransformPosition(FDiveBodyFrame::Eyes()),PoseEyes);
        if(eyeError>.01)
        { Error=FString::Printf(TEXT("Rendered eye anchor error %.6f cm"),eyeError);return false; }
        if(!head.GetUnitAxis(EAxis::X).Equals(PoseGaze,.001))
        { Error=TEXT("Head does not follow gaze");return false; }
        if(!Parts[Mask]->GetComponentTransform().Equals(head,.01))
        { Error=TEXT("Mask detached from head");return false; }
        if(RegulatorBlend>.9999&&!Parts[Regulator]->GetComponentTransform().TransformPosition(FDiveBodyFrame::Mouth()).Equals(head.TransformPosition(FDiveBodyFrame::Mouth()),.1))
        { Error=TEXT("Second stage detached from mouth");return false; }
    }
    if(MaxArmLengthError>.01) { Error=TEXT("Arm solver changes limb lengths");return false; }
    if(!Reference.Contains(TEXT("pelvis"))||FMath::Abs(Reference[TEXT("pelvis")].GetLocation().Z-98)>1)
    { Error=TEXT("Unexpected avatar import units/rest pose");return false; }
    return true;
}

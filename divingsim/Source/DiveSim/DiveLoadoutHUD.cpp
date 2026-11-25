#include "DiveHUD.h"
#include "DivePawn.h"
#include "DiveEquipment.h"
#include "DiveAvatar.h"
#include "Engine/TextureRenderTarget2D.h"

void ADiveHUD::LoadoutMenu()
{
    const auto L=[this](const TCHAR* de,const TCHAR* en){return Diver->Local(de,en);};
    const auto& c=Diver->bDiveActive?Diver->Simulation.Settings:Diver->PendingConfig;
    const Dive::Model prepared(c);const auto& s=Diver->bDiveActive?Diver->Simulation.Data:prepared.Data;
    const FLinearColor white(.87f,.94f,.92f,1),muted(.42f,.59f,.57f,1),accent(.43f,.87f,.69f,1);
    Button(TEXT("avatar_view"),bAvatarView?L(TEXT("Liste"),TEXT("List")):L(TEXT("3D ansehen"),TEXT("3D view")),1320,158,180,34);
    Field(TEXT("cylinder_material"),L(TEXT("FLASCHENMATERIAL"),TEXT("CYLINDER MATERIAL")),c.CylinderMaterial==1?TEXT("Aluminium"):L(TEXT("Stahl"),TEXT("Steel")),650,250,265);
    Field(TEXT("bcd"),TEXT("BCD"),UTF8_TO_TCHAR(Dive::BCDEquipment::At(c.BCDModel).Item.Name),943,250,265);
    Field(TEXT("torch"),L(TEXT("LAMPE"),TEXT("TORCH")),UTF8_TO_TCHAR(Dive::Torch::At(c.TorchModel).Name),1236,250,265);
    if(bAvatarView&&Diver->Equipment&&Diver->Equipment->Avatar)
    {
        auto* avatar=Diver->Equipment->Avatar.Get();
        if(auto* texture=avatar->InspectionImage())
            DrawTexture(texture,OffsetX+650*Scale,OffsetY+335*Scale,540*Scale,360*Scale,0,0,1,1,FLinearColor::White,BLEND_Opaque);
        Text(L(TEXT("AUSRUESTUNG ANSEHEN"),TEXT("INSPECT EQUIPMENT")),1210,345,17,accent);
        Text(FString::Printf(TEXT("%.0f L  /  %s"),c.CylinderLitres,c.CylinderMaterial?TEXT("Aluminium"):*L(TEXT("Stahl"),TEXT("Steel"))),1210,380,17,white);
        Button(TEXT("avatar_left"),TEXT("<"),1210,420,125,38);
        Button(TEXT("avatar_right"),TEXT(">"),1350,420,125,38);
        Button(TEXT("avatar_zoom"),avatar->bInspectionClose?L(TEXT("Ganzkoerper"),TEXT("Full body")):L(TEXT("Details"),TEXT("Close up")),1210,475,265,38);
        if(!Diver->bDiveActive)
            Button(TEXT("avatar_pose"),avatar->bPreviewSwimming?L(TEXT("Stehende Vorschau"),TEXT("Standing preview")):L(TEXT("Schwimmende Vorschau"),TEXT("Swimming preview")),1210,530,290,38);
        else Text(L(TEXT("Haltung in der Pause fixiert"),TEXT("Pose frozen while paused")),1210,535,14,muted);
        Text(FString::Printf(TEXT("%.2f kg"),s.TotalMassKg),1210,590,24,white);
        Text(FString::Printf(TEXT("BCD %.2f / %.0f L"),s.BCDLitres,Dive::BCDEquipment::At(c.BCDModel).CapacityLitres),1210,627,17,white);
        Text(L(TEXT("Spielmodelle / keine Markenprodukte"),TEXT("Game models / no branded products")),1210,669,13,muted);
        return;
    }
    Text(L(TEXT("AUSRUESTUNG"),TEXT("EQUIPMENT")),650,340,14,muted);
    Text(L(TEXT("MASSE / kg"),TEXT("MASS / kg")),955,340,14,muted);
    Text(L(TEXT("VOLUMEN / L"),TEXT("VOLUME / L")),1120,340,14,muted);
    Text(L(TEXT("NETTO / kg*"),TEXT("NET / kg*")),1320,340,14,muted);
    int row=0;
    auto item=[&](const FString& label,double mass,double litres)
    {
        const float y=370+row++*22;
        Text(label,650,y,16,white);Text(FString::Printf(TEXT("%.2f"),mass),955,y,16,white);
        Text(FString::Printf(TEXT("%.2f"),litres),1120,y,16,white);
        Text(FString::Printf(TEXT("%+.2f"),litres-mass),1320,y,16,white);
    };
    const auto cylinder=Dive::Loadout::Cylinder(c.CylinderLitres,c.CylinderMaterial);
    item(L(TEXT("Flasche + Ventil"),TEXT("Cylinder + valve"))+FString::Printf(TEXT(" / %.0f L"),c.CylinderLitres),cylinder.DryKg,cylinder.DisplacedLitres);
    const auto& bcd=Dive::BCDEquipment::At(c.BCDModel);
    item(L(TEXT("BCD leer"),TEXT("BCD empty")),bcd.Item.DryKg,bcd.Item.DisplacedLitres);
    const auto& suit=Dive::SuitEquipment::At(c.DrySuit);
    item(c.DrySuit?L(TEXT("Trockenanzug"),TEXT("Dry suit")):L(TEXT("Nassanzug 7 mm"),TEXT("Wet suit 7 mm")),suit.Item.DryKg,s.SuitLitres);
    const auto lamp=Dive::Loadout::Lamp(c.TorchModel);item(L(TEXT("Lampe"),TEXT("Torch")),lamp.DryKg,lamp.DisplacedLitres);
    const TCHAR* de[]={TEXT("Regler + Schlaeuche"),TEXT("Flossen"),TEXT("Maske"),TEXT("Computer"),TEXT("Kompass")};
    for(int i=0;i<Dive::Loadout::FixedCount;++i)
    { const auto& part=Dive::Loadout::Fixed(i);item(Diver->bEnglish?UTF8_TO_TCHAR(part.Name):de[i],part.DryKg,part.DisplacedLitres); }
    item(L(TEXT("Blei"),TEXT("Ballast")),c.WeightKg,c.WeightKg/Dive::Loadout::LeadKgPerLitre);
    Text(FString::Printf(TEXT("%s %.2f kg  +  %s 80 kg  +  %s %.3f kg  =  %.2f kg"),
        *L(TEXT("Ausr."),TEXT("Gear")),s.EquipmentDryKg,*L(TEXT("Koerper"),TEXT("Body")),*L(TEXT("Gas"),TEXT("Gas")),s.CylinderGasKg+s.BCDGasKg,s.TotalMassKg),650,605,16,accent);
    Text(FString::Printf(TEXT("BCD %.2f / %.0f L    %s %.2f bar    %s %+.2f kg"),s.BCDLitres,bcd.CapacityLitres,
        *L(TEXT("Flasche"),TEXT("Cylinder")),s.CylinderBar,*L(TEXT("Gesamtnetto"),TEXT("Total net")),s.DisplacedLitres-s.TotalMassKg),650,633,16,white);
    Text(Diver->bDiveActive?L(TEXT("Aktueller Zustand in der Pause; Ausruestung gesperrt."),TEXT("Current paused state; equipment choices locked.")):
        L(TEXT("Vorbereitung: bis 8 L BCD. Bei endlichem Gas aus der Flasche."),TEXT("Preparation: up to 8 L BCD. Drawn from the cylinder when gas is finite.")),650,661,14,muted);
    Text(L(TEXT("* Suesswasser: positiv = Auftrieb. Gestaltete Spielausruestung."),TEXT("* Freshwater: positive = buoyant. Fictional game equipment.")),650,687,14,muted);
}

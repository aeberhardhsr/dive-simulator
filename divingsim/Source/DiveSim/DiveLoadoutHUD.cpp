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
    Field(TEXT("cylinder_material"),L(TEXT("FLASCHENMATERIAL"),TEXT("CYLINDER MATERIAL")),c.CylinderMaterial==1?TEXT("Aluminium"):L(TEXT("Stahl"),TEXT("Steel")),470,210,310);
    Field(TEXT("bcd"),L(TEXT("BCD-SYSTEM"),TEXT("BCD SYSTEM")),UTF8_TO_TCHAR(Dive::BCDEquipment::At(c.BCDModel).Item.Name),800,210,310);
    Field(TEXT("weight"),L(TEXT("BLEIGEWICHT"),TEXT("BALLAST")),ConfigValue(TEXT("weight")),1130,210,310);
    Rect(470,310,990,70,FLinearColor(.025,.065,.063,.92));
    Text(L(TEXT("GESAMTNETTO-AUFTRIEB · SÜSSWASSER"),TEXT("TOTAL NET BUOYANCY · FRESH WATER")),486,320,13,muted);
    Text(FString::Printf(TEXT("%+.2f kg"),s.DisplacedLitres-s.TotalMassKg),486,345,22,accent);
    Text(FString::Printf(TEXT("%s %.2f kg  ·  BCD %.1f L"),*L(TEXT("Gesamtmasse"),TEXT("Total mass")),s.TotalMassKg,s.BCDLitres),670,350,15,white);
    Button(TEXT("avatar_view"),bAvatarView?L(TEXT("Tabelle anzeigen"),TEXT("Show table")):L(TEXT("3D-Vorschau"),TEXT("3D preview")),1240,330,200,32,bAvatarView);
    if(bAvatarView&&Diver->Equipment&&Diver->Equipment->Avatar)
    {
        auto* avatar=Diver->Equipment->Avatar.Get();
        if(auto* texture=avatar->InspectionImage())
            DrawTexture(texture,OffsetX+470*Scale,OffsetY+398*Scale,540*Scale,360*Scale,0,0,1,1,FLinearColor::White,BLEND_Opaque);
        Text(L(TEXT("AUSRÜSTUNG ANSEHEN"),TEXT("INSPECT EQUIPMENT")),1060,416,17,accent);
        Text(FString::Printf(TEXT("%.0f L  /  %s"),c.CylinderLitres,c.CylinderMaterial?TEXT("Aluminium"):*L(TEXT("Stahl"),TEXT("Steel"))),1060,451,17,white);
        Button(TEXT("avatar_left"),TEXT("←"),1060,495,175,40);
        Button(TEXT("avatar_right"),TEXT("→"),1250,495,175,40);
        Button(TEXT("avatar_zoom"),avatar->bInspectionClose?L(TEXT("Ganzkörper"),TEXT("Full body")):L(TEXT("Details"),TEXT("Close up")),1060,551,365,40);
        if(!Diver->bDiveActive)
            Button(TEXT("avatar_pose"),avatar->bPreviewSwimming?L(TEXT("Stehende Vorschau"),TEXT("Standing preview")):L(TEXT("Schwimmende Vorschau"),TEXT("Swimming preview")),1060,607,365,40);
        else TextBlock(L(TEXT("Haltung in der Pause fixiert"),TEXT("Pose frozen while paused")),1060,607,365,15,muted,2);
        Text(FString::Printf(TEXT("BCD %.2f / %.0f L"),s.BCDLitres,Dive::BCDEquipment::At(c.BCDModel).CapacityLitres),1060,672,17,white);
        TextBlock(L(TEXT("Spielmodelle · keine Markenprodukte"),TEXT("Game models · no branded products")),1060,713,365,13,muted,2);
        return;
    }
    Text(L(TEXT("AUSRÜSTUNG"),TEXT("EQUIPMENT")),486,402,13,muted);
    Text(L(TEXT("MASSE / kg"),TEXT("MASS / kg")),950,402,13,muted);
    Text(L(TEXT("VOLUMEN / L"),TEXT("VOLUME / L")),1110,402,13,muted);
    Text(L(TEXT("NETTO / kg*"),TEXT("NET / kg*")),1300,402,13,muted);
    int row=0;
    auto item=[&](const FString& label,double mass,double litres)
    {
        const float y=435+row++*24;
        Line(470,y-7,1460,y-7,FLinearColor(.12,.22,.2,.7),.5);
        TextBlock(label,486,y,445,16,white,1);
        const FString m=FString::Printf(TEXT("%.2f"),mass),v=FString::Printf(TEXT("%.2f"),litres),n=FString::Printf(TEXT("%+.2f"),litres-mass);
        Text(m,1040-TextWidth(m,16),y,16,white);
        Text(v,1220-TextWidth(v,16),y,16,white);
        Text(n,1440-TextWidth(n,16),y,16,litres>=mass?accent:white);
    };
    const auto cylinder=Dive::Loadout::Cylinder(c.CylinderLitres,c.CylinderMaterial);
    item(L(TEXT("Flasche + Ventil"),TEXT("Cylinder + valve"))+FString::Printf(TEXT(" / %.0f L"),c.CylinderLitres),cylinder.DryKg,cylinder.DisplacedLitres);
    const auto& bcd=Dive::BCDEquipment::At(c.BCDModel);
    item(L(TEXT("BCD leer"),TEXT("BCD empty")),bcd.Item.DryKg,bcd.Item.DisplacedLitres);
    const auto& suit=Dive::SuitEquipment::At(c.DrySuit);
    item(c.DrySuit?L(TEXT("Trockenanzug"),TEXT("Dry suit")):L(TEXT("Nassanzug 7 mm"),TEXT("Wet suit 7 mm")),suit.Item.DryKg,s.SuitLitres);
    const auto lamp=Dive::Loadout::Lamp(c.TorchModel);item(L(TEXT("Lampe"),TEXT("Torch")),lamp.DryKg,lamp.DisplacedLitres);
    const TCHAR* de[]={TEXT("Regler + Schläuche"),TEXT("Flossen"),TEXT("Maske"),TEXT("Computer"),TEXT("Kompass")};
    for(int i=0;i<Dive::Loadout::FixedCount;++i)
    { const auto& part=Dive::Loadout::Fixed(i);item(Diver->bEnglish?UTF8_TO_TCHAR(part.Name):de[i],part.DryKg,part.DisplacedLitres); }
    item(L(TEXT("Blei"),TEXT("Ballast")),c.WeightKg,c.WeightKg/Dive::Loadout::LeadKgPerLitre);
    TextBlock(FString::Printf(TEXT("%s %.2f kg  +  %s 80 kg  +  %s %.3f kg  =  %.2f kg"),
        *L(TEXT("Ausrüstung"),TEXT("Gear")),s.EquipmentDryKg,*L(TEXT("Körper"),TEXT("Body")),*L(TEXT("Gas"),TEXT("Gas")),s.CylinderGasKg+s.BCDGasKg,s.TotalMassKg),486,692,950,15,accent,1);
    Text(Diver->bDiveActive?L(TEXT("Aktueller Zustand in der Pause; Ausrüstung gesperrt."),TEXT("Current paused state; equipment choices locked.")):
        L(TEXT("Vorbereitung: bis 8 L BCD. Bei endlichem Gas aus der Flasche."),TEXT("Preparation: up to 8 L BCD. Drawn from the cylinder when gas is finite.")),486,721,13,muted);
    Text(L(TEXT("* Süsswasser: positiv = Auftrieb. Gestaltete Spielausrüstung."),TEXT("* Freshwater: positive = buoyant. Fictional game equipment.")),486,744,12,muted);
}

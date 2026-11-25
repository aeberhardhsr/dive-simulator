#pragma once
#include <algorithm>
#include <cmath>
#include "Torch.h"

namespace Dive
{
// Fictional generic equipment. Masses include the indicated complete assembly;
// none of these presets describes or certifies a commercial product.
struct EquipmentItem
{
    const char* Id;
    const char* Name;
    double DryKg,DisplacedLitres;
};
struct BCDEquipment
{
    EquipmentItem Item;
    double CapacityLitres;
    static constexpr int Count=3;
    static const BCDEquipment& At(int index)
    {
        static const BCDEquipment values[]={
            {{"jacket","Jacket",3.3,2.7},18},
            {{"travel","Travel",2.2,1.8},12},
            {{"wing","Wing",4.2,3.4},24}
        };
        return values[std::clamp(index,0,Count-1)];
    }
};
struct SuitEquipment
{
    EquipmentItem Item;
    double CompressibleLitres,CompressionExponent,Cooling,ColdScale;
    static const SuitEquipment& At(bool dry)
    {
        static const SuitEquipment wet={{"wet7","Wet 7 mm",3.2,2.0},6.2,.7,.000007,1};
        static const SuitEquipment drySuit={{"dry","Dry suit",4.5,3.0},4.5,.7,.0000015,.25};
        return dry?drySuit:wet;
    }
    double VolumeAt(double pressureRatio) const
    { return Item.DisplacedLitres+CompressibleLitres/std::pow(std::max(.1,pressureRatio),CompressionExponent); }
};
struct Loadout
{
    static constexpr double DiverMassKg=80,DiverVolumeLitres=82;
    static constexpr double AirKgPerSurfaceLitre=.0012,LeadKgPerLitre=11.34,InitialBCDLitres=8;
    static constexpr int FixedCount=5;
    static const EquipmentItem& Fixed(int index)
    {
        static const EquipmentItem items[]={
            {"regulator","Regulator and hoses",2.4,.9},
            {"fins","Fins",1.8,.8},
            {"mask","Mask",.3,.15},
            {"computer","Computer",.25,.16},
            {"compass","Compass",.12,.08}
        };
        return items[std::clamp(index,0,FixedCount-1)];
    }
    static EquipmentItem Cylinder(double litres,int material)
    {
        // Capacity is interior water volume; outside displacement also includes
        // metal and valve. A complete cylinder is never represented by gas alone.
        const bool aluminium=material==1;
        const double dry=aluminium?1.35*litres+.8:1.15*litres+1;
        const double metalDensity=aluminium?2.7:7.8;
        return {aluminium?"aluminium":"steel",aluminium?"Aluminium":"Steel",dry,
            litres+(dry-.8)/metalDensity+.1};
    }
    static EquipmentItem Lamp(int index)
    {
        const auto& torch=Torch::At(index);
        return {torch.Id,torch.Name,torch.DryKg,torch.DisplacedLitres};
    }
    struct Balance
    {
        double EquipmentDryKg=0,CylinderGasKg=0,BCDGasKg=0;
        double TotalMassKg=0,DisplacedLitres=0,SuitLitres=0,NetKg=0;
    };
    static Balance Evaluate(double cylinderLitres,int material,int bcd,bool dry,int torch,
        double ballastKg,double cylinderBar,double bcdSurfaceLitres,double ambientBar,double surfaceBar,double lungDelta=0)
    {
        Balance result;
        auto add=[&](const EquipmentItem& item) { result.EquipmentDryKg+=item.DryKg;result.DisplacedLitres+=item.DisplacedLitres; };
        add(Cylinder(cylinderLitres,material));add(BCDEquipment::At(bcd).Item);add(Lamp(torch));
        for(int i=0;i<FixedCount;++i) add(Fixed(i));
        const auto& suit=SuitEquipment::At(dry);
        result.EquipmentDryKg+=suit.Item.DryKg+ballastKg;
        result.SuitLitres=suit.VolumeAt(ambientBar/surfaceBar);
        result.CylinderGasKg=cylinderBar*cylinderLitres*AirKgPerSurfaceLitre;
        result.BCDGasKg=bcdSurfaceLitres*AirKgPerSurfaceLitre;
        result.TotalMassKg=DiverMassKg+result.EquipmentDryKg+result.CylinderGasKg+result.BCDGasKg;
        result.DisplacedLitres+=DiverVolumeLitres+result.SuitLitres+ballastKg/LeadKgPerLitre+
            bcdSurfaceLitres/ambientBar+lungDelta;
        // Freshwater density is 1 kg/L, matching Dive::Density.
        result.NetKg=result.DisplacedLitres-result.TotalMassKg;
        return result;
    }
};
}

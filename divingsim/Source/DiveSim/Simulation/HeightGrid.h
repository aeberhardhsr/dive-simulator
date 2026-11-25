#pragma once
#include <cmath>
#include <cstdint>
#include <vector>

namespace Dive
{
// Each sample is a surveyed cell centre, stored north-to-south in centimetres
// relative to the scenario's reference water elevation. -32768 means unmeasured.
struct HeightTile
{
    int Columns=0, Rows=0;
    double East=0, North=0;
    std::vector<int16_t> Centimetres;
    bool Cell(double east,double north,double& metres) const
    {
        if(!std::isfinite(east)||!std::isfinite(north)||east<East||north<North||
            east>=East+Columns||north>=North+Rows) return false;
        const int x=static_cast<int>(std::floor(east-East));
        const int y=Rows-1-static_cast<int>(std::floor(north-North));
        if(x<0 || y<0 || x>=Columns || y>=Rows) return false;
        const auto index=static_cast<size_t>(y)*Columns+x;
        if(index>=Centimetres.size() || Centimetres[index]==INT16_MIN) return false;
        metres=Centimetres[index]*.01;
        return true;
    }
};

struct HeightMosaic
{
    std::vector<HeightTile> Tiles;
    bool Cell(double east,double north,double& metres) const
    {
        for(const auto& tile:Tiles) if(tile.Cell(east,north,metres)) return true;
        return false;
    }
    bool Sample(double east,double north,double& metres) const
    {
        if(!std::isfinite(east)||!std::isfinite(north)) return false;
        // The dataset uses a common integer-metre lattice, centres at n + 0.5.
        const double x=std::floor(east-.5)+.5, y=std::floor(north-.5)+.5;
        const double tx=east-x, ty=north-y;
        double a,b,c,d;
        if(!Cell(x,y,a)||!Cell(x+1,y,b)||!Cell(x,y+1,c)||!Cell(x+1,y+1,d)) return false;
        metres=(a*(1-tx)+b*tx)*(1-ty)+(c*(1-tx)+d*tx)*ty;
        return true;
    }
};
}

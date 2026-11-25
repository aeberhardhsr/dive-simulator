#pragma once
#include <cmath>
#include <cstdint>
#include <vector>

namespace Dive
{
// Dense, north-to-south cell-centred raster for the surrounding land model.
struct SurfaceGrid
{
    int Columns=0, Rows=0;
    double West=0, North=0, Spacing=8;
    std::vector<float> Heights;
    bool Sample(double east,double north,double& metres) const
    {
        if(!std::isfinite(east)||!std::isfinite(north)||Spacing<=0||Columns<2||Rows<2) return false;
        const double x=(east-West)/Spacing-.5, y=(North-north)/Spacing-.5;
        if(x<0||y<0||x>=Columns-1||y>=Rows-1) return false;
        const int col=static_cast<int>(x),row=static_cast<int>(y);
        const size_t index=static_cast<size_t>(row)*Columns+col;
        if(index+Columns+1>=Heights.size()) return false;
        const double a=Heights[index], b=Heights[index+1],c=Heights[index+Columns],d=Heights[index+Columns+1];
        if(!std::isfinite(a)||!std::isfinite(b)||!std::isfinite(c)||!std::isfinite(d)||
            a==-9999||b==-9999||c==-9999||d==-9999) return false;
        const double tx=x-col,ty=y-row;
        metres=(a*(1-tx)+b*tx)*(1-ty)+(c*(1-tx)+d*tx)*ty;
        return true;
    }
};

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

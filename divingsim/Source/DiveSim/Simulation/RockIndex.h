#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace Dive
{
// Conservative XY broad phase only. The caller retains its exact 3D tests.
// Each centre has one owner; queries return original insertion order so force
// sums and early-exit decisions do not depend on hash-table iteration order.
class RockIndex
{
    struct Cell
    {
        int32_t X,Y;
        bool operator==(const Cell& b) const {return X==b.X&&Y==b.Y;}
    };
    struct Hash
    {
        size_t operator()(Cell c) const
        {return std::hash<uint64_t>{}((uint64_t(uint32_t(c.X))<<32)|uint32_t(c.Y));}
    };
    static constexpr double Width=800;
    std::unordered_map<Cell,std::vector<int32_t>,Hash> Cells;
    std::vector<int32_t> All;
    double MaxRadius=0;
    bool Fallback=false;
    static bool Coordinate(double value,int32_t& result)
    {
        const double cell=std::floor(value/Width);
        if(!std::isfinite(cell)||std::abs(cell)>1000000000.) return false;
        result=int32_t(cell);return true;
    }
public:
    void Reset() {Cells.clear();All.clear();MaxRadius=0;Fallback=false;}
    void Add(double x,double y,double radius)
    {
        const auto id=int32_t(All.size());All.push_back(id);
        Cell cell{};
        if(!Coordinate(x,cell.X)||!Coordinate(y,cell.Y)||!std::isfinite(radius)||radius<0)
        {Fallback=true;return;}
        MaxRadius=std::max(MaxRadius,radius);Cells[cell].push_back(id);
    }
    size_t Size() const {return All.size();}
    std::vector<int32_t> Query(double ax,double ay,double bx,double by,double padding,bool full=false) const
    {
        if(full||Fallback||!std::isfinite(padding)||padding<0||
           !std::isfinite(ax)||!std::isfinite(ay)||!std::isfinite(bx)||!std::isfinite(by)) return All;
        // Sub-millimetre guard covers float rounding in the caller's radii.
        const double reach=MaxRadius+padding+.01;
        Cell lo{},hi{};
        if(!Coordinate(std::min(ax,bx)-reach,lo.X)||!Coordinate(std::min(ay,by)-reach,lo.Y)||
           !Coordinate(std::max(ax,bx)+reach,hi.X)||!Coordinate(std::max(ay,by)+reach,hi.Y)) return All;
        // Long teleports/diagnostics must remain bounded, without dropping rocks.
        if((int64_t(hi.X)-lo.X+1)*(int64_t(hi.Y)-lo.Y+1)>4096) return All;
        std::vector<int32_t> result;
        for(int32_t y=lo.Y;y<=hi.Y;++y) for(int32_t x=lo.X;x<=hi.X;++x)
        {
            const auto found=Cells.find({x,y});
            if(found!=Cells.end()) result.insert(result.end(),found->second.begin(),found->second.end());
        }
        std::sort(result.begin(),result.end());return result;
    }
};
}

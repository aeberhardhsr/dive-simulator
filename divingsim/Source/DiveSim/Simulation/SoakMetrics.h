#pragma once
#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace Dive
{
// Fixed memory regardless of test duration; last bin includes frames >=1024 ms.
struct SoakMetrics
{
    std::vector<uint64_t> Histogram=std::vector<uint64_t>(4097,0);
    uint64_t Frames=0,Over60=0,Over30=0,Over100=0;
    double Seconds=0,MaximumMS=0;
    bool Add(double seconds)
    {
        if(!std::isfinite(seconds)||seconds<=0) return false;
        const double ms=seconds*1000;
        if(!std::isfinite(ms)) return false;
        ++Frames; Seconds+=seconds; MaximumMS=std::max(MaximumMS,ms);
        ++Histogram[static_cast<size_t>(std::min(4096.,std::floor(ms*4)))];
        Over60+=ms>1000./60; Over30+=ms>1000./30; Over100+=ms>100;
        return true;
    }
    // Upper bound, within 0.25 ms except the overflow bin (returns exact max).
    double PercentileUpper(double fraction) const
    {
        if(!Frames) return 0;
        const uint64_t target=static_cast<uint64_t>(std::max(1.,std::ceil(std::clamp(fraction,0.,1.)*Frames)));
        uint64_t sum=0;
        for(size_t i=0;i<Histogram.size();++i)
        { sum+=Histogram[i]; if(sum>=target) return i==4096?MaximumMS:(i+1)*.25; }
        return MaximumMS;
    }
};
}

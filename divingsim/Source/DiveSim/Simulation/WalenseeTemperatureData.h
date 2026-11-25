#pragma once
// Generated numeric facts: Eawag ThermDis, Walensee seasonal temperature profiles.
// Provenance: Documentation/WALENSEE_TEMPERATURE.json; regenerate with Tools/import_temperature_profile.py.
namespace Dive::WalenseeTemperatureData
{
inline constexpr int Count=17;
inline constexpr double Depths[Count]={0,1,2.5,5,7.5,10,12.5,15,20,30,40,60,80,100,120,140,144};
inline constexpr double Profiles[4][Count]={
    {5.836,5.836,5.839,5.824,5.821,5.821,5.818,5.818,5.812,5.815,5.806,5.652,5.455,5.385,5.347,5.312,5.312},
    {8.176,7.958,7.682,7.37,7.167,6.97,6.855,6.739,6.491,6.121,5.748,5.397,5.203,5.127,5.076,5.061,5.064},
    {17.085,16.721,16.285,15.324,14,12.794,11.624,10.776,9.494,7.603,6.588,5.745,5.409,5.33,5.248,5.206,5.206},
    {12.455,12.394,12.312,12.076,11.727,11.376,11.106,10.812,10.233,8.488,7.258,5.952,5.588,5.43,5.358,5.324,5.321},
};
}

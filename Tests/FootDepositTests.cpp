#include "../divingsim/Source/DiveSim/Simulation/FootDeposit.h"
#include <iostream>
#include <limits>

int main()
{
    using D=Dive::FootDeposit;
    int failures=0;
    auto check=[&](bool value,const char* why){if(!value){++failures;std::cerr<<why<<'\n';}};
    for(double radius:{126.,200.,337.}) for(double angle:{0.,.7,2.4})
    {
        int raised=0;
        for(int y=-900;y<=900;y+=13) for(int x=-900;x<=900;x+=13)
        {
            const double h=D::Height(x,y,radius,angle,.31);
            check(std::isfinite(h)&&h>=0&&h<=D::Maximum,"Deposit must be bounded and finite");
            if(std::hypot(x,y)>radius*D::Reach) check(h==0,"Deposit escaped its spatial index radius");
            raised+=h>10;
        }
        check(raised>50,"Deposit has a spatially meaningful footprint");
        const double x=radius*.9,y=radius*.4;
        const double rotated=D::Height(x*std::cos(angle)-y*std::sin(angle),x*std::sin(angle)+y*std::cos(angle),radius,angle,.31);
        check(std::abs(rotated-D::Height(x,y,radius,0,.31))<1.e-9,"Rotated deposit must retain its shape");
        check(std::abs(D::Height(1.e-7,0,radius,angle,.31)-D::Height(-1.e-7,0,radius,angle,.31))<1.e-6,"No angular seam at the core");
    }
    check(D::Height(0,0,337,0,0)>60,"Large cores receive substantial embedded relief");
    check(D::Height(0,0,std::numeric_limits<double>::quiet_NaN(),0,0)==0,"Invalid radius rejected");
    std::cout<<"Foot deposits: "<<failures<<" failures\n";
    return failures?1:0;
}

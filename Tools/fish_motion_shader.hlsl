// UV.y is flipped by FBX import. UV1 stores normalized root-relative Y/Z;
// UV2 stores a discrete fin type and root X. No engine wall-clock time is used.
float sway=sin(Cycle+Phase-Weight*3)*Amplitude*Weight;
float kind=floor(Rig.x*8+.5);
float rootX=.5-Rig.y;
float angle=0;
if(kind==1 || kind==2)
    angle=(kind==1?.09:.035)*sin(Cycle+Phase+rootX*7)*clamp(Effort,.5,1.4);
if(kind>=3 && kind<=6)
{
    float side=(kind==3 || kind==5)?1:-1;
    bool pelvic=kind>=5;
    float stroke=pelvic?.10:lerp(.28,.13,saturate(Effort-1));
    angle=side*(stroke*sin(FinCycle+Phase+(pelvic?.8:0))+.18*Turn*side);
}
float dy=Offset.x-.5, dz=.5-Offset.y;
float deltaY=(dy*(cos(angle)-1)-dz*sin(angle))*Length;
float deltaZ=(dy*sin(angle)+dz*(cos(angle)-1))*Length;
return float3(RX,RY,RZ)*(sway+deltaY)+float3(UX,UY,UZ)*deltaZ;

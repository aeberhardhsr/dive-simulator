"""Deterministic original fish atlases. No reference photograph pixels are used."""
from pathlib import Path
import hashlib
import json
import numpy as np
from PIL import Image
from fish_profiles import FISH

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'Art/Fish';OUT.mkdir(parents=True,exist_ok=True)
SIZE=2048

def smooth(a,b,x):
    t=np.clip((x-a)/(b-a),0,1);return t*t*(3-2*t)

def noise(x,y,seed=0):
    ix=np.floor(x);iy=np.floor(y);fx=x-ix;fy=y-iy
    fx=fx*fx*(3-2*fx);fy=fy*fy*(3-2*fy)
    def h(a,b):return np.mod(np.sin(a*127.1+b*311.7+seed*91.13)*43758.5453,1)
    return (h(ix,iy)*(1-fx)+h(ix+1,iy)*fx)*(1-fy)+(h(ix,iy+1)*(1-fx)+h(ix+1,iy+1)*fx)*fy

def main():
    y,x=np.mgrid[0:SIZE,0:SIZE].astype(np.float32);u=(x+.5)/SIZE;v=1-(y+.5)/SIZE
    angle=v/.72*np.pi*2;z=np.sin(angle)
    records=[]
    for index,(name,cfg) in enumerate(FISH.items()):
        top,side,belly={
            'Perch':((.18,.23,.12),(.54,.58,.35),(.79,.78,.62)),
            'Roach':((.14,.20,.20),(.64,.67,.65),(.82,.83,.76)),
            'Pike':((.15,.23,.12),(.36,.44,.25),(.75,.76,.57)),
            'Whitefish':((.19,.23,.24),(.61,.64,.63),(.83,.83,.75)),
            'Burbot':((.20,.22,.12),(.43,.40,.24),(.66,.60,.39)),
        }[name]
        dorsal=smooth(.02,.90,z)[...,None];ventral=smooth(.25,.95,-z)[...,None]
        color=np.array(side)[None,None,:]*(1-dorsal)+np.array(top)[None,None,:]*dorsal
        color=color*(1-ventral)+np.array(belly)[None,None,:]*ventral
        broad=noise(u*13,v*21,index)-.5;fine=noise(u*210,v*150,index+20)-.5
        color*=1+broad[...,None]*.13+fine[...,None]*.055
        # Staggered overlapping scale margins; scale density differs by model.
        cols=cfg['scales'];rows=36 if name=='Roach' else 44
        yy=v/.72*rows;row=np.floor(yy);xx=u*cols+(row%2)*.5
        sx=xx-np.floor(xx);sy=yy-row
        curve=.26+.58*(1-(2*sy-1)**2)
        edge=np.exp(-((sx-curve)/.055)**2)
        detail=(1-smooth(.74,.84,u))*(1-smooth(.8,.98,np.abs(z)))
        if name=='Burbot':detail*=.07 # tiny embedded scales; smooth mottled skin
        if name=='Pike':detail*=.35
        scale_variation=noise(np.floor(xx),row,index+40)-.5
        color*=1+detail[...,None]*(scale_variation[...,None]*.09-edge[...,None]*.12)
        height=detail*((sx-.5)*.16-edge*.23)
        rough=.30+edge*.06*detail+fine*.04
        # Narrow lateral line with pore spacing, fading into the head.
        line=np.exp(-((z-.08)/.018)**2)*(1-smooth(.72,.84,u))
        color*=1-line[...,None]*.14
        height-=line*.08
        if name=='Perch':
            bars=np.zeros_like(u)
            for j,c in enumerate([.12,.23,.35,.47,.59,.70]):
                bend=c+.009*np.sin(angle*4+j)+.012*z
                bar=np.exp(-((u-bend)/(.022+.008*(1-z)))**4)
                bars=np.maximum(bars,bar)
            bars*=smooth(-.75,-.10,z)*(1-smooth(.77,.85,u))
            color*=1-bars[...,None]*.64
        elif name=='Pike':
            spot=smooth(.60,.73,noise(u*48,v*38,44))*(1-smooth(.60,.9,z))*(1-smooth(.78,.86,u))
            color=color*(1-spot[...,None]*.62)+np.array([.74,.75,.46])*spot[...,None]*.62
        elif name=='Burbot':
            marble=noise(u*23+noise(u*7,v*11,5)*3,v*28,65)
            dark=smooth(.28,.72,marble)*(1-smooth(.5,.98,-z))
            color*=1-dark[...,None]*.60
            height+=fine*.06
        # Fins: a padded stripe with longitudinal rays and branching striae.
        fin=(v>.735)&(v<.945);fv=np.clip((v-.76)/.17,0,1)
        ray=(.5+.5*np.cos(u*np.pi*48))**14
        branches=(.5+.5*np.cos((u*96+fv*.85)*np.pi))**12*smooth(.3,.8,fv)
        finvalue=.43+.12*ray+.045*branches+.06*(1-fv)+fine*.04
        color[fin]=np.repeat(finvalue[...,None],3,axis=2)[fin]
        height[fin]=(ray*.22+branches*.04)[fin]
        rough[fin]=.37
        # Utility swatches at V .95..1: gill margin, lip, iris and pupil.
        utility=v>=.95
        color[utility]=(.12,.13,.095);height[utility]=0;rough[utility]=.32
        lip=utility&(u>.25)&(u<.4);color[lip]=(.51,.48,.32)
        iris=utility&(u>.44)&(u<.70)
        iriscolor=(.71,.25,.07) if name=='Roach' else (.65,.57,.30)
        striae=.8+.2*np.cos(np.arctan2((v-.975)*5,u-.57)*55)
        color[iris]=(striae[...,None]*np.array(iriscolor))[iris];rough[iris]=.20
        pupil=utility&(u>.73);color[pupil]=(.012,.018,.015);rough[pupil]=.10
        # Height-derived tangent normals. Texture row increases opposite UV V;
        # output uses DirectX green convention for Unreal. Preview reverses G.
        dy,dx=np.gradient(height)
        strength=SIZE*.004
        normal=np.stack([-dx*strength,-dy*strength,np.ones_like(dx)],axis=-1)
        normal/=np.linalg.norm(normal,axis=-1)[...,None]
        maps={'Color':np.clip(color,0,1),'Normal':normal*.5+.5,
              'Surface':np.stack([np.ones_like(rough),np.clip(rough,0,1),np.where(fin,.35,1.)],axis=-1)}
        files={}
        for kind,data in maps.items():
            path=OUT/f'{name}_{kind}.png';Image.fromarray(np.uint8(np.clip(data*255,0,255))).save(path)
            files[kind]={'file':path.relative_to(ROOT).as_posix(),'sha256':hashlib.sha256(path.read_bytes()).hexdigest()}
        records.append(dict(name=name,maps=files))
        print('FISH_TEXTURES',name,flush=True)
    report={'size':SIZE,'provenance':'Original deterministic procedural artwork; reference photos were not sampled or redistributed.',
            'normal_convention':'DirectX','assets':records}
    (ROOT/'Documentation/FISH_TEXTURES.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')

if __name__=='__main__':main()

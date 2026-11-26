"""Authored morphology controls in fractions of total length; not specimen scans."""
X = [-.37, -.29, -.18, -.03, .12, .25, .36, .45, .50]
FISH = {
    'Perch': dict(length=.30, width=[.012,.025,.045,.065,.075,.065,.045,.018,.003],
        height=[.024,.045,.080,.120,.130,.116,.078,.039,.013],
        centre=[0,0,0,.006,.006,0,-.012,-.025,-.028], eye=.365, eye_r=.020, scales=77),
    'Roach': dict(length=.24, width=[.012,.022,.038,.052,.060,.057,.040,.018,.003],
        height=[.023,.043,.080,.125,.135,.118,.076,.037,.012],
        centre=[0,0,0,0,0,0,-.006,-.015,-.017], eye=.365, eye_r=.023, scales=54),
    'Pike': dict(length=.75, width=[.014,.025,.038,.047,.047,.048,.039,.031,.012],
        height=[.024,.042,.062,.072,.074,.063,.041,.022,.012],
        centre=[0,0,0,0,0,-.003,-.012,-.020,-.020], eye=.305, eye_r=.012, scales=110),
    'Whitefish': dict(length=.38, width=[.013,.022,.038,.052,.061,.056,.038,.014,.003],
        height=[.024,.042,.072,.108,.118,.105,.065,.031,.012],
        centre=[0,0,0,0,.003,0,-.008,-.022,-.024], eye=.37, eye_r=.019, scales=100),
    'Burbot': dict(length=.55, width=[.013,.019,.032,.045,.056,.070,.064,.041,.014],
        height=[.027,.038,.052,.063,.073,.070,.050,.030,.017],
        centre=[0,0,0,0,0,-.004,-.012,-.017,-.019], eye=.36, eye_r=.013, scales=150),
}

def shape(name,x,key):
    """Clamped cubic Hermite interpolation avoids the old polygonal profile."""
    ys=FISH[name][key]; x=max(X[0],min(X[-1],x))
    i=next((j for j in range(len(X)-1) if x<=X[j+1]),len(X)-2)
    d=X[i+1]-X[i]; t=(x-X[i])/d
    def slope(k):
        lo=max(0,k-1); hi=min(len(X)-1,k+1)
        return (ys[hi]-ys[lo])/(X[hi]-X[lo])
    value=(2*t**3-3*t*t+1)*ys[i]+(t**3-2*t*t+t)*d*slope(i)+(-2*t**3+3*t*t)*ys[i+1]+(t**3-t*t)*d*slope(i+1)
    return max(min(ys[i],ys[i+1]),min(max(ys[i],ys[i+1]),value))

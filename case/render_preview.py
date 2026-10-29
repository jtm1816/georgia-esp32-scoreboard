"""Render the generated CAD triangle data using an opaque CPU depth buffer."""
from pathlib import Path
import math
import numpy as np
from PIL import Image, ImageDraw, ImageFont

ROOT=Path(__file__).resolve().parent
data=np.load(ROOT/'render-data.npz')

def render(az,el,w=640,h=650):
    az,el=map(math.radians,(az,el))
    direction=np.array([math.cos(el)*math.cos(az),math.cos(el)*math.sin(az),math.sin(el)])
    right=np.cross([0,0,1],direction);right/=np.linalg.norm(right);up=np.cross(direction,right)
    vertices=np.concatenate([data[k] for k in data.files if k.endswith('_vertices')])
    center=(vertices.max(0)+vertices.min(0))/2
    proj=(vertices-center)@np.array([right,up]).T
    scale=min((w-65)/np.ptp(proj,axis=0)[0],(h-80)/np.ptp(proj,axis=0)[1]);mid=(proj.max(0)+proj.min(0))/2
    img=np.full((h,w,3),[238,238,236],dtype=np.uint8);depth=np.full((h,w),-np.inf)
    light=np.array([-.3,-.7,1.]);light/=np.linalg.norm(light)
    for name in ['stand','rear','screen','bezel','letters']:
        v=data[name+'_vertices'];faces=data[name+'_faces'];base=data[name+'_color'].astype(float)
        if name in ['rear','stand']:base=np.array([53.,53.,56.])
        vv=v-center;px=(vv@right-mid[0])*scale+w/2;py=-(vv@up-mid[1])*scale+h/2;pz=vv@direction
        for face in faces:
            points=v[face];n=np.cross(points[1]-points[0],points[2]-points[0]);length=np.linalg.norm(n)
            if length<1e-10:continue
            n/=length
            if n@direction<=1e-8:continue
            x=px[face];y=py[face];z=pz[face]
            xmin=max(0,int(np.floor(x.min())));xmax=min(w-1,int(np.ceil(x.max())))
            ymin=max(0,int(np.floor(y.min())));ymax=min(h-1,int(np.ceil(y.max())))
            if xmax<xmin or ymax<ymin:continue
            den=(y[1]-y[2])*(x[0]-x[2])+(x[2]-x[1])*(y[0]-y[2])
            if abs(den)<1e-10:continue
            xx,yy=np.meshgrid(np.arange(xmin,xmax+1)+.5,np.arange(ymin,ymax+1)+.5)
            a=((y[1]-y[2])*(xx-x[2])+(x[2]-x[1])*(yy-y[2]))/den
            b=((y[2]-y[0])*(xx-x[2])+(x[0]-x[2])*(yy-y[2]))/den;c=1-a-b
            zz=a*z[0]+b*z[1]+c*z[2];target=depth[ymin:ymax+1,xmin:xmax+1]
            mask=(a>=-1e-7)&(b>=-1e-7)&(c>=-1e-7)&(zz>target)
            target[mask]=zz[mask];img[ymin:ymax+1,xmin:xmax+1][mask]=np.clip(base*(.65+.35*max(0,n@light)),0,255)
    return Image.fromarray(img)

canvas=Image.new('RGB',(1980,950),(238,238,236));d=ImageDraw.Draw(canvas)
def font(s,b=False):
    candidates=['C:/Windows/Fonts/'+('arialbd.ttf' if b else 'arial.ttf'),'/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf']
    return ImageFont.truetype(next(p for p in candidates if Path(p).exists()),s)
d.text((990,45),'GEORGIA | ESP32-035 3.5-INCH CASE',font=font(33,True),fill='#222222',anchor='mm')
for i,(title,az,el) in enumerate([('Front / assembled',-70,20),('Straight-back USB-C power port',115,18),('30 degrees back from vertical',0,0)]):
    canvas.paste(render(az,el),(20+i*650,145));d.text((340+i*650,118),title,font=font(24),fill='#333333',anchor='mm')
d.text((990,845),'Red bezel + white letters | Black shell and PETG stand | 125 x 88 x 30 mm body',font=font(23),fill='#333333',anchor='mm')
d.text((990,888),'94.5 x 47.9 mm hole spacing | 14 x 5.5 mm power opening | Physical fit unverified',font=font(20),fill='#555555',anchor='mm')
canvas.save(ROOT/'case-preview.png')

"""Parametric Georgia CYD enclosure. Run: pip install manifold3d trimesh numpy fonttools
Then python design_case.py. All lengths are millimeters. See ASSEMBLY.md before printing.
"""
from pathlib import Path
import json, math, os
import numpy as np
import manifold3d as md
import trimesh
from fontTools.ttLib import TTFont
from fontTools.pens.basePen import BasePen

OUT = Path(__file__).resolve().parent
P = dict(board_w=86.0,board_h=50.0,board_thickness=1.6,hole_spacing_x=78.0,hole_spacing_y=42.0,
         case_w=100.0,case_h=80.0,case_depth=24.0,wall=2.4,corner=6.0,
         screen_window_w=60.0,screen_window_h=45.5,screen_pocket_w=70.6,screen_pocket_h=50.6,
         board_front_z=5.8,insert_od=4.6,insert_length=4.0,insert_pocket_depth=4.3,
         boss_od=7.2,screw_clearance=3.4,screw_head_d=6.2,screw_head_recess=1.8,
         usb_w=14.0,usb_h=5.5,usb_y=-12.0,tilt_from_vertical=30.0)
if (OUT/'parameters.json').exists():P.update(json.loads((OUT/'parameters.json').read_text()))

def rr(w,h,r):
    return md.CrossSection.square((w-2*r,h-2*r),True).offset(r,circular_segments=48)
def box(w,h,d,z=0,x=0,y=0):return md.Manifold.cube((w,h,d),True).translate((x,y,z+d/2))
def cyl(d,h,z,x,y):return md.Manifold.cylinder(h,d/2,circular_segments=64).translate((x,y,z))
def union(shapes):
    out=md.Manifold()
    for s in shapes:out=out+s
    return out

holes=[(sx*P['hole_spacing_x']/2,sy*P['hole_spacing_y']/2) for sx in (-1,1) for sy in (-1,1)]
outer=rr(P['case_w'],P['case_h'],P['corner'])
bezel=outer.extrude(P['wall'])
bezel-=rr(P['screen_window_w'],P['screen_window_h'],1.2).extrude(12).translate((0,0,-1))
for x,y in holes:
    bezel+=cyl(P['boss_od'],P['board_front_z']-2,2,x,y)
bezel-=rr(P['screen_pocket_w'],P['screen_pocket_h'],0.6).extrude(8).translate((0,0,1.8))
for x,y in holes:
    bezel-=cyl(P['insert_od'],P['insert_pocket_depth']+0.1,
               P['board_front_z']-P['insert_pocket_depth'],x,y)

# Rounded back shell with a 2.4 mm back wall; front is open.
rear=outer.extrude(P['case_depth']-P['wall']).translate((0,0,P['wall']))
rear-=rr(P['case_w']-2*P['wall'],P['case_h']-2*P['wall'],P['corner']-P['wall']).extrude(
    P['case_depth']-2*P['wall']+0.1).translate((0,0,P['wall']-0.1))
pcb_back=P['board_front_z']+P['board_thickness']
for x,y in holes:
    rear+=cyl(P['boss_od'],P['case_depth']-P['wall']-pcb_back+0.1,pcb_back,x,y)
    rear-=cyl(P['screw_clearance'],P['case_depth']-pcb_back+2,pcb_back-1,x,y)
    rear-=cyl(P['screw_head_d'],P['screw_head_recess']+0.1,
              P['case_depth']-P['screw_head_recess'],x,y)

class OutlinePen(BasePen):
    def __init__(self, gs):super().__init__(gs);self.contours=[];self.path=[]
    def _moveTo(self,p):self.path=[p]
    def _lineTo(self,p):self.path.append(p)
    def _qCurveToOne(self,a,b):
        p=self._getCurrentPoint()
        for t in np.linspace(0,1,13)[1:]:self.path.append(tuple((1-t)**2*np.array(p)+2*(1-t)*t*np.array(a)+t*t*np.array(b)))
    def _curveToOne(self,a,b,c):
        p=self._getCurrentPoint()
        for t in np.linspace(0,1,17)[1:]:self.path.append(tuple((1-t)**3*np.array(p)+3*(1-t)**2*t*np.array(a)+3*(1-t)*t*t*np.array(b)+t**3*np.array(c)))
    def _closePath(self):
        if len(self.path)>2:self.contours.append(self.path)
        self.path=[]
    def _endPath(self):self._closePath()

font_candidates=[os.environ.get('GEORGIA_CASE_FONT',''),'C:/Windows/Fonts/ariblk.ttf',
 '/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf','/Library/Fonts/Arial Bold.ttf']
font_path=next((f for f in font_candidates if f and Path(f).exists()),None)
if not font_path:raise RuntimeError('Set GEORGIA_CASE_FONT to a bold TrueType font path.')
font=TTFont(font_path);gs=font.getGlyphSet();cmap=font.getBestCmap();scale=7.0/font['OS/2'].sCapHeight if hasattr(font['OS/2'],'sCapHeight') else 7.0/(font['head'].unitsPerEm*.73)

def word(label,top):
    advances=[font['hmtx'][cmap[ord(c)]][0]*scale+0.45 for c in label]
    cur=-sum(advances)/2;out=md.CrossSection()
    for c,adv in zip(label,advances):
        pen=OutlinePen(gs);gs[cmap[ord(c)]].draw(pen)
        cs=md.CrossSection(pen.contours,md.FillRule.EvenOdd).scale((scale,scale))
        cx=cur+adv/2;angle=(-cx if top else cx)/120*180/math.pi
        baseline=(30-cx*cx/240) if top else (-37+cx*cx/240)
        cs=cs.translate((-adv/2,0)).rotate(angle).translate((cx,baseline))
        out=out+cs;cur+=adv
    return out
words=word('GEORGIA',True)+word('BULLDOGS',False)
letters=words.extrude(1.2).translate((0,0,-0.6))
bezel-=words.offset(.12,circular_segments=16).extrude(.75).translate((0,0,-.1))

# Case leans 30 degrees back from vertical. In local case space +z is inward.
a=math.radians(P['tilt_from_vertical']);s,c=math.sin(a),math.cos(a)
lift=P['case_h']/2*c+P['case_depth']*s+3
pose=np.array([[1,0,0,0],[0,s,c,0],[0,c,-s,lift]])
def posed(shape):return shape.transform(pose)

# Horizontal plug axis: vertical rear mounting pad, independent of screen tilt.
# Build the pad in desk coordinates, then return it to case coordinates.
inv=np.linalg.inv(np.vstack([pose,[0,0,0,1]]))[:3]
port_center_z=3.0+8.0  # pad bottom rests on the 3 mm stand base
port_local_y=(port_center_z+s*P['case_depth']-lift)/c
port_center_y=s*port_local_y+c*P['case_depth']
port_back_y=port_center_y+8.0
pad_world=box(22,16,16,port_center_z-8,0,port_back_y-8)
pad_local=pad_world.transform(inv)
# Limit the pad to the outside of the inner rear wall, preserving the cavity.
pad_local=pad_local ^ box(200,200,80,P['case_depth']-P['wall'])
rear+=pad_local
# Hollow the pad from inside so the adapter seats in a 2.4 mm vertical face.
recess_world=box(18,40,12,port_center_z-6,0,port_back_y-P['wall']-20)
rear-=recess_world.transform(inv)
# Exact 14 x 5.5 opening; cable enters horizontally, parallel to the desk.
port_world=box(P['usb_w'],40,P['usb_h'],port_center_z-P['usb_h']/2,0,port_back_y-16)
rear-=port_world.transform(inv)

# Rear-only cradle; its front edge is trimmed behind the bezel bottom edge.
stand=rr(94,74,4).extrude(3).translate((0,13,0))
back=P['case_depth']+0.35
y0=-P['case_h']/2;y1=15
front0=(s*y0+c*back,c*y0-s*back+lift)
front1=(s*y1+c*back,c*y1-s*back+lift)
profile=[(front0[0],2),(48,2),(48,5),front1,front0]
brace=md.CrossSection([profile],md.FillRule.EvenOdd).extrude(6)
for x in (-43,37):
    stand+=brace.transform([[0,0,1,x],[1,0,0,0],[0,1,0,0]])
# Solid lower plinth follows the underside of the tilted case down to the base.
# Its front edge aligns with the bottom edge of the red bezel.
base_front=-P['case_h']/2*s
bottom_front=(base_front, -P['case_h']/2*c+lift)
bottom_back=(base_front+P['case_depth']*c, -P['case_h']/2*c-P['case_depth']*s+lift)
plinth_profile=[(bottom_front[0],2),(bottom_back[0],2),bottom_back,bottom_front]
plinth=md.CrossSection([plinth_profile],md.FillRule.EvenOdd).extrude(P['case_w']-12)
stand+=plinth.transform([[0,0,1,-(P['case_w']-12)/2],[1,0,0,0],[0,1,0,0]])
stand-=box(200,200,200,-1,0,base_front-100)

def mesh(shape):
    m=shape.simplify(0.005).to_mesh64();return trimesh.Trimesh(np.asarray(m.vert_properties)[:,:3],np.asarray(m.tri_verts),process=True)

models={
 'front-bezel-red.stl':bezel,
 'lettering-white.stl':letters.translate((0,0,.6)),
 'rear-shell-black.stl':rear.rotate((180,0,0)).translate((0,0,P['case_depth'])),
 'stand-black.stl':stand,
}
report={}
for name,shape in models.items():
    m=mesh(shape)
    m.apply_translation((0,0,-m.bounds[0,2]))
    if not m.is_watertight or m.volume<=0:raise RuntimeError(f'Invalid solid: {name}')
    m.export(OUT/name)
    report[name]={'watertight':bool(m.is_watertight),'volume_mm3':round(float(m.volume),2),
                  'bounds_mm':np.round(m.bounds,3).tolist(),'components':len(m.split())}

# A colored assembly, for viewing only. Individual STLs are already print-oriented.
scene=trimesh.Scene()
for name,shape,color in [('red bezel',posed(bezel),[186,12,47,255]),('white lettering',posed(letters),[245,245,245,255]),
                         ('black rear',posed(rear),[30,30,32,255]),('black stand',stand,[30,30,32,255])]:
    m=mesh(shape);m.visual.vertex_colors=np.tile(color,(len(m.vertices),1));scene.add_geometry(m,node_name=name)
scene.export(OUT/'case-assembly.glb')
# Drawing-only screen reference; not included among printable parts.
glass=posed(rr(59.5,45,1).extrude(.3).translate((0,0,1.8)))
gm=mesh(glass);gm.visual.vertex_colors=np.tile([12,15,19,255],(len(gm.vertices),1));scene.add_geometry(gm,node_name='screen reference - do not print')
scene.export(OUT/'case-assembly.glb')
np.savez_compressed(OUT/'render-data.npz',**{
    key:val for name,shape,color in [('bezel',posed(bezel),[186,12,47]),('letters',posed(letters),[245,245,245]),
        ('rear',posed(rear),[30,30,32]),('stand',stand,[30,30,32]),('screen',glass,[12,15,19])]
    for key,val in [(name+'_vertices',mesh(shape).vertices),(name+'_faces',mesh(shape).faces),(name+'_color',color)]})
(OUT/'parameters.json').write_text(json.dumps(P,indent=2)+'\n')
(OUT/'validation.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))

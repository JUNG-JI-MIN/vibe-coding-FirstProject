"""Read-only OBJ topology inspection; does not build or run the game."""
from pathlib import Path
import json, collections
import numpy as np

root=Path(__file__).parent
points=[];groups={};current='';faces=[]
for line in (root/'SimpleGame/Res/Station_Early_Model.obj').read_text().splitlines():
    row=line.split()
    if not row: continue
    if row[0]=='v': points.append(tuple(map(float,row[1:4])))
    elif row[0] in ('o','g'): current=row[1]
    elif row[0]=='f':
        index=[int(v.split('/')[0]) for v in row[1:]]
        poly=[points[i-1 if i>0 else len(points)+i] for i in index]
        groups.setdefault(current,[]).extend(poly)
        faces.append((current,np.array(poly,dtype=float)))

def side(a,b,c): return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
def inside(p,a,b,c):
    s=[side(a,b,p),side(b,c,p),side(c,a,p)]
    return not (min(s)<-1e-9 and max(s)>1e-9)
def triangles(poly):
    normal=np.cross(poly,np.roll(poly,-1,axis=0)).sum(axis=0)
    axis=int(np.argmax(np.abs(normal)));plane=np.delete(poly,axis,axis=1)
    area=sum(plane[i,0]*plane[(i+1)%len(poly),1]-plane[(i+1)%len(poly),0]*plane[i,1] for i in range(len(poly)))
    sign=1 if area>=0 else -1;indices=list(range(len(poly)));out=[]
    while len(indices)>=3:
        if len(indices)==3:
            out.append(poly[indices]);break
        for k,b in enumerate(indices):
            a=indices[k-1];c=indices[(k+1)%len(indices)]
            if side(plane[a],plane[b],plane[c])*sign<=1e-6: continue
            if any(inside(plane[j],plane[a],plane[b],plane[c]) for j in indices if j not in (a,b,c)): continue
            out.append(poly[[a,b,c]]);indices.pop(k);break
        else:
            for k,b in enumerate(indices):
                a=indices[k-1];c=indices[(k+1)%len(indices)]
                if abs(side(plane[a],plane[b],plane[c]))<1e-6: indices.pop(k);break
            else:
                if len(indices)==4:
                    out.extend([poly[[indices[0],indices[1],indices[2]]],poly[[indices[0],indices[2],indices[3]]]])
                    return out
                raise ValueError('Untriangulatable polygon')
    projected=[np.delete(t,axis,axis=1) for t in out]
    tri_area=sum(abs(side(t[0],t[1],t[2])) for t in projected)
    if abs(abs(area)-tri_area)>max(1e-4,abs(area)*1e-5) and len(poly)!=4: raise ValueError('Area mismatch')
    return out

size=404;axis=np.arange(size)*.5-100.75
xx,zz=np.meshgrid(axis,axis);floor=np.zeros((size,size),bool);walls=[]
count=0
for name,poly in faces:
    if name.startswith('Player'):continue
    try: pieces=triangles(poly)
    except ValueError as error:
        print(name,poly.tolist())
        raise ValueError(f'{name}: {error}')
    count+=len(pieces)
    for t in pieces:
        if name.startswith('Floor_') and np.max(np.abs(t[:,1]))<.01:
            p=t[:,[0,2]];x0=max(0,int((p[:,0].min()+101)/.5));x1=min(size-1,int((p[:,0].max()+101)/.5))
            z0=max(0,int((p[:,1].min()+101)/.5));z1=min(size-1,int((p[:,1].max()+101)/.5))
            x,z=xx[z0:z1+1,x0:x1+1],zz[z0:z1+1,x0:x1+1]
            s=[(p[(i+1)%3,0]-p[i,0])*(z-p[i,1])-(p[(i+1)%3,1]-p[i,1])*(x-p[i,0]) for i in range(3)]
            mask=~((np.minimum.reduce(s)<-1e-4)&(np.maximum.reduce(s)>1e-4))
            floor[z0:z1+1,x0:x1+1]|=mask
        solid=name.startswith(('Walls_','DAMAGED','Door_')) or name.startswith('Hab_') and not name.startswith('Hab_Bed')
        if solid and t[:,1].min()<1.8 and t[:,1].max()>.3 and np.ptp(t[:,1])>.1:
            p=t[:,[0,2]];a,b=max(((p[i],p[j]) for i in range(3) for j in range(i+1,3)),key=lambda q:np.linalg.norm(q[0]-q[1]))
            walls.append((a,b))
walk=floor.copy()
for a,b in walls:
    x0=max(0,int((min(a[0],b[0])+100.2)/.5));x1=min(size-1,int((max(a[0],b[0])+101.8)/.5))
    z0=max(0,int((min(a[1],b[1])+100.2)/.5));z1=min(size-1,int((max(a[1],b[1])+101.8)/.5))
    x,z=xx[z0:z1+1,x0:x1+1],zz[z0:z1+1,x0:x1+1];edge=b-a
    t=np.clip(((x-a[0])*edge[0]+(z-a[1])*edge[1])/max(np.dot(edge,edge),1e-5),0,1)
    distance=np.sqrt((x-a[0]-t*edge[0])**2+(z-a[1]-t*edge[1])**2)
    walk[z0:z1+1,x0:x1+1]&=distance>=.365
for name,values in groups.items():
    p=np.array(values);lo=p.min(axis=0);hi=p.max(axis=0)
    equipment=not name.startswith(('Floor_','Walls_','Ceiling','Door_','Vent_','Label_','DAMAGED','Player')) and not (name.startswith('Hab_') and not name.startswith('Hab_Bed'))
    if equipment and lo[1]<1.8 and hi[1]>.3:
        dx=xx-np.clip(xx,lo[0],hi[0]);dz=zz-np.clip(zz,lo[2],hi[2]);walk&=dx*dx+dz*dz>=.35**2

def nearest(p):
    candidates=np.where(walk);dist=(xx[candidates]-p[0])**2+(zz[candidates]-p[1])**2;i=int(np.argmin(dist));return (int(candidates[0][i]),int(candidates[1][i]))
anchors={'Docking':(0,69),'Control':(-3,-5),'Warehouse':(-29,60),'PowerDoor':(-75,23),'Wiring':(-83,28),'Valve':(-83,13),
         'Medical':(51,-23),'Habitation':(58,18),'LifeSupport':(-83,-18),'Research':(0,-66),'Escape':(73,-72),'Fuel':(69,-73),'Clamp':(78,-73),'Pressure':(73,-78)}
start=nearest(anchors['Docking']);seen={start};queue=collections.deque([start])
while queue:
    z,x=queue.popleft()
    for dz,dx in ((-1,0),(1,0),(0,-1),(0,1)):
        cell=(z+dz,x+dx)
        if 0<=cell[0]<size and 0<=cell[1]<size and walk[cell] and cell not in seen:seen.add(cell);queue.append(cell)
result={key:{'position':[float(xx[nearest(p)]),float(zz[nearest(p)])],'reachable':nearest(p) in seen} for key,p in anchors.items()}
print(json.dumps({'triangles':count,'reachable_cells':len(seen),'anchors':result},ensure_ascii=False,indent=2))
if not all(r['reachable'] for r in result.values()):raise SystemExit('Disconnected gameplay anchor')

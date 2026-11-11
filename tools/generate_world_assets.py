#!/usr/bin/env python3
"""Generate native logo run meshes and mapped Monza footprints without runtime image dependencies."""
import json
import hashlib
from pathlib import Path
from generate_track_assets import to_scene
ROOT=Path(__file__).resolve().parents[1]
def build():
 text='#pragma once\n#include <array>\n#include <string_view>\nnamespace apex3d::three {\n'
 logos=json.loads((ROOT/'data/team-logo-pixels.json').read_text());runs=[];specs=[]
 assert len(logos)==10 and len({logo['team']for logo in logos})==10
 for logo in logos:
  start=len(runs);w=logo['width'];h=logo['height'];p=logo['pixels']
  assert len(p)==w*h and all(len(c)==4 and all(0<=v<=255 for v in c)for c in p)
  assert hashlib.sha256((ROOT/f'assets/team-logos/{logo["team"]}.webp').read_bytes()).hexdigest()==logo['sha256']
  for y in range(h):
   x=0
   while x<w:
    col=p[y*w+x];end=x+1
    while end<w and p[y*w+end]==col:end+=1
    if col[3]:runs.append([x,y,end-x,*col[:3]])
    x=end
  specs.append((logo['team'],w,h,start,len(runs)-start))
 text+='struct LogoRun {int x,y,width,r,g,b;};\n'
 text+=f'inline constexpr std::array<LogoRun,{len(runs)}> logoRuns{{{{\n'
 text+=''.join('    {'+', '.join(map(str,r))+'},\n'for r in runs)+'}};\n'
 text+='struct TeamLogo {std::string_view slug;int width,height,first,count;};\n'
 text+='inline constexpr std::array<TeamLogo,10> teamLogos{{\n'+''.join('    {"'+a+'", '+', '.join(map(str,[b,c,d,e]))+'},\n'for a,b,c,d,e in specs)+'}};\n'
 features=json.loads((ROOT/'data/monza-scenery.json').read_text())['features'];points=[];polys=[]
 for f in features:
  # OSM longitude/latitude -> scene units with the measured track's own transform (no warping).
  p=f['source_points'];p=p[:-1]if p[0]==p[-1]else p;p=[tuple(round(v,6)for v in to_scene(lon,lat))for lon,lat in p]
  polys.append((f['kind'],f['name'],f['height_m'],f['roof']!='flat',len(points),len(p)));points+=p
 text+='struct MapPoint {double x,y;};\n'
 text+=f'inline constexpr std::array<MapPoint,{len(points)}> mapPoints{{{{\n'+''.join(f'    {{{x}, {y}}},\n'for x,y in points)+'}};\n'
 text+='struct MapFootprint {bool wood;std::string_view name;double height;bool pitched;int first,count;};\n'
 text+=f'inline constexpr std::array<MapFootprint,{len(polys)}> mapFootprints{{{{\n'
 text+=''.join('    {'+str(k=='wood').lower()+', '+json.dumps(name)+', '+str(h)+', '+str(roof).lower()+', '+str(a)+', '+str(n)+'},\n'for k,name,h,roof,a,n in polys)+'}};\n}\n'
 return text
if __name__=='__main__':
 import argparse
 parser=argparse.ArgumentParser();parser.add_argument('--check',action='store_true');args=parser.parse_args();p=ROOT/'include/apex3d/generated/monza_world.hpp';text=build()
 if args.check:assert p.read_text()==text
 else:p.write_text(text)

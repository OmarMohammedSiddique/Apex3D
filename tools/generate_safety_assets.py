#!/usr/bin/env python3
"""Embed audited race flags, intro artwork and mapped gravel areas."""
import json,hashlib
from pathlib import Path
from generate_track_assets import to_scene
ROOT=Path(__file__).resolve().parents[1]
def build():
 text='#pragma once\n#include <array>\n#include <string_view>\nnamespace apex3d::cp3 {\n'
 events=json.loads((ROOT/'data/monza-2025-flags.json').read_text())['events']
 text+='struct FlagEvent {int raceMs;std::string_view flag;int sector,driver;};\n'
 text+=f'inline constexpr std::array<FlagEvent,{len(events)}> flagEvents{{{{\n'
 for e in events:text+='    {'+str(e['race_ms'])+', "'+e['flag']+'", '+str(e['sector'])+', '+str(e['driver'])+'},\n'
 text+='}};\n'
 logo=json.loads((ROOT/'data/f1-logo-pixels.json').read_text());assert hashlib.sha256((ROOT/'assets/f1-logo.png').read_bytes()).hexdigest()==logo['sha256'];w=logo['width'];h=logo['height'];runs=[]
 for y in range(h):
  x=0
  while x<w:
   p=logo['pixels'][y*w+x];end=x+1
   while end<w and logo['pixels'][y*w+end]==p:end+=1
   if p[3]>=100:runs.append((x,y,end-x))
   x=end
 text+='struct IntroRun {int x,y,width;};\n'
 text+=f'inline constexpr int introWidth={w},introHeight={h};\ninline constexpr std::array<IntroRun,{len(runs)}> introLogo{{{{\n'
 text+=''.join('    {'+', '.join(map(str,r))+'},\n'for r in runs)+'}};\n'
 patches=json.loads((ROOT/'data/monza-gravel.json').read_text())['patches'];points=[];spans=[]
 for patch in patches:
  # Same rotation/shift/scale as the measured track, so run-off stays beside the real road.
  p=[tuple(round(v,6)for v in to_scene(lon,lat))for lon,lat in patch['source_points']];spans.append((len(points),len(p)));points+=p
 text+='struct GravelPoint {double x,y;};\nstruct GravelPatch {int first,count;};\n'
 text+=f'inline constexpr std::array<GravelPoint,{len(points)}> gravelPoints{{{{\n'+''.join(f'    {{{x}, {y}}},\n'for x,y in points)+'}};\n'
 text+=f'inline constexpr std::array<GravelPatch,{len(spans)}> gravelPatches{{{{\n'+''.join(f'    {{{a}, {b}}},\n'for a,b in spans)+'}};\n}\n'
 return text
if __name__=='__main__':
 import argparse
 p=argparse.ArgumentParser();p.add_argument('--check',action='store_true');args=p.parse_args();out=ROOT/'include/apex3d/generated/monza_safety.hpp';value=build()
 if args.check:assert out.read_text()==value
 else:out.write_text(value)

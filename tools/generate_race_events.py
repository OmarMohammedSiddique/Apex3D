#!/usr/bin/env python3
"""Embed inspected OpenF1 position changes and observed DRS intervals."""
import csv,argparse
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def build():
 text='#pragma once\n#include <array>\nnamespace apex3d::cp3 {\n'
 for slug,kind,fields in [('positions','PositionEvent',['driver_number','race_ms','position']),('drs','DrsInterval',['driver_number','start_ms','end_ms'])]:
  rows=list(csv.DictReader((ROOT/f'data/monza-2025-{slug}.csv').open()))
  text+=f'struct {kind} {{int driver,'+('raceMs,position;'if slug=='positions'else'startMs,endMs;')+'};\n'
  text+=f'inline constexpr std::array<{kind},{len(rows)}> recorded{slug.title()}{{{{\n'
  for r in rows:
   vals=[int(r[k])for k in fields];assert 1<=vals[0]<=87
   if slug=='positions':assert 1<=vals[2]<=20
   else:assert 0<=vals[1]<vals[2]
   text+='    {'+', '.join(map(str,vals))+'},\n'
  text+='}};\n'
 return text+'}\n'
if __name__=='__main__':
 parser=argparse.ArgumentParser();parser.add_argument('--check',action='store_true');args=parser.parse_args();out=ROOT/'include/apex3d/generated/monza_events_2025.hpp';value=build()
 if args.check:assert out.read_text()==value,'Race events stale'
 else:out.write_text(value)
 print('Race event tables verified'if args.check else'Race event tables generated')

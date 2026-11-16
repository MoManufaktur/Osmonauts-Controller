#!/usr/bin/env python3
"""Builds docs/osmonauts_spuelvorgaenge_3d.html (self-contained, works offline).

usage: npm pack three@0.158.0 && tar xzf three-0.158.0.tgz
       python3 tools/animation/build.py package/build/three.min.js docs/osmonauts_spuelvorgaenge_3d.html
URL parameters of the page: ?program=tap|auto|filter|disinfect|aquastop|all &step=N &t=0..1 &ui=0 &static=1 &lite=1
"""
import os, sys
here = os.path.dirname(os.path.abspath(__file__))
three_path, out = sys.argv[1], sys.argv[2]
tpl = open(os.path.join(here, 'template.html'), encoding='utf-8').read()
three = open(three_path, encoding='utf-8').read()
first = three.index('\n')
if three[:first].startswith('console.warn('):   # deprecation warning of the UMD build
    three = 'void 0,' + three[first + 1:]
three = three.replace('</script', '<\\/script')
open(out, 'w', encoding='utf-8').write(tpl.replace('/*THREEJS*/', '/* three.js r158 – MIT License – https://threejs.org */\n' + three))
print('written', out)

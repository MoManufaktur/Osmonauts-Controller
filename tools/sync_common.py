#!/usr/bin/env python3
"""Osmonauts – copies the shared core (firmware/common) into the src/ folder of both sketches.
The Arduino IDE only compiles files inside the sketch folder, so each sketch carries a copy.
Edit only firmware/common, then run:   python3 tools/sync_common.py
Check (CI):                             python3 tools/sync_common.py --check
"""
import filecmp, os, shutil, sys
root = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'firmware')
common = os.path.join(root, 'common')
sketches = ['osmonauts_controller', 'osmonauts_esp32']
check = '--check' in sys.argv
bad = []
for sk in sketches:
    dst = os.path.join(root, sk, 'src')
    os.makedirs(dst, exist_ok=True)
    for f in sorted(os.listdir(common)):
        a, b = os.path.join(common, f), os.path.join(dst, f)
        if not os.path.exists(b) or not filecmp.cmp(a, b, shallow=False):
            if check:
                bad.append(f'{sk}/src/{f}')
            else:
                shutil.copyfile(a, b)
                print('updated', f'{sk}/src/{f}')
if bad:
    sys.exit('out of sync with firmware/common (run tools/sync_common.py): ' + ', '.join(bad))
print('src/ folders in sync with firmware/common')

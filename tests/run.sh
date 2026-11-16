#!/bin/bash
# Osmonauts – PC simulation of the controller (no Arduino needed)
# usage:  tests/run.sh <config file> <test> [test ...]
# example: tests/run.sh config_elias.h tap stop disinfect
# tests:  tap tap2 stop stopfilter disinfect busy filterchange auto
#         tapduringauto tapduringpost aquastop bounce
cd "$(dirname "$0")"
FW=$(cd ../firmware/osmonauts_controller && pwd)
CFG=$1; shift
sed "s|^#include \"config.h\".*|#include \"$FW/$CFG\"|" $FW/osmonauts_controller.ino > /tmp/osmonauts_sim.cpp
g++ -O2 -std=gnu++17 -w -Imock -I$FW -DSKETCH='"/tmp/osmonauts_sim.cpp"' test.cpp -o /tmp/osmonauts_sim || exit 1
fail=0
for t in "$@"; do /tmp/osmonauts_sim $t || fail=1; done
exit $fail

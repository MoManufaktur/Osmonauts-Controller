#!/bin/bash
# Osmonauts – PC simulation of the controller (no Arduino needed)
# usage:  tests/run.sh <config file> <test> [test ...]
# example: tests/run.sh config_elias.h tap stop disinfect
#          tests/run.sh ../osmonauts_esp32/config_esp32_relay4.h filter   (ESP32 config, procedures only)
# tests:  tap tap2 stop stopfilter disinfect busy filterchange auto
#         tapduringauto tapduringpost aquastop bounce filter
cd "$(dirname "$0")"
FW=$(cd ../firmware/osmonauts_controller && pwd)   # Arduino sketch; the core in src/ is shared with the ESP32 sketch
CFG=$1; shift
sed "s|^#include \"config.h\".*|#include \"$FW/$CFG\"|" $FW/osmonauts_controller.ino > /tmp/osmonauts_sim.cpp
g++ -O2 -std=gnu++17 -w -DOSMO_SIM -Imock -I$FW -DSKETCH='"/tmp/osmonauts_sim.cpp"' test.cpp -o /tmp/osmonauts_sim || exit 1
fail=0
for t in "$@"; do /tmp/osmonauts_sim $t || fail=1; done
exit $fail

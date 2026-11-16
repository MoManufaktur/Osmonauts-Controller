// Osmonauts – simulation tests: build and run with tests/run.sh
// (c) 2026 Kai Mommsen & Elias Dörre · SPDX-License-Identifier: CC-BY-NC-SA-4.0
#include "Arduino.h"
#include <vector>
#include <cstdlib>

unsigned long sim_ms = 0;
int sim_pin_out[64];
int sim_pin_in[64];
int sim_analog[64];
std::string sim_serial;
SerialMock Serial;

struct Ev { unsigned long t; std::string s; };
std::vector<Ev> timeline;

#include SKETCH

std::string relaisState() {
  std::string s = "----";
  if (sim_pin_out[relais1] == ON) s[0] = '1';
  if (sim_pin_out[relais2] == ON) s[1] = '2';
  if (sim_pin_out[relais3] == ON) s[2] = '3';
  if (sim_pin_out[relais4] == ON) s[3] = '4';
  return s;
}
void sim_on_write(uint8_t) {
  std::string s = relaisState();
  if (timeline.empty() || timeline.back().s != s) timeline.push_back({sim_ms, s});
}

void runFor(unsigned long d) {
  unsigned long end = sim_ms + d;
  while (sim_ms < end) { loop(); sim_ms += 1; }
}
void press(int pin, unsigned long dur) {
  sim_pin_in[pin] = LOW; runFor(dur); sim_pin_in[pin] = HIGH; runFor(300);
}
void pressBouncy(int pin, unsigned long dur) {
  for (int i = 0; i < 8; i++) { sim_pin_in[pin] = (i % 2) ? HIGH : LOW; runFor(3); }
  sim_pin_in[pin] = LOW; runFor(dur);
  for (int i = 0; i < 8; i++) { sim_pin_in[pin] = (i % 2) ? LOW : HIGH; runFor(3); }
  sim_pin_in[pin] = HIGH; runFor(300);
}

// segments of at least 1 s (relais switching steps of rst ms are hidden)
struct Seg { std::string s; double start, dur; };
std::vector<Seg> segments(unsigned long from = 0) {
  std::vector<Seg> out;
  for (size_t i = 0; i < timeline.size(); i++) {
    unsigned long t0 = timeline[i].t, t1 = (i + 1 < timeline.size()) ? timeline[i + 1].t : sim_ms;
    if (t0 < from) continue;
    double dur = (t1 - t0) / 1000.0;
    if (dur < 1.0 || dur < (4.0 * rst + 100) / 1000.0) continue;
    if (!out.empty() && out.back().s == timeline[i].s) { out.back().dur = (t1 / 1000.0) - out.back().start; continue; }
    out.push_back({timeline[i].s, t0 / 1000.0, dur});
  }
  return out;
}
void printSegs(const std::vector<Seg> &v) {
  for (auto &g : v) printf("    t=%8.1f s  %s  %8.1f s\n", g.start, g.s.c_str(), g.dur);
}

int fails = 0;
void check(bool ok, const char *what) { printf("  [%s] %s\n", ok ? " OK " : "FAIL", what); if (!ok) fails++; }
bool near(double a, double b, double tol = 2.0) { return a > b - tol && a < b + tol; }
int countStr(const std::string &hay, const std::string &n) {
  int c = 0; size_t p = 0; while ((p = hay.find(n, p)) != std::string::npos) { c++; p += n.size(); } return c;
}
// expected sequence of active (non-idle) segments
bool expectSeq(const std::vector<Seg> &v, std::vector<std::pair<std::string, double>> exp, double tol = -1) {
  if (tol < 0) tol = 2.0 + 3.0 * rst / 1000.0;
  std::vector<Seg> act;
  for (auto &g : v) if (g.s != "----") act.push_back(g);
  if (act.size() != exp.size()) { printf("    expected %zu active segments, got %zu\n", exp.size(), act.size()); return false; }
  for (size_t i = 0; i < exp.size(); i++)
    if (act[i].s != exp[i].first || !near(act[i].dur, exp[i].second, tol)) {
      printf("    segment %zu: expected %s %.0f s, got %s %.1f s\n", i, exp[i].first.c_str(), exp[i].second, act[i].s.c_str(), act[i].dur);
      return false;
    }
  return true;
}

const double M = durMembraneFlushFirst / 1000.0, S = durStandWaterFlush / 1000.0, FI = durFilterProcessShort / 1000.0,
             M2 = durMembraneFlushSecond / 1000.0, WAIT = intervalFlushAfterTapping / 1000.0;

int main(int argc, char **argv) {
  std::string test = argc > 1 ? argv[1] : "";
  for (int i = 0; i < 64; i++) { sim_pin_in[i] = HIGH; sim_analog[i] = 1023; sim_pin_out[i] = -1; }
#ifdef WATER_LEVEL_SENSOR
  for (int i = 0; i < 64; i++) sim_analog[i] = 0;
#endif
  setup();
  timeline.clear();
  sim_on_write(0);

  if (test == "tap") {
    printf("T1 tapping after power-on: pre-flush, filter, post-flush after waiting time\n");
    runFor(1000); press(pushbtnPIN1, 200);
    runFor((M + S + FI + WAIT + M2 + 60) * 1000);
    auto v = segments(); printSegs(v);
    check(expectSeq(v, {{"1--4", M}, {"1-3-", S}, {"12--", FI}, {"1--4", M2}}), "sequence and durations");
    check(mode == MODE_IDLE, "back to idle");
  }
  else if (test == "tap2") {
    printf("T2 second tapping within 15 min: no pre-flush\n");
    runFor(1000); press(pushbtnPIN1, 200);
    runFor((M + S + FI + WAIT + M2 + 60) * 1000);
    unsigned long t = sim_ms;
    press(pushbtnPIN1, 200);
    runFor((FI + WAIT + M2 + 60) * 1000);
    auto v = segments(t); printSegs(v);
    check(expectSeq(v, {{"12--", FI}, {"1--4", M2}}), "only filter + post-flush");
  }
  else if (test == "stop") {
    printf("T3 stop during pre-flush, stray stop in idle, then full tapping\n");
    runFor(1000); press(pushbtnPIN1, 200);
    runFor(5000);
    press(pushbtnPIN1, intervalToLongPress + 200);   // long press = stop
    check(relaisState() == "----" && mode == MODE_IDLE, "stop: all valves closed, idle");
    runFor(10 * 60 * 1000UL);
    check(relaisState() == "----", "stays off (no remaining step running)");
    press(pushbtnPIN1, intervalToLongPress + 200);   // stray stop in idle
    runFor(5000);
    unsigned long t = sim_ms;
    press(pushbtnPIN1, 200);
    runFor((M + S + FI + 10) * 1000);
    auto v = segments(t); printSegs(v);
    check(expectSeq(v, {{"1--4", M}, {"1-3-", S}, {"12--", FI}}), "next tapping has full durations");
  }
  else if (test == "stopfilter") {
    printf("T4 stop during filtering: post-flush follows after waiting time\n");
    runFor(1000); press(pushbtnPIN1, 200);
    runFor((M + S + 30) * 1000);
#ifdef PUSHBTN2_ENABLE
    press(pushbtnPIN2, 200);
#else
    press(pushbtnPIN1, intervalToLongPress + 200);
#endif
    check(relaisState() == "----", "filter stopped");
    runFor((WAIT + M2 + 30) * 1000);
    auto v = segments(); printSegs(v);
    std::vector<Seg> act; for (auto &g : v) if (g.s != "----") act.push_back(g);
    check(!act.empty() && act.back().s == "1--4" && near(act.back().dur, M2, 2.0 + 3.0 * rst / 1000.0), "post-flush after stop");
  }
#ifdef PUSHBTN_3_4_ENABLE
  else if (test == "disinfect") {
    printf("T5 disinfection: all waiting times respected\n");
    runFor(1000); press(pushbtnPIN3, intervalToLongPress + 200);
    runFor((unsigned long)(2 * intervalInjectionH2O2 + intervalMoveH2O2 + disinfectionPulses * (intervalMoveH2O2 + intervalWaitDesinfect) + 3 * intervalFinalFlushes) + 120000UL);
    auto v = segments(); printSegs(v);
    double I = intervalInjectionH2O2 / 1000.0, MV = intervalMoveH2O2 / 1000.0, FF = intervalFinalFlushes / 1000.0;
    std::vector<std::pair<std::string, double>> exp = {{"---4", I}, {"1-3-", MV}, {"---4", I}};
    for (int i = 0; i < disinfectionPulses; i++) exp.push_back({"12--", MV});
    exp.push_back({"1--4", FF}); exp.push_back({"1-3-", FF}); exp.push_back({"12--", FF});
    check(expectSeq(v, exp), "sequence and durations");
    int pauses = 0; for (auto &g : v) if (g.s == "----" && near(g.dur, intervalWaitDesinfect / 1000.0, 3)) pauses++;
    check(pauses == disinfectionPulses, "pauses between pulses");
    check(mode == MODE_IDLE && forceFlush, "idle, next tapping forced to pre-flush");
  }
  else if (test == "busy") {
    printf("T6 tapping request during disinfection is refused\n");
    runFor(1000); press(pushbtnPIN3, intervalToLongPress + 200);
    runFor(30000);
    press(pushbtnPIN1, 200);
    runFor(5000);
    check(mode == MODE_DISINFECTION && relaisState() == "---4", "disinfection keeps running");
    check(sim_serial.find("Busy") != std::string::npos, "busy message");
  }
  else if (test == "filterchange") {
    printf("T7 filter exchange: 15 min membrane + 15 min standing water\n");
    runFor(1000); press(pushbtnPIN4, intervalToLongPress + 200);
    runFor((durMembraneFlushNewFilter + durStandWaterFlushNewFilter) + 60000UL);
    auto v = segments(); printSegs(v);
    check(expectSeq(v, {{"1--4", durMembraneFlushNewFilter / 1000.0}, {"1-3-", durStandWaterFlushNewFilter / 1000.0}}), "sequence and durations");
  }
#endif
  else if (test == "auto") {
    printf("T8 automatic flush after intervalAutomaticFlush, and not again directly afterwards\n");
    runFor(intervalAutomaticFlush + (M + S + M2 + 60) * 1000);
    unsigned long t1 = sim_ms;
    runFor(intervalAutomaticFlush / 2);
    auto v = segments(); printSegs(v);
    check(expectSeq(v, {{"1--4", M}, {"1-3-", S}, {"1--4", M2}}), "one automatic flush");
    check(segments(t1).size() <= 1, "no repetition directly afterwards");
  }
  else if (test == "tapduringauto") {
    printf("T9 tapping interrupts automatic flush cleanly\n");
    runFor(intervalAutomaticFlush + 5000);
    check(mode == MODE_AUTO_FLUSH, "automatic flush running");
    unsigned long t = sim_ms;
    press(pushbtnPIN1, 200);
    runFor((M + S + FI + 10) * 1000);
    auto v = segments(t); printSegs(v);
    check(expectSeq(v, {{"1--4", M}, {"1-3-", S}, {"12--", FI}}), "full tapping incl. pre-flush");
    runFor(intervalAutomaticFlush / 2);
    check(countStr(sim_serial, "Start automatic flush") == 1, "automatic flush not restarted at once");
  }
  else if (test == "tapduringpost") {
    printf("T10 tapping during post-flush: post-flush cancelled, repeated later, nothing stuck\n");
    runFor(1000); press(pushbtnPIN1, 200);
    runFor((M + S + FI + WAIT + 12) * 1000);
    check(mode == MODE_POST_FLUSH, "post-flush running");
    unsigned long t = sim_ms;
    press(pushbtnPIN1, 200);
    runFor((FI + WAIT + M2 + 60) * 1000);
    auto v = segments(t); printSegs(v);
    check(expectSeq(v, {{"12--", FI}, {"1--4", M2}}), "filter, then complete post-flush");
  }
  else if (test == "aquastop") {
    printf("T11 aqua stop during filtering: valves closed, buttons locked\n");
    runFor(1000); press(pushbtnPIN1, 200);
    runFor((M + S + 20) * 1000);
#ifdef WATER_LEVEL_SENSOR
    sim_analog[aquaStopPin] = 1023;
#else
    sim_analog[aquaStopPin] = 10;
#endif
    unsigned long t = sim_ms;
    runFor(10000);
    unsigned long tOff = 0; for (auto &e : timeline) if (e.t >= t && e.s == "----") { tOff = e.t; break; }
    check(tOff && tOff - t <= (aquaStopTriggerCount + 1) * intervalAquaStopCheck + 4 * rst, "valves closed within debounce time");
    printf("    reaction time %.1f s\n", (tOff - t) / 1000.0);
    sim_analog[aquaStopPin] = sim_analog[0];   // sensor dry again
    press(pushbtnPIN1, 200);
    runFor(intervalAutomaticFlush + 60000UL);
    check(relaisState() == "----" && mode == MODE_AQUASTOP, "stays stopped (latched), no auto flush");
  }
  else if (test == "bounce") {
    printf("T12 bouncing button contacts: exactly one short press\n");
    runFor(1000); pressBouncy(pushbtnPIN1, 300);
    runFor(2000);
    check(countStr(sim_serial, "short press") == 1 && countStr(sim_serial, "long press") == 0, "one short press");
    check(mode == MODE_TAPPING, "tapping started");
  }
  else { printf("unknown test\n"); return 2; }

  if (getenv("VERBOSE")) printf("%s\n", sim_serial.c_str());
  return fails ? 1 : 0;
}

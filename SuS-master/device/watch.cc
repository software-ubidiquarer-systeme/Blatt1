#include "config.h"
#include "machine/timer.h"
#include "device/plugbox.h"
#include "meeting/bellringer.h"
#include "syscall/guarded_organizer.h"
#include "device/watch.h"

Watch Watch::watch(10000);

Watch::Watch(unsigned long us) : Timer(us), system_ticks(0) {}

void Watch::windup() {
  plugin();
}

void Watch::epilogue() {
  system_ticks++;
  Bellringer::instance().check();
  Guarded_Organizer::instance().Organizer::resume();
}

void delay_us(unsigned int us) {
  Watch::instance().delay_us(us);
}

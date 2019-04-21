#include "config.h"
#include <msp430.h>
#include "device/plugbox.h"
#include "machine/timer.h"

Timer::Timer(unsigned long us) {
  ticktime = 1000000L/us;
}

void Timer::plugin() {
  /* Interrupthandler anmelden */
  // FIXME: Umstellen auf CC0 statt Misc!
  Plugbox::instance().assign(Plugbox::Timer0_Misc, *this);

  /** Timer 0: Systemtimer **/
#if 1
  // Stromsparende Variante
  /* Kein weiterer Teiler */
  TA0EX0 = 0;

  /* Maximalen Zaehlerwert einstellen */
  TA0CCR0 = (32878 / ticktime) - 1;

  /* Timer 0 zaehlt ACLK aufwaerts */
  TA0CTL = TASSEL__ACLK | ID_0 | MC__UP;
#endif

#if 0
  // Exakte Variante
  /* Timer 0 zaehlt SMCLK/8 aufwaerts */
  /* Weiteren /4-Teiler einfuegen */
  TA0EX0 = (4-1);

  /* Maximalen Zaehlerwert einstellen */
  TA0CCR0 = (CONFIG_CPU_FREQUENCY / 8 / 4 / ticktime) - 1;

  /* Timer 0 zaehlt SMCLK/8 aufwaerts */
  TA0CTL = TASSEL__SMCLK | ID_3 | MC__UP;
#endif

  /* Timer-Interrupts zulassen */
  TA0CTL |= TAIE;


  /** Timer 1: Mikrosekunden-Zaehler **/
  /* Zusaetzlicher Divisor 3 -> SMCLK/12 */
  TA1EX0 = 2;
  /* SMCLK/4 aufwaerts zaehlen */
  TA1CTL = TASSEL__SMCLK | ID_2 | MC__STOP; // erstmal gestoppt lassen
}

bool Timer::prologue() {
  if (TA0IV == 0x0e) {
    /* Timer overflow */
    return true;
  } else
    /* Andere Timer-Interrupts ignorieren */
    return false;
}

void Timer::delay_us(unsigned int us) {
  /* Grob, aber ausreichend */
  for (unsigned int i=0; i<us/5; i++)
    asm volatile ("nop");
}

/** Timer 1: Mikrosekunden-Zaehler **/
void Timer::start() {
  TA1CTL = TACLR | TASSEL__SMCLK | ID_2 | MC__CONTINOUS;
}

void Timer::stop() { 
  TA1CTL = MC__STOP; // oder: &=~ MC__STOP; ?
}

unsigned int Timer::getcycles() {
  return TA1R;
}

#include "config.h"
#include <msp430.h>
#include "machine/system.h"

/* Compile-Zeit-Berechnungen, um eine gueltige CPU-Frequenz sicherzustellen */
#define XT1_FREQUENCY  32768
#define FLL_MULTIPLIER ((CONFIG_CPU_FREQUENCY + XT1_FREQUENCY/2) / XT1_FREQUENCY)

#if FLL_MULTIPLIER >= 1024
#  error Kein Multiplikator fuer die gewuenschte Frequenz moeglich!
#endif

#if CONFIG_CPU_FREQUENCY < 32768
#  error Gewuenschte CPU-Frequenz ist zu niedrig!
#endif

/* Zyklen-basierte Verzoegerungsfunktion */
/* Timer an dieser Stelle noch nicht verwendbar, da die Takte noch nicht stabil sind */
static void delay_loop_1(unsigned int loops) {
  /*
           jmp     label         2
           nop                   1
           add     #-1, r15      1
    label: tst     r15           1
           jnz     $-6           2
           ret                   2?
     => 6 + 5*loops Zyklen
  */
  while (loops--) asm volatile("nop");
}

/* Wartet immer etwas zu lange */
#define delay_cycles(cycles) delay_loop_1((cycles) / 5)

static void vcore_increase(unsigned int level) {
  /* Power Managemant-Register entsperren */
  PMMCTL0_H = PMMPW_H;

  /* Neue Schwelle fuer High-Side setzen */
  SVSMHCTL = SVSHE | (SVSHRVL0 * level)
           | SVMHE | (SVSMHRRL0 * level);

  /* Neue Schwelle fuer Low-Side setzen */
  SVSMLCTL = SVSLE | SVMLE | (SVSMLRRL0 * level);

  /* Warten bis Supervisor bereit */
  while ((PMMIFG & SVSMLDLYIFG) == 0) ;

  /* Fehlerflags zuruecksetzen */
  PMMIFG &= ~(SVMLVLRIFG | SVMLIFG);

  /* Neue VCore einstellen */
  PMMCTL0_L = PMMCOREV0 * level;

  /* Warten bis neues VCore-Level erreicht */
  if ((PMMIFG & SVMLIFG))
    while ((PMMIFG & SVMLVLRIFG) == 0) ;

  /* low-side auf neue Schwelle setzen */
  SVSMLCTL = SVSLE | (SVSLRVL0 * level);

  /* Power-Management-Register wieder sperren */
  PMMCTL0_H = 0;
}

static void vcore_init(void) {
  /* VCore auf Maximum erhoehen */
  while ((PMMCTL0_L & 3) != 3) {
    vcore_increase((PMMCTL0_L & PMMCOREV_3) + 1);
  }
}

static void clocks_init(void) {
  /* Pins fuer Quarzverwendung einstellen */
  P5SEL |= 3;

  /* 32kHz Qaurz an, geringste Treiberstaerke, interner Lastkondensator */
  UCSCTL6 = XT2OFF | XCAP_3;

  /* XT1 als FLL-Referenz verwenden, Divisor 1 */
  UCSCTL3 = SELREF__XT1CLK | FLLREFDIV__1;

  /* XT1 fuer ACLK verwenden, DCO als MCLK/SMCLK */
  UCSCTL4 = SELA__XT1CLK | SELS__DCOCLK | SELM__DCOCLK;

  /* Alle Teiler zuruecksetzen */
  UCSCTL5 = DIVPA__1 | DIVA__1 | DIVS__1 | DIVM__1;

  /* Diese Abfragen sollten vom Compiler auf eine Zuweisung reduziert werden */
  if (CONFIG_CPU_FREQUENCY      <   530000)
    UCSCTL1 = DCORSEL_0;
  else if (CONFIG_CPU_FREQUENCY <  1100000)
    UCSCTL1 = DCORSEL_1;
  else if (CONFIG_CPU_FREQUENCY <  2340000)
    UCSCTL1 = DCORSEL_2;
  else if (CONFIG_CPU_FREQUENCY <  4600000)
    UCSCTL1 = DCORSEL_3;
  else if (CONFIG_CPU_FREQUENCY <  9100000)
    UCSCTL1 = DCORSEL_4;
  else if (CONFIG_CPU_FREQUENCY < 17200000)
    UCSCTL1 = DCORSEL_5;
  else
    UCSCTL1 = DCORSEL_6;

  UCSCTL1 = DCORSEL_5;                // DCO range 6..23MHz (worst case)
  UCSCTL2 = FLLD__1 | FLL_MULTIPLIER; // Schleifenmultiplikator 1
  _BIC_SR(SCG0);                      // FLL einschalten

  /* Darauf warten, dass die FLL stabil ist - aufgeteilt um Ueberlauf zu vermeiden */
  // FIXME: Sollte vom Takt abhaengen
  delay_cycles(250000);
  delay_cycles(125000);

  /* Warten bis Quarz und DCO stabil sind */
  do {
    /* Fehlerflags zuruecksetzen */
    UCSCTL7 = 0;
    /* "oscillator fault" interrupt flag zuruecksetzen */
    SFRIFG1 &= ~OFIFG;
  } while (SFRIFG1 & OFIFG);

  /* XT2 wird vom Radio bei Bedarf automatisch eingeschaltet */

  /* Watchdog einschalten, Ausloeseverzoegerung ca. 50 Sekunden */
  WDTCTL = WDTPW | WDTSSEL__VLO | WDTCNTCL | WDTIS__512K;
}

#define CALL_RFBSL()   ((void (*)())0x1000)()

void system_call_bsl(void) {
  __dint();
  asm volatile ("nop"); // CPU-Erratum-Workaround

  /* Display loeschen, in unterer Zeile "bSL" ausgeben */
  LCDBMEMCTL |= LCDCLRM | LCDCLRBM;
  LCDMEM[ 8] = 0x58;
  LCDMEM[ 9] = 0x3d;
  LCDMEM[10] = 0x7c;

  // Watchdog auf schnellste Reaktion einstellen
  WDTCTL = WDTPW | WDTSSEL__VLO | WDTCNTCL | WDTIS__64;

  // Warten bis der WDT zuschlaegt, damit der BSL-Trigger aufgerufen wird
  while (1) ;
}


/*
Der neue TI-Compiler ignoriert die Sektion init1, weshalb
die Funktion normal definiert wird, und über den Aspekt
check_wdt_reset.ah vor die main-Funktion geweoben wird.

Die Direktive TI_COMPILER wird je nach Compiler im Makefile
definiert oder eben nicht.
*/
void
#ifndef TI_COMPILER
#ifndef __acweaving // for clang 3.6
     __attribute__ ((naked))
#endif /* __acweaving */
     __attribute__ ((section(".init1")))
#endif
check_wdt_reset(void) {

  /* Reset-Grund pruefen, bei Watchdog-Zuschlag BSL aufrufen */
  if (SYSRSTIV == SYSRSTIV_WDTTO) {
    SYSRSTIV = 0;

    /* In unterer Zeile "bSL" ausgeben */
	LCDBMEMCTL |= LCDCLRM | LCDCLRBM;
    LCDMEM[ 8] = 0x58;
    LCDMEM[ 9] = 0x3d;
    LCDMEM[10] = 0x7c;

    /* Timer A0 zuruecksetzen, da der BSL sonst nicht laeuft */
    TA0CTL = 0;
    TA0CCTL0 = 0;
    TA0R = 0;
    TA0EX0 = 0;

    __dint();
    asm volatile ("nop"); // CPU-Erratum-Workaround
    CALL_RFBSL();
  }
}

void system_init(void) {
  vcore_init();
  clocks_init();
}

void system_restore(void) {
  vcore_increase(0);
  UCSCTL1 = DCORSEL_0;
  UCSCTL2 = 0;
  WDTCTL = WDTPW + WDTHOLD;                 // Stop watchdog timer
  UCSCTL6 |= XT1OFF | XT2OFF;
}

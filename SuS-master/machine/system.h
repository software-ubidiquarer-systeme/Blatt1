#ifndef SYSTEM_H
#define SYSTEM_H

/* Watchdog zuruecksetzen */
#define watchdog_reset() do { WDTCTL = WDTPW | WDTCNTCL; } while (0)

/*
 * VCore and Takte konfigurieren, Watchdog einschalten.
 */
void system_init(void);
/*
 * VCore and Takte in den Ausgangszustand versetzen, Watchdog ausschalten.
 * (Gegenbenfalls eingeschaltete) Peripherie abschalten.
 */
void system_restore(void);

void system_call_bsl(void);

/* Bootstrap-Loader aufrufen */
#ifdef TI_COMPILER
void check_wdt_reset(void);
#endif

#endif

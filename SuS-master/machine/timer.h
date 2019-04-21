#ifndef TIMER_H
#define TIMER_H

#include "guard/gate.h"

class Button;

class Timer : public Gate {
private:
  Timer(const Timer &copy);

protected:
  /* Speichert die Ticks pro Sekunde */
  unsigned long ticktime;

  /* Us = Mikrosekunden pro Tick */
  Timer(unsigned long us);

public:
  /* Klasse in Plugbox anmelden und Timer starten */
  void plugin();

  /* Interner Interrupt-Kram */
  bool prologue();

  /* Wartet ungefaehr die angegebene Zahl von Mikrosekunden, eher mehr */
  static void delay_us(unsigned int us);


  /** Timer 1: Mikrosekunden-Zaehler **/
  /* Startet den Mikrosekunden-Zaehler. Ist dieser bereits gestartet, so wird neugestartet. */
  static void start();
  
  /* Stoppt den Mikrosekunden-Zaehler. Der gezaehlte Wert bleibt dabei erhalten. */
  static void stop();
  
  /* Mikrosekunden-Zaehler auslesen */
  static unsigned int getcycles();
};

#endif

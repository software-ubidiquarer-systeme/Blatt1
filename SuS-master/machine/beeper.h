#ifndef BEEPER_H
#define BEEPER_H

class Beeper {
private:
  static Beeper beeper;// Singleton Instanz

  Beeper(const Beeper &copy);
  Beeper();

  unsigned int remaining_ticks;

  void check();

public:

  /* Methode zum Zugriff auf die Singleton Instanz */
  static Beeper& instance() { return beeper; }

  void init();

  void beep(unsigned int frequency, unsigned int milliseconds);
};

#endif

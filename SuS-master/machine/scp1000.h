/* SCP1000 Druck- und Temperatursensor */
#ifndef SCP1000_H
#define SCP1000_H

#include <msp430.h>

#ifndef BV
#define BV(x) (1 << (x))
#endif

/* I/O-Pins mit dem Ready-Signal des Sensors */
#define SCP1000_DRDY_BIT BV(6)
#define SCP1000_DRDY_IN  P2IN

class SCP1000 {
private:
  SCP1000(const SCP1000 &copy);

protected:
  SCP1000() { init_sensor(); };

public:
  struct Mode {
    enum mode_t {
      /* 15 Bit Dauermessung, ca. 9Hz */
      HIGH_SPEED      = 0x09,

      /* 17 Bit Dauermessung, ca. 1.8 Hz */
      HIGH_RESOLUTION = 0x0a,

      /* 15 Bit Dauermessung, ca. 1 Hz */
      ULTRA_LOW_POWER = 0x0b,

      /* 17 Bit Einzelmessung */
      LOW_POWER       = 0x0c
    };
  };

  /* Initialisiert den Sensor */
  void init_sensor();

  /* Setzt den Auswertemodus */
  virtual void set_mode(Mode::mode_t mode);

  /* Loest eine einzelne Messung im Low-Power-Modus aus */
  void trigger_measurement() {
    set_mode(Mode::LOW_POWER);
  }

  /* Prueft, ob die Messung abgeschlossen ist */
  /* Wird durch Auslesen des Drucks zurueckgesetzt */
  bool data_available() {
    return SCP1000_DRDY_IN & SCP1000_DRDY_BIT;
  }

  /* Liest den zuletzt gemessenen Temperaturwert aus */
  int read_temp();

  /* Liest den zuletzt gemessenen Druckwert aus */
  long read_pressure();
};

#endif

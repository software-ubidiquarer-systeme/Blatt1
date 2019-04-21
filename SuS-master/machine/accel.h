/* CMA3000 Beschleunigungssensor */
#ifndef ACCEL_H
#define ACCEL_H

#include "guard/gate.h"
#include "syscall/guarded_semaphore.h"

class Accel : public Gate {
private:
  Accel(const Accel &copy);

  Guarded_Semaphore spi_sema;

  unsigned char spi_byte(unsigned char byte);

protected:
  Accel();

public:
  struct Register {
    enum register_t {
      WHO_AM_I   = (0x00 << 2),
      REVID      = (0x01 << 2),
      CTRL       = (0x02 << 2),
      STATUS     = (0x03 << 2),
      RSTR       = (0x04 << 2),
      INT_STATUS = (0x05 << 2),
      DOUTX      = (0x06 << 2),
      DOUTY      = (0x07 << 2),
      DOUTZ      = (0x08 << 2),
      MDTHR      = (0x09 << 2),
      MDFFTMR    = (0x0a << 2),
      FFTHR      = (0x0b << 2),
      I2C_ADDR   = (0x0c << 2)
    };
  };

  struct Mode {
    enum mode_t {
      POWERDOWN      = (0<<1),
      MEASURE_100HZ  = (1<<1),
      MEASURE_400HZ  = (2<<1),
      MEASURE_40HZ   = (3<<1),
      MOTION_DETECT  = (4<<1),
      FREEFALL_100HZ = (5<<1),
      FREEFALL_400HZ = (6<<1)
    };
  };

  struct Range {
    enum range_t {
      RANGE_8G = 0,
      RANGE_2G = (1<<7)
    };
  };

  /* Internes Interruptzeugs */
  bool prologue();
  void epilogue();

  /* Meldet die Klasse in der Plugbox an */
  void plugin();

  /* Setzt den Auswertemodus und die Empfindlichkeit des Sensors */
  void set_mode(Mode::mode_t mode, Range::range_t range);

  /* Liest ein beliebiges Register des Sensors */
  virtual signed char read_register(Register::register_t reg);

  /* Liest die Beschleunigung in X/Y/Z-Richtung */
  signed char read_x();

  signed char read_y();

  signed char read_z();

  /* Liefert true, wenn der Sensor frische Messdaten liefert. */
  /* Wird vom Sensor nach Auslesen zurueckgesetzt.            */
  bool data_available();

};

#endif

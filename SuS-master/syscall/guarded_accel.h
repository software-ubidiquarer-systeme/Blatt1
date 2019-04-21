/*****************************************************************************/
/* Betriebssysteme                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*                         G U A R D E D _ A C C E L                         */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/* Schnittstelle von Anwendungsthreads zum Accel-Objekt                      */
/*****************************************************************************/

#ifndef GUARDED_ACCEL
#define GUARDED_ACCEL

#include "machine/accel.h"
#include "guard/secure.h"

class Guarded_Accel : public Accel {
private:
  Guarded_Accel(const Guarded_Accel &copy); // Verhindere Kopieren
  Guarded_Accel();

public:
  /* Methode zum Zugriff auf die Singleton Instanz */
  static Guarded_Accel& instance() {
    Secure sec;
    static Guarded_Accel accel; // Singleton Instanz
    return accel;
  }

  void set_mode(Mode::mode_t mode, Range::range_t range);
  signed char read_register(Register::register_t reg);
};

#endif

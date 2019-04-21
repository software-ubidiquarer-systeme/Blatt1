#include "config.h"
#include <msp430.h>
#include "device/plugbox.h"
#include "device/watch.h"
#include "machine/accel.h"

#ifndef BV
#define BV(x) (1 << (x))
#endif

#define CMA3000_INT_IN  P2IN
#define CMA3000_INT_BIT BV(5)

#define CMA_WRITE (1<<1)

/* CMA_CTRL bits */
#define G_RANGE   (1<<7)
#define INT_LEVEL (1<<6)
#define MDET_EXIT (1<<5)
#define I2C_DIS   (1<<4)
#define MODE2     (1<<3)
#define MODE1     (1<<2)
#define MODE0     (1<<1)
#define INT_DIS   (1<<0)

#define CMA_CSB BV(1)

static inline void set_ss(int state) {
  if (state)
    PJOUT |= CMA_CSB;
  else
    PJOUT &= ~CMA_CSB;
}

/* Nur waehrend der Initialisierung verwendet, */
/* benutzt busy-waiting weil die Semaphore zu  */
/* diesem Zeitpunkt noch nicht verwendbar ist. */
static unsigned char __attribute__((noinline)) spi_byte_wait(unsigned char byte) {
  UCB0TXBUF = byte;

  while (UCB0STAT & UCBUSY)
    ;

  UCB0IFG &= ~(UCRXIFG | UCTXIFG);

  return UCB0RXBUF;
}

unsigned char Accel::spi_byte(unsigned char byte) {
  UCB0TXBUF = byte;

  /* Warten bis die Interruptroutine uns aufweckt */
  spi_sema.Semaphore::wait();

  return UCB0RXBUF;
}

void Accel::set_mode(Mode::mode_t mode, Range::range_t range) {
  set_ss(0);
  spi_byte_wait(Register::CTRL | CMA_WRITE);
  spi_byte_wait(I2C_DIS | mode | range);
  set_ss(1);
}

signed char Accel::read_register(Register::register_t reg) {
  signed char result;

  set_ss(0);
  spi_byte_wait(reg);
  result = spi_byte_wait(0xff);
  set_ss(1);

  return result;
}

Accel::Accel() : spi_sema(0) {}

void Accel::plugin() {
  /* Der Sensor kann sowohl I2C als auch SPI sprechen,  */
  /* wird hier wegen I2C-Errata im SPI-Modus verwendet. */

  /* Chip-Select-Leitung */
  PJOUT |= CMA_CSB;
  PJDIR |= CMA_CSB;

  /**** USCI B fuer SPI verwenden ****/
  /* im Reset halten */
  UCB0CTL1 |= UCSWRST;

  /* Master SPI Modus, clock phase 1, inaktiv low, MSB first, synchron */
  UCB0CTL0 = UCCKPH | UCMSB | UCMST | UCMODE_0 | UCSYNC;

  /* SMCLK als Taktquelle verwenden, im Reset bleiben */
  UCB0CTL1 = UCSSEL__SMCLK | UCSWRST;

  /* ca. 400kHz Bitrate */
  UCB0BR0 = CONFIG_CPU_FREQUENCY / 400000;

  /* Portkonfiguration umstellen */
  PMAPPWD = PMAPPW;
  PMAPCTL = PMAPRECFG; /* Spaetere Umstellungen zulassen */
  P1MAP5  = PM_UCB0SOMI;
  P1MAP6  = PM_UCB0SIMO;
  P1MAP7  = PM_UCB0CLK;
  P1SEL  |= BV(5) | BV(6) | BV(7);

  /* Interrupt-Pin */
  P2DIR &= BV(5);
  P2OUT |= BV(5);
  P2REN |= BV(5);

  /* USCI aus Reset holen */
  UCB0CTL1 &= ~UCSWRST;

  set_ss(1);

  /**** Sensor initialisieren ****/
  /* Revisionsnummer auslesen bis sie != 0 ist */
  while (1) {
    unsigned char result;

    set_ss(0);
    spi_byte_wait(Register::REVID);
    result = spi_byte_wait(0xff);
    set_ss(1);

    if (result != 0)
      break;

    Watch::instance().delay_us(10000);
  }

  /* I2C abschalten, Stromsparmodus einschalten */
  set_ss(0);
  spi_byte_wait(Register::CTRL | CMA_WRITE);
  spi_byte_wait(I2C_DIS | Mode::POWERDOWN | Range::RANGE_2G);
  set_ss(1);

  /* In Plugbox eintragen */
  Plugbox::instance().assign(Plugbox::UniSerial_B0, *this);

  /* Interrupt-Pin des Sensors als Eingang */
  P2DIR &= ~BV(5);
  P2OUT |= BV(5);
  P2REN |= BV(5);

  /* Empfangs-Interrupts freigeben */
  UCB0IFG &= ~UCRXIFG;
  UCB0IE |= UCRXIE;
}

bool Accel::prologue() {
  if (UCB0IV != 0)
    return true;
  else
    return false;
}

void Accel::epilogue() {
  /* Daten sind da, spi_byte aufwecken */
  spi_sema.Semaphore::signal();
}

signed char Accel::read_x() {
  return read_register(Register::DOUTX);
}

signed char Accel::read_y() {
  return read_register(Register::DOUTY);
}

signed char Accel::read_z() {
  return read_register(Register::DOUTZ);
}

/* Liefert true, wenn der Sensor frische Messdaten liefert. */
/* Wird vom Sensor nach Auslesen zurueckgesetzt.            */
bool Accel::data_available() {
  return CMA3000_INT_IN & CMA3000_INT_BIT;
}

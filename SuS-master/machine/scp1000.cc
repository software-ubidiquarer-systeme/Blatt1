#include "config.h"
#include <msp430.h>
#include "machine/scp1000.h"
#include "device/watch.h"

/* Das im Header ausgelassene Makro fuer DRDY */
#define SCP1000_DRDY_DIR P2DIR

/* I2C-Adresse des Sensors */
#define SCP_I2C_ADDRESS 0x22

/* Register des Sensors */
#define SCP_REVID     0x00
#define SCP_DATAWR    0x01
#define SCP_ADDRPTR   0x02
#define SCP_OPERATION 0x03
#define SCP_OPSTATUS  0x04
#define SCP_RSTR      0x06
#define SCP_STATUS    0x07
#define SCP_DATARD8   0x7f
#define SCP_DATARD16  0x80
#define SCP_TEMPOUT   0x81

/* Indirekte Register des Sensors */
#define SCPI_CFG      0x00
#define SCPI_TWIADD   0x05

/* Werte fuer das SCPI_CFG-Register */
#define SCPI_CFG_HIGHRES 0x05
#define SCPI_CFG_LOWRES  0x0d

/* Operationscodes des Sensors */
#define SCPOP_NONE        0x00
#define SCPOP_RD_INDIRECT 0x01
#define SCPOP_WR_INDIRECT 0x02
#define SCPOP_RD_EEPROM   0x05
#define SCPOP_WR_EEPROM   0x06
#define SCPOP_INIT        0x07
#define SCPOP_SELFTEST    0x0f
/* Messmodi sind in Mode::mode_t definiert */

/* --------- Software-I2C-Implementierung aus sd2iec uebernommen ---------- */

/* Halb-Zyklus-Verzoegerung in Schleifendurchlaeufen */
#define SOFTI2C_DELAY 100

#define SDA BV(2)
#define SCL BV(3)
#define I2C_IN  PJIN
#define I2C_DIR PJDIR
#define I2C_OUT PJOUT
#define I2C_REN PJREN

static void delay_loop(unsigned int loops) {
  for (unsigned int i=0; i<loops; i++)
    asm volatile("nop");
}

static void set_scl(int state) {
  if (state) {
    /* SCL high */
    I2C_DIR &= ~SCL;
    I2C_OUT |=  SCL;
  } else {
    /* SCL low */
    I2C_OUT &= ~SCL;
    I2C_DIR |=  SCL;
  }
}

static void set_sda(int state) {
  if (state) {
    /* SDA high */
    I2C_DIR &= ~SDA;
    I2C_OUT |=  SDA;
  } else {
    /* SDA low */
    I2C_OUT &= ~SDA;
    I2C_DIR |=  SDA;
  }
}

static void start_condition(void) {
  set_sda(1);
  set_scl(1);
  delay_loop(SOFTI2C_DELAY);
  set_sda(0);
  delay_loop(SOFTI2C_DELAY);
  set_scl(0);
}

static void stop_condition(void) {
  set_sda(0);
  delay_loop(SOFTI2C_DELAY);
  set_scl(1);
  delay_loop(SOFTI2C_DELAY);
  set_sda(1);
  delay_loop(SOFTI2C_DELAY);
}


/* Liefert den Wert des Acknowledge-Bits */
static int i2c_send_byte(int value) {
  int i;

  for (i=8; i!=0; i--) {
    set_scl(0);
    delay_loop(SOFTI2C_DELAY/2);
    set_sda(value & 128);
    delay_loop(SOFTI2C_DELAY/2);
    set_scl(1);
    delay_loop(SOFTI2C_DELAY);
    value <<= 1;
  }
  set_scl(0);
  delay_loop(SOFTI2C_DELAY/2);
  set_sda(1);
  delay_loop(SOFTI2C_DELAY/2);
  set_scl(1);
  delay_loop(SOFTI2C_DELAY/2);
  i = !!(I2C_IN & SDA);
  delay_loop(SOFTI2C_DELAY/2);
  set_scl(0);

  return i;
}

/* Liefert das gelesene Byte */
static int i2c_recv_byte(int sendack) {
  int i;
  int value = 0;

  set_sda(1);
  delay_loop(SOFTI2C_DELAY/2);
  for (i=8;i!=0;i--) {
    delay_loop(SOFTI2C_DELAY/2);
    set_scl(1);
    delay_loop(SOFTI2C_DELAY/2);
    value = (value << 1) + !!(I2C_IN & SDA);
    delay_loop(SOFTI2C_DELAY/2);
    set_scl(0);
    delay_loop(SOFTI2C_DELAY/2);
  }
  set_sda(!sendack);
  delay_loop(SOFTI2C_DELAY/2);
  set_scl(1);
  delay_loop(SOFTI2C_DELAY);
  set_scl(0);
  set_sda(1);

  return value;
}

/* Liefert 1 wenn die Adresse nicht ge-ACK-t wurde */
static int i2c_write_register(int address, int reg, int val) {
  start_condition();
  if (i2c_send_byte(address)) {
    stop_condition();
    return 1;
  }
  i2c_send_byte(reg);
  i2c_send_byte(val);
  stop_condition();
  return 0;
}

/* Liefert -1 wenn die Adresse nicht ge-ACK-t wurde, sonst Registerinhalt */
static int i2c_read_register(int address, int reg) {
  int val;

  start_condition();
  if (i2c_send_byte(address)) {
    stop_condition();
    return -1;
  }
  i2c_send_byte(reg);
  start_condition();
  if (i2c_send_byte(address|1)) {
    stop_condition();
    return -1;
  }
  val = i2c_recv_byte(0);
  stop_condition();
  return val;
}

/* Liefert 1 wenn die Adresse nicht ge-ACK-t wurde */
static int i2c_read_registers(int address, int startreg, int count, void *data) {
  unsigned char *bytedata = (unsigned char *)data;
  int i;

  start_condition();
  if (i2c_send_byte(address)) {
    stop_condition();
    return 1;
  }
  i2c_send_byte(startreg);
  start_condition();
  if (i2c_send_byte(address|1)) {
    stop_condition();
    return 1;
  }
  for (i=0;i<count-1;i++)
    bytedata[i] = i2c_recv_byte(1);
  bytedata[i] = i2c_recv_byte(0);
  stop_condition();
  return 0;
}

/* --------- Ende des aus sd2iec uebernommenen Codes ---------- */

void SCP1000::init_sensor() {
  /* Der Sensor braucht 60ms nach dem Einschalten */
  Watch::instance().delay_us(60000);

  /* I2C-Leitungen mit Pullup initialisieren */
  I2C_DIR &= ~(SDA | SCL);
  I2C_OUT |=   SDA | SCL;
  I2C_REN |=   SDA | SCL;

  /* DRDY-Pin */
  SCP1000_DRDY_DIR &= ~SCP1000_DRDY_BIT;

  /* Startup-Status pruefen */
  int i,res;
  for (i=0; i<10; i++) {
    res = i2c_read_register(SCP_I2C_ADDRESS, SCP_STATUS);

#ifdef CONFIG_PESSIMIST
    if (res < 0)
      /* Sensor nicht vorhanden */
      return;
#endif

    if ((res & 1) == 0)
      /* Initialisierung fertig */
      break;

    Watch::instance().delay_us(10000);
  }

#ifdef CONFIG_PESSIMIST
  if (i == 10)
    /* Initialisation fehlgeschlagen */
    return;
#endif

#if 0
  /* Auskommentiert, da der Test nach Reinitialisierung fehlschlaegt */
  /* auf EEPROM-Checksummen-Fehler pruefen */
  res = i2c_read_register(SCP_I2C_ADDRESS, SCP_DATARD8);
# ifdef CONFIG_PESSIMIST
  if (res < 0)
    /* Sensor hat kein ACK gesendet */
    return;
# endif

  if ((res & 1) == 0) {
    /* EEPROM Checksummen-Fehler */
    return;
  }
#endif

  /* Aufloesung auf 17 Bit setzen */
  i2c_write_register(SCP_I2C_ADDRESS, SCP_DATAWR,    SCPI_CFG_HIGHRES);
  i2c_write_register(SCP_I2C_ADDRESS, SCP_ADDRPTR,   SCPI_CFG);
  i2c_write_register(SCP_I2C_ADDRESS, SCP_OPERATION, SCPOP_WR_INDIRECT);

  /* Low-Power-Modus einschalten */
  i2c_write_register(SCP_I2C_ADDRESS, SCP_OPERATION, Mode::LOW_POWER);

  return;
}

void SCP1000::set_mode(Mode::mode_t mode) {
  i2c_write_register(SCP_I2C_ADDRESS, SCP_OPERATION, mode);
}

int SCP1000::read_temp() {
  unsigned char bytes[2];

  if (i2c_read_registers(SCP_I2C_ADDRESS, SCP_TEMPOUT, 2, bytes))
    return -1;

  return ((bytes[0] & 0x3f) << 8) | bytes[1];
}

long SCP1000::read_pressure() {
  unsigned char bytes[3];

  if (i2c_read_registers(SCP_I2C_ADDRESS, SCP_DATARD16, 2, bytes))
    return -1;

  bytes[2] = i2c_read_register(SCP_I2C_ADDRESS, SCP_DATARD8);

  return ((long)(bytes[2] & 0x07) << 16) | (bytes[0] << 8) | bytes[1];
}

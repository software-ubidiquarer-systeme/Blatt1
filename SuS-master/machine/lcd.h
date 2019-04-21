/*
 * lcd.h
 *
 * Objekt zur grundlegenden Ansteuerung des LCDs der Chronos-Uhr
 *
 * Kann im Augenblick nur initialisieren (im Konstruktor) und
 * alle Segmente ausschalten.
 */

#ifndef LCD_H
#define LCD_H
// defniere Segmente auf dem LCD um sie später einfacher zu verknüpfen
// wenn upper == true --> dann oben , sonst unten
#define Segment_A(upper) (upper == true ? 0b00010000 : 0b00000001)
#define Segment_B(upper) (upper == true ? 0b00100000 : 0b00000010)
#define Segment_C(upper) (upper == true ? 0b01000000 : 0b00000100)
#define Segment_D(upper) (upper == true ? 0b10000000 : 0b00001000)
#define Segment_E(upper) (upper == true ? 0b00000100 : 0b01000000)
#define Segment_F(upper) (upper == true ? 0b00000001 : 0b00010000)
#define Segment_G(upper) (upper == true ? 0b00000010 : 0b00100000)


class LCD {
private:
  static LCD lcd; // Singleton Instanz

  LCD(const LCD &copy); // Ve1rhindere Kopieren
  LCD();

  // Hinweis: Speicherbereich des LCD-Controllers (Kap. 23 des slau259)
  enum { LCD_MEMORY_START = 0x0a00 + 0x20, /* LCDM1  */
         LCD_MEMORY_END   = 0x0a00 + 0x2d  /* LCDM14 */ };

public:
   /* Methode zum Zugriff auf die Singleton Instanz */
  static LCD& instance() {
    return lcd;
  }

  // Alle Segmente ausschalten
  void clear();

  /*
   * Fuer die zu implementierenden Funktionen gelten folgende Konventionen:
   *
   * "upper_line" legt fest, ob die obere (upper_line=true) oder untere
   * (upper_line=false) Zeile des LCDs angesteuert werden soll.
   *
   * "pos" legt die Position innerhalb einer Zeile fest, von links nach rechts:
   * Die obere Zeile hat 4 Positionen:    1234
   * Die untere Zeile hat 5 Positionen:  12345
   *11
   */

  // Zeigt eine einzelne Ziffer "digit" an
  void show_digit(unsigned int digit, unsigned int pos, bool upper_line=false);

  // Unterfunktion für show_digit, die die Ziffer in Segmente darstellt
  int transform_digit(unsigned int digit, bool upper_line);

  // Zeigt ein einzelnes Zeichen "letter" an
  void show_char(const char letter, unsigned int pos, bool upper_line=true);

  // Zeigt eine ganze Zahl "number" an
  void show_number(long int number, bool upper_line=false);

  // Zeigt den String "text" an
  void show_string(const char *text, bool upper_line=true);
};

#endif

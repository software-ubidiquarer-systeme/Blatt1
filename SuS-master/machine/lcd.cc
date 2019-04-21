/*
 * lcd.cc
 *
 * Objekt zur grundlegenden Ansteuerung des LCDs der Chronos-Uhr
 *
 * Kann im Augenblick nur initialisieren (im Konstruktor) und
 * alle Segmente ausschalten.
 */

#include <msp430.h>

#include "machine/lcd.h"


LCD LCD::lcd; // Singleton Instanz

LCD::LCD() {
  // Display-Speicher loeschen
  LCDBMEMCTL |= LCDCLRBM | LCDCLRM;

  // LCD_FREQ = ACLK/16/8 = 256Hz
  // Framefrequenz = 256Hz/4 = 64Hz, LCD mux 4, LCD on
  LCDBCTL0 = LCDDIV_15 | LCDPRE__8 | LCD4MUX | LCDON;

  // Blinkfrequenz = ACLK/8/4096 = 1Hz
  LCDBBLKCTL = LCDBLKPRE0 | LCDBLKPRE1 |
               LCDBLKDIV0 | LCDBLKDIV1 | LCDBLKDIV2 | LCDBLKMOD0;

  // I/O to COM outputs
  P5SEL |= (BIT5 | BIT6 | BIT7);
  P5DIR |= (BIT5 | BIT6 | BIT7);

  // LCD-Ausgabe aktivieren
  LCDBPCTL0 = 0xFFFF;
  LCDBPCTL1 = 0xFF;
}

void LCD::clear() { LCDBMEMCTL |= LCDCLRBM | LCDCLRM; }


// implementierung für eine Ziffer show_digit

void LCD::show_digit(unsigned int digit, unsigned int pos, bool upper_line){
  // also standardanzeige unten
  // Erkenntis:
  // obere Zahlen (von links nach Rechts): für x = 1, 2, 3, 5 mit spezfischer Segmentdarstellung
  // erstelle eine Klasse für Segmentdarstellung

  // definiere Anfangsadresse
  char *LCD_BASE = reinterpret_cast<char*>(0x0a00);
  char *addr;
  int offset = 0x20;
  addr = LCD_BASE + offset ; // addr zeigt auf Seg0 und Seg1 (s. Datenblatt)

  // prüfe ob alle Eingaben korrekt und setze addr auf die richtige Adresse
  if(!(digit > 9)){
    if(upper_line == true){
      // oben ---> x = 1, 2, 3, 5 wobei Anfang bei 1 ----> 5
      if(pos >= 1 && pos <= 4){
        if(pos == 4) pos++; // nicht schön !!
      }else{      // Fehlerbehandlung
      }
    }else{        // unten ---> 7<= x <= 11 wobei Anfang bei 11 ----> 7
      if(pos >= 1 && pos <= 5){
        pos = 12 - pos;
      }else{      //Fehlerbehandlung
      }
    }
  //------------------------------------------------------
  // hier wird dann die Zahl in die ausgewählte Speicheradresse geschrieben
  addr += pos;
  *addr = transform_digit(digit, upper_line);
  }else{
    //Fehlerbehandlung
  }

  //Ende
}
int LCD::transform_digit(unsigned int digit, bool upper_line){
  int res = 0;
  if(digit == 0 || digit == 2 || digit == 8){
    res = Segment_A(upper_line) | Segment_B(upper_line) |
          Segment_D(upper_line) | Segment_E(upper_line);
    if(digit == 8 || digit == 0){
      res = res | Segment_C(upper_line) | Segment_F(upper_line);
      if(digit == 8) res = res | Segment_G(upper_line);
    }else{
      res = res | Segment_G(upper_line);
    }
  }else if(digit == 1 || digit == 3 || digit == 4 || digit == 7){
    res = Segment_C(upper_line) | Segment_B(upper_line);
    if(digit == 7 || digit == 3){
      res = res | Segment_A(upper_line);
      if(digit == 3) res = res | Segment_G(upper_line) | Segment_D(upper_line);
    }else if(digit == 4){
      res = res | Segment_F(upper_line) | Segment_G(upper_line);
    }

  }else if(digit == 5 || digit == 6 ||  digit == 9){
    res = Segment_A(upper_line) | Segment_F(upper_line) | Segment_G(upper_line) |
          Segment_C(upper_line) | Segment_D(upper_line);
    if(digit == 9){
        res = res | Segment_B(upper_line);
    }else if(digit == 6){
      res = res | Segment_E(upper_line);
    }
  }
  return res;
}

void LCD::show_char(const char letter, unsigned int pos, bool upper_line){
  char *LCD_BASE = reinterpret_cast<char*>(0x0a00);
  char *addr;
  int offset = 0x20;

  addr = LCD_BASE + offset ; // addr zeigt auf Seg0 und Seg1 (s. Datenblatt)
  // switch case
  //letter = tolower(letter);
  unsigned int res = 0;
  switch (letter) {
    case 'a': res = Segment_A(upper_line) | Segment_B(upper_line) | Segment_C(upper_line) | Segment_E(upper_line) | Segment_F(upper_line) | Segment_G(upper_line); break;
    case 'b': res = Segment_C(upper_line) | Segment_D(upper_line) | Segment_E(upper_line) | Segment_F(upper_line) | Segment_G(upper_line);break;
    case 'c': res = Segment_D(upper_line) | Segment_E(upper_line) | Segment_G(upper_line);break;
    case 'd': res = Segment_B(upper_line) | Segment_C(upper_line) | Segment_D(upper_line) | Segment_E(upper_line) | Segment_E(upper_line);break;
    case 'e': res = Segment_A(upper_line) | Segment_F(upper_line) | Segment_D(upper_line) | Segment_E(upper_line) | Segment_G(upper_line);break;
    case 'f': res = Segment_A(upper_line) | Segment_F(upper_line) | Segment_G(upper_line) | Segment_E(upper_line) | Segment_E(upper_line);break;
    case 'g': res = Segment_A(upper_line) | Segment_F(upper_line) | Segment_G(upper_line) | Segment_E(upper_line) | Segment_C(upper_line);break;
    case 'h': res = Segment_B(upper_line) | Segment_F(upper_line) | Segment_G(upper_line) | Segment_E(upper_line) | Segment_C(upper_line);break;
    case 'i': res = Segment_C(upper_line) | Segment_B(upper_line);break;
    case 'j': res = Segment_A(upper_line) | Segment_B(upper_line) | Segment_C(upper_line) | Segment_D(upper_line) | Segment_E(upper_line) | Segment_G(upper_line);break;
    case 'k': res = Segment_E(upper_line) | Segment_F(upper_line) | Segment_G(upper_line);break;
    case 'l': res = Segment_D(upper_line) | Segment_E(upper_line) | Segment_F(upper_line) | Segment_G(upper_line);break;
    case 'm': res = Segment_A(upper_line) | Segment_B(upper_line) | Segment_C(upper_line) | Segment_D(upper_line) | Segment_G(upper_line);break;
    case 'n': res = Segment_C(upper_line) | Segment_E(upper_line) | Segment_G(upper_line);break;
    case 'o': res = Segment_C(upper_line) | Segment_D(upper_line) | Segment_E(upper_line) | Segment_G(upper_line);break;
    case 'p': res = Segment_A(upper_line) | Segment_B(upper_line) | Segment_E(upper_line) | Segment_F(upper_line) | Segment_G(upper_line) ;break;
    case 'q': res = Segment_A(upper_line) | Segment_B(upper_line) | Segment_C(upper_line) | Segment_F(upper_line) | Segment_G(upper_line) ;break;

    case 'r': res = Segment_E(upper_line)| Segment_G(upper_line); break;
    case 's': res = Segment_A(upper_line) | Segment_C(upper_line) | Segment_D(upper_line) | Segment_F(upper_line) | Segment_G(upper_line); break;
    case 't': res = Segment_B(upper_line) | Segment_C(upper_line) | Segment_G(upper_line); break;

    case 'u': res = Segment_C(upper_line) | Segment_D(upper_line) | Segment_E(upper_line); break;
    case 'v': res = Segment_B(upper_line) | Segment_C(upper_line) | Segment_D(upper_line) | Segment_E(upper_line) | Segment_F(upper_line); break;

    case 'w': res = Segment_B(upper_line) | Segment_C(upper_line) | Segment_D(upper_line) | Segment_E(upper_line) | Segment_F(upper_line) | Segment_G(upper_line); break;
    case 'x': res = Segment_B(upper_line) | Segment_E(upper_line) | Segment_G(upper_line) ;break;
    case 'y': res = Segment_B(upper_line) | Segment_C(upper_line) | Segment_F(upper_line) | Segment_G(upper_line) ;break;
    case 'z': res = Segment_A(upper_line) | Segment_B(upper_line) | Segment_D(upper_line) | Segment_E(upper_line) | Segment_G(upper_line) ;break;
    case '=': res = Segment_D(upper_line) | Segment_G(upper_line) ;break;
    case '-': res = Segment_G(upper_line); break;

  default: break;
}
  if(upper_line)
  {
    if (pos == 4)
    {
      pos++;
    }
    addr += pos;
  }
  else
  {
    addr += 12-pos;
  }
  *addr = res;

}
void LCD::show_number(long int number, bool upper_line)
{
  int numbers[6];
  int i = 1;
  int k = 1;
  while (number > 0)
  {
    numbers[i] = number%10;
    number /= 10;
    i++;
  }
  i--;
  while (i > 0)
  {
    show_digit(numbers[k], i, upper_line);
    k++;
    i--;
  }
}

//erstelle

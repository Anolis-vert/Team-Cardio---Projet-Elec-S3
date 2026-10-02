#include <Wire.h>
#include <Arduino.h>
#include <U8g2lib.h>

/*

La bibliothèque Adafruit_SSD1306 prenait trop de mémoire pour l'Arduino Nano (1024 octet par écran pour 2048 octets de mémoire).

Nous avons donc du trouver une autre bibliothèque demandant moins de RAM afin de contrôler les deux écrans.

La bibliothèque que nous utiliseront est donc U8g2.

*/

// Taille des écrans Oled
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64


// Création des objets représentant les écrans avec U8g2
U8G2_SSD1306_128X64_NONAME_1_HW_I2C display1(U8G2_R0, U8X8_PIN_NONE);
U8G2_SSD1306_128X64_NONAME_1_HW_I2C display2(U8G2_R0, U8X8_PIN_NONE);

//Définition de variables globales (utilisées dans loop())
long int temps_0 = millis();
long int prev_time = millis();
short ind = 0;

//Setup()
void setup() {
  //On démarre la communication à 115000
  Serial.begin(115200);

  //On enregistre les adresses I2C des écrans
  display1.setI2CAddress(0x3C << 1); //Ecran 1 : 0x3C (la résistance n'a pas été modifiée)
  display2.setI2CAddress(0x3D << 1); //Ecran 2 : 0x3D (la résistance a été déplacée et ressoudée)

  //Initialisation des écrans
  display1.begin();
  display2.begin();

  //Défini la police d'écriture des textes
  display1.setFont(u8g2_font_ncenB14_tr); //Grande
  display2.setFont(u8g2_font_6x10_tf); //Petite

  //Couleur du texte
  display1.setFontMode(1);
  display2.setFontMode(1);
}


//--------------------------------------------------------------------------------------------------------------------------------------------

/*Affichage des écrans*/


void drawScreen1(U8G2_SSD1306_128X64_NONAME_1_HW_I2C &display, int heure, short ind){
  display.firstPage();
  do{
    display.setDrawColor(1);
    display.drawLine(10,SCREEN_HEIGHT/2, SCREEN_WIDTH-10, SCREEN_HEIGHT/2);
    display.setCursor(SCREEN_WIDTH/2-26, 3*SCREEN_HEIGHT/8);

    setHeure(heure, ind, display);
    if(ind == 0){
      drawHeart(SCREEN_WIDTH/4, 9*SCREEN_HEIGHT/16, display);
    }
    display.setCursor(SCREEN_WIDTH/2, 13*SCREEN_HEIGHT/16);
    display.print("096");
  } while (display.nextPage());
}


void drawScreen2(U8G2_SSD1306_128X64_NONAME_1_HW_I2C &display){
  display.firstPage();
  do {
    display.drawLine(2, SCREEN_HEIGHT-2, SCREEN_WIDTH-10, SCREEN_HEIGHT-2);
    display.drawLine(2, 10, 2, SCREEN_HEIGHT-2);
    display.drawTriangle(SCREEN_WIDTH-10, SCREEN_HEIGHT-4, SCREEN_WIDTH-10, SCREEN_HEIGHT-1,SCREEN_WIDTH-7, SCREEN_HEIGHT-2);
    display.setCursor(1,7);
    display.print("A");
    display.setCursor(SCREEN_WIDTH-6, SCREEN_HEIGHT-1);
    display.print("s");
  } while (display.nextPage());
}


//--------------------------------------------------------------------------------------------------------------------------------------------

/*Affichage de l'heure*/

void setHeure(int heure, short ind, U8G2_SSD1306_128X64_NONAME_1_HW_I2C &display){
  if(heure/60<10){
    display.print(0);
    display.print(heure/60);
  }

  else{
    display.print(heure/60);
  }

  if(ind == 1){
    display.setDrawColor(0);
  }
  display.print(":");
  display.setDrawColor(1);

  if (heure%60<10){
    display.print(0);
    display.print(heure%60);
  }
  else{
    display.print(heure%60);
  }
}


//--------------------------------------------------------------------------------------------------------------------------------------------



// Bitmap 16x16 d'un coeur (1 bit par pixel)
const unsigned char heart16x16[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x78, 0x1e, 0xfc, 0x3f,
  0xfe, 0x7f, 0xfe, 0x7f, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xfe, 0x7f, 0xfe, 0x7f, 0xfc, 0x3f,
  0xf8, 0x1f, 0xf0, 0x0f, 0xc0, 0x03, 0x00, 0x00
};

void drawHeart(int x, int y, U8G2_SSD1306_128X64_NONAME_1_HW_I2C &display){
  display.drawXBMP(x, y, 16, 16, heart16x16);
}


//--------------------------------------------------------------------------------------------------------------------------------------------
void loop() {
  // put your main code here, to run repeatedly:
  long int heure;
  long int tmp_retenu = millis();
  heure = (tmp_retenu-temps_0)/60000;
  if (heure >= 1440){
    temps_0 = tmp_retenu;
    heure = 0;
  }
  if(tmp_retenu-prev_time >= 500){
    ind = 1-ind;
    prev_time = tmp_retenu;
  }
  drawScreen1(display1, heure, ind);
  drawScreen2(display2);

}

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>


/*

La bibliothèque Adafruit_SSD1306 prenait trop de mémoire pour l'Arduino Nano (1024 octet par écran pour 2048 octets de mémoire).

Nous avons donc du trouver une autre bibliothèque demandant moins de RAM afin de contrôler les deux écrans.

La bibliothèque que nous utiliseront est donc U8g2.

*/


// Taille des écrans Oled
#define WIDTH 128
#define HEIGHT 64


// Création des objets représentant les écrans avec U8g2
U8G2_SSD1306_128X64_NONAME_1_HW_I2C display1(U8G2_R0, U8X8_PIN_NONE);
U8G2_SSD1306_128X64_NONAME_1_HW_I2C display2(U8G2_R0, U8X8_PIN_NONE);


//Setup()
void setup() {
  Serial.begin(115000);

  //On enregistre les adresses I2C des écrans
  display1.setI2CAddress(0x3C << 1);   // = 0x78
  display2.setI2CAddress(0x3D << 1);   // = 0x7A

  //Initialisation des écrans
  display1.begin();
  display2.begin();
}

void loop() {
  //Ecriture d'un rectangle plein sur l'écran 1
  display1.firstPage();
  do {
    display1.drawBox(0, 0, WIDTH, HEIGHT);
  } while (display1.nextPage());

  //Effaçage de l'écran 2
  display2.clear();

  //Délai d'une seconde pour prendre la photo (cf test unitaires)
  delay(1000);

  //Ecriture d'un rectangle plein sur l'écran 2
  display2.firstPage();
  do {
    display2.drawBox(0, 0, WIDTH, HEIGHT);
  } while (display2.nextPage());

  //Effaçage de l'écran 1;
  display1.clear();

  //Délai d'une seconde pour prendre la deuxième photo
  delay(1000);
}
#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>


/*

La bibliothèque Adafruit_SSD1306 prenait trop de mémoire pour l'Arduino Nano (1024 octet par écran pour 2048 octets de mémoire).

Nous avons donc du trouver une autre bibliothèque demandant moins de RAM afin de contrôler les deux écrans.

La bibliothèque que nous utiliseront est donc U8g2.

*/


//Définition des pin correspondant aux LEDs
#define LED_J 10  //Jaune
#define LED_V 11  //Vert
#define LED_R 12  //Rouge


//Pin d'entrée du boutton : D2
#define ENTREE 2


// Taille des écrans Oled
#define WIDTH 128
#define HEIGHT 64


// Création des objets représentant les écrans avec U8g2
U8G2_SSD1306_128X64_NONAME_1_HW_I2C display1(U8G2_R0, U8X8_PIN_NONE);
U8G2_SSD1306_128X64_NONAME_1_HW_I2C display2(U8G2_R0, U8X8_PIN_NONE);

int i = 0;

//Setup()
void setup() {
  Serial.begin(115000);

  //On enregistre les adresses I2C des écrans
  display1.setI2CAddress(0x3C << 1);   // = 0x78
  display2.setI2CAddress(0x3D << 1);   // = 0x7A

  //Initialisation des écrans
  display1.begin();
  display2.begin();

  //Initialisation du pin d'entrée avec une interruption
  pinMode(ENTREE, INPUT);
  attachInterrupt(digitalPinToInterrupt(ENTREE), interruption, RISING);

  // Mise des pin en mode OUTPUT
  pinMode(LED_J, OUTPUT);
  pinMode(LED_V, OUTPUT);
  pinMode(LED_R, OUTPUT);
}

//Interruption : Incrémente de 1 le nombre d'appuis et l'affiche
void interruption(){
  //incrémentation
  i++;
  //Affichage
  Serial.println(i);
}

void leds(){
  int ret = i%3;
  switch(ret){
    case 0 :
      digitalWrite(LED_J, LOW);
      digitalWrite(LED_V, LOW);
      digitalWrite(LED_R, HIGH);

    case 1:
      digitalWrite(LED_J, LOW);
      digitalWrite(LED_V, HIGH);
      digitalWrite(LED_R, LOW);

    case 2:
      digitalWrite(LED_J, HIGH);
      digitalWrite(LED_V, LOW);
      digitalWrite(LED_R, LOW);
    }
}

void loop() {
  //Ecriture d'un rectangle plein sur l'écran 1
  display1.firstPage();
  do {
    display1.drawBox(0, 0, WIDTH, HEIGHT);
  } while (display1.nextPage());

  //Effaçage de l'écran 2
  display2.clear();

  leds();

  //Délai d'une seconde pour prendre la photo (cf test unitaires)
  delay(500);

  //Ecriture d'un rectangle plein sur l'écran 2
  display2.firstPage();
  do {
    display2.drawBox(0, 0, WIDTH, HEIGHT);
  } while (display2.nextPage());

  //Effaçage de l'écran 1;
  display1.clear();

  leds();
  //Délai d'une seconde pour prendre la deuxième photo
  delay(500);
}
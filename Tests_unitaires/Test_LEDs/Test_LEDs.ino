#include <Arduino.h>

//Définition des pin correspondant aux LEDs
#define LED_J 10  //Jaune
#define LED_V 11  //Vert
#define LED_R 12  //Rouge

void setup() {
  // Mise des pin en mode OUTPUT
  pinMode(LED_J, OUTPUT);
  pinMode(LED_V, OUTPUT);
  pinMode(LED_R, OUTPUT);
}

void loop() {
  digitalWrite(LED_J, LOW);
  digitalWrite(LED_V, LOW);
  digitalWrite(LED_R, HIGH);
  delay(500);

  digitalWrite(LED_J, LOW);
  digitalWrite(LED_V, HIGH);
  digitalWrite(LED_R, LOW);
  delay(500);

  digitalWrite(LED_J, HIGH);
  digitalWrite(LED_V, LOW);
  digitalWrite(LED_R, LOW);
  delay(500);

}

#include <Arduino.h>

/*
Test unitaire du boutton en pull-up.
Essais de correction du rebond avec un condensateur en parrallèle du boutton
*/

//Pin d'entrée du boutton : D2
#define ENTREE 2

//Nombre d'appuie du boutton
int i = 0;

void setup() {
  //Communication à la vitesse utilisée lors du projet pour voir s'il y a des rebonds détectés
  Serial.begin(115200);
  //Initialisation du pin d'entrée avec une interruption
  pinMode(ENTREE, INPUT);
  attachInterrupt(digitalPinToInterrupt(ENTREE), interruption, RISING);

}

//Interruption : Incrémente de 1 le nombre d'appuis et l'affiche
void interruption(){
  //incrémentation
  i+=1;
  //Affichage
  Serial.println(i);
}

//loop()
void loop() {
  // On ne veut que les interruption, il n'y a pas besoin d'écrire dans le loop()
}

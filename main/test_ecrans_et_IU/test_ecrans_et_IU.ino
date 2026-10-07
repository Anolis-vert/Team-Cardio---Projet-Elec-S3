//inclusion des bibliothèques utilisées
#include <Wire.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include <EEPROM.h>

/*

La bibliothèque Adafruit_SSD1306 prenait trop de mémoire pour l'Arduino Nano (1024 octet par écran pour 2048 octets de mémoire).
Nous avons donc du trouver une autre bibliothèque demandant moins de RAM afin de contrôler les deux écrans.
La bibliothèque que nous utiliseront est donc U8g2.

Beaucoups de commentaires dans ce code ne seront pas utile à sa compréhension, ils sont là uniquement comme pense bête lors de la présentation orale du projet.
Ils ne sont pas générés à l'IA, mais ne pouvant pas retenir l'intégralité des informations trouvées et utilisées pour ce projet, je préfère commenter afin de ne pas oublier lors de la démonstration.
Je vous prie de ne pas en tenir vigueur lors de la correction de ce code.
Merci beaucoup,
Cordialement.

*/


//--------------------------------------------------------------------------------------------------------------------------------------------


/*Définition des constantes*/

// Taille des écrans Oled
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

//Définition des pins de l'arduino utilisés
#define JAUNE 10        //LED jaune
#define VERT 11         //LED verte
#define ROUGE 12        //LED rouge
#define BUZZER 5        //Buzzer
#define BOUTTON1 2      //Boutton jaune
#define BOUTTON2 3      //Boutton rouge
#define SWENC A1        //Boutton de l'encodeur rotatif
#define DTENC 9         //broche DT de l'encodeur rotatif
#define CLKENC 4        //Broche Clock de l'encodeur rotatif
#define CLKRTC 6        //Broche Clock du module RTC DS1302
#define DATARTC 7       //Broche Data du module RTC DS1302
#define RSTRTC 8        //Broche Reset du module RTC DS1302
#define WPSE A0         //Module WPSE340


//Définition des registres du DS1302 (partie microcontrôleur)
#define SECRTC 0    //Secondes
#define MINRTC 1    //Minutes
#define HRRTC 2     //Heures
#define DATERTC 3   //Jours du mois (1-31)
#define MOISRTC 4   //Mois
#define JOURRTC 5   //Jour de la semaine (1-7)
#define ANRTC 6     //Année
#define WPRTC 7     //Write protect (0.Autorise / 2.Interdit l'écriture sur les autres registres)


//--------------------------------------------------------------------------------------------------------------------------------------------


/* Création des objets représentant les écrans avec U8g2 */

U8G2_SSD1306_128X64_NONAME_1_HW_I2C display1(U8G2_R0, U8X8_PIN_NONE);   //Ecran 1
U8G2_SSD1306_128X64_NONAME_1_HW_I2C display2(U8G2_R0, U8X8_PIN_NONE);   //Ecran 2


//--------------------------------------------------------------------------------------------------------------------------------------------


/*Définition de variables globales (utilisées dans loop())*/

//Affichages
long int prev_time = millis();              //Moment de démarrage (utilisé dans la gestion de ind)
short ind = 0;                              //Indice (utilisé dans la gestion des timing)
bool indEcran = true;                         //Ecran à afficher à 0x3D

//WPSE et battement crdiaque
volatile int bpm;                           //Battements par minutes
volatile int signalWPSE;                    //Signale reçu du WPSE340
volatile int freqInd = 600;                 //Temps entre les deux dernières détections de battement cardiaque
volatile boolean pulsation = false;         //Battement cardiaque en cours
volatile boolean battementTrouve = false;   //Battement cardiaque détecté
volatile int freqCard[10];                  //Liste des dix dernienières fréquence cardiaques
volatile unsigned long horloge = 0;         //Horloge utilisée pour le WPSE340
volatile unsigned long dernBat = 0;         //Heure du dernier battement cardiaque
volatile int max = 512;                     //Amplitude max du battement cardiaque
volatile int min = 512;                     //Amplitude min du battement cardiaque
volatile int seuil = 525;                   //Seuil de détection du battement
volatile int amp = 100;                     //Amplitude de la pulsation
volatile boolean premBat = true;            //Le battement en cours est le premier
volatile boolean deuxBat = false;           //Le battement en cours est le deuxième

//Courbe
static int nbrPoints = 59;                  //Nombre de points retenus
static int echelle = 2;                     //Nombre de pixel que prend une mesure
int courbe[118];                            //Liste des valeurs à afficher sur l'écran 2
int indCourbe = 0;                          //indice parcourant la courbe pour enregistrer les 118 ernières valeurs
unsigned long dernierPoint = 0;             //Moment du dernier point tracé

//Sauvegarde des données
struct save_t{                              //Structure des enregistrements
  int bpmMoyen;
  int heure;
  int min;
  int jour;
  int mois;
  int annee;
}




//--------------------------------------------------------------------------------------------------------------------------------------------


/*Enregistrement du poul*/




//--------------------------------------------------------------------------------------------------------------------------------------------

/*Obtention du poul en BPM*/

//Calcule des battements  : ISR (Interrupt Service Routine), appelé toutes les deux millisecondes comptées par le timer 1
ISR(TIMER1_COMPA_vect){
  cli();                              //Désactive les interruptions
  signalWPSE = analogRead(WPSE);      //Lit la valeur envoyée par le WPSE
  horloge += 2;                       //Note le moment de lecture du WPSE
  int tempsPasse = horloge-dernBat;   //Note le temps écoulé depuis le dernier battement

  //Min et max du battement (tempsPasse > (freqInd/5)*3) évite d'avoir des bruits parasites)
  if (signalWPSE<seuil && tempsPasse > (freqInd/5)*3){
    if (signalWPSE < min){    //Si le signal est inférieur au minimum, on le retient comme le minimum
      min = signalWPSE;
    }
  }

  //Si le signal est supérieur au maximum, on le retient en maximum
  if(signalWPSE>seuil && signalWPSE > max){
    max = signalWPSE;
  }

  //Calcul du battement cardiaque
  if(tempsPasse > 250){
    //Evite encore le bruit
    if ((signalWPSE > seuil) && (pulsation == false) && (tempsPasse > (freqInd/5)*3)){
      pulsation = true;               //Une pulsation est en cours
      freqInd = horloge - dernBat;    //mesure le temps entre deux battements
      dernBat = horloge;              //Enregistre le moment du dernier battement

      //Si c'est le deuxième battement, on l'enregistre partout pour ne pas avoir une valeur manquante
      if(deuxBat){
        deuxBat = false;
        for(int i = 0; i<10; i++){
          freqCard[i] = freqInd;
        }
      }

      //On ne prends pas le premier battement car il est sans doute abberant
      if(premBat){
        premBat = false;
        deuxBat = true;
        sei();
        return;
      }

      //Moyenne des 10 dernières fréquences
      word moyenneFreq = 0;            //total des 10 dernières fréquences (word est non signé sur deux octets et permet d'aller jusqu'à 65 535, ce qui est assez pour la somme des fréquences comparé à un int)
      //Addition des 10 derniers battements
      for (int i = 0; i<9; i++){
        freqCard[i] = freqCard[i+1];    //On décale les derniers battements pour enlever le premier de la liste des fréquences cardiaques
        moyenneFreq += freqCard[i];     //On ajoute les huits derniers battements
      }
      freqCard[9] = freqInd;           //On ajoute de dernier battement à la liste des fréquences cardiaques
      moyenneFreq+= freqCard[9];       //On ajoute le dernier battement à la moyenne des battements cardiaques
      moyenneFreq/=10;                 //On obtient la moyenne en divisant par le nombre total de fréquences retenue
      bpm = 60000/moyenneFreq;         //Calcul du bpm : combien de battement tiennent dans une minute (60 000 ms)
      battementTrouve = true;          //On a trouvé le battement cardiaque
    }
  }

  //battement terminé : on remet les valeurs à celles de base
  if(signalWPSE < seuil && pulsation == true){
    pulsation = false;    //On n'a plus de poul
    amp = max-min;        //l'amplitude est crête maximum - crête minimum
    max = seuil;          //On réinitialise le max au seuil (512)
    min = seuil;          //On réinitialise le max au seuil (512)
  }

  //si on n'a pas eu de battement pendant 2.5s, on réinitialise les valeurs
  if (tempsPasse > 2500){
    seuil = 512;          //On réinitialise le seuil (512)
    max = 512;            //On réinitialise le max au seuil (512)
    min = 512;            //On réinitialise le min au seuil (512)
    dernBat = horloge;    //On remet le dernier battement au temps de l'horloge
    premBat = true;       //On reprends au premier battement
    deuxBat = false;      //Si c'est le premier batement, ça ne peut pas être le deuxième
  }

  sei();    //Autorise les interruptions

}


//--------------------------------------------------------------------------------------------------------------------------------------------


/*Gestion de l'écriture et de la lecture sur les registres du DS1302*/

//Conversion BCD(écrit sur 4 bit) et décimal
uint8_t bcd_dec(uint8_t oct){
  return (oct >> 4) * 10 + (oct & 0x0F);    //BCD à décimal
}
uint8_t dec_bcd(uint8_t oct){
  return ((oct / 10) << 4) | (oct % 10);    //Décimal à BCD
}


//Ecriture et lecture des registres depuis un octet (uint8_t)
void ecritBit(uint8_t oct){
  pinMode(DATARTC, OUTPUT);
  for(uint8_t i = 0; i<8; i++){
    digitalWrite(DATARTC, (oct>>i) & 1); //On écrit le bit numéro i sur la pin DATARTC (&1 sert à mettre le reste des bits envoyés à 0)
    delayMicroseconds(2);
    digitalWrite(CLKRTC, HIGH);          //On déclanche un front montant sur l'horloge => bit envoyé
    delayMicroseconds(2);
    digitalWrite(CLKRTC, LOW);           //On remet la clock à LOW pour povoir redéclancher un front montant au bit suivant
    delayMicroseconds(2);
  }
}
uint8_t litBit(){
  pinMode(DATARTC, INPUT);
  uint8_t oct = 0;
  for(uint8_t i = 0; i<8; i++){
    if(digitalRead(DATARTC)){oct = oct | (1<<i);}   //Remplis un par un les bits de oct
    digitalWrite(CLKRTC, HIGH);                     //Met la clock à HIGH pour pouvoir déclancher un front descendant
    delayMicroseconds(2);
    digitalWrite(CLKRTC, LOW);                      //Déclanche un front descendant => le DS1302 envoie le prochain bit
    delayMicroseconds(2);
  }
  return oct;
}


//Ecriture et lecture sur un registre entier
void ecritReg(uint8_t reg, uint8_t val){
  digitalWrite(RSTRTC, HIGH);   //L'état haut démarre la communication avec le RTC
  delayMicroseconds(4);
  ecritBit(0x80 | (reg << 1));  //On dit au DS1302 ce qu'on veut faire : 0x80 = 10000000 => 1(toujours 1) 0(horloge) 00000(registre modifié avec reg<<1) 0(ecriture)
  ecritBit(val);                //Envoie de la donnée
  digitalWrite(RSTRTC, LOW);    //L'état bas coupe la communication avcec le RTC
  delayMicroseconds(4);
}
uint8_t litReg(uint8_t reg){
  digitalWrite(RSTRTC, HIGH);   //L'état haut démarre la communication avec le RTC
  delayMicroseconds(4);
  ecritBit(0x81 | (reg<<1));    //On dit au DS1302 ce qu'on veut faire : 0x80 = 10000000 => 1(toujours 1) 0(horloge) 00000(registre modifié avec reg<<1) 1(lecture)
  uint8_t oct = litBit();       //Lecture de la donnée
  digitalWrite(RSTRTC, LOW);    //L'état bas coupe la communication avcec le RTC
  delayMicroseconds(4);
  return oct;
}


//--------------------------------------------------------------------------------------------------------------------------------------------


/*Calcule de la date et de l'heure*/

//Trouver le jours du mois
uint8_t sakamoto(uint16_t annee, uint8_t mois, uint8_t jour){
  static const uint8_t t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
  if (mois < 3){
    annee -= 1;
  }
  uint8_t semaine = (annee + annee / 4 - annee / 100 + annee / 400 + t[mois-1] + jour) %7;   
  return semaine == 0 ? 7 : semaine;
}


//Initialisation de l'heure actuelle
void setheure(){
  //__TIME__ => "hh:mm:ss", ex "21:36:40"
  uint8_t h = (__TIME__[0] - '0') * 10 + (__TIME__[1] - '0');   //'2'-'0'*10+'1'-'0' = 2*10+1 = 21
  uint8_t m = (__TIME__[3] - '0') * 10 + (__TIME__[4] - '0');   //'3'-'0'*10+'6'-'0' = 3*10+6 = 36
  uint8_t s = (__TIME__[6] - '0') * 10 + (__TIME__[7] - '0');   //'4'-'0'*10+'0'-'0' = 4*10+0 = 40

  //__DATE__ => "Mmm dd yyyy", ex Sep  9 2026
  //Mois
  char moisActuel[4] = {__DATE__[0], __DATE__[1], __DATE__[2], 0};     //{'S','e','p'} 
  const char *moisAnnee = "JanFebMarAprMayJunJulAugSepOctNovDec";   //Liste des mois possibles
  uint8_t mois = (strstr(moisAnnee, moisActuel)-moisAnnee)/3 +1;    //0x.. - 0x24/3+1 = 24/3+1 = 8+1 = 9 => on est au 9ème mois de l'année

  //Jours
  uint8_t jour = (__DATE__[4] == ' ' ? 0 : (__DATE__[4] - '0') * 10) + (__DATE__[5] - '0');  //' ' ? 0 remplace un espace par 0. " 9" => '0'-'0'*10+'9'-'0' = 0*10+9=9
  uint16_t annee = atoi(__DATE__ + 7);                                                   //Convertit la date en int à partir du 7ème élément => "(Sep  9 )2026" => "2026" = 2026 (en unit16_t car trop gros pour un seul octet)

  //Ecriture sur les registre du DS1302
  ecritReg(WPRTC, 0x00);                                      //Autorisation d'écrire
  ecritReg(SECRTC, dec_bcd(s));                               //Seconde
  ecritReg(MINRTC, dec_bcd(m));                               //Minute
  ecritReg(HRRTC, dec_bcd(h));                                //Heure
  ecritReg(DATERTC, dec_bcd(jour));                           //Jours du mois
  ecritReg(JOURRTC, dec_bcd(sakamoto(annee, mois, jour)));    //Jours de la semaine
  ecritReg(ANRTC, dec_bcd(annee)%100);                        //Année (seulement les deux derniers chiffres => 26)
  ecritReg(WPRTC, 0x80);                                      //Interdiction d'écrire
}


//--------------------------------------------------------------------------------------------------------------------------------------------


/*Affichage de l'heure*/

void afficheHeure(short ind, U8G2_SSD1306_128X64_NONAME_1_HW_I2C &display){
  uint8_t m = bcd_dec(litReg(MINRTC));   //Récupération des minutes
  uint8_t h = bcd_dec(litReg(HRRTC));    //Récupération des heures

  //Affichage des heures
  if (h < 10) {
    display.print("0");
  }        
  display.print(h);

  //Affichage du ':' clignotant
  if(ind%2 == 1){
    display.setDrawColor(0);
  }
  display.print(":");
  display.setDrawColor(1);

  //Affichage des minutes
  if (m < 10){
    display.print("0");
  }
  display.print(m);
}


//--------------------------------------------------------------------------------------------------------------------------------------------


/*Creation des images affichées (Bitmap)*/

// Coeur
const unsigned char heart16x16[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x78, 0x1e, 0xfc, 0x3f,
  0xfe, 0x7f, 0xfe, 0x7f, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xfe, 0x7f, 0xfe, 0x7f, 0xfc, 0x3f,
  0xf8, 0x1f, 0xf0, 0x0f, 0xc0, 0x03, 0x00, 0x00
};
//Affichage du coeur
void drawHeart(int x, int y, U8G2_SSD1306_128X64_NONAME_1_HW_I2C &display){
  display.drawXBMP(x, y, 16, 16, heart16x16);
}


//--------------------------------------------------------------------------------------------------------------------------------------------


/* Gestion de l'affichage des battements cardiaques*/

//Allumage des LEDS et du buzzer
void allumeLeds(int bpm, bool chmtInd){
  //Rythme entre 60 et 100 => Normal => LED verte allumée et buzzer à 1500Hz
  if(bpm >=60 && bpm <= 100){ 
    digitalWrite(ROUGE, LOW);
    digitalWrite(VERT, HIGH);
    digitalWrite(JAUNE, LOW);
    if(chmtInd){
      tone(BUZZER, 1500, 100);
    }   
  }
  //Rythme entre 40 et 60 => Bas => LED jaune allumée et buzzer à 500Hz
  else if(bpm >40 && bpm < 60){
    digitalWrite(ROUGE, LOW);
    digitalWrite(VERT, LOW);
    digitalWrite(JAUNE, HIGH);
    if(chmtInd){
      tone(BUZZER, 500, 100);
    } 
  }
  //Rythme entre 100 et 140 => Haut => LED rouge allumée et buzzer à 2500Hz
  else if(bpm >100.0 && bpm < 140.0){
    digitalWrite(ROUGE, HIGH);
    digitalWrite(VERT, LOW);
    digitalWrite(JAUNE, LOW);
    if(chmtInd){
      tone(BUZZER, 2500, 100);
    } 
  }
  //Rythme inférieur à 40 ou supérieur à 140 => Valeurs abberantes => aucune LED allumée et buzzer éteint
  else{
    digitalWrite(ROUGE, LOW);
    digitalWrite(VERT, LOW);
    digitalWrite(JAUNE, LOW);
  }
}


//--------------------------------------------------------------------------------------------------------------------------------------------


/*Affichage des écrans*/

//Ecran 1
void drawScreen1(U8G2_SSD1306_128X64_NONAME_1_HW_I2C &display, short ind, int bpm){
  display.firstPage();
  do{
    //Trace la ligne horizontale au millieu
    display.setDrawColor(1);
    display.drawLine(10,SCREEN_HEIGHT/2, SCREEN_WIDTH-10, SCREEN_HEIGHT/2);
    display.setCursor(SCREEN_WIDTH/2-26, 3*SCREEN_HEIGHT/8);

    //Affiche l'heure actuelle
    afficheHeure(ind, display);

    //Affiche le coeur en le faisant clignoter si la valeur n'est pas aberrante
    if(ind%2 == 0 || bpm<40 || bpm >140){
      drawHeart(SCREEN_WIDTH/4, 9*SCREEN_HEIGHT/16, display);
    }

    //Affichage du BPM
    display.setCursor(SCREEN_WIDTH/2, 13*SCREEN_HEIGHT/16);
    //Si on a une valeur supérieur à 100, on l'affiche comme ça
    if(bpm <=140 && bpm >= 100){
      display.print(bpm);
    }
    //Si c'est inférieur à 100, on affiche le '0' puis la valeur pour avoir trois caractères
    else if(bpm<100 && bpm >=40){
      display.print("0");
      display.print(bpm);
    }
    //Si c'est inférieur à 10, on fait clignoter l'affichage car c'est une valeur potentiellement aberrante et on affiche deux '0' devant pour avoir trois caractères
    else if(bpm <10){
      if(ind%2 == 0){
        display.print("00");
        display.print(bpm);
      }
      else{
        display.print("---");
      }
    }
    //Si c'est inférieur à 40, on fait clignoter l'affichage car c'est une valeur potentiellement aberrante et on affiche encore le '0' devant
    else if(bpm <40){
      if(ind%2 == 0){
        display.print("0");
        display.print(bpm);
      }
      else{
        display.print("---");
      }
    }
    //Si c'est supérieur à 140, on fait clignoter l'affichage car c'est une valeur potentiellement aberrante et on affiche comme ça
    else if(bpm >140){
      if(ind%2 == 0){
        display.print(bpm);
      }
      else{
        display.print("---");
      }
    }
    //Si il n'y a pas de valeur, on affiche rien
    else{
      display.print("---");
    }
  } while (display.nextPage());
}

//Ecran 2 : graphique
void drawScreen2(U8G2_SSD1306_128X64_NONAME_1_HW_I2C &display){
  display.firstPage();
  do {
    //Affiche les axes
    display.drawLine(2, SCREEN_HEIGHT-2, SCREEN_WIDTH-10, SCREEN_HEIGHT-2);                                                     //axe vertical(y, ordonnées, amplitude)
    display.drawLine(2, 10, 2, SCREEN_HEIGHT-2);                                                                                //axe horizontale (x, abscisses, temps)
    display.drawTriangle(SCREEN_WIDTH-10, SCREEN_HEIGHT-4, SCREEN_WIDTH-10, SCREEN_HEIGHT-1,SCREEN_WIDTH-7, SCREEN_HEIGHT-2);   //Triangle sur l'axe horizontal
    display.setCursor(1,7);                                                                                                     //Positionne le curseur sur l'axe des abcsisses
    display.print("A");                                                                                                         //Ecrit l'étiquette des abcsisses (A : amplitude)
    display.setCursor(SCREEN_WIDTH-6, SCREEN_HEIGHT-1);                                                                         //Positionne le curseur sur l'axe des ordonnées
    display.print("t");                                                                                                         //Ecrit l'étiquette des ordonnées (t : temps)

    //Ecrit la courbe
    int xPrec = 0, yPrec = 0;                                 //Les précédents x et y
    for(int i = 0; i < nbrPoints; i++){                       //Pour chaque valeurs de WPSE enregistrées
      int y = map(courbe[i], 0, 300, SCREEN_HEIGHT-3, 11);    //Remet les valeurs de WPSE entre 61 (y=0) et 11 (y=300)
      y = constrain(y, 11, SCREEN_HEIGHT-3);                  //Bloque les valeurs max(61) et min(11) de y
      int x = 4 + i * echelle;                                //remet x à l'échelle (chaque i prends 2px)
      if(i > 0 && i != indCourbe){                            //si il y a un autre point avant i et que i n'est pas le max
        display.drawLine(xPrec, yPrec, x, y);                 //On trace la ligne entre al valeur précédente et la nouvell epour tracer un graphique et pas un nuage de point
      }
      xPrec = x;                                              //On sauvegarde le nouveau x comme l'ancien
      yPrec = y;                                              //On sauvegarde le nouveau y comme l'ancien
    }
  } while (display.nextPage());
}


//Ecran 2 bis (Ecran 3) : enregistrements
void drawScreen3(U8G2_SSD1306_128X64_NONAME_1_HW_I2C &display){
  display.firstPage();
  do{
    display.setCursor(1,7);
    display.print(bpm);
  }while (display.nextPage());
}


//--------------------------------------------------------------------------------------------------------------------------------------------

/*Interruption*/

//WPSE
void initInterrupt(){
  noInterrupts();                                       //Bloque les interruptions
  TCCR1A = 0;                                           //Active le Clear Timer on Compare match (CTC) qui redémarre le Timer1 à une certaine valeur (ici 124). Désactive les PWM des pins 9 et 10 qui y sont liés
  TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10);    //Passe la fréquence de 16MHz à 62 500MHz
  OCR1A = 499;                                          //Met la comparaison du Timer 1 à 124 (500Hz)
  TIMSK1 = (1 << OCIE1A);                               //Autorise l'interruption quand le Timer1 atteint OCR2A (interruption sur comparaison, ici 124)
  interrupts();                                         //Réautorise les interruptions
}

//Boutton de changement d'écran
void interuptBoutton(){
  indEcran = !indEcran;   //On inverse l'indice
}


//--------------------------------------------------------------------------------------------------------------------------------------------


/*Setup et loop*/

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

  //Initialisation des pins de sotie et mise sur LOW (éteint)
  pinMode(JAUNE, OUTPUT);       //LED verte (D10)
  pinMode(VERT, OUTPUT);        //LED verte (D11)
  pinMode(ROUGE, OUTPUT);       //LED rouge (D12)
  pinMode(BUZZER, OUTPUT);      //Buzzer (D5)
  pinMode(CLKRTC, OUTPUT);      //Broche Clock du module RTC DS1302 (D6)
  pinMode(RSTRTC, OUTPUT);      //Broche Reset du module RTC DS1302 (D8)
  pinMode(WPSE, INPUT);         //Module WPSE340 (A0)
  pinMode(SWENC, INPUT);        //Boutton de l'encodeur rotatif (A1)
  pinMode(DTENC, INPUT);        //Broche DT de l'encodeur rotatif (D9)
  pinMode(CLKENC, INPUT);       //Broche Clock de l'encodeur rotatif (D6)
  digitalWrite(JAUNE, LOW);     //LED jaune éteinte
  digitalWrite(VERT, LOW);      //LED verte éteinte
  digitalWrite(ROUGE, LOW);     //LED rouge éteinte
  digitalWrite(BUZZER, LOW);    //Buzzer éteint
  digitalWrite(CLKRTC, LOW);    //Broche Clock du module RTC DS1302 à LOW
  digitalWrite(RSTRTC, LOW);    //Broche Reset du module RTC DS1302 à LOW

  //Attache d'interruptions aux pins concernés
  attachInterrupt(digitalPinToInterrupt(BOUTTON1), interuptBoutton, RISING);    //Boutton jaune (D2)
  attachInterrupt(digitalPinToInterrupt(BOUTTON2), interuptSave, RISING);       //Boutton rouge (D3)

  //Reprise du module RTC
  if(litReg(SECRTC) & 0x80){   //Si le bit 7 des secondes est à 1, l'horloge était arrêtée.
    setheure();                //On met l'heure sur l'heure de compilation
  }

  //initialisation de la courbe
  for (int i = 0; i<118; i++){
    courbe[i] = 0;                //On initialise toutes les valeurs du WPSE à 0 
  }

  //enregistrement des adresses de sauvegarde des bpm
  
  
  //Initialise la procédure d'interruption du timer1 (calcul du BPM)
  initInterrupt();
}


//loop
void loop() {
  //modification de l'indice
  long int tmp_retenu = millis();     //On retient le temps en cours
  bool chmtInd = false;               //On note que l'indice n'a pas été modifié
  if(tmp_retenu-prev_time >= 500){    //Si le temps écoulé est de 500ms ou plus, on doit changer l'indice
    ind = 1+ind;                      //on incrémente le changement d'indice
    prev_time = tmp_retenu;           //on enregistre le nouveau temps de changement d'indice
    chmtInd = (ind%2 == 0);           //On note le changement d'indice toutes les secondes
  }

  //Battement cardiaque trouvé
  if(battementTrouve){
    Serial.print("BPM: ");      //On affiche les bpm sur le moniteur série (debug)
    Serial.print(bpm);          //On affiche les bpm sur le moniteur série (debug)
    battementTrouve = false;    //On marque qu'il n'y a plus de changement cardiaque
  }

  //Obtention de la courbe toutes les 40ms
  if (millis() - dernierPoint >= 40) {
    dernierPoint = millis();                        //On enregistre le moment actuel
    courbe[indCourbe] = signalWPSE - 300;           //On enregistre le signal du WPSE dans la liste
    indCourbe++;                                    //On incrémente l'indice pour enregistrer au prochain la prochaine fois
    if (indCourbe >= nbrPoints) {indCourbe = 0;}    //Si on a atteint le bout de la liste, on repart au début
  }

  //Allume les LEDs en fonctino des BPM
  allumeLeds(bpm, chmtInd);

  //Affiche les écrans
  drawScreen1(display1, ind, bpm);    //Ecran 1
  if(indEcran){                       //S'il faut afficher l'écran 2 (indEcran en true)
    drawScreen2(display2);            //Ecran 2
  }
  else{                               //Sinon (indEcran en false  )
    drawScreen3(display2);            //Ecran 3
  }
}

//inclusion des bibliothèques utilisées
#include <Wire.h>
#include <Arduino.h>
#include <U8g2lib.h>

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
#define JAUNE 10    //LED jaune
#define VERT 11     //LED verte
#define ROUGE 12    //LED rouge
#define BUZZER 5    //Buzzer
#define BOUTTON 2   //Boutton
#define CLKRTC 6    //Broche Clock du module RTC DS1302
#define DATARTC 7   //Broche Data du module RTC DS1302
#define RSTRTC 8    //Broche Reset du module RTC DS1302


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

long int prev_time = millis();  //Moment de démarrage (utilisé dans la gestion de ind)
short ind = 0;                  //Indice (utilisé dans la gestion des timing)
float bpm = 0;


//--------------------------------------------------------------------------------------------------------------------------------------------


/*Gestion de l'écriture et de la lecture sur les registres du DS1302*/

//Conversion BCD (écrit sur 4 bit) et décimal
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
void allumeLeds(float bpm, bool chmtInd){
  //Rythme entre 60 et 100 => Normal => LED verte allumée et buzzer à 1500Hz
  if(bpm >60.0 && bpm < 100.0){ 
    digitalWrite(ROUGE, LOW);
    digitalWrite(VERT, HIGH);
    digitalWrite(JAUNE, LOW);
    if(chmtInd){
      tone(BUZZER, 1500, 100);
    }   
  }
  //Rythme entre 40 et 60 => Bas => LED jaune allumée et buzzer à 500Hz
  else if(bpm >40.0 && bpm < 60.0){
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
void drawScreen1(U8G2_SSD1306_128X64_NONAME_1_HW_I2C &display, short ind, float bpm){
  display.firstPage();
  do{
    display.setDrawColor(1);
    display.drawLine(10,SCREEN_HEIGHT/2, SCREEN_WIDTH-10, SCREEN_HEIGHT/2);
    display.setCursor(SCREEN_WIDTH/2-26, 3*SCREEN_HEIGHT/8);

    afficheHeure(ind, display);
    if(ind%2 == 0){
      drawHeart(SCREEN_WIDTH/4, 9*SCREEN_HEIGHT/16, display);
    }
    display.setCursor(SCREEN_WIDTH/2, 13*SCREEN_HEIGHT/16);
    if(bpm > 140 || bpm < 40){
      display.print("---");
    }
    else if(bpm<100){
      display.print("0");
      display.print(static_cast<int>(bpm));
    }
    else{
      display.print(static_cast<int>(bpm));
    }
  } while (display.nextPage());
}

//Ecran 2 : graphique
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
  pinMode(JAUNE, OUTPUT);
  pinMode(VERT, OUTPUT);
  pinMode(ROUGE, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(CLKRTC, OUTPUT);
  pinMode(RSTRTC, OUTPUT);
  digitalWrite(JAUNE, LOW);
  digitalWrite(VERT, LOW);
  digitalWrite(ROUGE, LOW);
  digitalWrite(BUZZER, LOW);
  digitalWrite(CLKRTC, LOW);
  digitalWrite(RSTRTC, LOW);

  //Reprise du module RTC
  if(litReg(SECRTC) & 0x80){   //Si le bit 7 des secondes est à 1, l'horloge était arrêtée.
    setheure();                //On met l'heure sur l'heure de compilation
  }
}


//loop
void loop() {
  long int tmp_retenu = millis();
  bool chmtInd = false;
  if (bpm<200){
    bpm++;
  }
  else{bpm=0;};
  if(tmp_retenu-prev_time >= 500){
    ind = 1+ind;
    prev_time = tmp_retenu;
    chmtInd = (ind%2 == 0);
  }
  allumeLeds(bpm, chmtInd);
  drawScreen1(display1, ind, bpm);
  drawScreen2(display2);
}

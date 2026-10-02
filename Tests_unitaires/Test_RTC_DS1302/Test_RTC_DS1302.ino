/*
  Test RTC DS1302 - Arduino Nano (ATmega328P)
  Câblage :
    VCC  -> 5V
    GND  -> GND
    CLK  -> D6
    DAT  -> D7
    RST  -> D8 (aussi appelé CE)

  Sans bibliothèque externe : communication bit-bang.
  Si l'horloge est arrêtée (première mise sous tension / pile absente),
  elle est réglée automatiquement sur l'heure de compilation.
  Moniteur série : 9600 bauds.
*/
    
const uint8_t PIN_CLK = 6;
const uint8_t PIN_DAT = 7;
const uint8_t PIN_RST = 8;

// Registres du DS1302
const uint8_t REG_SEC   = 0;
const uint8_t REG_MIN   = 1;
const uint8_t REG_HOUR  = 2;
const uint8_t REG_DATE  = 3;
const uint8_t REG_MONTH = 4;
const uint8_t REG_DAY   = 5;
const uint8_t REG_YEAR  = 6;
const uint8_t REG_WP    = 7;

uint8_t bcd2dec(uint8_t b) { return (b >> 4) * 10 + (b & 0x0F); }
uint8_t dec2bcd(uint8_t d) { return ((d / 10) << 4) | (d % 10); }

void writeByte(uint8_t v) {
  pinMode(PIN_DAT, OUTPUT);
  for (uint8_t i = 0; i < 8; i++) {      // LSB en premier
    digitalWrite(PIN_DAT, (v >> i) & 1);
    delayMicroseconds(2);
    digitalWrite(PIN_CLK, HIGH);
    delayMicroseconds(2);
    digitalWrite(PIN_CLK, LOW);
    delayMicroseconds(2);
  }
}

uint8_t readByte() {
  pinMode(PIN_DAT, INPUT);
  uint8_t v = 0;
  for (uint8_t i = 0; i < 8; i++) {
    if (digitalRead(PIN_DAT)) v |= (1 << i);
    digitalWrite(PIN_CLK, HIGH);
    delayMicroseconds(2);
    digitalWrite(PIN_CLK, LOW);
    delayMicroseconds(2);
  }
  return v;
}

uint8_t readReg(uint8_t reg) {
  digitalWrite(PIN_RST, HIGH);
  delayMicroseconds(4);
  writeByte(0x81 | (reg << 1));
  uint8_t v = readByte();
  digitalWrite(PIN_RST, LOW);
  delayMicroseconds(4);
  return v;
}

void writeReg(uint8_t reg, uint8_t val) {
  digitalWrite(PIN_RST, HIGH);
  delayMicroseconds(4);
  writeByte(0x80 | (reg << 1));
  writeByte(val);
  digitalWrite(PIN_RST, LOW);
  delayMicroseconds(4);
}

// Jour de la semaine (1 = lundi ... 7 = dimanche)
uint8_t dayOfWeek(uint16_t y, uint8_t m, uint8_t d) {
  static const uint8_t t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (m < 3) y--;
  uint8_t w = (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7; // 0 = dimanche
  return w == 0 ? 7 : w;
}

void setToCompileTime() {
  // __TIME__ = "hh:mm:ss"   __DATE__ = "Mmm dd yyyy"
  uint8_t h  = (__TIME__[0] - '0') * 10 + (__TIME__[1] - '0');
  uint8_t mi = (__TIME__[3] - '0') * 10 + (__TIME__[4] - '0');
  uint8_t s  = (__TIME__[6] - '0') * 10 + (__TIME__[7] - '0');

  char mon[4] = {__DATE__[0], __DATE__[1], __DATE__[2], 0};
  const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
  uint8_t month = (strstr(months, mon) - months) / 3 + 1;
  uint8_t day   = (__DATE__[4] == ' ' ? 0 : (__DATE__[4] - '0') * 10) + (__DATE__[5] - '0');
  uint16_t year = atoi(__DATE__ + 7);

  writeReg(REG_WP, 0x00);                    // désactive la protection en écriture
  writeReg(REG_SEC,   dec2bcd(s));           // bit 7 = 0 -> horloge démarrée
  writeReg(REG_MIN,   dec2bcd(mi));
  writeReg(REG_HOUR,  dec2bcd(h));           // mode 24 h
  writeReg(REG_DATE,  dec2bcd(day));
  writeReg(REG_MONTH, dec2bcd(month));
  writeReg(REG_DAY,   dec2bcd(dayOfWeek(year, month, day)));
  writeReg(REG_YEAR,  dec2bcd(year % 100));
  writeReg(REG_WP, 0x80);                    // réactive la protection
}

void print2(uint8_t v) {
  if (v < 10) Serial.print('0');
  Serial.print(v);
}

void setup() {
  Serial.begin(9600);
  pinMode(PIN_CLK, OUTPUT);
  pinMode(PIN_RST, OUTPUT);
  digitalWrite(PIN_CLK, LOW);
  digitalWrite(PIN_RST, LOW);

  Serial.println(F("=== Test DS1302 ==="));

  if (readReg(REG_SEC) & 0x80) {             // bit CH = horloge arrêtée
    Serial.println(F("Horloge arretee -> reglage sur l'heure de compilation"));
    setToCompileTime();
  } else {
    Serial.println(F("Horloge deja en marche"));
  }
}

void loop() {
  uint8_t s  = bcd2dec(readReg(REG_SEC) & 0x7F);
  uint8_t mi = bcd2dec(readReg(REG_MIN));
  uint8_t h  = bcd2dec(readReg(REG_HOUR) & 0x3F);
  uint8_t d  = bcd2dec(readReg(REG_DATE));
  uint8_t mo = bcd2dec(readReg(REG_MONTH));
  uint8_t y  = bcd2dec(readReg(REG_YEAR));

  print2(d);  Serial.print('/');
  print2(mo); Serial.print(F("/20"));
  print2(y);  Serial.print(' ');
  print2(h);  Serial.print(':');
  print2(mi); Serial.print(':');
  print2(s);  Serial.println();

  delay(1000);
}
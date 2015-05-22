// Librairie du projet
// Project libraries
#include <SPI.h>
#include <dht11.h>
#include <Timezone.h>
#include <Wire.h>
#include <Time.h>
#include <DS1302RTC.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include "LedControl.h"

// Init Broche Horloge
// Set pins clock:  CE, IO,CLK
DS1302RTC RTC(13, 12, 11);

// Optional connection for RTC module
// Connection obtionnel du module RTC
//#define DS1302_GND_PIN 33
//#define DS1302_VCC_PIN 35

//Definition des boutons
//DEFINE Buttons
#define PIN_SET_MODE_BUTTON 2
#define PIN_ADD_BUTTON 4
#define PIN_SUB_BUTTON 7
#define PIN_LCD_LIGHT 8
#define DEBUG 1

// Definitin du LCD
//DEFINE LCD
#define I2C_ADDR    0x27 // <<----- Ajouter adresse ici. Decouverte avec I2C Scanner / Add your address here.  Find it from I2C Scanner
#define BACKLIGHT_PIN     3
#define En_pin  2
#define Rw_pin  1
#define Rs_pin  0
#define D4_pin  4
#define D5_pin  5
#define D6_pin  6
#define D7_pin  7
LiquidCrystal_I2C	lcd(I2C_ADDR,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin);

// Variables
int setMode = 0; // Mode de changement / Changing mode
int tmpMillis = 0;  // Variable de compte millis / count millis variable
time_t lastTime;  // Enregistrement de l'heure actuel / save actual time

// Sauvegarde etat des boutons
// Save status buttons.
char buttonMode;
char buttonAdd;
char buttonSub;
char buttonLight;

// Tableau des jours de la semaine
// Days of the week array
String jourSem[8] = {"","DIM", "LUN","MAR","MER","JEU","VEN","SAM"};

// Tableau des differents mode
// Array of the modes
String mode[7] = {" ","h","m","J","M","A", "W"};

// Preparation des objets
// Object instencehiate
time_t thisTime;
tmElements_t tm;


// Initialisation des variables de Lecture Temperature/humidite
// Initiate variable for temp/humidity reading
dht11 DHT11;
#define DHT11PIN 3


/*
 Init LedControl Matrix
 pin 5 connecte a DataIn 
 pin 10 connecte a CLK 
 pin 9 connecte a LOAD 
 We have only a single MAX72XX.
*/
/*
 Init LedControl Matrix
 pin 5 is connected to the DataIn 
 pin 10 is connected to the CLK 
 pin 9 is connected to LOAD 
 We have only a single MAX72XX.
*/
LedControl lc=LedControl(5,10,9,1);
// Tableau des nombre binaire de 0 a 9
// Array of binary number from 0 to 9
byte chiffres[10]={B00000000,
                   B00000001,
                   B00000010,
                   B00000011,
                   B00000100,
                   B00000101,
                   B00000110,
                   B00000111,
                   B00001000,
                   B00001001};

// Variable compte temps pour eteindre LCD
// Timer variable for turning off LCD
int timerLcdLight;

void setup()
{
  // Configuration des entrees boutons et sortis LCD
  // Setting up input buttons and output LCD
  pinMode(PIN_SET_MODE_BUTTON,INPUT);
  pinMode(PIN_ADD_BUTTON,INPUT);
  pinMode(PIN_SUB_BUTTON,INPUT);
  pinMode(PIN_LCD_LIGHT,INPUT);
  
  // Activation de resistances puul-up interne
  // Enable the build-in pull-up resistor
  digitalWrite(PIN_SET_MODE_BUTTON,HIGH);
  digitalWrite(PIN_ADD_BUTTON,HIGH);
  digitalWrite(PIN_SUB_BUTTON,HIGH);
  digitalWrite(PIN_LCD_LIGHT,HIGH);
  
  // Activation du LCD
  // Enabling LCD
  lcd.begin (16,2); //  <<----- My LCD was 16x2
  // Active le backlight
  // Switch on the backlight
  lcd.setBacklightPin(BACKLIGHT_PIN,POSITIVE);
  lcd.setBacklight(HIGH);
  lcd.home (); // go home
  lcd.clear();

  // Configuration de la communication serie
  // Setup Serial connection
  Serial.begin(115200);
  
  // Test de l'horloge
  // Test clock
  Serial.println("DS1302RTC Read Test");
  Serial.println("-------------------");
  
  // Activate RTC module
//  digitalWrite(DS1302_GND_PIN, LOW);
//  pinMode(DS1302_GND_PIN, OUTPUT);

//  digitalWrite(DS1302_VCC_PIN, HIGH);
//  pinMode(DS1302_VCC_PIN, OUTPUT);
  
  Serial.println("RTC module activated");
  Serial.println();
  delay(500);
  
  if (RTC.haltRTC()) {
    Serial.println("The DS1302 is stopped.  Please run the SetTime");
    Serial.println("example to initialize the time and begin running.");
    Serial.println();
  }
  if (!RTC.writeEN()) {
    Serial.println("The DS1302 is write protected. This normal.");
    Serial.println();
  }

  lastTime = 0;

  buttonMode = HIGH;
  buttonAdd = HIGH;
  buttonSub = HIGH;
  buttonLight = HIGH;
  timerLcdLight = 120;
  
  analogReference(INTERNAL);
  
  /*
  The MAX72XX is in power-saving mode on startup,
  we have to do a wakeup call
  */
  lc.shutdown(0,false);
  /* Set the brightness to a medium values */
  lc.setIntensity(0,8);
  /* and clear the display */
  lc.clearDisplay(0);
}

void loop()
{
  
  if ((tmpMillis == 1000) && DEBUG){
    tmpMillis = 0;
    
    if(timerLcdLight > 0){
      timerLcdLight--;
    }
    readTimeRTC();
    displayTime();
  }

  // Vérifi les boutons presser
  buttonPress();
  
  // Wait one second before repeating :)
  tmpMillis = tmpMillis + 100;

  if(timerLcdLight == 0){
    lcd.setBacklight(LOW);
    lcd.noDisplay();
  }

  delay (100);
}

// Affichage des infos
void displayTime(){
  displaySerial();
  displayLCD();
  displayMatrix();
}

// Display Serial
void displaySerial(){
  Serial.print("UNIX Time: ");
  Serial.print(thisTime);

  if (! RTC.read(tm)) {
    Serial.print("  Time = ");
    print2digits(tm.Hour);
    Serial.write(':');
    print2digits(tm.Minute);
    Serial.write(':');
    print2digits(tm.Second);
    Serial.print(", Date (D/M/Y) = ");
    Serial.print(tm.Day);
    Serial.write('/');
    Serial.print(tm.Month);
    Serial.write('/');
    Serial.print(tmYearToCalendar(tm.Year));
    Serial.print(", DoW = ");
    Serial.print(tm.Wday);
    Serial.print(", Mode = ");
    Serial.print(setMode);
    Serial.print(", Timer = ");
    Serial.print(timerLcdLight);
    Serial.println();
    
  } else {
    Serial.println("DS1302 read error!  Please check the circuitry.");
    Serial.println();
    delay(9000);
  }
}

// Display LCD
void displayLCD(){
  if (! RTC.read(tm)) {
      // LCD Display
      lcd.setCursor(0,0);
      if (tm.Hour < 10){
        lcd.print("0");
      }
      lcd.print(tm.Hour);
      if ( (tm.Second % 2) == 0) {
        lcd.print(':');
      } else {
        lcd.print(' ');
      }
      if (tm.Minute < 10){
        lcd.print("0");
      }
      lcd.print(tm.Minute);
      if ( (tm.Second % 2) == 0) {
        lcd.print(':');
      } else {
        lcd.print(' ');
      }
      if (tm.Second < 10){
        lcd.print("0");
      }
      lcd.print(tm.Second);

      lcd.setCursor(9,0);
      lcd.print(readTempHumidity());

      lcd.setCursor(0,1);
      if (tm.Day < 10){
        lcd.print("0");
      }
      lcd.print(tm.Day);
      lcd.print('/');
      if (tm.Month < 10){
        lcd.print("0");
      }
      lcd.print(tm.Month);
      lcd.print('/');
      lcd.print(tmYearToCalendar(tm.Year));
      lcd.setCursor(11,1);
      lcd.print(jourSem[tm.Wday]);
      lcd.setCursor(15,1);
      lcd.print(mode[setMode]);
    
    } else {
      Serial.println("DS1302 read error!  Please check the circuitry.");
      Serial.println();
      delay(9000);
    }
}

// Display Matrix
void displayMatrix(){
  int heures, minutes, secondes, jour, mois, annee, dizaines, unites;
  
  heures = tm.Hour;
  minutes = tm.Minute;
  secondes = tm.Second;
  
  jour = tm.Day;
  mois = tm.Month;
  annee = tm.Year;
  
  dizaines = heures % 10;
  unites = heures / 10;
  
  lc.setRow(0,7,chiffres[unites]);  
  lc.setRow(0,6,chiffres[dizaines]);
  lc.setRow(0,5,chiffres[0]);

  dizaines = minutes % 10;
  unites = minutes / 10;

  lc.setRow(0,4,chiffres[unites]);
  lc.setRow(0,3,chiffres[dizaines]);
  lc.setRow(0,2,chiffres[0]);

  dizaines = secondes % 10;
  unites = secondes / 10;

  lc.setRow(0,0,chiffres[dizaines]);
  lc.setRow(0,1,chiffres[unites]);
  
  // Ajout d'une seconde matrix pour la date
  // Code a ajouter

/*
  dizaines = jour % 10;
  unites = jour / 10;
  
  lc.setRow(1,7,chiffres[unites]);  
  lc.setRow(1,6,chiffres[dizaines]);
  lc.setRow(1,5,chiffres[0]);


  dizaines = mois % 10;
  unites = mois / 10;

  lc.setRow(1,4,chiffres[unites]);
  lc.setRow(1,3,chiffres[dizaines]);
  lc.setRow(1,2,chiffres[0]);

  dizaines = annee % 10;
  unites = annee / 10;

  lc.setRow(1,0,chiffres[dizaines]);
  lc.setRow(1,1,chiffres[unites]);
*/
}

// Si ecran LCD off allume et reset timer
void chkBackOffTurnOn(){
  if (timerLcdLight == 0){
    lcd.display();
    lcd.setBacklight(HIGH);
  }
  timerLcdLight=120;
}

// ajout du zero pour affichage sur Serial Port
void print2digits(int number) {
  if (number >= 0 && number < 10)
    Serial.write('0');
  Serial.print(number);
}

// Verifie bouton presser
// Check button mode
// Mode 0 = nothing
// Mode 1 = Change Hours
// Mode 2 = Change Minutes
// Mode 3 = Change day
// Mode 4 = Change months
// Mode 5 = change year  
// Mode 6 = Day of the week
void buttonPress(){
  char tmp;
  char tmpReadBMode;
  char tmpReadBAdd;
  char tmpReadBSub;
  char tmpReadBLight;
  
  tmpReadBLight = digitalRead(PIN_LCD_LIGHT);
  if ((buttonLight == HIGH) && (tmpReadBLight == LOW)){
    //lcd.setBacklight(HIGH);
    //timerLcdLight=120;
    chkBackOffTurnOn();
  }
  buttonLight = tmpReadBLight;

  tmpReadBMode = digitalRead(PIN_SET_MODE_BUTTON);
  if ((buttonMode == HIGH) && (tmpReadBMode == LOW)){
    chkBackOffTurnOn();
    setMode = setMode + 1;
    if (setMode == 7) {
      setMode = 0;
    }
  }
  buttonMode = tmpReadBMode;
  
  switch (setMode){
    case 1: // change hour
      tmpReadBAdd = digitalRead(PIN_ADD_BUTTON);
      int tmpHour;
      tmpHour = tm.Hour;
      if ((buttonAdd == HIGH) && (tmpReadBAdd == LOW)){
        // allume ecrane si fermer.
        chkBackOffTurnOn();
        // calcul changement d'heure
        tmpHour = calculChange(setMode,1,tmpHour);
      }
      buttonAdd = tmpReadBAdd;

      tmpReadBSub = digitalRead(PIN_SUB_BUTTON);
      if ((buttonSub == HIGH) && (tmpReadBSub == LOW)){
        chkBackOffTurnOn();
        // calcul changement d'heure
        tmpHour = calculChange(setMode,2,tmpHour);
      }
      buttonSub = tmpReadBSub;

      if(tm.Hour != tmpHour){
        tm.Hour = tmpHour;
        RTC.write(tm);
      }      
      break;
    case 2: // change minutes
      tmp = digitalRead(PIN_ADD_BUTTON);
      int tmpMinute;
      tmpMinute = tm.Minute;
      if ((buttonAdd == HIGH) && (tmp == LOW)){
        // allume ecrane si fermer.
        chkBackOffTurnOn();
        // calcul changement de Minute
        tmpMinute = calculChange(setMode,1,tmpMinute);
      }
      buttonAdd = tmp;

      tmp = digitalRead(PIN_SUB_BUTTON);
      if ((buttonSub == HIGH) && (tmp == LOW)){
        // allume ecrane si fermer.
        chkBackOffTurnOn();
        // calcul changement de Minute
        tmpMinute = calculChange(setMode,2,tmpMinute);
      }
      buttonSub = tmp;
      if(tm.Minute != tmpMinute){
        tm.Minute = tmpMinute;
        RTC.write(tm);
      }          
      break;
    case 3: // change day
      tmp = digitalRead(PIN_ADD_BUTTON);
      int tmpDay;
      tmpDay = tm.Day;
      if ((buttonAdd == HIGH) && (tmp == LOW)){
        // allume ecrane si fermer.
        chkBackOffTurnOn();
        // calcul changement de Minute
        tmpDay = calculChange(setMode,1,tmpDay);
      }
      buttonAdd = tmp;

      tmp = digitalRead(PIN_SUB_BUTTON);
      if ((buttonSub == HIGH) && (tmp == LOW)){
        // allume ecrane si fermer.
        chkBackOffTurnOn();
        // calcul changement de Minute
        tmpDay = calculChange(setMode,2,tmpDay);
      }
      buttonSub = tmp;

      if(tm.Day != tmpDay){
        tm.Day = tmpDay;
        RTC.write(tm);
      }      
      break;
    case 4: // change months
      tmp = digitalRead(PIN_ADD_BUTTON);
      int tmpMonth;
      tmpMonth = tm.Month;
      if ((buttonAdd == HIGH) && (tmp == LOW)){
        // allume ecrane si fermer.
        chkBackOffTurnOn();
        // calcul changement de Minute
        tmpMonth = calculChange(setMode,1,tmpMonth);
      }
      buttonAdd = tmp;

      tmp = digitalRead(PIN_SUB_BUTTON);
      if ((buttonSub == HIGH) && (tmp == LOW)){
        // allume ecrane si fermer.
        chkBackOffTurnOn();
        // calcul changement de Minute
        tmpMonth = calculChange(setMode,2,tmpMonth);
      }
      buttonSub = tmp;
      
      if(tm.Month != tmpMonth){
        tm.Month = tmpMonth;
        RTC.write(tm);
      }    
      break;
    case 5:  // change year
      tmp = digitalRead(PIN_ADD_BUTTON);
      int tmpYear;
      tmpYear = tm.Year;
      if ((buttonAdd == HIGH) && (tmp == LOW)){
        // allume ecrane si fermer.
        chkBackOffTurnOn();
        // calcul changement de Minute
        tmpYear = calculChange(setMode,1,tmpYear);
      }
      buttonAdd = tmp;

      tmp = digitalRead(PIN_SUB_BUTTON);
      if ((buttonSub == HIGH) && (tmp == LOW)){
        // allume ecrane si fermer.
        chkBackOffTurnOn();
        // calcul changement de Minute
        tmpYear = calculChange(setMode,2,tmpYear);
      }
      buttonSub = tmp;
      
      if(tm.Year != tmpYear){
        tm.Year = tmpYear;
        RTC.write(tm);
      }          
      break;
    case 6:
      tmp = digitalRead(PIN_ADD_BUTTON);
      int tmpWday;
      tmpWday = tm.Wday;
      if ((buttonAdd == HIGH) && (tmp == LOW)){
        // allume ecrane si fermer.
        chkBackOffTurnOn();
        // calcul changement de Minute
        tmpWday = calculChange(setMode,1,tmpWday);
      }
      buttonAdd = tmp;

      tmp = digitalRead(PIN_SUB_BUTTON);
      if ((buttonSub == HIGH) && (tmp == LOW)){
        // allume ecrane si fermer.
        chkBackOffTurnOn();
        // calcul changement de Minute
        tmpWday = calculChange(setMode,2,tmpWday);
      }
      buttonSub = tmp;
      
      if(tm.Wday != tmpWday){
        tm.Wday = tmpWday;
        RTC.write(tm);
      }          
      break;
  }
}

// Calcule chagement Heure/Date
// (Mode,{ 1 - addition, 2 - soustraction}, valeur)
int calculChange(int modeType, int operatorChoice, int value1){
  // Initialisation des valeurs pour différents calculs
  // Matrix[setMode][y] {Max, Min, resetValueMax, resetValueMin}
  int newValue;
  int matrix[7][4] = { {0,0,0,0},
                       {24,-1, 0, 23},
                       {60,-1,0,59},
                       {0,0,0,0},
                       {13,0,1,12},
                       {70,-1,0,69},
                       {8,0,1,7} };
  Serial.println("in function calculChange");
  if (modeType != 3){
    if (operatorChoice == 1) {
      value1++;
      if (value1 == matrix[modeType][0]) {
        newValue = matrix[modeType][2];
        return newValue;
      } else {
        return value1;
      }
    } else if (operatorChoice == 2) {
      value1--;
      if (value1 == matrix[modeType][1]) {
        newValue = matrix[modeType][3];
        return newValue;
      } else {
        return value1;
      }
    }
  }

  if (modeType == 3) {
    if (operatorChoice == 1) {
      value1++;
      if (value1 > numberDayMonth(tm.Month, tm.Year)) {
        newValue = 1;
        return newValue;
      } else {
        return value1;
      }
    } else if (operatorChoice == 2) {
      value1--;
      if (value1 < 1) {
        newValue = numberDayMonth(tm.Month, tm.Year);
        return newValue;
      } else {
        return value1;
      }
    }
  }
}

// Lecture temperature
// prend une moyenne de 60 lectures
String readTempHumidity(){
  String reading;
  
  int chk = DHT11.read(DHT11PIN);
  
  if ( (tm.Minute % 2) == 0) {
    reading = String((float)DHT11.humidity)  + " %";
  } else {
    reading = String((float)DHT11.temperature) + "*C";
  }
  
  return reading;
}

// Lire time sur rtc
void readTimeRTC(){
  thisTime = RTC.get();
  tmElements_t tm;
  breakTime(thisTime, tm);
}

// retourne true si annee bi sinon retourne false
boolean leapYearTest(int Y){
  if ( ((1970+Y)>0) && !((1970+Y)%4) && ( ((1970+Y)%100) || !((1970+Y)%400) ) ){
    return true;
  } else {
    return false;
  }
}

// retourne le nomdre de jour du mois selon l'année.
int numberDayMonth(int numMonth, int Y){
  if ((numMonth == 1) || (numMonth == 3) || (numMonth == 5) || (numMonth == 7) || (numMonth == 8) || (numMonth == 10) || (numMonth == 12)) {
    return 31;
  } else if ((numMonth == 4) || (numMonth == 6) || (numMonth == 9) || (numMonth == 11)) {
    return 30;
  } else if ((numMonth == 2)) {
    if (leapYearTest(Y)){
      return 29;      
    } else {
      return 28;      
    } 
  }  
}

//Celsius to Fahrenheit conversion
double Fahrenheit(double celsius)
{
	return 1.8 * celsius + 32;
}

// fast integer version with rounding
//int Celcius2Fahrenheit(int celcius)
//{
//  return (celsius * 18 + 5)/10 + 32;
//}


//Celsius to Kelvin conversion
double Kelvin(double celsius)
{
	return celsius + 273.15;
}

// dewPoint function NOAA
// reference (1) : http://wahiduddin.net/calc/density_algorithms.htm
// reference (2) : http://www.colorado.edu/geography/weather_station/Geog_site/about.htm
//
double dewPoint(double celsius, double humidity)
{
	// (1) Saturation Vapor Pressure = ESGG(T)
	double RATIO = 373.15 / (273.15 + celsius);
	double RHS = -7.90298 * (RATIO - 1);
	RHS += 5.02808 * log10(RATIO);
	RHS += -1.3816e-7 * (pow(10, (11.344 * (1 - 1/RATIO ))) - 1) ;
	RHS += 8.1328e-3 * (pow(10, (-3.49149 * (RATIO - 1))) - 1) ;
	RHS += log10(1013.246);

        // factor -3 is to adjust units - Vapor Pressure SVP * humidity
	double VP = pow(10, RHS - 3) * humidity;

        // (2) DEWPOINT = F(Vapor Pressure)
	double T = log(VP/0.61078);   // temp var
	return (241.88 * T) / (17.558 - T);
}

// delta max = 0.6544 wrt dewPoint()
// 6.9 x faster than dewPoint()
// reference: http://en.wikipedia.org/wiki/Dew_point
double dewPointFast(double celsius, double humidity)
{
	double a = 17.271;
	double b = 237.7;
	double temp = (a * celsius) / (b + celsius) + log(humidity*0.01);
	double Td = (b * temp) / (a - temp);
	return Td;
}

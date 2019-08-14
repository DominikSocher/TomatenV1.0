#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <HardWire.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <Taster.h>
//initalisierung lcd und Dht Sensor
LiquidCrystal_I2C lcd(0x27,20,4);
DHT dht(9, DHT11);
//initilisierung Taster
Taster S1Display(500); //Taster Display An/Aus
Taster S2Handbetireb(500); //Taster Wechsel Handbetrieb
Taster S3Pumpe(500); //Taster Pumpe An/Aus

//Sensor Definition
#define LDRPin 0
#define ErdPin 1

//Aktoren Definition
#define Pumpe 10
#define StatusLED 5
#define PumpenLED 6
#define LuftLED 7

//Sensor Variabeln
int Lumen = 0;
int Temperatur = 0;
int Feuchtigkeit = 0;
int Erdfeuchte = 0;

//Timer Variabeln
unsigned long ZeitAbfrage = 1000;
unsigned long StartTimer = 0;
unsigned long AbgelaufeneZeit = 0;
int Sekunden = 120;

//Merker
bool MerkerPumpeTaster = false;
bool MerkerHand = false;
bool MerkerDisplay = false;
bool MerkerAutomat = false;
bool MerkerPumpe = false;

//String
String Zeichen = "X";


//Funktion zur Analogen Sensor Auswertung
int LesSensor(int AnalogPin)
{
  int AnalogWert = 0;
   //10x lesen Analogwert
  for(int i=0; i<10; i++)
  {
    AnalogWert += analogRead(AnalogPin); 
  }
  AnalogWert= AnalogWert/10; //Teilen durch 10 um mittel zu kriegen
  
  return AnalogWert;
}

//Zusammenfassung aller Sensoren und schreiben auf lokale Variablen
void Sensoren()
{
  int Tag = 50; //Sollwert Helligkeit
  Lumen = LesSensor(LDRPin);
  Lumen = map(Lumen, 1203, 0, 0, 100);

  if(Lumen > Tag) //Wenn hell les die restlichen Sensoren
  {
  Erdfeuchte = LesSensor(ErdPin);
  Erdfeuchte = map(Erdfeuchte, 1023, 0 , 0 ,100);
  Temperatur = dht.readTemperature();
  Feuchtigkeit = dht.readHumidity();
  }  
}

void Betrieb()
{
  MerkerHand = S2Handbetireb.selbsthaltung();
  if(!MerkerHand)
  {
    MerkerAutomat = false;
    MerkerPumpeTaster = S3Pumpe.selbsthaltung();
    if(!MerkerPumpeTaster)
    {
      digitalWrite(Pumpe, LOW);
    }
    else if(MerkerPumpeTaster)
    {
      digitalWrite(Pumpe, HIGH);
    }
  }
  else if(MerkerHand)
  {
  int Licht = 60; //Sollwert Helligkeit
  int Erde = 40; //Sollwert Feuchtikeit in Erde
  MerkerAutomat = true;

    if(Lumen < Licht)
    {
      digitalWrite(Pumpe, HIGH);
    }
    else if (Erdfeuchte<Erde)
    {
      digitalWrite(Pumpe, LOW);
    }
    else if (Erdfeuchte>=Erde)
    {
      digitalWrite(Pumpe, HIGH);
    }
  }
}


//Schreiben auf den LCD

void Display()
{
  MerkerDisplay = S1Display.selbsthaltung(); //Schreibt Taster auf Merker

  if (!MerkerDisplay) //Display An
  {
  lcd.backlight();
  //Schreiben sämtlicher Sensor Werte auf den Display
  lcd.setCursor(4,0);
  lcd.print("Sensordaten");
  lcd.setCursor(0,1);
  lcd.print("Licht:");
  lcd.print(Lumen);
  lcd.print("% ");
  lcd.print("Erde: ");
  lcd.print(Erdfeuchte);
  lcd.print("%");
  lcd.setCursor(0,2);
  lcd.print("Temp:");
  lcd.print(Temperatur);
  lcd.print("oC ");
  lcd.print(" LuftF: ");
  lcd.print(Feuchtigkeit);
  lcd.print("%");

  lcd.setCursor(0,3);
  lcd.print("Auto [ ]");
  if(MerkerAutomat == true)//Zeichen Pumpe An
  {
    lcd.setCursor(6,3);
    lcd.print(Zeichen);
  }
  else if(MerkerAutomat == false) //Zeichen Pumpe Aus
  {
    lcd.setCursor(16,2);
    lcd.print(" ");
  }
  lcd.setCursor(10,3);
  lcd.print("Hand [ ]");
    if(MerkerHand == false) //Zeichen Handbetrieb An
    {
      lcd.setCursor(16,3);
      lcd.print(Zeichen);
    }
      else if(MerkerHand == true) //Zeichen Handbetrieb An
      {
        lcd.setCursor(16,3);
        lcd.print(" ");
      }
  lcd.setCursor(0,0);
  }

  else if (MerkerDisplay) //Display Aus
  {
    lcd.clear();
    lcd.noBacklight();
  }
}
 
void MeldeLED()
{
  int Luftfeuchte = 80;
  digitalWrite(StatusLED, HIGH); //direktes Einschalten bei start geht aus wenn Spannungslos

  if(Luftfeuchte<=Feuchtigkeit) //Wenn Luftfeuchtigkeit im Haus höher 80% leuchtet rote LED
  {
    digitalWrite(LuftLED,HIGH);
  }
  else
  {
    digitalWrite(LuftLED, LOW);
  }
  MerkerPumpe=digitalRead(Pumpe); //liest Status am PumpenPin und speichert ihn

  if(!MerkerPumpe) //Pumpe an - LED 
  {
    digitalWrite(PumpenLED, HIGH);
  }
  else
  {
    digitalWrite(PumpenLED, LOW);
  }
}






void setup() 
{
  S1Display.SetupTaster(13, INPUT_PULLUP); //Taster Display
  S2Handbetireb.SetupTaster(12, INPUT_PULLUP); //Taster Hand
  S3Pumpe.SetupTaster(11, INPUT_PULLUP); //Taster Pumpe
  pinMode(Pumpe,OUTPUT);           //Pumpen Pin
  pinMode(StatusLED,OUTPUT); //AusgangLED
  pinMode(PumpenLED,OUTPUT); //AusgangLED
  pinMode(LuftLED,OUTPUT); //AusgangLED

  lcd.init(); //Start Display
  dht.begin(); //Start dht Sensor

  StartTimer = millis(); //Timer zählt ab boot
  Sensoren();//Direktes Abfragen der Sensoren beim boot
  


}

void loop() 
{
  AbgelaufeneZeit = millis() - StartTimer;
  Betrieb();
  Display();
  MeldeLED();
  
  if(AbgelaufeneZeit > (ZeitAbfrage*Sekunden)) //Timer lässt Sensoren im Intervall Sekunden laufen.
  {                                            //Sekunden ist global und kann geänadert werden.
  Sensoren();
  StartTimer = millis();
  }
}
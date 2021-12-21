#include "ESP8266_MQTT_Conf.h"  //Includedatei enthält alles für WiFi, Konfiguration und MQTT
#include <Wire.h>  //Notwendig für SHT30
#include "ClosedCube_SHT31D.h" //Bibliothek für SHT30
#include <MQUnifiedsensor.h>  //Bibliothek für Gassensor

//Parameter für den Gassensor
/************************Hardware Related Macros************************************/
#define         Board                   ("ESP8266")
#define         Pin                     (A0)  //Analog input 
/***********************Software Related Macros************************************/
#define         Type                    ("MQ-2") //MQ2
#define         Voltage_Resolution      (3.3)
#define         ADC_Bit_Resolution      (10) // For arduino UNO/MEGA/NANO
#define         RatioMQ2CleanAir        (9.83) //RS / R0 = 9.83 ppm 


//Spezifische Einträge für das Konfigurationsformular
String param = "["
  "{"
  "'name':'thema',"
  "'label':'MQTT Thema Temp.',"
  "'type':"+String(INPUTTEXT)+","
  "'default':'envy'"
  "},"
  "{"
  "'name':'sensor',"
  "'label':'Name des Sensors',"
  "'type':"+String(INPUTTEXT)+","
  "'default':'SHT30'"
  "},"
  "{"
  "'name':'typ',"
  "'label':'Welches Gas?',"
  "'type':"+String(INPUTSELECT)+","
  "'default':'1',"
  "'options':["
  "{'v':0,'l':'Wasserstoff'},"
  "{'v':1,'l':'LPG'},"
  "{'v':2,'l':'Kohlenmonoxyd'},"
  "{'v':3,'l':'Alkohol'},"
  "{'v':4,'l':'Propan'}"
  "]},"
  "{"
  "'name':'fahrenheit',"
  "'label':'Fahrenheit',"
  "'type':"+String(INPUTCHECKBOX)+","
  "'default':'0'"
  "}"
  "]";


//SHT Sensor Instanz
ClosedCube_SHT31D sht;
//MQ2 Sensor Instanz
MQUnifiedsensor MQ2(Board, Voltage_Resolution, ADC_Bit_Resolution, Pin, Type);

//Einstellwerte für verschiedene Gase
const float gase[5][2] {
  {987.99,-2.162},        //Wasserstoff
  {574.25,-2.222},        //LPG
  {36974,-3.109},         //Kohlenmonoxyd
  {3616.1,-2.675},        //Alkohol
  {658.71,-2.168}         //Propan
};

//Daten an den MQTT Broker senden
void publishData() {
  //Daten von SHT30 lesen die hohe Wiederholbarkeit verringert das Rauschen. 
  //Der Parameter 50 gibt den Timeout an
  SHT31D result = sht.readTempAndHumidityPolling(SHT3XD_REPEATABILITY_HIGH, 50);
  //Messwerte in Variable übernehmen
  float humidity = result.rh;
  float temperature = result.t;
  String unit = "°C";
  temperature = temperature - 6; //Korrektur wegen Erwärmung durch Gassensor und Controller.
  //Falls Fahrenheit gewählt wurde, die Temperatur umrechnen
  if (conf.getBool("fahrenheit")) {
    temperature= temperature * 1.8 +32;
    unit="°F";
  }
  //Messwerte vom Gassensor
  MQ2.update(); // Update die Spannung am Analogeingang wird gemessen
  float gas=MQ2.readSensor(); //Die Gaskonzentration in ppm ermitteln
  char buffer[1000]; //Buffer für die JSON Zeichenkette
  //Der temporäre Speicher für das JSON Dokument wird angelegt
  //Wir reservieren 500 Zeichen
  StaticJsonDocument<500> doc;
  //Das JSON Objekt wird mit Daten befüllt
  doc["sensor"]=conf.getValue("sensor"); //Name des Sensors
  doc["t"]=temperature;    //Temperatur
  doc["tu"]=unit;          //Einheit
  doc["h"]=humidity;       //Feuchte
  doc["hu"]="%";
  doc["g"]=gas;            //Gaskonzentration
  doc["gu"]="ppm";
  //Aus dem Objekt wird die JSON Zeichenkette im Buffer gebildet
  uint16_t n = serializeJson(doc, buffer);
  //Kontrollausgabe
  Serial.println(buffer);
  //Der Messwert wird mit dem Thema an den Broker gesendet
  if (!client.publish(conf.getValue("thema"), buffer, n)) {
    Serial.print("Fehler beim Senden an MQTT Server = ");
    Serial.println(client.state());
  }
  Serial.printf("Temperatur = %f %s Feuchtigkeit = %f %% Gaskonzentration = %f ppm\n",temperature,unit.c_str(),humidity,gas);
}


void setup() {
  Serial.begin(74880); //Damit auch Bootmeldungen des ESP8266 angezeigt werden
  //I2C Bus starten
  Wire.begin();
  //Start des SHT30
  sht.begin(0x44);
  //Seriennummer des SHT30 anzeigen
  Serial.print("Serial #");
  Serial.println(sht.readSerialNumber());
  //Die Setupfunktion in der Include Datei aufrufen
  //WLAN und Konfiguration werden vorbereitet
  //Verbindung zum MQTT Broker wird hergestellt
  //Der Parameter param enthält zusätzliche Formulareinträge
  //für das Konfigurationsformular, true bedeutet dass wir einen 
  //Sensor haben
  ESP_MQTT_setup(param,true);
  //Callbackfunktion zum Senden der Daten registrieren
  onPublish = publishData;
  //Das Regressionsmodell für die Gaskonzentration wird gewählt
  MQ2.setRegressionMethod(1); //_PPM =  a*ratio^b
  int gas = conf.getInt("typ");
  //Die Parameter für das in der Konfiguration gewählte
  //Gas werden gesetzt
  MQ2.setA(gase[gas][0]); MQ2.setB(gase[gas][1]); 
  //Gas Sensor Bibliothek initialisieren
  MQ2.init(); 
  //Gassensor kalibrieren
  Serial.print("Calibrating please wait.");
  float calcR0 = 0;
  for(int i = 1; i<=10; i ++)
  {
    MQ2.update(); // Update data, the arduino will be read the voltage on the analog pin
    calcR0 += MQ2.calibrate(RatioMQ2CleanAir);
    Serial.print(".");
  }
  MQ2.setR0(calcR0/10);
  Serial.println("  done!.");
  
  if(isinf(calcR0)) {Serial.println("Warning: Conection issue founded, R0 is infite (Open circuit detected) please check your wiring and supply"); while(1);}
  if(calcR0 == 0){Serial.println("Warning: Conection issue founded, R0 is zero (Analog pin with short circuit to ground) please check your wiring and supply"); while(1);}
}

void loop() {
  //Schleifenfunktion der eingebundenen Hilfsdatei aufrufen
  //Sorgt dafür, dass die Messwerte in regelmäßigen 
  //Zeitabständen ermittelt und an den Broker gesendet werden
  ESP_MQTT_loop();
}

/***************************************************************************
Wasserzähler auslesen mit Magnetometer HMC5883L
PFu: 19.12.2024
***************************************************************************/

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>
// WLAN
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
// #include <ESPAsyncTCP.h>
// Vorbelegung WLAN Arbeitsplatz
const char* ssid1 = "#deine SSID";
const char* password1 = "deinPasswort";
const char* ssid2 = "alternative SSID";
const char* password2 = "anderesPasswort";
String newHostname = "D1Wasserzaehler";
#include <InfluxDbClient.h>
#define INFLUXDB_URL "http://192.168.22.148:8086"
#define INFLUXDB_DB_NAME "klima"
#define INFLUXDB_USER "esp8266"
#define INFLUXDB_PASSWORD "dadidumm"

// InfluxDB client instance for InfluxDB 1
InfluxDBClient influx_client(INFLUXDB_URL, INFLUXDB_DB_NAME);
// measurement in Influx
Point sensor("CH_Wasserverbrauch");
// Initialize the Wifi client library
WiFiClient wifi_client;
ESP8266WiFiMulti wifiMulti;
boolean connectioWasAlive = true;

#define publish_delay 10000 // 10000=10s, 600000 = 10 Minutes
unsigned long last_publish = 0;
unsigned long now = 0;
unsigned int crossings = 0;
unsigned int zaehlerstand = 0;

int new_val = 0;
int old_val = 0;
int y = 0;
int min_y = -10;
int max_y =  10;
boolean changed = false;
boolean first = false;
Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12021);


/*******************************************************************
* Messdaten in die INfluxDB schreiben
*********************************************************************/
void parameter2influx() {
  // Check server connection
  if (influx_client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(influx_client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(influx_client.getLastErrorMessage());
  }
  // Store measured value into point
  sensor.clearFields();
  sensor.addField("Liter", crossings*5);
  sensor.addField("zaehlerstand", zaehlerstand);
  sensor.addField("B_Signal", WiFi.RSSI());
  sensor.addField("Mac", WiFi.macAddress());
  sensor.addField("Hostname", WiFi.getHostname());
  // Print what are we exactly writing
  Serial.print("Writing: ");
  Serial.println(influx_client.pointToLineProtocol(sensor));
  // Write point
  if (!influx_client.writePoint(sensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(influx_client.getLastErrorMessage());
  }
};

/*********************************************************************
* abhängig vom Standort das passende WLAN nehmen
*********************************************************************/
void monitorWiFi(){
  if (wifiMulti.run() != WL_CONNECTED){
    if (connectioWasAlive == true){
      connectioWasAlive = false;
      Serial.print("Looking for WiFi ");
    }
    Serial.print(".");
    delay(500);
  }
  else if (connectioWasAlive == false){
    connectioWasAlive = true;
    Serial.printf(" connected to %s\n", WiFi.SSID().c_str());
  }
}

/*******************************************************************
* setup
*********************************************************************/
void setup(void) {
  // start serial port
  Serial.begin(9600);
  while(!Serial);   // waits for serial port to connect
  delay(200);
  Serial.println();
  // WiFi initialisieren
  //Serial.print("Connecting to ");
  //Serial.println(ssid);  
  WiFi.hostname(newHostname.c_str());
  WiFi.begin(ssid2, password2);
  //wifiMulti.addAP(ssid1, password1);
  //wifiMulti.addAP(ssid2, password2);
  int t=0;
  while (WiFi.status() != WL_CONNECTED) {
      ++t;
      delay(500);
      Serial.print(".");
      if (t>50){ESP.restart();}
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  // Initialise the sensor
  if(!mag.begin())
  {
    /* There was a problem detecting the HMC5883 ... check your connections */
    Serial.println("Ooops, no HMC5883 detected ... Check your wiring!");
    while(1);
  }
  // Set InfluxDB 1 authentication params
  influx_client.setConnectionParamsV1(INFLUXDB_URL, INFLUXDB_DB_NAME, INFLUXDB_USER, INFLUXDB_PASSWORD);
  // Add constant tags - only once
  sensor.addTag("device", "CH_Wemos_D1_mini_R3");
}
/*******************************************************************
* loop
*********************************************************************/
void loop(void) {
  monitorWiFi();
  now = millis();
  /* Get a new sensor event */ 
  sensors_event_t event; 
  mag.getEvent(&event);
  y=event.magnetic.y;
  // bei den ersten paar Umdrehungen Min Max Grenzen ermitteln
  if (y <= min_y){
    min_y = y;
  } 
  if (y >= max_y){
    max_y = y;
  }
  //Serial.print(y);Serial.print(" min:");Serial.print(min_y);Serial.print(" -->max ");Serial.println(max_y);  
  old_val = new_val;
  new_val = map(y, min_y+5, max_y-5, 0, 180);
  //Serial.print(old_val);Serial.print(" <--> ");Serial.println(new_val);
  changed = (old_val < 0 && new_val > 0) || (old_val > 0 && new_val < 0);
  if(changed) {
    if (new_val <= 0) {
      crossings += 1; // Vorbeiflug am Sensor = 10 Liter, 1/2 Umdrehung 5 Liter
      zaehlerstand = zaehlerstand + 5;
      Serial.print("crossing:");Serial.println(crossings);
      delay(50);
    }
  }

  if(crossings > 0 && (now - last_publish) >= publish_delay) {
    Serial.print("Abgabe=");Serial.print(crossings*5);Serial.println(" Liter");
    // wegschreiben an InfluxDB
    parameter2influx();
    crossings = 0;
    last_publish = now;
  }
  delay(50);
}

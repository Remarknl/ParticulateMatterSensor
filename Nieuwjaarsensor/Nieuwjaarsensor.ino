#include <Ticker.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ESP8266HTTPClient.h>

// sensor
int pin = D2;
unsigned long duration;
unsigned long starttime;
unsigned long sampletime_ms = 20000;
unsigned long lowpulseoccupancy = 0;
float sumRatio = 0;
int avgValCounter = 0;
int numAvgValues = 9;

struct InfluxDB {
  String username;
  String password;
  String host;
  String dbName;
  int port;
};

InfluxDB ordinaDB = {"username", "password", "host", "db name", 8086};
Ticker ticker;

void setup() {
  Serial.begin(9600);
  pinMode(pin,INPUT);
  pinMode(BUILTIN_LED, OUTPUT);
  
  ticker.attach(0.7, tick);
  
  Serial.print("ChipID = ");
  Serial.println(ESP.getChipId());

  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);
  if (!wifiManager.autoConnect("Ordina_Fijnstofsensor")) {
    Serial.println("Verbinden mislukt");
    ESP.reset();
    delay(1000);
  }
  ticker.detach();
  digitalWrite(BUILTIN_LED, true);
  starttime = millis();
}

void loop() {
  sendToDatabase(ordinaDB, "ordina", String(ESP.getChipId()), averageReadings());
}

float sensorRead() {
  while ((millis()-starttime) < sampletime_ms) {
    duration = pulseIn(pin, LOW);
    lowpulseoccupancy = lowpulseoccupancy+duration;
  }
  float ratio = lowpulseoccupancy/(sampletime_ms*10.0);  // Integer 0-100%
  Serial.print("ratio = ");
  Serial.println(ratio);
  lowpulseoccupancy = 0;
  starttime = millis();
  return ratio;
}

void sendToDatabase(InfluxDB db, String table, String id, float value) {
  Serial.println("Data naar database verzenden...");
  HTTPClient http;
  http.begin(String("http://" + db.username + ":" + db.password + "@" + db.host + ":" + db.port + "/write?db=" + db.dbName));
  http.addHeader("Content-Type", "application/form-data");
  int httpCode = http.POST(String(table + ",id=" + id + " meetwaarde=" + value));
  http.writeToStream(&Serial);
  Serial.print("Http response code = ");
  Serial.println(httpCode);
  if(httpCode == 204) blinkTimes(1); else blinkTimes(3);
  http.end();
}

float averageReadings() {
  while (avgValCounter < numAvgValues){
    sumRatio += sensorRead();
    avgValCounter++;
  }
  float average = sumRatio / ((float) numAvgValues);
  Serial.print("average = ");
  Serial.println(average);
  avgValCounter = 0;
  sumRatio = 0;
  return average;
}

void blinkTimes(int times) {
  for(int i = 0; i < times * 2; i++) {
    tick();
    delay(700);
  }
}

void tick()
{
  //toggle state
  int state = digitalRead(BUILTIN_LED);
  digitalWrite(BUILTIN_LED, !state);
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  ticker.attach(0.1, tick);
}

#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

// lock, target_lock defs 
#define LOCK_OPEN true
#define LOCK_CLOSED false

// status defs
#define OK_STATUS true
#define ERROR_STATUS false

// proximity defs
#define PIR_NEAR true
#define PIR_FAR false

// proximity defs
#define LU_LIGHT true
#define LU_DARK false

/* Sensors Pins */
#define LUMINOSITY_PIN  33
#define PROXIMITY_PIN   34
#define TEMPERATURE_PIN 35
/* Sensors Pins */

/* Actuator Pins */
#define RED_PIN     2
#define GREEN_PIN   0
#define LOCK_PIN    15
#define BUZZER_PIN  4
/* Actuator Pins */

/* Sensors */
class Samples {
public:
  Samples(int MAX_SAMPLES = 100, float TEMPERATURE_CONSTANT = (5.0 * 1000.0) / (1023.0 * 3.0 * 100.0), int LUMINOSITY_THRESHOLD = 6)
  : MAX_SAMPLES(MAX_SAMPLES), TEMPERATURE_CONSTANT(TEMPERATURE_CONSTANT), LUMINOSITY_THRESHOLD(LUMINOSITY_THRESHOLD) { 
    clear();
  }
  
  void clear() {
    temperature_acc = 0;
    luminosity  = LU_DARK;
    proximity   = PIR_FAR;
    num_samples = 0;
  }
  
  void sample() {
    if (num_samples < MAX_SAMPLES) {
      ++num_samples;
      temperature_acc += analogRead(TEMPERATURE_PIN) * TEMPERATURE_CONSTANT;
  
      if (luminosity == LU_DARK) {
        if (analogRead(TEMPERATURE_PIN) <= LUMINOSITY_THRESHOLD) {
          luminosity = LU_LIGHT;
        }
      }
  
      if (proximity == PIR_FAR) {
        if (digitalRead(PROXIMITY_PIN))
          proximity = PIR_NEAR;
      }
    } else {
      clear();    
    }
  }

  void print() {
    Serial.print("Temperature: ");
    Serial.println(temperature());
    Serial.print("Proximity: ");
    Serial.println(proximity  == PIR_FAR ? "far"   : "near");
    Serial.print("Luminosity: ");
    Serial.println(luminosity == LU_DARK ? "dark"  : "light");   
  }
  
  const float TEMPERATURE_CONSTANT;
  const int LUMINOSITY_THRESHOLD;

  bool luminosity;
  bool proximity; 
  
  float temperature() {
    return temperature_acc / float(num_samples);
  }
  
private:
  float temperature_acc;
  int num_samples;
  const int MAX_SAMPLES;
};
/* Sensors */

/* Actuators */
bool lock_state = LOCK_CLOSED;
/* Actuators */

/* WiFi Global Variables */
const char* ssid = "CINGUESTS";
const char* password = "acessocin";

WebServer server(80);
/* WiFi Global Variables */

/* Sensors Global Variables */
Samples samples;

void handleGet() {
  Serial.println("GET /");

  const size_t capacity = JSON_OBJECT_SIZE(4) + 140;
  
  DynamicJsonDocument doc(capacity);
//  doc["temperature"] = samples.temperature();
//  doc["proximity"]   = (samples.proximity == PIR_NEAR) ? "near" : "far";
//  doc["luminosity"]  = (samples.luminosity == LU_LIGHT) ? "light" : "dark";
//  doc["lock_state"]  = (lock_state == LOCK_CLOSED) ? "closed" : "open";  

  doc["temperature"] = random(180, 240) / 10.0;
  doc["proximity"]   = random(1, 0) ? "near" : "far";
  doc["luminosity"]  = random(1,0) ? "light" : "dark";
  doc["lock_state"]  = (lock_state == LOCK_CLOSED) ? "closed" : "open";  
  
  Serial.println("My Response: ");
  serializeJsonPretty(doc, Serial);
  Serial.println();
    
  String content;
  serializeJsonPretty(doc, content);

  server.send(200, "application/json", content);  
}

void handlePost() {
  Serial.println("POST /");
  
  const size_t capacity = JSON_OBJECT_SIZE(1) + 60;
  DynamicJsonDocument doc(capacity);
  DeserializationError err = deserializeJson(doc, server.arg("plain")).code();

  if (err) {
    String response = "Deserialization error " + String(err.c_str()) + "\nMessage: ";
    response += server.arg("plain");

    Serial.println(response);
    server.send(400, "text/plain", response);
  } else {
    server.send(200);
  }
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += server.method();
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup() {
  Serial.begin(115200);

  /* Pin Modes */

  /* Pin Modes */
  
  /* WiFi */
  Serial.print("Connecting to: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  
  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }
    
  Serial.println();
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();

  server.on("/", HTTP_GET, handleGet);
  server.on("/", HTTP_POST, handlePost);
  server.onNotFound(handleNotFound);
  server.begin();

  Serial.println("HTTP server started");
  /* WiFi */
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    server.handleClient();
  } else {
    Serial.println("Wifi not connected");
  }

  samples.sample();

  samples.print();
  Serial.print("Lock State: ");
  Serial.println(lock_state == LOCK_OPEN ? "open" : "closed");
  
  delay(50);
}

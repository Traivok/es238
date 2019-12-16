#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <string.h>
#include <EasyBuzzer.h>

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
#define TEMPERATURE_PIN 32
#define LUMINOSITY_PIN  33
#define PROXIMITY_PIN   34
/* Sensors Pins */

/* Actuator Pins */
#define RED_PIN     4
#define GREEN_PIN   0
#define LOCK_PIN    13
#define BUZZER_PIN  12
/* Actuator Pins */

/* Sensors */
class Samples {
public:
  Samples(int MAX_SAMPLES = 100, float TEMPERATURE_CONSTANT = (4.0 * 1000.0) / (4095.0 * 3.0 * 10.0), int LUMINOSITY_THRESHOLD = 1100, int PROXIMITY_TRESHOLD = 3000, int FIRE_THRESHOLD = 30)
  : MAX_SAMPLES(MAX_SAMPLES), TEMPERATURE_CONSTANT(TEMPERATURE_CONSTANT), LUMINOSITY_THRESHOLD(LUMINOSITY_THRESHOLD), PROXIMITY_TRESHOLD(PROXIMITY_TRESHOLD), FIRE_THRESHOLD(FIRE_THRESHOLD) { 
    clear();
  }
  
  void clear() {
    temperature_acc = 0;
    luminosity  = LU_DARK;
    proximity   = PIR_FAR;
    num_samples = 0;
  }
  
  void sample() {
    if (num_samples >= MAX_SAMPLES) {
      clear();
    }

    ++num_samples;
    temperature_acc += analogRead(TEMPERATURE_PIN) * TEMPERATURE_CONSTANT;

    if (luminosity == LU_DARK) {
      if (analogRead(LUMINOSITY_PIN) <= LUMINOSITY_THRESHOLD) {
        luminosity = LU_LIGHT;
      }
    }

    if (proximity == PIR_FAR) {
      if (analogRead(PROXIMITY_PIN) >= PROXIMITY_TRESHOLD)
        proximity = PIR_NEAR;
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
  const int PROXIMITY_TRESHOLD;
  const int FIRE_THRESHOLD;
  

  bool luminosity;
  bool proximity; 
  
  float temperature() {
    return temperature_acc / float(num_samples);
  }

  bool isItHot(){
    if (temperature() >= FIRE_THRESHOLD){
      return true;
    }
    else{
      return false;
    }
  }

  bool isItClose(){
    return proximity;
  }

  bool isItBright(){
    return luminosity;
  }
  
  
private:
  float temperature_acc;
  int num_samples;
  const int MAX_SAMPLES;
};
/* Sensors */

char lock_target[10] = "";

/* Actuators */
bool lock_state = LOCK_CLOSED;
/* Actuators */

/* WiFi Global Variables */
const char* ssid = "Lenovo K6";
const char* password = "perdiojogo";

WebServer server(80);
/* WiFi Global Variables */

/* Sensors Global Variables */
Samples samples;

void handleGet() {
  Serial.println("GET /");

  const size_t capacity = JSON_OBJECT_SIZE(4) + 140;
  
  DynamicJsonDocument doc(capacity);
  doc["temperature"] = samples.temperature();
  doc["proximity"]   = (samples.proximity == PIR_NEAR) ? "near" : "far";
  doc["luminosity"]  = (samples.luminosity == LU_LIGHT) ? "light" : "dark";
  doc["lock_state"]  = (lock_state == LOCK_CLOSED) ? "closed" : "open";  
  
  Serial.println("My Response: ");
  serializeJsonPretty(doc, Serial);
  Serial.println();
    
  String content;
  serializeJsonPretty(doc, content);

  server.send(200, "application/json", content);  
}

void handleOptions() {
  Serial.println("OPTIONS /");

  server.sendHeader("Access-Control-Allow-Methods", "*");
  server.sendHeader("Access-Control-Allow-Headers", "*");

  server.send(200);
}

void handlePost() {
  Serial.println("POST /");

  const size_t capacity = JSON_OBJECT_SIZE(1) + 60;
  DynamicJsonDocument doc(capacity);

  String postBody = server.arg("plain");

  DeserializationError err = deserializeJson(doc, postBody).code();

  if (err) {
    String response = "Deserialization error " + String(err.c_str()) + "\nMessage: ";
    response += server.arg("plain");

    Serial.println(response);
    server.send(400, "text/plain", response);
  } else {
    Serial.println("Post information: ");
    serializeJsonPretty(doc, Serial);
    Serial.println();

    server.send(200);

    strcpy(lock_target, doc["lock_target"]);
  }
}

void handleNotFound() {
  if (server.method() == HTTP_OPTIONS) {
      Serial.println("OPTIONS /");
      server.sendHeader("Access-Control-Max-Age", "10000");
      server.sendHeader("Access-Control-Allow-Methods", "PUT,POST,GET,OPTIONS");
      server.sendHeader("Access-Control-Allow-Headers", "*");
      server.send(204);
    } else {
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
}

void setup() {
  Serial.begin(115200);

  /* Pin Modes */
  pinMode(LUMINOSITY_PIN, INPUT);
  pinMode(PROXIMITY_PIN, INPUT);
  pinMode(TEMPERATURE_PIN, INPUT);

  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(LOCK_PIN, OUTPUT);
  EasyBuzzer.setPin(BUZZER_PIN);
  /* Pin Modes */
  
  /* WiFi */
  Serial.println();
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
  server.enableCORS(true);
  server.begin();

  Serial.println("HTTP server started");
  /* WiFi */

  delay(5000);
}

int ignore_buzzer_counter = 0;
int lock_delay = 0;
bool internal_lock_target;

void loop() {
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    server.handleClient();
  } else {
    Serial.println("Wifi not connected");
  }

  samples.sample();
  samples.print();

  if (samples.isItClose() || samples.isItBright()) {
    if (ignore_buzzer_counter-- == 0) {
      ignore_buzzer_counter = 50;
      Serial.println("Proximity or Luminosity threshold reached. Beeping...");
      EasyBuzzer.beep(220, 20, []() { Serial.println("Beep Stopped"); });
      }
  }
  
  EasyBuzzer.update();
  
  if (samples.isItHot()) {
    Serial.println("Critical Temperature!");
    digitalWrite(RED_PIN, HIGH); //will turn on red LED
    digitalWrite(GREEN_PIN, LOW); //will turn off green LED
  } else {
    digitalWrite(RED_PIN, LOW);
    digitalWrite(GREEN_PIN, HIGH);
  }

  if (lock_delay <= 0) {
    bool aux_lock_target;
    if (String(lock_target).equals("open"))
      aux_lock_target = LOCK_OPEN;
    else
      aux_lock_target = LOCK_CLOSED;
    
    if (aux_lock_target != lock_state) {
      lock_delay = 100;
      internal_lock_target = aux_lock_target;
      Serial.println("Turning motor: ON");
      digitalWrite(LOCK_PIN, HIGH);
    }
  } else {
    --lock_delay;
    if (lock_delay <= 0) {
      lock_state = internal_lock_target;
      Serial.println("Turning motor: OFF");
      digitalWrite(LOCK_PIN, LOW);
    }
  }
  
  Serial.print("Lock State: ");
  Serial.println(lock_state == LOCK_OPEN ? "open" : "closed");
  
  Serial.println();
  delay(50);
}

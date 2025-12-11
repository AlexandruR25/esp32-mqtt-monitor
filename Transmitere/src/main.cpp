#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

// ================= CONFIGURARE DEEP SLEEP =================
#define TIME_TO_SLEEP  6        // 600 secunde = 10 minute
#define uS_TO_S_FACTOR 1000000ULL // Factor conversie

// ================= CONFIGURARE HARDWARE =================
#define LED_PIN 8      // LED Status (C3 SuperMini)
#define DHTPIN  4      // Pin Date Senzor
#define DHTTYPE DHT11
#define PUMP_PIN 2     // Pin Pompa

DHT dht(DHTPIN, DHTTYPE);

// ================= CONFIGURARE RETEA =================
const char* ssid        = "HOME";        
const char* password    = "nustiucum25"; 

const char* mqtt_server = "45529ea6f341410aa5a4fd3d6d0509b8.s1.eu.hivemq.cloud";
const int   mqtt_port   = 8883;
const char* mqtt_user   = "esp_1";
const char* mqtt_pass   = "Oananebunamea12!";
const char* pub_topic   = "esp/device1/data";

WiFiClientSecure net;
PubSubClient client(net);

// ---------------- 1. FUNCTIA DE SOMN (Trebuie sa fie prima!) -----------------
void goToSleep() {
  Serial.println("Noapte buna! Ne vedem in 10 minute.");
  Serial.flush(); // Asteapta sa se termine de trimis textul

  // Stingem tot pentru siguranta
  digitalWrite(LED_PIN, HIGH); 
  digitalWrite(PUMP_PIN, LOW);
  
  // Oprim WiFi
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  // Setam timer-ul
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  
  // START SOMN
  esp_deep_sleep_start();
}

// ---------------- 2. SETUP WIFI RAPID -----------------
void connectToWiFi() {
  Serial.print("Conectare WiFi...");
  WiFi.mode(WIFI_STA);
  
  // Putere redusa (11dBm)
  WiFi.setTxPower(WIFI_POWER_11dBm); 
  
  WiFi.begin(ssid, password);

  int attempts = 0;
  // Incercam maxim 20 de puncte (aprox 10 secunde)
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" OK!");
    digitalWrite(LED_PIN, LOW); // Aprindem LED scurt
  } else {
    Serial.println(" ESEC! Nu am net. Ma culc la loc.");
    // Acum compilatorul stie cine e goToSleep pentru ca e definita mai sus
    goToSleep();
  }
}

// ---------------- 3. SETUP (Ruleaza la trezire) -----------------
void setup() {
  Serial.begin(115200);
  delay(1000); 

  // Configurare Pini
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // Pornim stins
  
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW); 

  // Initializare Senzor
  dht.begin();
  delay(2000); // Timp incalzire senzor

  // Citire Date
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (isnan(t) || isnan(h)) {
    Serial.println("Eroare citire DHT!");
    t = 0.0;
    h = 0.0;
  } else {
    Serial.print("Temp: "); Serial.print(t);
    Serial.print(" Hum: "); Serial.println(h);
  }

  // Conectare Net
  connectToWiFi();

  // Trimite MQTT
  net.setInsecure();
  client.setServer(mqtt_server, mqtt_port);

  Serial.print("Conectare MQTT...");
  String clientId = "ESP32-Sleep-" + String(random(0xffff), HEX);

  if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
    Serial.println(" OK!");
    
    String msg = "{\"temp\":" + String(t, 1) + ",\"humidity\":" + String(h, 0) + "}";
    client.publish(pub_topic, msg.c_str());
    Serial.println("Date trimise: " + msg);
    
    delay(500); 
    client.disconnect();
  } else {
    Serial.print(" Esec MQTT rc=");
    Serial.println(client.state());
  }

  // INTRARE IN SOMN
  goToSleep();
}

// ---------------- 4. LOOP (Neutilizat) -----------------
void loop() {
  // Aici nu ajunge niciodata
}

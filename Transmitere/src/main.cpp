#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <time.h>
#include <DHT.h> // Include biblioteca DHT

// --- Configurații Senzor ---
#define DHTPIN 5        // Pinul ESP32 la care este conectat senzorul DHT
#define DHTTYPE DHT11   // Poate fi DHT11, DHT22 sau AM2302
DHT dht(DHTPIN, DHTTYPE);

// --- Configurații Rețea și MQTT ---
const char* ssid = "RDC";
const char* wifi_password = "nustiucum25";

const char* mqtt_server = "45529ea6f341410aa5a4fd3d6d0509b8.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_user = "esp_1";
const char* mqtt_pass = "Oananebunamea12!";
const char* publish_topic = "esp/device1/data";

WiFiClientSecure net;
PubSubClient client(net);

// --- Funcții Rețea și Timp (Preluate din codul anterior) ---
void setupTime() {
    Serial.println("Setare timp NTP...");
    configTime(0, 0, "pool.ntp.org", "time.google.com");
    time_t now = time(nullptr);
    while (now < 1600000000) { delay(500); now = time(nullptr); Serial.print("."); }
    Serial.println();
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        Serial.print("Timp NTP setat: ");
        Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    }
}

void setup_wifi() {
    delay(10);
    Serial.print("Conectare la "); Serial.println(ssid);
    WiFi.begin(ssid, wifi_password);
    int max_attempts = 20;
    while (WiFi.status() != WL_CONNECTED && max_attempts > 0) {
        delay(500); Serial.print("."); max_attempts--;
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Conectat la Wi-Fi!");
        Serial.print("Adresa IP: "); Serial.println(WiFi.localIP());
    } else {
        Serial.println("Eroare de conectare la Wi-Fi!");
    }
}

void reconnect() {
    while (!client.connected()) {
        Serial.print("Încercare conexiune MQTT...");
        String clientId = "esp32_publisher_" + String((uint32_t)ESP.getEfuseMac());
        if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
            Serial.println("Conectat la MQTT!");
        } else {
            Serial.print("Eșec, codul de stare: "); Serial.print(client.state());
            Serial.println(". Încerc din nou în 2 secunde...");
            delay(2000);
        }
    }
}

// --- Funcții Setup și Loop ---

void setup() {
    Serial.begin(115200);
    Serial.println("\n--- PORNIT ---");

    setup_wifi();

    if (WiFi.status() == WL_CONNECTED) {
        setupTime();
        net.setInsecure();
        client.setServer(mqtt_server, mqtt_port);
        // Inițializează senzorul DHT
        dht.begin();
        Serial.println("Senzor DHT inițializat.");
    } else {
        Serial.println("Nu se poate continua fără conexiune Wi-Fi.");
    }
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        delay(5000);
        setup_wifi();
        return;
    }

    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    static unsigned long last = 0;
    // Citirea datelor la fiecare 5 secunde
    if (millis() - last > 1000) {
        last = millis();

        // 1. Citirea datelor de la senzorul DHT
        float h = dht.readHumidity();
        float t = dht.readTemperature();

        // 2. Verifică dacă citirea a eșuat
        if (isnan(h) || isnan(t)) {
            Serial.println("Eroare la citirea de la senzorul DHT!");
            return; // Nu trimite date invalide
        }

        // 3. Formatează mesajul JSON
        String msg = "{\"temp\":" + String(t, 2) + ",\"humidity\":" + String(h, 2) + "}";

        // 4. Publică mesajul
        if (client.publish(publish_topic, msg.c_str())) {
            Serial.print("Publicat cu succes pe [");
            Serial.print(publish_topic);
            Serial.print("]: ");
            Serial.println(msg);
        } else {
            Serial.println("Eroare la publicarea mesajului MQTT.");
        }
    }
}
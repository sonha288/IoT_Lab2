#include <WiFi.h>
#include <Arduino_MQTT_Client.h>
#include <ThingsBoard.h>
#include "DHT20.h"
#include <Wire.h>

// WiFi & MQTT
#define WIFI_SSID "hasonnn"
#define WIFI_PASSWORD "28082004"
#define TOKEN "knvzbj9qjf96dj9wwagm"
#define THINGSBOARD_SERVER "app.coreiot.io"
#define THINGSBOARD_PORT 1883U

// Cấu hình thời gian
#define INTERVAL 5000U
unsigned long lastCheckTime = 0;

WiFiClient wifiClient;
Arduino_MQTT_Client mqttClient(wifiClient);
ThingsBoard tb(mqttClient);
DHT20 dht20;

bool wasWiFiConnected = false;  // Lưu trạng thái WiFi trước đó
bool wasMQTTConnected = false;  // Lưu trạng thái MQTT trước đó

void connectWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    if (wasWiFiConnected) {
      Serial.println("⚠️ Mất kết nối WiFi. Đang thử lại...");
      wasWiFiConnected = false;
    }

    Serial.println("📡 Đang kết nối WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  } else if (!wasWiFiConnected) {
    Serial.println("✅ WiFi đã kết nối!");
    wasWiFiConnected = true;
  }
}

void connectMQTT() {
  if (!tb.connected()) {
    if (wasMQTTConnected) {
      Serial.println("⚠️ Mất kết nối ThingsBoard. Đang thử lại...");
      wasMQTTConnected = false;
    }

    Serial.println("🔌 Đang kết nối ThingsBoard...");
    if (tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT)) {
      Serial.println("✅ MQTT đã kết nối ThingsBoard!");
      wasMQTTConnected = true;
    } else {
      Serial.println("❌ Kết nối ThingsBoard thất bại!");
    }
  }
}

void sendSensorData() {
  dht20.read();
  float temp = dht20.getTemperature();
  float hum = dht20.getHumidity();

  if (!isnan(temp) && !isnan(hum)) {
    Serial.printf("🌡 Nhiệt độ: %.2f°C, 💧 Độ ẩm: %.2f%%\n", temp, hum);
    tb.sendTelemetryData("temperature", temp);
    tb.sendTelemetryData("humidity", hum);
  } else {
    Serial.println("❌ Lỗi đọc cảm biến DHT20!");
  }
}

void runApp() {
  if (tb.connected()) {
    tb.loop();
  }

  if (millis() - lastCheckTime > INTERVAL) {
    lastCheckTime = millis();

    connectWiFi();

    if (WiFi.status() == WL_CONNECTED) {
      connectMQTT();
      if (tb.connected()) {
        sendSensorData();
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // Cho Serial ổn định
  Serial.println("🚀 Bắt đầu khởi động hệ thống...");

  Wire.begin();
  dht20.begin();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void loop() {
  runApp();
}

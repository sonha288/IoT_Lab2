#include <WiFi.h>
#include <Arduino_MQTT_Client.h>
#include <ThingsBoard.h>
#include "DHT20.h"
#include "Wire.h"

// Định nghĩa chân kết nối
#define LED_PIN 2
#define SDA_PIN GPIO_NUM_21
#define SCL_PIN GPIO_NUM_22

// Thông tin WiFi và MQTT
constexpr char WIFI_SSID[] = "hasonnn";
constexpr char WIFI_PASSWORD[] = "28082004";
constexpr char TOKEN[] = "knvzbj9qjf96dj9wwagm";
constexpr char THINGSBOARD_SERVER[] = "app.coreiot.io";
constexpr uint16_t THINGSBOARD_PORT = 1883U;
constexpr uint32_t SERIAL_DEBUG_BAUD = 115200U;
constexpr uint16_t telemetrySendInterval = 3000U;

// Biến thời gian để quản lý tác vụ RTOS
unsigned long lastWiFiCheck = 0;
unsigned long lastMQTTCheck = 0;
unsigned long lastTelemetrySend = 0;

// Khai báo đối tượng kết nối
WiFiClient wifiClient;
Arduino_MQTT_Client mqttClient(wifiClient);
ThingsBoard tb(mqttClient);
DHT20 dht20;

// Khai báo trước các hàm
void WiFiTask();
void MQTTTask();
void SensorTask();

struct Task {
  void (*function)();
  unsigned long interval;
  unsigned long lastRun;
};

// Mảng chứa các tác vụ
Task tasks[] = {
  {WiFiTask, 5000, 0},
  {MQTTTask, 1000, 0},
  {SensorTask, telemetrySendInterval, 0}
};

void WiFiTask() {
  static bool isWiFiConnected = false;
  
  // Chỉ thử kết nối WiFi khi chưa kết nối
  if (WiFi.status() != WL_CONNECTED && !isWiFiConnected) {
    Serial.println("Đang kết nối WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    Serial.println("WiFi đã kết nối!");
    isWiFiConnected = true;  // Đánh dấu WiFi đã kết nối
  }
}

void MQTTTask() {
  static bool isMQTTConnected = false;

  // Kiểm tra và kết nối MQTT nếu chưa kết nối
  if (!tb.connected() && !isMQTTConnected) {
    Serial.println("Đang kết nối ThingsBoard...");
    if (tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT)) {
      Serial.println("Kết nối thành công!");
      isMQTTConnected = true;  // Đánh dấu MQTT đã kết nối
    } else {
      Serial.println("Kết nối thất bại!");
    }
  }

  if (isMQTTConnected) {
    tb.loop();
  }
}

void SensorTask() {
  dht20.read();
  float temperature = dht20.getTemperature();
  float humidity = dht20.getHumidity();

  if (!isnan(temperature) && !isnan(humidity)) {
    Serial.printf("Nhiệt độ: %.2f°C, Độ ẩm: %.2f%%\n", temperature, humidity);
    tb.sendTelemetryData("temperature", temperature);
    tb.sendTelemetryData("humidity", humidity);
  } else {
    Serial.println("Lỗi đọc cảm biến DHT20!");
  }
}

void runScheduler() {
  unsigned long currentMillis = millis();
  for (Task &task : tasks) {
    if (currentMillis - task.lastRun >= task.interval) {
      task.lastRun = currentMillis;
      task.function();
    }
  }
}

void setup() {
  Serial.begin(SERIAL_DEBUG_BAUD);
  pinMode(LED_PIN, OUTPUT);
  Wire.begin(SDA_PIN, SCL_PIN);
  dht20.begin();
}

void loop() {
  runScheduler();
}

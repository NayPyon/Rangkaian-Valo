#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// ================= KONFIGURASI HIVEMQ =================
const char* mqtt_server = "b35e18dea1b94479a14baa6c480eeb3d.s1.eu.hivemq.cloud"; 
const char* mqtt_user = "VALO"; 
const char* mqtt_pass = "Nayaka_ingram190206"; 
const int mqtt_port = 8883; // Port wajib TLS/Secure untuk HiveMQ
// ======================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

Servo myServo;
const int servoPin = 18;
const int btnStart = 12; 
const int btnBottle = 14; 

bool isProcessing = false;
int bottleCount = 0;
unsigned long lastAnimTime = 0;
int animFrame = 0;

// Objek Jaringan
WiFiClientSecure espClient;
PubSubClient client(espClient);

void connectWiFi() {
  Serial.print("Menghubungkan ke Wi-Fi Wokwi-GUEST");
  WiFi.begin("Wokwi-GUEST", "", 6); // Wokwi pakai channel 6 agar cepat
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Terhubung!");
}

void connectMQTT() {
  // HiveMQ menggunakan sertifikat SSL/TLS, kita set ke insecure untuk simulasi
  espClient.setInsecure(); 
  client.setServer(mqtt_server, mqtt_port);
  
  while (!client.connected()) {
    Serial.print("Menghubungkan ke MQTT HiveMQ...");
    String clientId = "ESP32_EcoPoint_";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("Berhasil Terhubung!");
      // Nanti ESP32 akan mendengarkan perintah buka mesin di topik ini
      client.subscribe("ecopoint/mesin/perintah"); 
    } else {
      Serial.print("Gagal, status=");
      Serial.print(client.state());
      Serial.println(" Coba lagi dalam 5 detik");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(btnStart, INPUT_PULLUP);
  pinMode(btnBottle, INPUT_PULLUP);

  myServo.attach(servoPin);
  myServo.write(90);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED gagal dimuat"));
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(10, 25);
  display.println("Menghubungkan...");
  display.display();

  // Hubungkan ke Internet & MQTT
  connectWiFi();
  connectMQTT();

  display.clearDisplay();
  display.setCursor(10, 25);
  display.println("ECO-POINT SIAP!");
  display.display();
  Serial.println("Mesin Standby dan Terkoneksi...");
}

void loop() {
  // Wajib ada agar MQTT tidak terputus (Keep-Alive)
  if (!client.connected()) {
    connectMQTT();
  }
  client.loop();

  bool startPressed = digitalRead(btnStart) == LOW;
  bool bottleDetected = digitalRead(btnBottle) == LOW;

  if (startPressed && !isProcessing) {
    isProcessing = true;
    bottleCount = 0;
    Serial.println("Sesi Dimulai!");
    delay(200);
  }

  if (isProcessing) {
    unsigned long currentMillis = millis();
    if (currentMillis - lastAnimTime >= 500) {
      lastAnimTime = currentMillis;
      animFrame = (animFrame + 1) % 4;
      
      display.clearDisplay();
      display.setCursor(0, 10);
      display.print("Memproses");
      for(int i=0; i<animFrame; i++) display.print(".");
      
      display.setCursor(0, 35);
      display.print("Botol Masuk: ");
      display.print(bottleCount);
      display.display();
    }

    if (bottleDetected) {
      bottleCount++;
      Serial.print("Botol Valid Masuk! Total: ");
      Serial.println(bottleCount);
      
      myServo.write(180);
      delay(500);
      myServo.write(90);
      
      // Kirim pesan setiap ada botol masuk ke MQTT!
      String payload = "{\"botol_total\": " + String(bottleCount) + "}";
      client.publish("ecopoint/mesin/update", payload.c_str());
      
      delay(200);
    }
  }
}
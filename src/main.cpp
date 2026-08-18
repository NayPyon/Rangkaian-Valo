#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

Servo pintuServo;
const int servoPin = 18;
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// PIN UNTUK 3 TOMBOL
const int btnPlastik = 19;
const int btnLogam = 4;
const int btnReject = 5;

bool lastPlastik = HIGH;
bool lastLogam = HIGH;
bool lastReject = HIGH;

bool isPintuTerbuka = false; 
int countPlastik = 0, countLogam = 0, countReject = 0;

const char* mqtt_server = "b35e18dea1b94479a14baa6c480eeb3d.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_user = "VALO"; 
const char* mqtt_pass = "Nayaka_ingram190206"; 

WiFiClientSecure espClient;
PubSubClient client(espClient);

void tampilkanLayar(String baris1, String baris2, int ukuran = 1) {
  display.clearDisplay();
  display.setTextSize(ukuran);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.println(baris1);
  display.println("---------------------");
  display.setTextSize(1);
  display.println(baris2);
  display.display();
}

// LAYAR KHUSUS DASBOR SAMPAH
void tampilkanDasborSampah() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 5);
  display.println("  RINCIAN MASUK");
  display.println("---------------------");
  display.print(" Plastik : "); display.println(countPlastik);
  display.print(" Logam   : "); display.println(countLogam);
  display.print(" Reject  : "); display.println(countReject);
  display.display();
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) message += (char)payload[i];
  
  if (message.indexOf("sukses") > 0) {
    pintuServo.write(90); 
    isPintuTerbuka = true; 
    countPlastik = 0; countLogam = 0; countReject = 0; // Reset
    tampilkanDasborSampah();
  } 
  else if (message.indexOf("tutup") > 0) {
    pintuServo.write(0);  
    isPintuTerbuka = false; 
    tampilkanLayar("SESI SELESAI", "Terima Kasih!");
    delay(3000); 
    tampilkanLayar("ECOPOINT RVM", "Scan QR Code Anda.");
  }
}

void reconnect() {
  while (!client.connected()) {
    tampilkanLayar("MENYAMBUNG...", "Ke Server MQTT...");
    String clientId = "MesinEco_" + String(random(0, 0xffff), HEX);
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      client.subscribe("ecopoint/nayaka/status");
      tampilkanLayar("ECOPOINT RVM", "Scan QR Code Anda.");
    } else {
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(btnPlastik, INPUT_PULLUP);
  pinMode(btnLogam, INPUT_PULLUP);
  pinMode(btnReject, INPUT_PULLUP);
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  tampilkanLayar("STARTING...", "Memulai Sistem...");
  
  pintuServo.attach(servoPin);
  pintuServo.write(0); 
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  
  espClient.setInsecure(); 
  client.setBufferSize(1024); 
  client.setKeepAlive(60); 
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback); 
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  if (isPintuTerbuka) {
    // CEK TOMBOL PLASTIK
    bool currPlastik = digitalRead(btnPlastik);
    if (lastPlastik == HIGH && currPlastik == LOW) {
      countPlastik++;
      tampilkanDasborSampah();
      client.publish("ecopoint/nayaka/sensor", "plastik_masuk");
      delay(300);
    }
    lastPlastik = currPlastik;

    // CEK TOMBOL LOGAM
    bool currLogam = digitalRead(btnLogam);
    if (lastLogam == HIGH && currLogam == LOW) {
      countLogam++;
      tampilkanDasborSampah();
      client.publish("ecopoint/nayaka/sensor", "logam_masuk");
      delay(300);
    }
    lastLogam = currLogam;

    // CEK TOMBOL REJECT
    bool currReject = digitalRead(btnReject);
    if (lastReject == HIGH && currReject == LOW) {
      countReject++;
      tampilkanDasborSampah();
      client.publish("ecopoint/nayaka/sensor", "reject_masuk");
      delay(300);
    }
    lastReject = currReject;
  }

  // SCANNER QR
  if (Serial.available()) {
    String qrCode = Serial.readString();
    qrCode.trim();
    if (qrCode.length() > 0) {
      tampilkanLayar("MEMPROSES...", "Validasi QR Code...");
      client.publish("ecopoint/nayaka/scan", qrCode.c_str());
    }
  }
}
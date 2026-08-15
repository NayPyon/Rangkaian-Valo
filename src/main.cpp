#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- KONFIGURASI LAYAR OLED ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

Servo pintuServo;
const int servoPin = 18;
const char* ssid = "Wokwi-GUEST";
const char* password = "";

const int buttonPin = 19;
bool isPintuTerbuka = false; 
bool lastButtonState = HIGH;
int botolMasuk = 0; // Penghitung lokal untuk tampilan layar

// Konfigurasi HiveMQ Cloud
const char* mqtt_server = "b35e18dea1b94479a14baa6c480eeb3d.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_user = "VALO"; 
const char* mqtt_pass = "Nayaka_ingram190206"; 

WiFiClientSecure espClient;
PubSubClient client(espClient);

// --- FUNGSI PEMBARUAN LAYAR ---
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

// 1. FUNGSI CALLBACK (Diperbarui dengan Layar)
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.println("\n[MQTT] Balasan: " + message);
  
  if (message.indexOf("sukses") > 0) {
    Serial.println("[MESIN] ✅ VALID! Pintu Terbuka...");
    pintuServo.write(90); 
    isPintuTerbuka = true; 
    botolMasuk = 0; // Reset hitungan di mesin
    tampilkanLayar("PINTU TERBUKA", "Silakan masukkan\nsampah Anda.");
  } 
  else if (message.indexOf("tutup") > 0) {
    Serial.println("[MESIN] 🚪 Menutup pintu...");
    pintuServo.write(0);  
    isPintuTerbuka = false; 
    tampilkanLayar("SESI SELESAI", "Terima kasih!\nSampah: " + String(botolMasuk));
    delay(3000); // Tahan tulisan terima kasih 3 detik
    tampilkanLayar("ECOPOINT RVM", "Siap digunakan.\nScan QR Code Anda.");
  }
  else {
    Serial.println("[MESIN] ❌ Verifikasi Gagal!");
    tampilkanLayar("AKSES DITOLAK", "Kode QR tidak valid\natau sudah usang.");
    delay(2000);
    tampilkanLayar("ECOPOINT RVM", "Siap digunakan.\nScan QR Code Anda.");
  }
}

// 2. FUNGSI RECONNECT
void reconnect() {
  while (!client.connected()) {
    tampilkanLayar("MENYAMBUNG...", "Menghubungi Server\nHiveMQ...");
    Serial.print("Menyambungkan ke MQTT...");
    
    String clientId = "MesinEcoPoint_";
    clientId += String(random(0, 0xffff), HEX);
    
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println(" TERHUBUNG!");
      client.subscribe("ecopoint/nayaka/status");
      tampilkanLayar("ECOPOINT RVM", "Siap digunakan.\nScan QR Code Anda.");
    } else {
      delay(5000);
    }
  }
}

// 3. FUNGSI SETUP
void setup() {
  Serial.begin(115200);
  pinMode(buttonPin, INPUT_PULLUP);
  
  // Inisialisasi Layar OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED gagal dimuat"));
    for(;;);
  }
  tampilkanLayar("STARTING...", "Memulai Sistem...");

  pintuServo.attach(servoPin);
  pintuServo.write(0); 
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  
  espClient.setInsecure(); 
  client.setBufferSize(1024); 
  client.setKeepAlive(60); 
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback); 
}

// 4. FUNGSI LOOP
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // --- LOGIKA SENSOR SAMPAH ---
  if (isPintuTerbuka) {
    bool currentButtonState = digitalRead(buttonPin);
    
    if (lastButtonState == HIGH && currentButtonState == LOW) {
      Serial.println("[MESIN] ♻️ Sampah terdeteksi!");
      botolMasuk++; // Tambah angka di layar
      tampilkanLayar("MENGHITUNG...", "Botol Masuk: " + String(botolMasuk));
      
      client.publish("ecopoint/nayaka/sensor", "sampah_masuk");
      delay(300); 
    }
    lastButtonState = currentButtonState;
  }

  // --- LOGIKA SCANNER QR ---
  if (Serial.available()) {
    String qrCode = Serial.readString();
    qrCode.trim();
    
    if (qrCode.length() > 0) {
      tampilkanLayar("MEMPROSES...", "Mengecek kode QR\nke awan...");
      client.publish("ecopoint/nayaka/scan", qrCode.c_str());
    }
  }
}
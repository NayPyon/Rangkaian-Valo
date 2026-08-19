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

const char* mqtt_server = "b35e18dea1b94479a14baa6c480eeb3d.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_user = "VALO"; 
const char* mqtt_pass = "Nayaka_ingram190206"; 

WiFiClientSecure espClient;
PubSubClient client(espClient);

// FUNGSI LAYAR STANDAR
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

// FUNGSI ANIMASI: Saat pintu terbuka dan menunggu sampah
void tampilkanMenunggu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.println("PINTU TERBUKA");
  display.println("---------------------");
  display.println("Masukkan Botol /");
  display.println("Kaleng Anda Sekarang.");
  display.display();
}

// FUNGSI ANIMASI: Sukses masuk (Plastik/Logam)
void animasiSukses(String tipe) {
  for(int i = 0; i < 3; i++) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(15, 20);
    display.print("+1 ");
    display.print(tipe);
    display.display();
    delay(200);
    
    display.clearDisplay();
    display.display();
    delay(100);
  }
  // Kembali ke layar menunggu setelah animasi selesai
  tampilkanMenunggu();
}

// FUNGSI ANIMASI: Ditolak (Reject)
void animasiDitolak() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(10, 20);
  display.println("X DITOLAK");
  display.setTextSize(1);
  display.setCursor(0, 50);
  display.println("Bukan Plastik/Logam!");
  display.display();
  delay(1500);
  tampilkanMenunggu();
}


void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) message += (char)payload[i];

  Serial.println("\n[MQTT] Pesan masuk di topik: " + String(topic));
  Serial.println("[MQTT] Isi pesan: " + message);

  if (message.indexOf("sukses") > 0) {
    Serial.println("[AKSI] Membuka Pintu...");
    pintuServo.write(90); 
    isPintuTerbuka = true; 
    tampilkanMenunggu(); // Tampilkan layar standby masukin sampah
  } 
  else if (message.indexOf("tutup") > 0) {
    Serial.println("[AKSI] Menutup Pintu dan mengakhiri sesi.");
    pintuServo.write(0);  
    isPintuTerbuka = false; 
    tampilkanLayar("SESI SELESAI", "Terima Kasih!");
    delay(3000); 
    tampilkanLayar("ECOPOINT RVM", "Scan QR Code Anda.");
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("[WIFI] Menghubungkan ke MQTT Server...");
    tampilkanLayar("MENYAMBUNG...", "Ke Server MQTT...");
    
    String clientId = "MesinEco_" + String(random(0, 0xffff), HEX);
    
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("[MQTT] Berhasil Terhubung!");
      client.subscribe("ecopoint/nayaka/status");
      tampilkanLayar("ECOPOINT RVM", "Scan QR Code Anda.");
    } else {
      Serial.print("[MQTT] Gagal terhubung, kode error: ");
      Serial.println(client.state());
      Serial.println("[MQTT] Mencoba lagi dalam 5 detik...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n--- MULAI SISTEM ECOPOINT RVM ---");
  
  pinMode(btnPlastik, INPUT_PULLUP);
  pinMode(btnLogam, INPUT_PULLUP);
  pinMode(btnReject, INPUT_PULLUP);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("[ERROR] Layar OLED Gagal diinisialisasi!");
    for(;;); // Hentikan program jika OLED mati
  }
  
  tampilkanLayar("STARTING...", "Memulai Sistem...");

  pintuServo.attach(servoPin);
  pintuServo.write(0); 
  Serial.println("[HARDWARE] Servo dikunci di posisi 0 derajat.");

  Serial.println("[WIFI] Menghubungkan ke SSID: " + String(ssid));
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[WIFI] Terhubung! IP: " + WiFi.localIP().toString());

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
      Serial.println("[SENSOR] Sampah PLASTIK terdeteksi!");
      animasiSukses("PLASTIK");
      client.publish("ecopoint/nayaka/sensor", "plastik_masuk");
      delay(300); // Debounce
    }
    lastPlastik = currPlastik;

    // CEK TOMBOL LOGAM
    bool currLogam = digitalRead(btnLogam);
    if (lastLogam == HIGH && currLogam == LOW) {
      Serial.println("[SENSOR] Sampah LOGAM terdeteksi!");
      animasiSukses("LOGAM");
      client.publish("ecopoint/nayaka/sensor", "logam_masuk");
      delay(300);
    }
    lastLogam = currLogam;

    // CEK TOMBOL REJECT
    bool currReject = digitalRead(btnReject);
    if (lastReject == HIGH && currReject == LOW) {
      Serial.println("[SENSOR] Barang DITOLAK (Bukan plastik/logam)!");
      animasiDitolak();
      client.publish("ecopoint/nayaka/sensor", "reject_masuk");
      delay(300);
    }
    lastReject = currReject;
  }

  // SCANNER QR (Diambil dari input Serial Monitor)
  if (Serial.available()) {
    String qrCode = Serial.readString();
    qrCode.trim();
    if (qrCode.length() > 0) {
      Serial.println("[SCANNER] Membaca QR: " + qrCode);
      tampilkanLayar("MEMPROSES...", "Validasi QR Code...");
      client.publish("ecopoint/nayaka/scan", qrCode.c_str());
    }
  }
}
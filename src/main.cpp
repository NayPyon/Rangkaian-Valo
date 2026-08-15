#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

Servo pintuServo;
const int servoPin = 18;
const char* ssid = "Wokwi-GUEST";
const char* password = "";

const int buttonPin = 19;
bool isPintuTerbuka = false; // Pengingat status pintu
bool lastButtonState = HIGH;

// Konfigurasi HiveMQ Cloud milikmu
const char* mqtt_server = "b35e18dea1b94479a14baa6c480eeb3d.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_user = "VALO"; 
const char* mqtt_pass = "Nayaka_ingram190206"; 

WiFiClientSecure espClient;
PubSubClient client(espClient);

// 1. FUNGSI CALLBACK (Diperbaiki penutupnya & ditambah kondisi gagal)
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.println("\n[MQTT] Balasan dari Server: " + message);
  
  // LOGIKA PINTU BARU
  if (message.indexOf("sukses") > 0) {
    Serial.println("[MESIN] ✅ VALID! Pintu Terbuka 90 Derajat...");
    pintuServo.write(90); 
    isPintuTerbuka = true; 
  } 
  else if (message.indexOf("tutup") > 0) {
    Serial.println("[MESIN] 🚪 Sesi Selesai. Menutup pintu...");
    pintuServo.write(0);  
    isPintuTerbuka = false; 
  }
  else {
    Serial.println("[MESIN] ❌ Verifikasi Gagal! Pintu tetap tertutup.");
  }
} // <--- Kurung kurawal penutup ini yang tadi hilang!

// 2. FUNGSI RECONNECT
void reconnect() {
  while (!client.connected()) {
    Serial.print("Menyambungkan ke MQTT Private...");
    
    // Membuat ID acak
    String clientId = "MesinEcoPoint_";
    clientId += String(random(0, 0xffff), HEX);
    
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println(" TERHUBUNG!");
      client.subscribe("ecopoint/nayaka/status");
    } else {
      Serial.print(" Gagal, status=");
      Serial.print(client.state());
      Serial.println(" Coba lagi dalam 5 detik");
      delay(5000);
    }
  }
}

// 3. FUNGSI SETUP
void setup() {
  Serial.begin(115200);
  Serial.setTimeout(100);
  pinMode(buttonPin, INPUT_PULLUP);
  
  // Inisialisasi Servo
  pintuServo.attach(servoPin);
  pintuServo.write(0); // Pastikan pintu terkunci saat mesin baru nyala
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[SUKSES] WiFi Terhubung!");
  
  espClient.setInsecure(); 
  
  client.setBufferSize(1024); 
  client.setKeepAlive(60); 

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback); 
}

// 4. FUNGSI LOOP (Digabung antara Scanner QR dan Sensor Tombol)
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // --- LOGIKA SENSOR SAMPAH (TOMBOL) ---
  if (isPintuTerbuka) {
    bool currentButtonState = digitalRead(buttonPin);
    
    // Jika tombol ditekan (berubah dari tidak ditekan menjadi ditekan)
    if (lastButtonState == HIGH && currentButtonState == LOW) {
      Serial.println("[MESIN] ♻️ Sampah terdeteksi! Melapor ke server...");
      
      // Kirim sinyal ke Node.js
      client.publish("ecopoint/nayaka/sensor", "sampah_masuk");
      
      delay(300); // Anti-bouncing
    }
    lastButtonState = currentButtonState;
  }

  // --- LOGIKA SCANNER QR CODE (Dari Terminal) ---
  if (Serial.available()) {
    String qrCode = Serial.readString();
    qrCode.trim();
    
    if (qrCode.length() > 0) {
      Serial.println("\n-----------------------------------------");
      Serial.println("Mengecek kode: " + qrCode);
      
      client.publish("ecopoint/nayaka/scan", qrCode.c_str());
      Serial.println("Menunggu balasan dari server...");
    }
  }
}
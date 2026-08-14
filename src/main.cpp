#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";

// Konfigurasi HiveMQ Cloud milikmu
const char* mqtt_server = "b35e18dea1b94479a14baa6c480eeb3d.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_user = "VALO"; // <-- GANTI INI
const char* mqtt_pass = "Nayaka_ingram190206"; // <-- GANTI INI

WiFiClientSecure espClient;
PubSubClient client(espClient);

// 1. INI ADALAH FUNGSI CALLBACK YANG BENAR (Lengkap dengan isinya)
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.println("\n[MQTT] Balasan dari Server: " + message);
  
  // Mengecek apakah pesannya mengandung kata "sukses"
  if (message.indexOf("sukses") > 0) {
    Serial.println("[MESIN] ✅ Verifikasi Berhasil! MEMBUKA PINTU SERVO...");
  } else {
    Serial.println("[MESIN] ❌ Verifikasi Gagal! Pintu tetap tertutup.");
  }
  Serial.println("------------------------------------------");
  Serial.println("Silakan ketik kode QR selanjutnya...");
}

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
  
  // VITAMIN 1: Mencegah mesin melamun kelamaan saat membaca ketikanmu
  Serial.setTimeout(100); 

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[SUKSES] WiFi Terhubung!");
  
  espClient.setInsecure(); 
  
  // VITAMIN 2: Memperbesar kapasitas paru-paru mesin ke angka maksimal
  client.setBufferSize(1024); 
  
  // VITAMIN 3: Meminta kelonggaran waktu ke HiveMQ (60 detik) agar tidak gampang di-kick
  client.setKeepAlive(60); 

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback); 
}

// 4. FUNGSI LOOP
void loop() {
  // Menjaga koneksi MQTT
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Membaca ketikan tanpa harus menunggu \n
  if (Serial.available() > 0) {
    String qrCode = Serial.readString(); // <-- Diubah di sini
    qrCode.trim(); // Membersihkan spasi dan enter gaib
    
    // Kalau benar-benar ada teks yang masuk, baru proses
    if (qrCode.length() > 0) {
      Serial.println("\n[SCANNER] Kode terbaca: " + qrCode);
      Serial.println("[MESIN] Mengirim ke topik: ecopoint/nayaka/scan...");
      client.publish("ecopoint/nayaka/scan", qrCode.c_str());
    }
  }
}
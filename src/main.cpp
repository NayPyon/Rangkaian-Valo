#include <WiFi.h>
#include <WiFiClientSecure.h> // Library baru untuk jalur aman (TLS)
#include <PubSubClient.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";

// Konfigurasi HiveMQ Cloud milikmu
const char* mqtt_server = "b35e18dea1b94479a14baa6c480eeb3d.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_user = "ISI_USERNAME_KAMU"; 
const char* mqtt_pass = "ISI_PASSWORD_KAMU"; 

WiFiClientSecure espClient; // Menggunakan WiFi khusus keamanan
PubSubClient client(espClient);

// ... (Fungsi callback/telinga tetap sama) ...

void reconnect() {
  while (!client.connected()) {
    Serial.print("Menyambungkan ke MQTT Private...");
    
    String clientId = "MesinEcoPoint_";
    clientId += String(random(0, 0xffff), HEX);
    
    // Menambahkan username dan password ke dalam koneksi
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

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[SUKSES] WiFi Terhubung!");
  
  // Karena ini untuk simulasi, kita bypass pengecekan sertifikat SSL
  espClient.setInsecure(); 
  
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); // Menjaga koneksi MQTT tetap hidup

  // Membaca ketikan (Simulasi Scanner)
  if (Serial.available() > 0) {
    String qrCode = Serial.readStringUntil('\n');
    qrCode.trim();
    
    if (qrCode.length() > 0) {
      Serial.println("\n[SCANNER] Kode terbaca: " + qrCode);
      Serial.println("[MESIN] Mengirim ke topik: ecopoint/nayaka/scan...");
      // Mengirim data ke Node.js via HiveMQ
      client.publish("ecopoint/nayaka/scan", qrCode.c_str());
    }
  }
}
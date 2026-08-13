#include <WiFi.h>
#include <HTTPClient.h> // Library baru untuk mengirim data HTTP

// GANTI "192.168.X.X" DENGAN IPv4 LAPTOPMU (contoh: 192.168.1.5)
const char* serverUrl = "http://192.168.0.106:3000/api/verify-qr"; 

void setup() {
  Serial.begin(115200);
  Serial.println("Menghidupkan Mesin EcoPoint...");
  Serial.print("Menyambungkan ke WiFi");
  
  WiFi.begin("Wokwi-GUEST", "", 6);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\n[SUKSES] WiFi Terhubung!");
  Serial.println("IP Address Mesin: " + WiFi.localIP().toString());
  Serial.println("------------------------------------------");
  Serial.println("Mesin Siap! Ketik kode QR di bawah...");
}

void loop() {
  if (Serial.available() > 0) {
    String qrCode = Serial.readStringUntil('\n');
    qrCode.trim();
    
    if (qrCode.length() > 0) {
      Serial.println("\n[SCANNER] Kode terbaca: " + qrCode);
      Serial.println("[MESIN] Mengirim data ke Server Node.js...");
      
      // Mengecek apakah WiFi masih nyambung
      if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(serverUrl);
        
        // Memberi tahu server bahwa kita mengirim format JSON
        http.addHeader("Content-Type", "application/json");

        // Merakit paket data JSON secara manual
        String httpRequestData = "{\"qrCode\":\"" + qrCode + "\"}";
        
        // Menembakkan data lewat metode POST
        int httpResponseCode = http.POST(httpRequestData);
        
        // Membaca balasan dari satpam Node.js
        if (httpResponseCode > 0) {
          String response = http.getString();
          Serial.println("[SERVER] Kode HTTP: " + String(httpResponseCode));
          Serial.println("[SERVER] Balasan: " + response);
        } else {
          Serial.print("[ERROR] Pengiriman gagal. Kode Error: ");
          Serial.println(httpResponseCode);
        }
        
        http.end(); // Menutup koneksi biar hemat memori
      } else {
        Serial.println("[ERROR] WiFi Terputus!");
      }
      
      Serial.println("[MESIN] Menunggu QR Code selanjutnya...\n");
    }
  }
}
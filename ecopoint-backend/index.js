const mqtt = require('mqtt');
const { initializeApp, cert } = require('firebase-admin/app');
const { getFirestore } = require('firebase-admin/firestore');
const serviceAccount = require('./serviceAccountKey.json');

// --- Inisialisasi Firebase ---
initializeApp({
  credential: cert(serviceAccount)
});

// ---> INI DIA BARIS YANG HILANG! <---
// Kita harus memberitahu Node.js bahwa 'db' adalah penghubung ke Firestore
const db = getFirestore(); 
// -------------------------------------

// 2. Koneksi ke HiveMQ Cloud milikmu
const brokerUrl = 'mqtts://b35e18dea1b94479a14baa6c480eeb3d.s1.eu.hivemq.cloud:8883';

const options = {
  username: 'VALO', // Ganti dengan username dari Access Management
  password: 'Nayaka_ingram190206'  // Ganti dengan password dari Access Management
};

const client = mqtt.connect(brokerUrl, options);

// ... (Bagian subscribe dan logika Firebase tetap sama) ...

// 3. Nama "Grup Obrolan" (Topik)
// Aku tambahkan namamu biar datanya tidak nyasar dengan orang lain di public broker
const TOPIC_SCAN = 'ecopoint/nayaka/scan';
const TOPIC_STATUS = 'ecopoint/nayaka/status';

client.on('connect', () => {
  console.log(`🚀 [MQTT] Terhubung ke HiveMQ Broker!`);
  // Berlangganan ke topik scan dari ESP32
  client.subscribe(TOPIC_SCAN, (err) => {
    if (!err) console.log(`📡 [MQTT] Siaga mendengarkan topik: ${TOPIC_SCAN}`);
  });
});

// 4. Kalau ada pesan masuk dari ESP32
client.on('message', async (topic, message) => {
  if (topic === 'ecopoint/nayaka/scan') {
    const qrCode = message.toString().trim();
    console.log(`[SERVER] Menerima QR Code dari mesin: ${qrCode}`);

    try {
      // 1. Mencarinya berdasarkan field 'kode_sesi', BUKAN nama dokumen
      const sesiRef = db.collection('Sesi_Aktif');
      const snapshot = await sesiRef.where('kode_sesi', '==', qrCode).get();

      // 2. Mengecek apakah ada dokumen yang cocok
      if (snapshot.empty) {
        console.log(`[SERVER] ❌ Kode ${qrCode} TIDAK VALID.`);
        // Perintahkan ESP32 untuk menolak
        client.publish('ecopoint/nayaka/status', JSON.stringify({status: 'gagal'}));
      } else {
        console.log(`[SERVER] ✅ Kode ${qrCode} VALID! MEMBUKA PINTU...`);
        // Perintahkan ESP32 untuk membuka servo
        client.publish('ecopoint/nayaka/status', JSON.stringify({status: 'sukses'}));
        
        // (Opsional & Sangat Disarankan) 
        // Ubah status di Firebase agar kode ini tidak bisa dipakai 2x
        snapshot.forEach(doc => {
          doc.ref.update({ status: 'berhasil_digunakan' });
        });
      }
    } catch (error) {
      console.error('[SERVER] Firebase Error:', error);
    }
  }
});
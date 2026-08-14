const mqtt = require('mqtt'); // <--- BARIS INI YANG HILANG
const { initializeApp, cert } = require('firebase-admin/app');
const { getFirestore } = require('firebase-admin/firestore');
const serviceAccount = require('./serviceAccountKey.json');

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
  if (topic === TOPIC_SCAN) {
    const qrCode = message.toString().trim();
    console.log(`\n[SERVER] Menerima QR Code dari mesin: ${qrCode}`);

    try {
      const sesiRef = db.collection('Sesi_Aktif');
      const snapshot = await sesiRef.where('qr_code', '==', qrCode).get();

      if (snapshot.empty) {
        console.log(`[SERVER] ❌ Kode ${qrCode} TIDAK VALID.`);
        // Kirim perintah GAGAL ke mesin
        client.publish(TOPIC_STATUS, JSON.stringify({ status: 'gagal' }));
        return;
      }

      let userId = '';
      snapshot.forEach(doc => { userId = doc.id; });
      console.log(`[SERVER] ✅ Kode VALID! Milik pengguna: ${userId}`);
      
      // Kirim perintah BUKA PINTU ke mesin
      client.publish(TOPIC_STATUS, JSON.stringify({ status: 'sukses', user: userId }));

    } catch (error) {
      console.error('[SERVER] Firebase Error:', error);
    }
  }
});
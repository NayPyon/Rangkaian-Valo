const mqtt = require('mqtt');
const { initializeApp, cert } = require('firebase-admin/app');
const { getFirestore, FieldValue } = require('firebase-admin/firestore');
const serviceAccount = require('./serviceAccountKey.json');

// --- Inisialisasi Firebase ---
initializeApp({
  credential: cert(serviceAccount)
});
const db = getFirestore(); 

// 2. Koneksi ke HiveMQ Cloud milikmu
const brokerUrl = 'mqtts://b35e18dea1b94479a14baa6c480eeb3d.s1.eu.hivemq.cloud:8883';
const options = {
  username: 'VALO', 
  password: 'Nayaka_ingram190206' 
};
const client = mqtt.connect(brokerUrl, options);

// 3. Nama Topik
const TOPIC_SCAN = 'ecopoint/nayaka/scan';
const TOPIC_STATUS = 'ecopoint/nayaka/status';
const TOPIC_SENSOR = 'ecopoint/nayaka/sensor'; // <--- Topik Sensor

client.on('connect', () => {
  console.log(`🚀 [MQTT] Terhubung ke HiveMQ Broker!`);
  // Subscribe ke 2 topik sekaligus (Scan QR dan Sensor Sampah)
  client.subscribe([TOPIC_SCAN, TOPIC_SENSOR], (err) => {
    if (!err) console.log(`📡 [MQTT] Siaga mendengarkan scan & sensor!`);
  });
});

// 4. Kalau ada pesan masuk dari ESP32
client.on('message', async (topic, message) => {
  
  // --- LOGIKA 1: JIKA MENERIMA SCAN QR ---
  if (topic === TOPIC_SCAN) {
    const qrCode = message.toString().trim();
    console.log(`[SERVER] Menerima QR Code dari mesin: ${qrCode}`);

    try {
      const sesiRef = db.collection('Sesi_Aktif');
      const snapshot = await sesiRef.where('kode_sesi', '==', qrCode).get();

      if (snapshot.empty) {
        console.log(`[SERVER] ❌ Kode ${qrCode} TIDAK VALID.`);
        client.publish(TOPIC_STATUS, JSON.stringify({status: 'gagal'}));
      } else {
        console.log(`[SERVER] ✅ Kode ${qrCode} VALID! MEMBUKA PINTU...`);
        client.publish(TOPIC_STATUS, JSON.stringify({status: 'sukses'}));
        
        snapshot.forEach(doc => {
          doc.ref.update({ 
            status: 'pintu_terbuka',
            botol: 0,
            poin: 0
          });
        });
      }
    } catch (error) {
      console.error('[SERVER] Firebase Error:', error);
    }
  }

  // ---> INI DIA YANG BARU: LOGIKA 2 JIKA TOMBOL DITEKAN <---
  if (topic === TOPIC_SENSOR) {
    const aksi = message.toString().trim();
    if (aksi === 'sampah_masuk') {
      console.log(`[SERVER] ♻️ Menerima laporan: 1 Botol masuk! Menambah poin...`);
      
      try {
        // Otomatis tambah 1 botol dan 500 poin langsung di database!
        db.collection('Sesi_Aktif').doc('Nayaka').update({
          botol: FieldValue.increment(1),
          poin: FieldValue.increment(500)
        });
      } catch (error) {
        console.error('[SERVER] Gagal menambah poin:', error);
      }
    }
  }
});

// ---> PERUBAHAN 2: Mata-mata untuk memantau tombol Selesai dari HP
const sesiAktifRef = db.collection('Sesi_Aktif').doc('Nayaka');

sesiAktifRef.onSnapshot(docSnapshot => {
  if (docSnapshot.exists) {
    const data = docSnapshot.data();
    
    // Jika HP mengubah status menjadi selesai
    if (data.status === 'selesai') {
      console.log("\n[SERVER] 🛑 User menekan Selesai. Memerintahkan pintu ditutup!");
      
      // Mengirim perintah tutup ke ESP32
      client.publish(TOPIC_STATUS, JSON.stringify({status: 'tutup'}));
      
      // Mengembalikan status menjadi diam (agar tidak memicu loop)
      docSnapshot.ref.update({ status: 'menunggu_mesin' });
    }
  }
});
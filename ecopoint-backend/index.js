const mqtt = require('mqtt');
const { initializeApp, cert } = require('firebase-admin/app');
const { getFirestore } = require('firebase-admin/firestore');
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

client.on('connect', () => {
  console.log(`🚀 [MQTT] Terhubung ke HiveMQ Broker!`);
  client.subscribe(TOPIC_SCAN, (err) => {
    if (!err) console.log(`📡 [MQTT] Siaga mendengarkan topik: ${TOPIC_SCAN}`);
  });
});

// 4. Kalau ada pesan masuk dari ESP32
client.on('message', async (topic, message) => {
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
        
        // ---> PERUBAHAN 1: Update status untuk memicu UI HP berubah
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
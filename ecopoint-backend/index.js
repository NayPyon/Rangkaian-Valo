const mqtt = require('mqtt');
const { initializeApp, cert } = require('firebase-admin/app');
const { getFirestore, FieldValue } = require('firebase-admin/firestore');
const serviceAccount = require('./serviceAccountKey.json');

initializeApp({ credential: cert(serviceAccount) });
const db = getFirestore(); 

const brokerUrl = 'mqtts://b35e18dea1b94479a14baa6c480eeb3d.s1.eu.hivemq.cloud:8883';
const options = {
  username: 'VALO', 
  password: 'Nayaka_ingram190206' 
};
const client = mqtt.connect(brokerUrl, options);

const TOPIC_SCAN = 'ecopoint/nayaka/scan';
const TOPIC_STATUS = 'ecopoint/nayaka/status';
const TOPIC_SENSOR = 'ecopoint/nayaka/sensor'; 

client.on('connect', () => {
  console.log(`🚀 [MQTT] Terhubung ke HiveMQ Broker!`);
  client.subscribe([TOPIC_SCAN, TOPIC_SENSOR], (err) => {
    if (!err) console.log(`📡 [MQTT] Siaga mendengarkan scan & sensor!`);
  });
});

client.on('message', async (topic, message) => {
  // --- LOGIKA 1: SCAN QR ---
  if (topic === TOPIC_SCAN) {
    const qrCode = message.toString().trim();
    try {
      const sesiRef = db.collection('Sesi_Aktif');
      const snapshot = await sesiRef.where('kode_sesi', '==', qrCode).get();

      if (snapshot.empty) {
        console.log(`[SERVER] ❌ Kode ${qrCode} TIDAK VALID.`);
        client.publish(TOPIC_STATUS, JSON.stringify({status: 'gagal'}));
      } else {
        console.log(`[SERVER] ✅ Kode ${qrCode} VALID!`);
        client.publish(TOPIC_STATUS, JSON.stringify({status: 'sukses'}));
        
        snapshot.forEach(doc => {
          doc.ref.update({ 
            status: 'pintu_terbuka',
            botol_plastik: 0,
            botol_logam: 0,
            sampah_reject: 0
          });
        });
      }
    } catch (error) {
      console.error('[SERVER] Firebase Error:', error);
    }
  }

  // --- LOGIKA 2: SAMPAH MASUK ---
  if (topic === TOPIC_SENSOR) {
    const aksi = message.toString().trim();
    const docRef = db.collection('Sesi_Aktif').doc('Nayaka');

    try {
      if (aksi === 'plastik_masuk') {
        console.log(`[SERVER] 🍾 Plastik masuk! Menambah ke database...`);
        docRef.update({ botol_plastik: FieldValue.increment(1) });
      } else if (aksi === 'logam_masuk') {
        console.log(`[SERVER] 🥫 Logam masuk! Menambah ke database...`);
        docRef.update({ botol_logam: FieldValue.increment(1) });
      } else if (aksi === 'reject_masuk') {
        console.log(`[SERVER] ❌ Sampah Ditolak!`);
        docRef.update({ sampah_reject: FieldValue.increment(1) });
      }
    } catch (error) {
      console.error('[SERVER] Gagal update poin:', error);
    }
  }
});

// --- MATA-MATA SELESAI SESI (VERSUS BANK POIN) ---
db.collection('Sesi_Aktif').doc('Nayaka').onSnapshot(async (docSnapshot) => {
  if (docSnapshot.exists) {
    const data = docSnapshot.data();
    
    // Jika user menekan tombol 'Akhiri Sesi'
    if (data.status === 'selesai') {
      console.log("\n[SERVER] 🛑 Sesi selesai. Memproses Bank Poin...");

      const plastik = data.botol_plastik || 0;
      const logam = data.botol_logam || 0;
      const poinSesiIni = (plastik * 100) + (logam * 300);

      try {
        // 1. UPDATE DOMPET UTAMA (BANK)
        const userRef = db.collection('Users').doc('Nayaka');
        await userRef.set({
          total_poin: FieldValue.increment(poinSesiIni),
          total_plastik: FieldValue.increment(plastik),
          total_logam: FieldValue.increment(logam)
        }, { merge: true });

        // 2. CATAT KE BUKU RIWAYAT
        await db.collection('Riwayat').add({
          user: 'Nayaka',
          plastik: plastik,
          logam: logam,
          poin: poinSesiIni,
          tanggal: FieldValue.serverTimestamp()
        });

        console.log(`[SERVER] ✅ Berhasil menabung ${poinSesiIni} poin!`);

        // 3. RESET SESI & TUTUP PINTU
        client.publish(TOPIC_STATUS, JSON.stringify({status: 'tutup'}));
        
        docSnapshot.ref.update({ 
          status: 'menunggu_mesin',
          botol_plastik: 0,
          botol_logam: 0,
          sampah_reject: 0
        });

      } catch (error) {
        console.error('[SERVER] Gagal menabung poin:', error);
      }
    }
  }
});
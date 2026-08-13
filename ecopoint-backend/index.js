const express = require('express');
const cors = require('cors');

// 1. Cara BARU memanggil Firebase Admin (Sistem Modular)
const { initializeApp, cert } = require('firebase-admin/app');
const { getFirestore } = require('firebase-admin/firestore');

// 2. Memanggil Kunci VIP Firebase
const serviceAccount = require('./serviceAccountKey.json');

// 3. Menghidupkan akses Firebase Admin
initializeApp({
  credential: cert(serviceAccount)
});

const db = getFirestore();
const app = express();

// 4. Menyiapkan Middleware (agar bisa menerima data JSON dari ESP32)
app.use(cors());
app.use(express.json());

// 5. Membuat Rute API (Endpoint) untuk menerima lemparan dari mesin
app.post('/api/verify-qr', async (req, res) => {
  try {
    const { qrCode } = req.body;
    
    console.log(`\n[SERVER] Menerima permintaan verifikasi QR: ${qrCode}`);

    // Mengecek apakah kode ini ada di koleksi "Sesi_Aktif"
    const sesiRef = db.collection('Sesi_Aktif');
    const snapshot = await sesiRef.where('qr_code', '==', qrCode).get();

    if (snapshot.empty) {
      console.log(`[SERVER] ❌ Kode ${qrCode} TIDAK VALID atau sudah kedaluwarsa.`);
      return res.status(404).json({ success: false, message: 'QR Code tidak valid!' });
    }

    // Kalau kodenya ketemu
    let userId = '';
    snapshot.forEach(doc => {
      userId = doc.id; // Mendapatkan ID dokumen (biasanya nama/ID pengguna)
    });

    console.log(`[SERVER] ✅ Kode VALID! Milik pengguna: ${userId}`);

    // --- NANTI LOGIKA INJECT POIN & RIWAYAT KITA TARUH DI SINI ---

    res.status(200).json({ 
      success: true, 
      message: 'Kode berhasil diverifikasi!',
      user: userId
    });

  } catch (error) {
    console.error('[SERVER] Terjadi kesalahan:', error);
    res.status(500).json({ success: false, message: 'Server Error' });
  }
});

// 6. Menyalakan Server di Port 3000
const PORT = 3000;
app.listen(PORT, () => {
  console.log(`🚀 Satpam EcoPoint Backend menyala di http://localhost:${PORT}`);
});
# MiniStream - Tasarim Raporu

**Ogrenci:** Ahmet Eren KONUR - 2312101023 
**Tarih:** 16.03.2026

Bu projede Kaggle uzerinden indirilen Spotify 1.2M+ veri seti kullanildi. Veri dosyasi `data/sarkilar.csv` olarak yerlestirildi ve `csv_yukle()` fonksiyonu ile 10.000 kayitlik yukleme testi yapildi. Test sirasinda `sizeof(Sarki) = 328` byte olarak olculdu. Bu deger rapordaki tum bellek hesaplarinda temel alinmistir. Ayrica WSL uzerinde `test_temel`, `test_bellek`, `benchmark` ve Valgrind calistirildi. Tum esas testlerde `ERROR SUMMARY: 0 errors` sonucu alindi.

## 1. Kopya Modeli vs Pointer Modeli

### Hipotez
Beklentim pointer modelinin hem daha az bellek kullanmasi hem de daha hizli calismasiydi. Bunun nedeni kopya modelinde her listeye eklenen sarki icin yeniden `malloc` ve `memcpy` yapilmasi, pointer modelinde ise yalnizca mevcut `Sarki` nesnesinin adresinin diziye eklenmesidir. Bu fark teorik olarak zaten aciktir, fakat proje geregi bu farkin sayisal olarak kanitlanmasi gerekir.

### Olcum Sonuclari

Olcumler `N_SARKI = 10000`, `N_LISTE = 5000`, `SARKI_PER_LISTE = 50` ile alindi.

| Metrik | Kopya | Pointer | Fark |
|---|---|---|---|
| malloc sayisi | 270001 | 20001 | 13.50x daha az |
| Bellek (MB) | 82.13 MB | 3.93 MB | 20.90x daha az |
| Sure (ms) | 23.664 ms | 5.129 ms | 4.61x daha hizli |

Byte degerleri:

- Kopya modeli: `86120000` byte
- Pointer modeli: `4120000` byte

### Yorum
Olcum sonucu hipotezi dogruladi. Kopya modelinde 10.000 sarkilik havuz disinda, `5000 x 50 = 250000` adet ek sarki kopyasi uretilmistir. Bu yuzden toplam `270001` malloc gorulmustur. Pointer modelinde ise her sarki yalnizca bir kez uretildi ve listelere sadece adresleri eklendi; bu yuzden toplam `20001` malloc yeterli oldu. Bellek farki ozellikle belirgindir: 82.13 MB yerine 3.93 MB kullanildi. Bu yaklasik `20.9x` iyilesme demektir. Hiz tarafinda da pointer modeli avantaj sagladi, cunku kopya modelindeki her ekleme tam `Sarki` kopyalama maliyeti tasimaktadir. Sonuc olarak sistemin ana tasarim karari pointer modeli olmalidir. Bu proje icin "pointer daha iyi cunku daha kucuk" demek yeterli degildir; dogru ifade sudur: ayni senaryoda kopya modeli 82.13 MB, pointer modeli 3.93 MB harcamistir. Dolayisiyla pointer kullanimi soyut bir tercih degil, olculmus bir muhendislik kararidir.

## 2. Linked List vs Hash Map

### Benchmark Tablosu

Asagidaki olcumler normal calistirma sirasinda, `1000` arama sorgusu ile alinmistir:

| N | Linked List | Hash Map | Fark |
|---|---|---|---|
| 100 | 0.077 ms | 0.011 ms | 7.0x |
| 1.000 | 0.817 ms | 0.013 ms | 62.8x |
| 10.000 | 8.837 ms | 0.216 ms | 40.9x |
| 100.000 | 300.144 ms | 3.138 ms | 95.6x |

Valgrind altindaki benchmark da ayni egilimi korudu ve `ERROR SUMMARY: 0 errors` verdi. Valgrind ciktisi `rapor/valgrind/benchmark.txt` dosyasina kaydedildi.

### Yorum
Kucuk veri boyutunda bile `HashMap` daha hizlidir, ancak fark veri buyudukce dramatik hale gelir. `100` sarkida fark yalnizca `7x` iken `100000` sarkida `95.6x` seviyesine cikmistir. Sebep aciktir: `Linked List` arama icin ortalama olarak listenin yarisi kadar dugum dolasir, yani zaman karmasikligi `O(n)` olur. `HashMap` ise once kovayi hesaplar, sonra yalnizca ilgili zincirde arama yapar. Bu nedenle ortalama durumda `O(1)` davranisina yaklasir. Mevcut sabit `TABLO_BOYUTU = 1024` nedeniyle teorik olarak tam saf `O(1)` elde edilmese de pratikte linked list'e gore cok daha iyi sonuc vermistir.

Yine de "hash map her zaman kazanir" ifadesi mutlak degildir. Cok kucuk veri setlerinde linked list implementasyonu daha basit olabilir, ek tablo yapisi gerektirmez ve kod karmasikligi daha dusuktur. Ornegin 5-10 sarkilik bir sistemde hash map kurma maliyeti ve ek bellek ihtiyaci anlamsiz olabilir. Fakat bu projedeki senaryo 100.000 sarki ve yuzbinlerce kullanici hedefledigi icin linked list burada olceklenebilir bir cozum degildir. Bu nedenle rapor karari nettir: buyuk kataloglarda arama icin hash map tercih edilmelidir.

## 3. ref_sayisi Olmasaydi Ne Olurdu?

### Deney
Bu bolumde kontrollu bir hata deneyi yapildi. Ayni `Sarki` nesnesi iki farkli listeye eklendi. Normal tasarimda `sarki_sil()` fonksiyonu `ref_sayisi > 0` ise silmeyi reddeder. Deneyde bu kontrol yokmus gibi davranildi ve sarki zorla `free` edildi. Ardindan ikinci listedeki ayni sarkiya yeniden erisilmeye calisildi. Bu durum tipik bir `use-after-free` hatasidir.

Deney sonucu Valgrind ciktisi `rapor/valgrind/test_ref_hata.txt` dosyasina kaydedildi.

### Valgrind Ciktisi

```text
==395== Invalid read of size 1
==395==    by 0x1092B0: main (temp_ref_hata.c:18)
==395==  Address 0x4a74044 is 4 bytes inside a block of size 328 free'd
==395==    by 0x10A391: izlened_free (bellek_izci.c:19)
==395==    by 0x109292: sarki_sil_hatasi (temp_ref_hata.c:6)
==395==    by 0x109292: main (temp_ref_hata.c:17)
...
==395== ERROR SUMMARY: 25 errors from 4 contexts
```

### Yorum
Bu hata cok tehlikelidir cunku program bazen "calisiyor gibi" gorunebilir. Benim deneyimde ekrana `Shared Track` yazisi basilmaya devam etti, yani kullanici yuzeyden bakinca sorun yok sanabilir. Fakat Valgrind ayni anda `Invalid read` gostermistir. Bunun anlami sudur: program, artik kendisine ait olmayan bir bellek bolgesini okumaktadir. Bu bellek bazen henuz baska veriyle ezilmedigi icin eski icerigi gosterebilir; bazen de programi rastgele cokertebilir. En kotu tarafi, bu davranis deterministik degildir. Bugun calisan kod yarin ayni girdide segfault verebilir. Dolayisiyla `ref_sayisi` kontrolu sadece guzel bir ekleme degil, paylasilan sahiplik modelinin guvenlik mekanizmasidir. Bu kontrol kaldirilirsa sistem sessiz veri bozulmasi ve `use-after-free` hatasina acik hale gelir.

## 4. 10x Buyutme Analizi

### Hesaplama
Senaryo 500.000 kullanicidan 5.000.000 kullaniciya cikarildiginda, her kullanicida 20 liste ve her listede 50 sarki oldugunu varsayarsak sadece sarki referanslari icin gereken ham pointer miktari:

`5.000.000 x 20 x 50 x 8 = 40.000.000.000` byte

Bu deger yaklasik `37.25 GiB` yani ondalik hesapla `40 GB` eder.

Mevcut implementasyonda liste kapasitesi 10'dan baslayip `10 -> 20 -> 40 -> 80` seklinde buyuyor. 50 sarkilik bir liste sonunda kapasite 80 olur. Bu durumda pratik pointer dizisi maliyeti:

`5.000.000 x 20 x 80 x 8 = 64.000.000.000` byte

Bu da yaklasik `59.60 GiB` eder. Yani buyutme analizinde yalnizca mantiksal 50 sarki degil, kapasite stratejisinin de bellek maliyeti oldugu gorulmektedir.

`sizeof(CalmaListesi) = 72` byte olarak olculdu. Ek sorudaki hesap:

`5.000.000 x 20 x 72 = 7.200.000.000` byte

Yani sadece `CalmaListesi` struct'lari yaklasik `6.71 GiB` bellek tuketir.

`HashMap` yapisinin mevcut hali `1024` kova icerir. 64-bit sistemde bu yaklasik `1024 x 8 = 8192` byte, yani sadece `8 KiB` civarindadir. Bu nedenle yalnizca kullanici sayisi 10x buyurse ilk bellek darboazi hash map degil, playlist pointer dizileri ve playlist struct overhead'i olur.

### Mimari Degisiklik Onerileri
Sistemi 10x buyutmek gerekirse ilk degistirecegim sey sabit playlist kapasite stratejisi olurdu. Su an her 50 sarkilik liste 80 elemanlik yer kapliyor. Buyuk olcekte bu ciddi israf demektir. Daha sik paketli bir buyume stratejisi veya farkli bir dinamik dizi yapisi kullanilabilir. Ikinci olarak kullanici/liste metadata'si icin daha kompakt struct tasarimi dusunulmelidir. Ucuncu olarak hash map kova sayisi sabit 1024'ten dinamik hale getirilmelidir. Cunku sarki katalogu da 10x buyurse zincir uzunluklari artar ve arama performansi bozulur. Ozetle 10x kullanici buyumesinde ilk darboaz playlist pointer dizileri olur; 10x katalog buyumesinde ise ikinci darboaz sabit boyutlu hash map tasarimi olur.

## Teslim Oncesi Durum Ozeti

- `test_temel` calisti ve gecti.
- `test_bellek` calisti ve gecti.
- `benchmark` calisti ve gecti.
- `csv_yukle()` gercek Kaggle verisi ile test edildi.
- Valgrind ciktisi kaydedildi:
  - `rapor/valgrind/test_temel.txt`
  - `rapor/valgrind/test_bellek.txt`
  - `rapor/valgrind/benchmark.txt`
  - `rapor/valgrind/test_ref_hata.txt`
- Esas testlerin hepsinde sizinti yok ve `ERROR SUMMARY: 0 errors` sonucu alindi.

## 5. Opsiyonel Gorevler

### Secenek A (+5): `uretec.c` ile veri uretimi

`data/uretec.c` dosyasi PDF'teki istekle uyumlu olacak sekilde genisletildi:

- Sanatci havuzu: **50** adet
- Album havuzu: **100** adet

WSL ortaminda `./uretec` farkli satir sayilari ile calistirilip olusan CSV dosya boyutlari olculdu:

| Satir Sayisi | Dosya Boyutu |
|---|---|
| 100 | 12 KB |
| 1.000 | 114 KB |
| 10.000 | 1.2 MB |
| 100.000 | 12 MB |

Bu sonuc satir sayisi arttikca dosya boyutunun beklenen sekilde lineer buyudugunu gostermektedir.

### Secenek B (+10): Python backend + web dashboard

Opsiyonel web gosterimi icin iki yeni dosya eklendi:

- `backend/server.py`
- `dashboard/index.html`

`server.py`, `ministream.so` kutuphanesini `ctypes` ile yukleyip `deney_json()` fonksiyonundan benchmark verisini alir ve `http://localhost:8765/benchmark` endpoint'i uzerinden JSON olarak sunar. `index.html` ise bu endpoint'i okuyup Chart.js ile bellek/sure/arama karsilastirmalarini grafik halinde gosterir.

Bu kapsamda `src/ministream.c` icindeki `deney_json()` fonksiyonu da placeholder metin donmek yerine gercek benchmark degerlerini JSON formatinda uretir hale getirilmistir.

#ifndef MINISTREAM_H
#define MINISTREAM_H

#include <stddef.h>

#define LISTE_BASLANGIC_KAPASITE 10

typedef struct Sarki {
    int id;
    char baslik[100];
    char sanatci[100];
    char album[100];
    int sure_sn;
    int yil;
    int ref_sayisi;
    struct Sarki* sonraki;
} Sarki;

typedef struct {
    int id;
    char isim[50];
    Sarki** sarkilar;
    int sarki_sayisi;
    int kapasite;
} CalmaListesi;

typedef struct {
    int id;
    char isim[50];
    CalmaListesi** listeler;
    int liste_sayisi;
} Kullanici;

typedef struct HashMap HashMap;

Sarki* sarki_olustur(
    int id,
    const char* baslik,
    const char* sanatci,
    const char* album,
    int sure
);
int sarki_sil(Sarki* sarki);

CalmaListesi* liste_olustur(int id, const char* isim);
int liste_sarki_ekle(CalmaListesi* liste, Sarki* sarki);
void liste_sarki_cikar(CalmaListesi* liste, int idx);
void liste_temizle(CalmaListesi* liste);

Sarki* csv_yukle(const char* dosya_yolu, int limit, int* toplam);

Sarki* veri_uret_liste(int n);
HashMap* veri_uret_map(int n);
void liste_temizle_hepsi(Sarki* bas);

void kopya_modeli_test(int n_sarki, int n_liste, int sarki_per_liste);
void pointer_modeli_test(int n_sarki, int n_liste, int sarki_per_liste);

const char* deney_json(void);

#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "bellek_izci.h"
#include "hash_map.h"
#include "linked_list.h"
#include "ministream.h"

static void metin_kopyala(char* hedef, size_t hedef_boyut, const char* kaynak) {
    if (!hedef || hedef_boyut == 0) {
        return;
    }

    if (!kaynak) {
        hedef[0] = '\0';
        return;
    }

    strncpy(hedef, kaynak, hedef_boyut - 1);
    hedef[hedef_boyut - 1] = '\0';
}

Sarki* sarki_olustur(
    int id,
    const char* baslik,
    const char* sanatci,
    const char* album,
    int sure
) {
    Sarki* s = (Sarki*) izlenen_malloc(sizeof(Sarki));
    if (!s) {
        return NULL;
    }

    s->id = id;
    metin_kopyala(s->baslik, sizeof(s->baslik), baslik);
    metin_kopyala(s->sanatci, sizeof(s->sanatci), sanatci);
    metin_kopyala(s->album, sizeof(s->album), album);
    s->sure_sn = sure;
    s->yil = 0;
    s->ref_sayisi = 0;
    s->sonraki = NULL;

    return s;
}

int sarki_sil(Sarki* sarki) {
    if (!sarki) {
        return -1;
    }

    if (sarki->ref_sayisi > 0) {
        printf("UYARI: \"%s\" hala %d listede kullaniliyor!\n", sarki->baslik, sarki->ref_sayisi);
        return -1;
    }

    izlened_free(sarki, sizeof(Sarki));
    return 0;
}

CalmaListesi* liste_olustur(int id, const char* isim) {
    CalmaListesi* liste = (CalmaListesi*) izlenen_malloc(sizeof(CalmaListesi));
    if (!liste) {
        return NULL;
    }

    liste->id = id;
    metin_kopyala(liste->isim, sizeof(liste->isim), isim);
    liste->sarki_sayisi = 0;
    liste->kapasite = LISTE_BASLANGIC_KAPASITE;
    liste->sarkilar = (Sarki**) izlenen_malloc(sizeof(Sarki*) * (size_t) liste->kapasite);

    if (!liste->sarkilar) {
        izlened_free(liste, sizeof(CalmaListesi));
        return NULL;
    }

    return liste;
}

int liste_sarki_ekle(CalmaListesi* liste, Sarki* sarki) {
    if (!liste || !sarki) {
        return -1;
    }

    if (liste->sarki_sayisi >= liste->kapasite) {
        int yeni_kapasite = liste->kapasite * 2;
        Sarki** yeni_dizi = (Sarki**) realloc(liste->sarkilar, sizeof(Sarki*) * (size_t) yeni_kapasite);
        if (!yeni_dizi) {
            return -1;
        }
        liste->sarkilar = yeni_dizi;
        liste->kapasite = yeni_kapasite;
    }

    liste->sarkilar[liste->sarki_sayisi++] = sarki;
    sarki->ref_sayisi++;
    return 0;
}

void liste_sarki_cikar(CalmaListesi* liste, int idx) {
    if (!liste || idx < 0 || idx >= liste->sarki_sayisi) {
        return;
    }

    Sarki* cikarilan = liste->sarkilar[idx];
    if (cikarilan && cikarilan->ref_sayisi > 0) {
        cikarilan->ref_sayisi--;
    }

    int son_idx = liste->sarki_sayisi - 1;
    liste->sarkilar[idx] = liste->sarkilar[son_idx];
    liste->sarkilar[son_idx] = NULL;
    liste->sarki_sayisi--;
}

void liste_temizle(CalmaListesi* liste) {
    if (!liste) {
        return;
    }

    for (int i = 0; i < liste->sarki_sayisi; i++) {
        Sarki* s = liste->sarkilar[i];
        if (!s) {
            continue;
        }

        if (s->ref_sayisi > 0) {
            s->ref_sayisi--;
        }

        if (s->ref_sayisi == 0) {
            sarki_sil(s);
        }
    }

    izlened_free(liste->sarkilar, sizeof(Sarki*) * LISTE_BASLANGIC_KAPASITE);
    liste->sarkilar = NULL;
    izlened_free(liste, sizeof(CalmaListesi));
}

static void csv_alan_oku(const char* satir, int alan_no, char* hedef, int max_uzunluk) {
    int alan = 0;
    int i = 0;
    int tirnak_icinde = 0;

    while (alan < alan_no && satir[i] != '\0') {
        if (satir[i] == '"') {
            tirnak_icinde = !tirnak_icinde;
        } else if (satir[i] == ',' && !tirnak_icinde) {
            alan++;
        }
        i++;
    }

    int j = 0;
    tirnak_icinde = 0;

    while (satir[i] != '\0' && j < max_uzunluk - 1) {
        if (satir[i] == '"') {
            tirnak_icinde = !tirnak_icinde;
            i++;
            continue;
        }

        if (satir[i] == ',' && !tirnak_icinde) {
            break;
        }

        if (satir[i] == '\n' || satir[i] == '\r') {
            break;
        }

        hedef[j++] = satir[i++];
    }

    hedef[j] = '\0';
}

Sarki* csv_yukle(const char* dosya_yolu, int limit, int* toplam) {
    if (!dosya_yolu || !toplam || limit <= 0) {
        return NULL;
    }

    FILE* f = fopen(dosya_yolu, "r");
    if (!f) {
        printf("HATA: %s acilamadi!\n", dosya_yolu);
        return NULL;
    }

    char satir[4096];
    if (!fgets(satir, sizeof(satir), f)) {
        fclose(f);
        return NULL;
    }

    Sarki* bas = NULL;
    *toplam = 0;

    while (fgets(satir, sizeof(satir), f) && *toplam < limit) {
        Sarki* s = (Sarki*) izlenen_malloc(sizeof(Sarki));
        if (!s) {
            break;
        }

        s->id = *toplam;
        s->ref_sayisi = 0;
        s->sonraki = bas;

        csv_alan_oku(satir, 1, s->baslik, (int) sizeof(s->baslik));
        csv_alan_oku(satir, 2, s->album, (int) sizeof(s->album));
        csv_alan_oku(satir, 4, s->sanatci, (int) sizeof(s->sanatci));

        char gecici[32];
        csv_alan_oku(satir, 20, gecici, (int) sizeof(gecici));
        s->sure_sn = atoi(gecici) / 1000;

        csv_alan_oku(satir, 22, gecici, (int) sizeof(gecici));
        s->yil = atoi(gecici);

        bas = s;
        (*toplam)++;
    }

    fclose(f);
    printf("%d sarki yuklendi: %s\n", *toplam, dosya_yolu);
    return bas;
}

Sarki* veri_uret_liste(int n) {
    if (n <= 0) {
        return NULL;
    }

    Sarki* bas = NULL;

    for (int i = 0; i < n; i++) {
        char baslik[100];
        char sanatci[100];
        char album[100];

        snprintf(baslik, sizeof(baslik), "Track_%06d", i);
        snprintf(sanatci, sizeof(sanatci), "Sanatci_%03d", i % 200);
        snprintf(album, sizeof(album), "Album_%03d", i % 300);

        Sarki* s = sarki_olustur(i, baslik, sanatci, album, 120 + (i % 240));
        if (!s) {
            continue;
        }
        s->yil = 1990 + (i % 35);

        s->sonraki = bas;
        bas = s;
    }

    return bas;
}

HashMap* veri_uret_map(int n) {
    if (n <= 0) {
        return NULL;
    }

    HashMap* map = hashmap_olustur();
    if (!map) {
        return NULL;
    }

    for (int i = 0; i < n; i++) {
        char baslik[100];
        char sanatci[100];
        char album[100];

        snprintf(baslik, sizeof(baslik), "Track_%06d", i);
        snprintf(sanatci, sizeof(sanatci), "Sanatci_%03d", i % 200);
        snprintf(album, sizeof(album), "Album_%03d", i % 300);

        Sarki* s = sarki_olustur(i, baslik, sanatci, album, 120 + (i % 240));
        if (!s) {
            continue;
        }
        s->yil = 1990 + (i % 35);
        hashmap_ekle(map, s);
    }

    return map;
}

static double ms_olc(clock_t basla, clock_t bitis) {
    return ((double) (bitis - basla) * 1000.0) / CLOCKS_PER_SEC;
}

void kopya_modeli_test(int n_sarki, int n_liste, int sarki_per_liste) {
    if (n_sarki <= 0 || n_liste <= 0 || sarki_per_liste <= 0) {
        return;
    }

    srand(42);
    Sarki** havuz = (Sarki**) izlenen_malloc(sizeof(Sarki*) * (size_t) n_sarki);
    if (!havuz) {
        return;
    }

    for (int i = 0; i < n_sarki; i++) {
        char baslik[100];
        snprintf(baslik, sizeof(baslik), "Havuz_%06d", i);
        havuz[i] = sarki_olustur(i, baslik, "Sanatci", "Album", 180);
    }

    clock_t t1 = clock();

    for (int i = 0; i < n_liste; i++) {
        CalmaListesi* liste = liste_olustur(i, "Liste");
        if (!liste) {
            continue;
        }

        for (int j = 0; j < sarki_per_liste; j++) {
            Sarki* kaynak = havuz[rand() % n_sarki];
            if (!kaynak) {
                continue;
            }

            if (liste->sarki_sayisi >= liste->kapasite) {
                int yeni_kapasite = liste->kapasite * 2;
                Sarki** yeni_dizi = (Sarki**) realloc(liste->sarkilar, sizeof(Sarki*) * (size_t) yeni_kapasite);
                if (!yeni_dizi) {
                    break;
                }
                liste->sarkilar = yeni_dizi;
                liste->kapasite = yeni_kapasite;
            }

            Sarki* kopya = (Sarki*) izlenen_malloc(sizeof(Sarki));
            if (!kopya) {
                continue;
            }

            memcpy(kopya, kaynak, sizeof(Sarki));
            kopya->ref_sayisi = 0;
            kopya->sonraki = NULL;

            liste->sarkilar[liste->sarki_sayisi++] = kopya;
        }

        for (int j = 0; j < liste->sarki_sayisi; j++) {
            izlened_free(liste->sarkilar[j], sizeof(Sarki));
            liste->sarkilar[j] = NULL;
        }

        izlened_free(liste->sarkilar, sizeof(Sarki*) * LISTE_BASLANGIC_KAPASITE);
        izlened_free(liste, sizeof(CalmaListesi));
    }

    for (int i = 0; i < n_sarki; i++) {
        if (havuz[i]) {
            sarki_sil(havuz[i]);
        }
    }
    izlened_free(havuz, sizeof(Sarki*) * (size_t) n_sarki);

    clock_t t2 = clock();
    printf("Sure: %.3f ms\n", ms_olc(t1, t2));
}

void pointer_modeli_test(int n_sarki, int n_liste, int sarki_per_liste) {
    if (n_sarki <= 0 || n_liste <= 0 || sarki_per_liste <= 0) {
        return;
    }

    srand(42);
    Sarki** havuz = (Sarki**) izlenen_malloc(sizeof(Sarki*) * (size_t) n_sarki);
    if (!havuz) {
        return;
    }

    for (int i = 0; i < n_sarki; i++) {
        char baslik[100];
        snprintf(baslik, sizeof(baslik), "Havuz_%06d", i);
        havuz[i] = sarki_olustur(i, baslik, "Sanatci", "Album", 180);
        if (havuz[i]) {
            havuz[i]->ref_sayisi = 1;
        }
    }

    clock_t t1 = clock();

    for (int i = 0; i < n_liste; i++) {
        CalmaListesi* liste = liste_olustur(i, "Liste");
        if (!liste) {
            continue;
        }

        for (int j = 0; j < sarki_per_liste; j++) {
            Sarki* secilen = havuz[rand() % n_sarki];
            if (!secilen) {
                continue;
            }
            if (liste_sarki_ekle(liste, secilen) != 0) {
                break;
            }
        }

        liste_temizle(liste);
    }

    for (int i = 0; i < n_sarki; i++) {
        if (havuz[i]) {
            if (havuz[i]->ref_sayisi > 0) {
                havuz[i]->ref_sayisi--;
            }
            sarki_sil(havuz[i]);
            havuz[i] = NULL;
        }
    }
    izlened_free(havuz, sizeof(Sarki*) * (size_t) n_sarki);

    clock_t t2 = clock();
    printf("Sure: %.3f ms\n", ms_olc(t1, t2));
}

const char* deney_json(void) {
    static char json[2048];

    /* --- Kopya modeli --- */
    izci_sifirla();
    clock_t k1 = clock();
    kopya_modeli_test(1000, 500, 50);
    clock_t k2 = clock();
    int kopya_malloc  = izci_malloc_sayisi();
    size_t kopya_byte = izci_toplam_ayrildi();
    double kopya_ms   = ms_olc(k1, k2);

    /* --- Pointer modeli --- */
    izci_sifirla();
    clock_t p1 = clock();
    pointer_modeli_test(1000, 500, 50);
    clock_t p2 = clock();
    int ptr_malloc  = izci_malloc_sayisi();
    size_t ptr_byte = izci_toplam_ayrildi();
    double ptr_ms   = ms_olc(p1, p2);

    /* --- Arama benchmark (N=1000) --- */
    srand(42);
    izci_sifirla();
    Sarki* bas = veri_uret_liste(1000);
    clock_t s1 = clock();
    for (int i = 0; i < 1000; i++) sarki_ara_liste(bas, rand() % 1000);
    clock_t s2 = clock();
    double liste_ms = ms_olc(s1, s2);
    liste_temizle_hepsi(bas);

    izci_sifirla();
    HashMap* map = veri_uret_map(1000);
    clock_t m1 = clock();
    for (int i = 0; i < 1000; i++) sarki_ara_map(map, rand() % 1000);
    clock_t m2 = clock();
    double map_ms = ms_olc(m1, m2);
    hashmap_temizle(map);

    double bellek_farki = ptr_byte > 0 ? (double)kopya_byte / ptr_byte : 0.0;
    double hiz_farki    = ptr_ms  > 0  ? kopya_ms / ptr_ms  : 0.0;
    double arama_fark   = map_ms  > 0  ? liste_ms / map_ms  : 0.0;

    snprintf(
        json, sizeof(json),
        "{"
          "\"kopya\":{\"malloc_sayisi\":%d,\"toplam_byte\":%zu,\"sure_ms\":%.3f},"
          "\"pointer\":{\"malloc_sayisi\":%d,\"toplam_byte\":%zu,\"sure_ms\":%.3f},"
          "\"bellek_farki\":%.1f,"
          "\"hiz_farki\":%.1f,"
          "\"arama\":{\"n\":1000,\"linked_list_ms\":%.3f,\"hash_map_ms\":%.3f,\"fark_x\":%.1f}"
        "}",
        kopya_malloc, kopya_byte, kopya_ms,
        ptr_malloc,   ptr_byte,   ptr_ms,
        bellek_farki,
        hiz_farki,
        liste_ms, map_ms, arama_fark
    );

    return json;
}

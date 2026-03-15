#include <stdio.h>
#include <string.h>

#include "../src/bellek_izci.h"
#include "../src/ministream.h"

#define TEST(isim, kosul)                                                                                               \
    do {                                                                                                                \
        if (kosul) {                                                                                                    \
            printf("\u221a %s\n", isim);                                                                              \
        } else {                                                                                                        \
            printf("x %s - BASARISIZ\n", isim);                                                                       \
        }                                                                                                               \
    } while (0)

static void test_sarki_olustur(void) {
    printf("\n[TEST] sarki_olustur\n");
    izci_sifirla();

    Sarki* s = sarki_olustur(1, "Bohemian Rhapsody", "Queen", "A Night at the Opera", 355);

    TEST("sarki NULL degil", s != NULL);
    TEST("id dogru", s && s->id == 1);
    TEST("baslik dogru", s && strcmp(s->baslik, "Bohemian Rhapsody") == 0);
    TEST("ref_sayisi baslangicta 0", s && s->ref_sayisi == 0);
    TEST("1 malloc yapildi", izci_malloc_sayisi() == 1);

    izlened_free(s, sizeof(Sarki));
    TEST("SIZINTI yok", aktif_bellek() == 0);
}

static void test_liste_sarki_ekle(void) {
    printf("\n[TEST] liste_sarki_ekle\n");
    izci_sifirla();

    CalmaListesi* liste = liste_olustur(1, "Favoriler");
    Sarki* s1 = sarki_olustur(1, "Track A", "Sanatci", "Album", 180);
    Sarki* s2 = sarki_olustur(2, "Track B", "Sanatci", "Album", 200);

    liste_sarki_ekle(liste, s1);
    liste_sarki_ekle(liste, s2);

    TEST("2 sarki eklendi", liste && liste->sarki_sayisi == 2);
    TEST("s1 ref_sayisi artti", s1->ref_sayisi == 1);
    TEST("s2 ref_sayisi artti", s2->ref_sayisi == 1);
    TEST("pointer kopyalandi", liste->sarkilar[0] == s1);

    liste_temizle(liste);
    TEST("SIZINTI yok", aktif_bellek() == 0);
}

static void test_ref_sayisi(void) {
    printf("\n[TEST] ref_sayisi\n");
    izci_sifirla();

    Sarki* s = sarki_olustur(1, "Shared Track", "Sanatci", "Album", 210);
    CalmaListesi* l1 = liste_olustur(1, "Liste 1");
    CalmaListesi* l2 = liste_olustur(2, "Liste 2");

    liste_sarki_ekle(l1, s);
    liste_sarki_ekle(l2, s);

    TEST("2 listede ref_sayisi == 2", s->ref_sayisi == 2);

    liste_sarki_cikar(l1, 0);
    TEST("1 cikarinca ref_sayisi == 1", s->ref_sayisi == 1);

    int sonuc = sarki_sil(s);
    TEST("ref_sayisi>0 iken silinemez", sonuc == -1);

    liste_temizle(l1);
    liste_temizle(l2);
    TEST("SIZINTI yok", aktif_bellek() == 0);
}

int main(void) {
    printf("===== MiniStream Birim Testleri =====\n");
    test_sarki_olustur();
    test_liste_sarki_ekle();
    test_ref_sayisi();
    printf("\n===============================\n");
    return 0;
}

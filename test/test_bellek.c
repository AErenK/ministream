#include <stdio.h>

#include "../src/bellek_izci.h"
#include "../src/ministream.h"

static void test_toplu_olustur_sil(void) {
    printf("\n[BELLEK TEST] toplu olustur/sil\n");
    izci_sifirla();

    for (int i = 0; i < 10000; i++) {
        Sarki* s = sarki_olustur(i, "Test", "Sanatci", "Album", 200);
        izlened_free(s, sizeof(Sarki));
    }

    bellek_raporu_yazdir();
}

static void test_liste_bellek(void) {
    printf("\n[BELLEK TEST] liste bellek yonetimi\n");
    izci_sifirla();

    for (int i = 0; i < 100; i++) {
        CalmaListesi* l = liste_olustur(i, "Liste");
        for (int j = 0; j < 50; j++) {
            int id = i * 1000 + j;
            Sarki* s = sarki_olustur(id, "Track", "Artist", "Album", 180);
            liste_sarki_ekle(l, s);
        }
        liste_temizle(l);
    }

    bellek_raporu_yazdir();
}

int main(void) {
    printf("===== MiniStream Bellek Testleri =====\n");
    test_toplu_olustur_sil();
    test_liste_bellek();
    printf("\n===============================\n");
    return 0;
}

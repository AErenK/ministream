#include <stdio.h>

#include "../src/bellek_izci.h"
#include "../src/ministream.h"

static void sarki_sil_hatasi(Sarki* sarki) {
    izlened_free(sarki, sizeof(Sarki));
}

int main(void) {
    izci_sifirla();

    Sarki* ortak = sarki_olustur(1, "Shared Track", "Sanatci", "Album", 200);
    CalmaListesi* l1 = liste_olustur(1, "Liste 1");
    CalmaListesi* l2 = liste_olustur(2, "Liste 2");

    liste_sarki_ekle(l1, ortak);
    liste_sarki_ekle(l2, ortak);

    printf("ref_sayisi kontrolu yokmus gibi zorla free ediliyor...\n");
    sarki_sil_hatasi(ortak);

    printf("Ikinci listeden erisim: %s\n", l2->sarkilar[0]->baslik);

    l1->sarki_sayisi = 0;
    l2->sarki_sayisi = 0;
    liste_temizle(l1);
    liste_temizle(l2);
    return 0;
}

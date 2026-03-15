#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../src/bellek_izci.h"
#include "../src/hash_map.h"
#include "../src/linked_list.h"
#include "../src/ministream.h"

static double ms_olc_clock(clock_t t1, clock_t t2) {
    return ((double) (t2 - t1) * 1000.0) / CLOCKS_PER_SEC;
}

static void benchmark_arama(int n_sarki) {
    srand(42);

    izci_sifirla();
    Sarki* bas = veri_uret_liste(n_sarki);
    clock_t t1 = clock();
    for (int i = 0; i < 1000; i++) {
        sarki_ara_liste(bas, rand() % n_sarki);
    }
    clock_t t2 = clock();
    double liste_ms = ms_olc_clock(t1, t2);
    liste_temizle_hepsi(bas);

    izci_sifirla();
    HashMap* map = veri_uret_map(n_sarki);
    t1 = clock();
    for (int i = 0; i < 1000; i++) {
        sarki_ara_map(map, rand() % n_sarki);
    }
    t2 = clock();
    double map_ms = ms_olc_clock(t1, t2);
    hashmap_temizle(map);

    printf(
        "| %8d | %12.3f ms | %12.3f ms | %8.1fx |\n",
        n_sarki,
        liste_ms,
        map_ms,
        (map_ms > 0.0) ? (liste_ms / map_ms) : 0.0
    );
}

static void benchmark_model_karsilastirma(void) {
    int n_sarki = 10000;
    int n_liste = 5000;
    int sarki_per_liste = 50;

    printf("\n=== KOPYA MODELI ===\n");
    izci_sifirla();
    kopya_modeli_test(n_sarki, n_liste, sarki_per_liste);
    bellek_raporu_yazdir();

    printf("\n=== POINTER MODELI ===\n");
    izci_sifirla();
    pointer_modeli_test(n_sarki, n_liste, sarki_per_liste);
    bellek_raporu_yazdir();
}

int main(void) {
    printf("\n=== ARAMA BENCHMARK (1000 sorgu) ===\n");
    printf("|  N sarki |   LinkedList   |    HashMap     |   Fark   |\n");
    printf("|----------|----------------|----------------|----------|\n");

    int boyutlar[] = {100, 1000, 10000, 100000};
    for (int i = 0; i < 4; i++) {
        benchmark_arama(boyutlar[i]);
    }

    benchmark_model_karsilastirma();
    return 0;
}

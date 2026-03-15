#ifndef BELLEK_IZCI_H
#define BELLEK_IZCI_H

#include <stddef.h>

typedef struct {
    size_t toplam_ayrildi;
    size_t toplam_serbest;
    int malloc_sayisi;
    int free_sayisi;
} BellekIzci;

void* izlenen_malloc(size_t boyut);
void izlened_free(void* ptr, size_t boyut);
void izci_sifirla(void);
void bellek_raporu_yazdir(void);
size_t aktif_bellek(void);

int izci_malloc_sayisi(void);
int izci_free_sayisi(void);
size_t izci_toplam_ayrildi(void);
size_t izci_toplam_serbest(void);

#endif

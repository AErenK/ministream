#ifndef HASH_MAP_H
#define HASH_MAP_H

#include "ministream.h"

#define TABLO_BOYUTU 1024

struct HashMap {
    Sarki* kovalar[TABLO_BOYUTU];
};

int hash_fonk(int id);
HashMap* hashmap_olustur(void);
void hashmap_ekle(HashMap* map, Sarki* sarki);
Sarki* sarki_ara_map(HashMap* map, int id);
void hashmap_temizle(HashMap* map);

#endif

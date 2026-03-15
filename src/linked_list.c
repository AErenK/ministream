#include "bellek_izci.h"
#include "linked_list.h"

Sarki* sarki_ara_liste(Sarki* bas, int id) {
    Sarki* curr = bas;
    while (curr != NULL) {
        if (curr->id == id) {
            return curr;
        }
        curr = curr->sonraki;
    }
    return NULL;
}

void liste_temizle_hepsi(Sarki* bas) {
    Sarki* curr = bas;
    while (curr) {
        Sarki* sonraki = curr->sonraki;
        izlened_free(curr, sizeof(Sarki));
        curr = sonraki;
    }
}

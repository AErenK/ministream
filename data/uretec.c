#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static const char* SANATCILAR[] = {
    "Radiohead", "Daft Punk", "Portishead", "Massive Attack", "Moderat", "Nils Frahm",
    "Bonobo", "Tycho", "Caribou", "Nicolas Jaar", "James Blake", "Burial", "Autechre",
    "Aphex Twin", "Boards of Canada", "Four Tet", "Kiasmos", "The xx", "UNKLE", "Moby"
};

static const char* ALBUMLER[] = {
    "OK Computer", "Discovery", "Dummy", "Mezzanine", "There Is Love in You",
    "Selected Ambient Works", "Immunity", "Settle", "Untrue", "Play"
};

static void veri_uret(const char* dosya, int n) {
    FILE* f = fopen(dosya, "w");
    if (!f) {
        printf("HATA: %s acilamadi\n", dosya);
        return;
    }

    fprintf(f, "id,name,album,album_id,artists,artist_ids,track_number,disc_number,explicit,danceability,energy,key,loudness,mode,speechiness,acousticness,instrumentalness,liveness,valence,tempo,duration_ms,time_signature,year,release_date\n");

    for (int i = 0; i < n; i++) {
        const char* sanatci = SANATCILAR[rand() % (int) (sizeof(SANATCILAR) / sizeof(SANATCILAR[0]))];
        const char* album = ALBUMLER[rand() % (int) (sizeof(ALBUMLER) / sizeof(ALBUMLER[0]))];
        int sure_ms = (120 + rand() % 240) * 1000;
        int yil = 1990 + rand() % 36;

        fprintf(
            f,
            "id_%06d,Track_%06d,%s,alb_%03d,%s,art_%03d,1,1,false,0,0,0,0,0,0,0,0,0,0,0,%d,4,%d,2000-01-01\n",
            i,
            i,
            album,
            i % 100,
            sanatci,
            i % 50,
            sure_ms,
            yil
        );
    }

    fclose(f);
    printf("%d satir uretildi -> %s\n", n, dosya);
}

int main(int argc, char* argv[]) {
    int n = (argc > 1) ? atoi(argv[1]) : 100000;
    srand((unsigned int) time(NULL));
    veri_uret("data/sarkilar.csv", n);
    return 0;
}

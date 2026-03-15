# MiniStream

Bu repo, Algoritma ve Tasarim dersi kapsaminda hazirlanan `MiniStream` odev teslimidir.

## Icerik

- `src/` : Ana C kaynak kodlari
- `test/` : Birim testleri ve benchmark kodlari
- `data/` : Veri dosyalari ve `uretec.c`
- `backend/` : Opsiyonel Python backend
- `dashboard/` : Opsiyonel web arayuzu
- `rapor/` : Teslim raporu ve Valgrind ciktlari

## Teslim Dosyalari

- Ana rapor: `rapor/RAPOR.md`
- Valgrind ciktlari: `rapor/valgrind/`

## Derleme

```bash
make all
make test
make valgrind
```

## Opsiyonel Kisimlar

- `data/uretec.c` ile veri uretimi
- `backend/server.py` ile benchmark JSON servisi
- `dashboard/index.html` ile web gosterimi

## Not

Bu repo icin esas aciklama ve analizler `rapor/RAPOR.md` dosyasindadir.

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -g

SRC = src/ministream.c src/bellek_izci.c src/hash_map.c src/linked_list.c

all: test_temel benchmark

test_temel: $(SRC) test/test_temel.c
	$(CC) $(CFLAGS) $(SRC) test/test_temel.c -o test_temel

test_bellek: $(SRC) test/test_bellek.c
	$(CC) $(CFLAGS) $(SRC) test/test_bellek.c -o test_bellek

benchmark: $(SRC) test/benchmark.c
	$(CC) $(CFLAGS) $(SRC) test/benchmark.c -o benchmark

ministream.so: $(SRC)
	$(CC) -shared -fPIC $(CFLAGS) $(SRC) -o ministream.so

test: test_temel test_bellek
	./test_temel
	./test_bellek

valgrind: test_temel
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./test_temel

# Opsiyonel: veri ureticisi
uretec: data/uretec.c
	$(CC) $(CFLAGS) data/uretec.c -o uretec

# Opsiyonel: Python backend
backend: ministream.so
	python3 backend/server.py

clean:
	rm -f test_temel test_bellek benchmark ministream.so uretec

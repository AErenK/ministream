"""
MiniStream Backend - Python HTTP Wrapper
Makefile ile derlenen ministream.so'yu ctypes ile yukler ve
/benchmark endpoint'ini HTTP uzerinden servis eder.

Calistirmak icin:
    1. make ministream.so
    2. python3 backend/server.py

Test:
    curl http://localhost:8765/benchmark
"""

import ctypes
import csv
import json
import os
import random
import socket
import sqlite3
import threading
from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import parse_qs, urlparse

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SO_PATH = os.path.join(SCRIPT_DIR, "..", "ministream.so")
SO_PATH = os.path.normpath(SO_PATH)
CSV_PATH = os.path.normpath(os.path.join(SCRIPT_DIR, "..", "data", "sarkilar.csv"))
DB_PATH = os.path.normpath(os.path.join(SCRIPT_DIR, "..", "data", "sarkilar.sqlite3"))

lib = None
if os.path.exists(SO_PATH):
    lib = ctypes.CDLL(SO_PATH)
    lib.deney_json.restype = ctypes.c_char_p
    lib.deney_json.argtypes = []
else:
    print(f"UYARI: {SO_PATH} bulunamadi. /benchmark endpointi devre disi kalacak.")


db_lock = threading.Lock()


def _to_int(value, default=0):
    try:
        return int(value)
    except (TypeError, ValueError):
        return default


def _row_to_song(row, song_id):
    title = row.get("track_name") or row.get("name") or row.get("title") or "Bilinmeyen Sarki"
    artist = row.get("artists") or row.get("artist") or "Bilinmeyen Sanatci"
    album = row.get("album_name") or row.get("album") or "Bilinmeyen Album"
    duration_ms = _to_int(row.get("duration_ms"), 0)
    duration = max(1, duration_ms // 1000) if duration_ms > 0 else 180

    return {
        "id": song_id,
        "title": title.strip()[:120],
        "artist": artist.strip()[:120],
        "album": album.strip()[:120],
        "duration": duration,
    }


def _connect_db():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn


def _db_is_ready():
    if not os.path.exists(DB_PATH) or not os.path.exists(CSV_PATH):
        return False

    return os.path.getmtime(DB_PATH) >= os.path.getmtime(CSV_PATH)


def _tokenize_query(query):
    tokens = [token for token in query.lower().split() if token]
    return " ".join(f'"{token}"*' for token in tokens)


def _ensure_song_database():
    if _db_is_ready():
        return

    with db_lock:
        if _db_is_ready():
            return

        if not os.path.exists(CSV_PATH):
            return

        if os.path.exists(DB_PATH):
            os.remove(DB_PATH)

        print("[DB] sarki veritabani olusturuluyor...", flush=True)
        conn = sqlite3.connect(DB_PATH)
        try:
            conn.execute("PRAGMA journal_mode = WAL")
            conn.execute("PRAGMA synchronous = NORMAL")
            conn.execute("PRAGMA temp_store = MEMORY")
            conn.execute(
                """
                CREATE TABLE songs (
                    id INTEGER PRIMARY KEY,
                    title TEXT NOT NULL,
                    artist TEXT NOT NULL,
                    album TEXT NOT NULL,
                    duration INTEGER NOT NULL
                )
                """
            )
            conn.execute(
                """
                CREATE VIRTUAL TABLE songs_fts USING fts5(
                    title,
                    artist,
                    album,
                    content='songs',
                    content_rowid='id',
                    tokenize='unicode61 remove_diacritics 2'
                )
                """
            )

            with open(CSV_PATH, "r", encoding="utf-8", errors="ignore", newline="") as f:
                reader = csv.DictReader(f)
                rows = []
                for i, row in enumerate(reader, start=1):
                    song = _row_to_song(row, i)
                    rows.append((song["id"], song["title"], song["artist"], song["album"], song["duration"]))

                    if len(rows) >= 5000:
                        conn.executemany(
                            "INSERT INTO songs (id, title, artist, album, duration) VALUES (?, ?, ?, ?, ?)",
                            rows,
                        )
                        rows.clear()

                if rows:
                    conn.executemany(
                        "INSERT INTO songs (id, title, artist, album, duration) VALUES (?, ?, ?, ?, ?)",
                        rows,
                    )

            conn.execute(
                "INSERT INTO songs_fts(rowid, title, artist, album) SELECT id, title, artist, album FROM songs"
            )
            conn.execute("CREATE INDEX idx_songs_title ON songs(title)")
            conn.commit()
        finally:
            conn.close()

        print("[DB] sarki veritabani hazir.", flush=True)


def _rows_to_songs(rows):
    return [
        {
            "id": row["id"],
            "title": row["title"],
            "artist": row["artist"],
            "album": row["album"],
            "duration": row["duration"],
        }
        for row in rows
    ]


def _load_songs(limit=200, offset=0):
    _ensure_song_database()
    with _connect_db() as conn:
        rows = conn.execute(
            "SELECT id, title, artist, album, duration FROM songs ORDER BY id LIMIT ? OFFSET ?",
            (limit, offset),
        ).fetchall()
    return _rows_to_songs(rows)


def _load_random_songs(limit=50):
    _ensure_song_database()
    with _connect_db() as conn:
        row = conn.execute("SELECT MAX(id) AS max_id FROM songs").fetchone()
        max_id = row["max_id"] if row else 0
        if not max_id:
            return []

        sample_size = min(limit, max_id)
        random_ids = sorted(random.sample(range(1, max_id + 1), sample_size))
        placeholders = ",".join("?" for _ in random_ids)
        rows = conn.execute(
            f"SELECT id, title, artist, album, duration FROM songs WHERE id IN ({placeholders})",
            random_ids,
        ).fetchall()

    songs = _rows_to_songs(rows)
    random.shuffle(songs)
    return songs[:limit]


def _search_songs(query, limit=100, offset=0):
    _ensure_song_database()
    needle = query.lower().strip()
    if not needle:
        return [], 0

    match_query = _tokenize_query(needle)
    if not match_query:
        return [], 0

    with _connect_db() as conn:
        total_row = conn.execute(
            "SELECT COUNT(*) AS total FROM songs_fts WHERE songs_fts MATCH ?",
            (match_query,),
        ).fetchone()
        rows = conn.execute(
            """
            SELECT songs.id, songs.title, songs.artist, songs.album, songs.duration
            FROM songs_fts
            JOIN songs ON songs.id = songs_fts.rowid
            WHERE songs_fts MATCH ?
            ORDER BY bm25(songs_fts), songs.id
            LIMIT ? OFFSET ?
            """,
            (match_query, limit, offset),
        ).fetchall()

    return _rows_to_songs(rows), (total_row["total"] if total_row else 0)

class MiniStreamHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        parsed = urlparse(self.path)
        path = parsed.path

        if path == "/benchmark":
            self._serve_benchmark()
        elif path == "/songs":
            self._serve_songs(parsed.query)
        elif path in ("/", "/dashboard"):
            self._serve_redirect()
        else:
            self._send(404, "application/json", b'{"hata":"bulunamadi"}')

    def _serve_benchmark(self):
        if lib is None:
            body = json.dumps({"hata": "ministream.so bulunamadi"}).encode("utf-8")
            self._send(503, "application/json", body)
            return

        print("[GET /benchmark] benchmark calistiriliyor...", flush=True)
        try:
            raw = lib.deney_json()
            parsed = json.loads(raw)
            body = json.dumps(parsed, indent=2).encode("utf-8")
            self._send(200, "application/json", body)
            print("[GET /benchmark] OK", flush=True)
        except Exception as e:
            err = json.dumps({"hata": str(e)}).encode("utf-8")
            self._send(500, "application/json", err)

    def _serve_songs(self, query):
        params = parse_qs(query)
        q = (params.get("q", [""])[0] or "").strip()
        random_mode = (params.get("random", ["0"])[0] or "0").lower() in ("1", "true", "yes")

        limit = _to_int(params.get("limit", ["50" if random_mode else "100"])[0], 50 if random_mode else 100)
        offset = _to_int(params.get("offset", ["0"])[0], 0)
        limit = max(1, min(limit, 2000))
        offset = max(0, offset)

        total = None
        if q:
            songs, total = _search_songs(query=q, limit=limit, offset=offset)
        elif random_mode:
            songs = _load_random_songs(limit=limit)
        else:
            songs = _load_songs(limit=limit, offset=offset)

        body = json.dumps(
            {
                "count": len(songs),
                "offset": offset,
                "limit": limit,
                "total": total,
                "query": q,
                "random": random_mode,
                "songs": songs,
            },
            ensure_ascii=False,
        ).encode("utf-8")
        self._send(200, "application/json; charset=utf-8", body)

    def _serve_redirect(self):
        self.send_response(302)
        self.send_header("Location", "http://localhost:8765/songs")
        self.end_headers()

    def _send(self, code, content_type, body):
        self.send_response(code)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        try:
            self.wfile.write(body)
        except (BrokenPipeError, ConnectionAbortedError, socket.error):
            pass

    def log_message(self, fmt, *args):
        pass

HOST = "localhost"
PORT = 8765

if __name__ == "__main__":
    _ensure_song_database()
    server = HTTPServer((HOST, PORT), MiniStreamHandler)
    print(f"MiniStream backend baslatildi -> http://{HOST}:{PORT}/benchmark")
    print("Durdurmak icin: Ctrl+C")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nSunucu durduruldu.")
        server.server_close()
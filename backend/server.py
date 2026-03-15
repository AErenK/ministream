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
import json
import os
import sys
from http.server import BaseHTTPRequestHandler, HTTPServer

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SO_PATH = os.path.join(SCRIPT_DIR, "..", "ministream.so")
SO_PATH = os.path.normpath(SO_PATH)

if not os.path.exists(SO_PATH):
    print(f"HATA: {SO_PATH} bulunamadi.")
    print("Once 'make ministream.so' calistirin.")
    sys.exit(1)

lib = ctypes.CDLL(SO_PATH)
lib.deney_json.restype = ctypes.c_char_p
lib.deney_json.argtypes = []

class MiniStreamHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == "/benchmark":
            self._serve_benchmark()
        elif self.path in ("/", "/dashboard"):
            self._serve_redirect()
        else:
            self._send(404, "application/json", b'{"hata":"bulunamadi"}')

    def _serve_benchmark(self):
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

    def _serve_redirect(self):
        self.send_response(302)
        self.send_header("Location", "http://localhost:8765/benchmark")
        self.end_headers()

    def _send(self, code, content_type, body):
        self.send_response(code)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, fmt, *args):
        pass

HOST = "localhost"
PORT = 8765

if __name__ == "__main__":
    server = HTTPServer((HOST, PORT), MiniStreamHandler)
    print(f"MiniStream backend baslatildi -> http://{HOST}:{PORT}/benchmark")
    print("Durdurmak icin: Ctrl+C")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nSunucu durduruldu.")
        server.server_close()
import json
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.request import Request, urlopen
from urllib.error import HTTPError, URLError
from pathlib import Path

HOST = "127.0.0.1"
PORT = 8000
OLLAMA = "http://127.0.0.1:11434/api/chat"
ROOT = Path(__file__).resolve().parent

class Handler(BaseHTTPRequestHandler):
    def send_json(self, status, payload):
        data = json.dumps(payload).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(data)

    def do_OPTIONS(self):
        self.send_response(204)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()

    def do_POST(self):
        if self.path != "/api/chat":
            self.send_error(404)
            return
        try:
            length = int(self.headers.get("Content-Length", "0"))
            body = self.rfile.read(length)
            req = Request(OLLAMA, data=body,
                          headers={"Content-Type": "application/json"},
                          method="POST")
            with urlopen(req, timeout=300) as resp:
                data = resp.read()
            self.send_json(200, json.loads(data.decode("utf-8")))
        except HTTPError as e:
            detail = e.read().decode("utf-8", errors="replace")
            self.send_json(e.code, {"error": detail})
        except URLError:
            self.send_json(503, {"error":"Ollama is not running. Start Ollama and make sure gemma3:4b is installed."})
        except Exception as e:
            self.send_json(500, {"error": str(e)})

    def do_GET(self):
        from urllib.parse import urlparse, parse_qs
        if self.path.startswith("/api/research"):
            try:
                from urllib.parse import urlparse, parse_qs, quote
                qs = parse_qs(urlparse(self.path).query)
                q = qs.get("q", [""])[0].strip()
                if not q:
                    self.send_json(400, {"error":"Missing research query"})
                    return
                api = "https://en.wikipedia.org/w/api.php?action=query&list=search&srsearch=" + quote(q) + "&format=json&utf8=1&srlimit=5"
                req = Request(api, headers={"User-Agent":"Venky-Agent/1.0"})
                with urlopen(req, timeout=20) as resp:
                    payload = json.loads(resp.read().decode("utf-8"))
                results = [{"title":x.get("title",""),"snippet":x.get("snippet","")} for x in payload.get("query",{}).get("search",[])]
                self.send_json(200, {"query":q,"results":results})
            except Exception as e:
                self.send_json(502, {"error":"Research request failed: " + str(e)})
            return

        if self.path.startswith("/api/github/files"):
            try:
                qs = parse_qs(urlparse(self.path).query)
                path = qs.get("path", [""])[0]
                api = "https://api.github.com/repos/venkynova/va/contents/" + path
                req = Request(api, headers={"Accept":"application/vnd.github+json","User-Agent":"Venky-Agent"})
                with urlopen(req, timeout=20) as resp:
                    payload = json.loads(resp.read().decode("utf-8"))
                self.send_json(200, payload)
            except Exception as e:
                self.send_json(502, {"error":"GitHub file read failed: " + str(e)})
            return

        if self.path == "/api/github/issues":
            try:
                api = "https://api.github.com/repos/venkynova/va/issues?state=open&per_page=20"
                req = Request(api, headers={"Accept":"application/vnd.github+json","User-Agent":"Venky-Agent"})
                with urlopen(req, timeout=20) as resp:
                    payload = json.loads(resp.read().decode("utf-8"))
                self.send_json(200, payload)
            except Exception as e:
                self.send_json(502, {"error":"GitHub issues read failed: " + str(e)})
            return

        rel = self.path.split("?", 1)[0].lstrip("/") or "index.html"
        file_path = (ROOT / rel).resolve()
        if ROOT not in file_path.parents and file_path != ROOT:
            self.send_error(403)
            return
        if not file_path.is_file():
            self.send_error(404)
            return
        content_type = {
            ".html":"text/html; charset=utf-8",
            ".js":"application/javascript; charset=utf-8",
            ".css":"text/css; charset=utf-8",
            ".json":"application/json; charset=utf-8",
        }.get(file_path.suffix, "application/octet-stream")
        data = file_path.read_bytes()
        self.send_response(200)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

if __name__ == "__main__":
    print(f"Venky Agent running at http://{HOST}:{PORT}")
    print("Keep Ollama running in the background.")
    ThreadingHTTPServer((HOST, PORT), Handler).serve_forever()

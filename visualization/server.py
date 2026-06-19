#!/usr/bin/env python3

from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import argparse
import json
import re
import subprocess
import time


VIS_DIR = Path(__file__).resolve().parent
REPO_ROOT = VIS_DIR.parent
DEFAULT_DATASET = REPO_ROOT / "data" / "links.tsv"
DEFAULT_BINARY = REPO_ROOT / "wikiTraverser"


def clamp_int(value, fallback, minimum, maximum):
    try:
        parsed = int(value)
    except (TypeError, ValueError):
        return fallback
    return max(minimum, min(maximum, parsed))


def clamp_float(value, fallback, minimum, maximum):
    try:
        parsed = float(value)
    except (TypeError, ValueError):
        return fallback
    return max(minimum, min(maximum, parsed))


def clean_title(value, fallback):
    text = str(value or "").strip()
    return text if text else fallback


def safe_output_name(value):
    text = str(value or "").strip()
    if not text:
        return f"graph-custom-{int(time.time())}.json"
    if not text.endswith(".json"):
        text += ".json"
    text = Path(text).name
    return re.sub(r"[^A-Za-z0-9_.-]", "-", text)


class WikiTraverserHandler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(VIS_DIR), **kwargs)

    def send_json(self, status, payload):
        body = json.dumps(payload, indent=2).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_POST(self):
        if self.path != "/api/generate":
            self.send_error(404, "Unknown endpoint")
            return

        try:
            length = int(self.headers.get("Content-Length", "0"))
        except ValueError:
            length = 0

        try:
            payload = json.loads(self.rfile.read(length) or b"{}")
        except json.JSONDecodeError as exc:
            self.send_json(400, {"ok": False, "error": f"Invalid JSON: {exc}"})
            return

        if not DEFAULT_BINARY.exists():
            self.send_json(400, {
                "ok": False,
                "error": "wikiTraverser binary was not found. Run `make` from the project root first."
            })
            return

        nodes = clamp_int(payload.get("nodes"), 320, 20, 2000)
        depth = clamp_int(payload.get("depth"), 2, 1, 5)
        neighbors = clamp_int(payload.get("neighbors"), 36, 1, 200)
        iterations = clamp_int(payload.get("iterations"), 32, 1, 200)
        resolution = clamp_float(payload.get("resolution"), 1.0, 0.05, 8.0)
        start = clean_title(payload.get("start"), "Military_dictatorship")
        end = clean_title(payload.get("end"), "Michael_Jordan")
        output_name = safe_output_name(payload.get("output"))
        output_path = VIS_DIR / output_name

        command = [
            str(DEFAULT_BINARY),
            "export",
            str(DEFAULT_DATASET),
            str(output_path),
            "--start",
            start,
            "--end",
            end,
            "--nodes",
            str(nodes),
            "--depth",
            str(depth),
            "--neighbors",
            str(neighbors),
            "--cluster",
            "louvain",
            "--cluster-resolution",
            f"{resolution:.4f}",
            "--cluster-iterations",
            str(iterations),
        ]

        try:
            result = subprocess.run(
                command,
                cwd=str(REPO_ROOT),
                text=True,
                capture_output=True,
                timeout=120,
                check=False,
            )
        except subprocess.TimeoutExpired:
            self.send_json(504, {"ok": False, "error": "Graph generation timed out."})
            return

        if result.returncode != 0:
            self.send_json(500, {
                "ok": False,
                "error": result.stderr.strip() or result.stdout.strip() or "Graph generation failed.",
                "command": command,
            })
            return

        meta = {}
        try:
            with output_path.open("r", encoding="utf-8") as file:
                meta = json.load(file).get("meta", {})
        except (OSError, json.JSONDecodeError):
            meta = {}

        self.send_json(200, {
            "ok": True,
            "file": output_name,
            "meta": meta,
            "stdout": result.stdout.strip(),
        })


def main():
    parser = argparse.ArgumentParser(description="Serve the WikiTraverser visualizer with graph generation.")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8000)
    args = parser.parse_args()

    host = args.host
    port = args.port
    server = ThreadingHTTPServer((host, port), WikiTraverserHandler)
    print(f"Serving WikiTraverser visualizer at http://{host}:{port}/")
    print(f"Open http://localhost:{port}/ in your browser.")
    server.serve_forever()


if __name__ == "__main__":
    main()

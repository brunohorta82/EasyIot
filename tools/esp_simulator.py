#!/usr/bin/env python3
"""Interactive ESP32/ESP8266 Mock Server & Hardware Simulator for EasyIot / AquaDance.

Run this script to test the full Web Panel, musical partition sequencer, 2D fountain
simulation, REST endpoints, and Server-Sent Events (SSE) without needing physical hardware.

Usage:
    python tools/esp_simulator.py [port]
    Default port: 8080 (http://localhost:8080)
"""

from __future__ import annotations

import json
import mimetypes
import os
from pathlib import Path
import sys
import threading
import time
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse, parse_qs

if hasattr(sys.stdout, "reconfigure"):
    try:
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
        sys.stderr.reconfigure(encoding="utf-8", errors="replace")
    except Exception:
        pass

ROOT = Path(__file__).resolve().parents[1]
WEBPANEL_DIR = ROOT / "webpanel"

# Default simulated hardware configuration
SIMULATED_CONFIG = {
    "chipId": "ESP-SIM-9988",
    "nodeId": "OnOfre Simulador",
    "firmware": "EasyIot v9.199-SIM",
    "wifi": {
        "ssid": "Simulated_WiFi_5G",
        "ip": "192.168.1.150",
        "mask": "255.255.255.0",
        "gw": "192.168.1.1",
        "dns": "1.1.1.1",
        "dhcp": True,
        "rssi": -52
    },
    "mqtt": {
        "server": "192.168.1.100",
        "port": 1883,
        "user": "homeassistant",
        "enabled": True
    },
    "features": [
        {
            "id": "vlv_center",
            "name": "Jato Central (Válvula 1)",
            "driver": "GARDEN_VALVE",
            "state": 0,
            "inputs": [12],
            "outputs": [14]
        },
        {
            "id": "vlv_ring_1",
            "name": "Anel Norte (Válvula 2)",
            "driver": "GARDEN_VALVE",
            "state": 0,
            "inputs": [13],
            "outputs": [27]
        },
        {
            "id": "vlv_ring_2",
            "name": "Anel Sul (Válvula 3)",
            "driver": "GARDEN_VALVE",
            "state": 0,
            "inputs": [15],
            "outputs": [26]
        },
        {
            "id": "vlv_ring_3",
            "name": "Anel Este (Válvula 4)",
            "driver": "GARDEN_VALVE",
            "state": 0,
            "inputs": [4],
            "outputs": [25]
        },
        {
            "id": "vlv_ring_4",
            "name": "Anel Oeste (Válvula 5)",
            "driver": "GARDEN_VALVE",
            "state": 0,
            "inputs": [5],
            "outputs": [33]
        },
        {
            "id": "light_spot_1",
            "name": "Foco RGBW Subaquático 1",
            "driver": "LIGHT_DIMMER",
            "state": 0,
            "inputs": [18],
            "outputs": [19]
        },
        {
            "id": "light_spot_2",
            "name": "Foco RGBW Subaquático 2",
            "driver": "LIGHT_DIMMER",
            "state": 0,
            "inputs": [21],
            "outputs": [22]
        }
    ],
    "irrigation": {
        "enabled": True,
        "rainDelayMinutes": 0,
        "maxConcurrentZones": 2,
        "programs": [
            {
                "id": 1,
                "name": "Rega Matinal",
                "enabled": True,
                "startMinute": 420,
                "weekdays": 127,
                "zones": [
                    {"uniqueId": "vlv_center", "durationMinutes": 10},
                    {"uniqueId": "vlv_ring_1", "durationMinutes": 15}
                ]
            }
        ]
    },
    "aquadance": {
        "enabled": True,
        "shows": [
            {
                "id": 1,
                "name": "Cascata de Verão",
                "stepMs": 300,
                "totalSteps": 32,
                "loop": True,
                "tracks": [
                    {
                        "uniqueId": "vlv_center",
                        "trackType": 0,
                        "posX": 50,
                        "posY": 50,
                        "steps": [1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0],
                        "rgbw": [0x00E5FF] * 32
                    },
                    {
                        "uniqueId": "vlv_ring_1",
                        "trackType": 0,
                        "posX": 50,
                        "posY": 20,
                        "steps": [0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0],
                        "rgbw": [0x0066FF] * 32
                    },
                    {
                        "uniqueId": "vlv_ring_2",
                        "trackType": 0,
                        "posX": 50,
                        "posY": 80,
                        "steps": [0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0],
                        "rgbw": [0x0066FF] * 32
                    },
                    {
                        "uniqueId": "vlv_ring_3",
                        "trackType": 0,
                        "posX": 80,
                        "posY": 50,
                        "steps": [0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0],
                        "rgbw": [0x00FF88] * 32
                    },
                    {
                        "uniqueId": "vlv_ring_4",
                        "trackType": 0,
                        "posX": 20,
                        "posY": 50,
                        "steps": [0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0],
                        "rgbw": [0x00FF88] * 32
                    },
                    {
                        "uniqueId": "light_spot_1",
                        "trackType": 1,
                        "posX": 35,
                        "posY": 35,
                        "steps": [100, 50, 25, 0, 100, 50, 25, 0, 100, 100, 100, 100, 50, 0, 0, 0, 100, 50, 25, 0, 100, 50, 25, 0, 100, 100, 100, 100, 50, 0, 0, 0],
                        "rgbw": [0xFFCC00] * 32
                    },
                    {
                        "uniqueId": "light_spot_2",
                        "trackType": 2,
                        "posX": 65,
                        "posY": 65,
                        "steps": [1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0],
                        "rgbw": [0xFF00AA] * 32
                    }
                ]
            }
        ]
    }
}

# Simulator State & Background Engine
sim_running_show_id = None
sim_running_step = 0
sim_stop_requested = False
sim_lock = threading.Lock()


def aquadance_engine_worker() -> None:
    """Simulates the AquaDance non-blocking step sequencer."""
    global sim_running_show_id, sim_running_step, sim_stop_requested
    while True:
        with sim_lock:
            show_id = sim_running_show_id
            stop_req = sim_stop_requested

        if stop_req:
            with sim_lock:
                sim_running_show_id = None
                sim_running_step = 0
                sim_stop_requested = False
                # Close all valves and lights
                for f in SIMULATED_CONFIG["features"]:
                    f["state"] = 0
            print("\033[93m[AquaDance Engine] Parado. Válvulas e luzes fechadas.\033[0m")

        if show_id is not None:
            show = next((s for s in SIMULATED_CONFIG["aquadance"]["shows"] if s["id"] == show_id), None)
            if show:
                total = show.get("totalSteps", 32)
                step_ms = show.get("stepMs", 400)
                tracks = show.get("tracks", [])

                with sim_lock:
                    step = sim_running_step
                    # Actuate simulated fixtures
                    active_valves = []
                    active_lights = []
                    for t in tracks:
                        uid = t.get("uniqueId")
                        ttype = t.get("trackType", 0)
                        steps = t.get("steps", [])
                        val = steps[step] if step < len(steps) else 0
                        f = next((x for x in SIMULATED_CONFIG["features"] if x["id"] == uid), None)
                        if f:
                            f["state"] = val
                            if val > 0:
                                if ttype == 0:
                                    active_valves.append(f["name"])
                                else:
                                    active_lights.append(f"{f['name']} ({val}%)")

                    print(f"\033[96m[AquaDance Passo {step+1:02d}/{total:02d}]\033[0m Jatos: \033[92m{', '.join(active_valves) or 'Nenhum'}\033[0m | Luzes: \033[93m{', '.join(active_lights) or 'Nenhuma'}\033[0m")

                    sim_running_step += 1
                    if sim_running_step >= total:
                        if show.get("loop", False):
                            sim_running_step = 0
                        else:
                            sim_running_show_id = None
                            sim_running_step = 0
                            for f in SIMULATED_CONFIG["features"]:
                                f["state"] = 0
                            print("\033[92m[AquaDance] Coreografia terminada.\033[0m")

                time.sleep(step_ms / 1000.0)
            else:
                time.sleep(0.1)
        else:
            time.sleep(0.05)


class MockEspHandler(BaseHTTPRequestHandler):
    def log_message(self, format: str, *args: object) -> None:
        # Custom clean logger
        sys.stderr.write(f"[{self.log_date_time_string()}] {self.command} {self.path}\n")

    def send_json(self, data: dict | list, code: int = 200) -> None:
        body = json.dumps(data, indent=2).encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Headers", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.end_headers()
        self.wfile.write(body)

    def do_OPTIONS(self) -> None:
        self.send_response(204)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Headers", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.end_headers()

    def do_GET(self) -> None:
        parsed = urlparse(self.path)
        path = parsed.path

        if path in ("/config", "/api/config"):
            self.send_json(SIMULATED_CONFIG)
            return

        if path in ("/state", "/api/state"):
            with sim_lock:
                state_data = {
                    "running": sim_running_show_id is not None,
                    "showId": sim_running_show_id,
                    "step": sim_running_step,
                    "features": SIMULATED_CONFIG["features"],
                    "freeHeap": 184500,
                    "uptime": int(time.time()),
                    "rssi": -48
                }
            self.send_json(state_data)
            return

        if path in ("/aquadance", "/api/aquadance"):
            with sim_lock:
                aq = dict(SIMULATED_CONFIG["aquadance"])
                if sim_running_show_id is not None:
                    show = next((s for s in aq["shows"] if s["id"] == sim_running_show_id), None)
                    aq["running"] = {
                        "showId": sim_running_show_id,
                        "step": sim_running_step,
                        "totalSteps": show["totalSteps"] if show else 32,
                        "stepMs": show["stepMs"] if show else 400,
                        "loop": show["loop"] if show else False
                    }
                else:
                    aq["running"] = None
            self.send_json(aq)
            return

        if path == "/info":
            self.send_json({
                "chipId": SIMULATED_CONFIG["chipId"],
                "nodeId": SIMULATED_CONFIG["nodeId"],
                "version": "9.199-SIM",
                "featuresCount": len(SIMULATED_CONFIG["features"])
            })
            return

        # Serve static webpanel files
        rel_path = path.lstrip("/") or "index.html"
        file_path = WEBPANEL_DIR / rel_path
        if file_path.exists() and file_path.is_file():
            mime, _ = mimetypes.guess_type(str(file_path))
            content = file_path.read_bytes()
            self.send_response(200)
            self.send_header("Content-Type", mime or "text/plain")
            self.send_header("Content-Length", str(len(content)))
            self.send_header("Cache-Control", "no-cache")
            self.end_headers()
            self.wfile.write(content)
            return

        # Fallback to index.html
        index_file = WEBPANEL_DIR / "index.html"
        if index_file.exists():
            content = index_file.read_bytes()
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(content)))
            self.end_headers()
            self.wfile.write(content)
            return

        self.send_response(404)
        self.end_headers()

    def do_POST(self) -> None:
        global sim_running_show_id, sim_running_step, sim_stop_requested
        parsed = urlparse(self.path)
        path = parsed.path
        length = int(self.headers.get("Content-Length", 0))
        body_bytes = self.rfile.read(length) if length > 0 else b"{}"

        try:
            body = json.loads(body_bytes.decode("utf-8")) if body_bytes else {}
        except Exception:
            body = {}

        if path in ("/aquadance", "/api/aquadance"):
            if "shows" in body:
                SIMULATED_CONFIG["aquadance"]["shows"] = body["shows"]
            if "enabled" in body:
                SIMULATED_CONFIG["aquadance"]["enabled"] = body["enabled"]
            print("\033[92m[Simulador] Configuração AquaDance guardada com sucesso.\033[0m")
            self.send_json(SIMULATED_CONFIG["aquadance"])
            return

        if path in ("/aquadance/run", "/aquadance-run", "/api/aquadance/run"):
            show_id = body.get("showId", 1)
            with sim_lock:
                sim_running_show_id = int(show_id)
                sim_running_step = 0
                sim_stop_requested = False
            print(f"\033[92m[AquaDance] A iniciar reprodução da coreografia ID: {show_id}\033[0m")
            self.send_json({"result": "ok", "runningShowId": show_id})
            return

        if path in ("/aquadance/stop", "/aquadance-stop", "/api/aquadance/stop"):
            with sim_lock:
                sim_stop_requested = True
            self.send_json({"result": "stopped"})
            return

        if path in ("/control", "/api/control"):
            act_id = body.get("id")
            val = body.get("val", 0)
            for f in SIMULATED_CONFIG["features"]:
                if f["id"] == act_id:
                    f["state"] = val
                    print(f"\033[94m[Controlo Manual]\033[0m {f['name']} -> Estado: {val}")
            self.send_json({"result": "ok", "features": SIMULATED_CONFIG["features"]})
            return

        if path in ("/config", "/api/config"):
            SIMULATED_CONFIG.update(body)
            print("\033[92m[Simulador] Configuração geral atualizada.\033[0m")
            self.send_json(SIMULATED_CONFIG)
            return

        self.send_json({"result": "ok"})


def main() -> None:
    port = 8080
    if len(sys.argv) > 1:
        try:
            port = int(sys.argv[1])
        except ValueError:
            pass

    # Start the AquaDance engine background worker thread
    engine_thread = threading.Thread(target=aquadance_engine_worker, daemon=True)
    engine_thread.start()

    server = HTTPServer(("0.0.0.0", port), MockEspHandler)
    print("\n" + "=" * 60)
    print(f"🚀 \033[96mEasyIot / AquaDance ESP32/ESP8266 Simulador Online!\033[0m")
    print(f"🌐 Abra o seu navegador em: \033[92mhttp://localhost:{port}\033[0m")
    print(f"⛲ Jatos virtuais e focos RGBW carregados e prontos a simular.")
    print("=" * 60 + "\n")

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n🛑 Simulador terminado.")


if __name__ == "__main__":
    main()

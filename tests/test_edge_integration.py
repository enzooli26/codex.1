"""End-to-end TLS test for client -> central server -> charger edge server.

Uses temporary SQLite files and never touches the developer database.
Run after a release build: python tests/test_edge_integration.py
"""
from __future__ import annotations

import json
import os
import socket
import sqlite3
import ssl
import struct
import subprocess
import tempfile
import time
import uuid
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build-vscode"
SERVER = BUILD / "server" / "release" / "ev_server.exe"
DEVICE = BUILD / "device_simulator" / "release" / "ev_device_simulator.exe"
CERT = ROOT / "config" / "server-cert.pem"
KEY = ROOT / "config" / "server-key.pem"
PORT = 19527


class Client:
    def __init__(self):
        context = ssl.create_default_context(cafile=str(CERT))
        raw = socket.create_connection(("127.0.0.1", PORT), timeout=5)
        self.sock = context.wrap_socket(raw, server_hostname="localhost")

    def request(self, kind: str, payload: dict, timeout: float = 12) -> dict:
        request_id = uuid.uuid4().hex
        message = {"version": 1, "type": kind, "requestId": request_id,
                   "timestamp": int(time.time() * 1000), "payload": payload}
        body = json.dumps(message, ensure_ascii=False, separators=(",", ":")).encode()
        self.sock.sendall(struct.pack(">I", len(body)) + body)
        self.sock.settimeout(timeout)
        while True:
            size = struct.unpack(">I", self._read(4))[0]
            response = json.loads(self._read(size))
            if response.get("requestId") == request_id:
                return response

    def _read(self, size: int) -> bytes:
        data = b""
        while len(data) < size:
            part = self.sock.recv(size - len(data))
            if not part:
                raise ConnectionError("server closed the TLS connection")
            data += part
        return data

    def close(self):
        self.sock.close()


def wait_tls(deadline: float = 10):
    end = time.time() + deadline
    while time.time() < end:
        try:
            client = Client()
            return client
        except (OSError, ssl.SSLError):
            time.sleep(0.25)
    raise RuntimeError("central TLS server did not become ready")


def assert_ok(response: dict) -> dict:
    assert response.get("code") == 0, response
    return response.get("data", {})


def main():
    assert SERVER.exists() and DEVICE.exists(), "build the project before running this test"
    runtime = Path(tempfile.mkdtemp(prefix="charging-edge-test-"))
    central_db, edge_db = runtime / "central.db", runtime / "edge.db"
    config = runtime / "app.ini"
    config.write_text(
        f"[server]\nport={PORT}\n[tls]\ncertificate={CERT.as_posix()}\n"
        f"private_key={KEY.as_posix()}\n[database]\npath={central_db.as_posix()}\n"
        "[device]\ntoken=edge-test-token\n", encoding="utf-8")
    env = os.environ.copy()
    env["PATH"] = (r"D:\QtSDK\5.15.2\mingw81_64\bin;"
                   r"D:\QtSDK\Tools\mingw810_64\bin;"
                   r"D:\Anaconda\Library\bin;" + env.get("PATH", ""))
    env["QT_PLUGIN_PATH"] = r"D:\QtSDK\5.15.2\mingw81_64\plugins"
    flags = getattr(subprocess, "CREATE_NO_WINDOW", 0)
    server = device = None
    log_server = (runtime / "server.log").open("w", encoding="utf-8")
    log_device = (runtime / "device.log").open("w", encoding="utf-8")
    phone = "139" + f"{int(time.time()) % 100_000_000:08d}"
    password = "EdgeTest123"
    try:
        server = subprocess.Popen([str(SERVER), "--config", str(config)], env=env,
                                  stdout=log_server, stderr=subprocess.STDOUT,
                                  creationflags=flags)
        bootstrap = wait_tls()
        device = subprocess.Popen([str(DEVICE), "--host", "127.0.0.1", "--port", str(PORT),
                                   "--code", "DL-SW-001", "--database", str(edge_db),
                                   "--token", "edge-test-token"], env=env,
                                  stdout=log_device, stderr=subprocess.STDOUT,
                                  creationflags=flags)
        time.sleep(2)
        assert_ok(bootstrap.request("auth.user.register", {"phone": phone, "password": password,
                                                            "confirmPassword": password}))
        assert_ok(bootstrap.request("wallet.recharge", {"amount": 200, "password": password}))

        # TIME means minutes. 0.05 minute completes locally after about four seconds.
        started = assert_ok(bootstrap.request("charge.start", {"chargerId": 1,
                                                                 "mode": "TIME", "target": 0.05}))
        order_id = int(started["orderId"])
        deadline = time.time() + 12
        status = None
        while time.time() < deadline:
            orders = assert_ok(bootstrap.request("user.orders", {}))["items"]
            status = next(x["status"] for x in orders if int(x["id"]) == order_id)
            if status == "COMPLETED":
                break
            time.sleep(1)
        assert status == "COMPLETED", f"TIME order did not auto-complete: {status}"

        # Charger 2 has no connected edge process, so a three-party start must fail.
        offline = bootstrap.request("charge.start", {"chargerId": 2, "mode": "AMOUNT", "target": 5})
        assert offline.get("code") != 0 and "连接断开" in offline.get("message", ""), offline

        # Start a long order, interrupt the central server, then reconnect and settle it.
        active = assert_ok(bootstrap.request("charge.start", {"chargerId": 1,
                                                                "mode": "AMOUNT", "target": 100}))
        active_id = int(active["orderId"])
        bootstrap.close();server.terminate();server.wait(timeout=5);server = None
        time.sleep(4)  # the edge process continues updating its own SQLite database
        server = subprocess.Popen([str(SERVER), "--config", str(config)], env=env,
                                  stdout=log_server, stderr=subprocess.STDOUT,
                                  creationflags=flags)
        client = wait_tls();time.sleep(4)
        assert_ok(client.request("auth.user", {"phone": phone, "password": password}))
        stopped = assert_ok(client.request("charge.stop", {"orderId": active_id}))
        assert stopped["status"] == "COMPLETED" and stopped["duration"] >= 4, stopped
        client.close();time.sleep(1)

        with sqlite3.connect(edge_db) as db:
            local_status = db.execute(
                "SELECT status FROM charge_orders WHERE central_order_id=?", (active_id,)).fetchone()[0]
            assert local_status == "COMPLETED", local_status
        print("PASS: TLS three-party start, TIME maximum-power precheck/auto-stop, offline rejection,")
        print("      local SQLite metering during outage, reconnect reconciliation and settlement")
        print(f"Temporary test data: {runtime}")
    finally:
        for process in (device, server):
            if process and process.poll() is None:
                process.terminate()
                try: process.wait(timeout=5)
                except subprocess.TimeoutExpired: process.kill()
        log_server.close();log_device.close()


if __name__ == "__main__":
    main()

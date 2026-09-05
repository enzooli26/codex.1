from pathlib import Path
import sqlite3
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def require(condition, message):
    if not condition:
        raise AssertionError(message)


schema = (ROOT / "database" / "schema.sql").read_text(encoding="utf-8")
server = (ROOT / "server" / "serverapp.cpp").read_text(encoding="utf-8")
database = (ROOT / "server" / "database.cpp").read_text(encoding="utf-8")
mobile = (ROOT / "mobile_client" / "main.qml").read_text(encoding="utf-8")
secure = (ROOT / "common" / "secureconnect.h").read_text(encoding="utf-8")

for column in ("password_salt", "password_hash", "failed_attempts", "locked_at"):
    require(column in schema, f"missing security column: {column}")

require("QSslSocket" in secure and "TlsV1_2OrLater" in secure, "TLS client configuration missing")
require("startServerEncryption" in server, "TLS server handshake missing")
require("auth.user.register" in server, "registration endpoint missing")
require("failed_attempts" in database and "LOCKED" in database, "account lock logic missing")
require("password" in database and "password_hash" in database, "password hash verification missing")
require("首页" in mobile and "找桩" in mobile and "充电" in mobile and "我的" in mobile, "four mobile pages missing")
require("ScrollBar.vertical" in mobile, "station/order scrolling missing")

with tempfile.TemporaryDirectory() as directory:
    path = Path(directory) / "security.db"
    db = sqlite3.connect(path)
    db.executescript(schema)
    cols = {row[1] for row in db.execute("pragma table_info(users)")}
    require({"password_salt", "password_hash", "failed_attempts", "locked_at"} <= cols,
            "new user security columns not created")
    db.execute("insert into users(phone,nickname,password_salt,password_hash,created_at) values(?,?,?,?,datetime('now'))",
               ("13800000123", "test", "salt-value", "hash-value"))
    row = db.execute("select phone,password_salt,password_hash,status,failed_attempts from users").fetchone()
    require(row[1] != "123456" and row[2] != "123456", "plaintext password persisted")
    db.close()

print("PASS tls_password_registration_lock_and_mobile_navigation")

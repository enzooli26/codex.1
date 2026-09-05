import hashlib
import sqlite3
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def load_database():
    tmp = tempfile.NamedTemporaryFile(suffix=".db", delete=False)
    tmp.close()
    db = sqlite3.connect(tmp.name)
    db.execute("PRAGMA foreign_keys=ON")
    db.executescript((ROOT / "database" / "schema.sql").read_text(encoding="utf-8"))
    db.executescript((ROOT / "database" / "seed.sql").read_text(encoding="utf-8"))
    db.executescript((ROOT / "database" / "test_data.sql").read_text(encoding="utf-8"))
    return db, Path(tmp.name)


def main():
    db, path = load_database()
    try:
        assert db.execute("SELECT COUNT(*) FROM stations").fetchone()[0] == 2
        assert db.execute("SELECT COUNT(*) FROM chargers").fetchone()[0] == 3
        assert db.execute("SELECT COUNT(*) FROM users").fetchone()[0] == 3
        assert db.execute("SELECT COUNT(*) FROM charge_orders WHERE status='COMPLETED'").fetchone()[0] == 2
        expected = hashlib.sha256(b"123456").hexdigest()
        actual = db.execute("SELECT password_hash FROM admins WHERE username='admin'").fetchone()[0]
        assert actual == expected

        db.execute("INSERT INTO users(phone,nickname,balance,status,created_at) VALUES(?,?,?,?,datetime('now'))",
                   ("13800000001", "测试用户0001", 100.0, "NORMAL"))
        user_id = db.execute("SELECT id FROM users WHERE phone='13800000001'").fetchone()[0]
        db.execute("UPDATE chargers SET status='RESERVED' WHERE id=1 AND status='IDLE'")
        assert db.execute("SELECT changes()").fetchone()[0] == 1
        db.execute("INSERT INTO reservations(user_id,charger_id,status,expires_at,created_at) VALUES(?,?, 'ACTIVE', datetime('now','+20 minutes'), datetime('now'))",
                   (user_id, 1))
        reservation_id = db.execute("SELECT last_insert_rowid()").fetchone()[0]
        db.execute("UPDATE reservations SET status='CANCELLED',cancel_reason='USER_CANCELLED' WHERE id=?", (reservation_id,))
        db.execute("UPDATE chargers SET status='IDLE' WHERE id=1 AND status='RESERVED'")
        db.commit()
        assert db.execute("SELECT status FROM reservations WHERE id=?", (reservation_id,)).fetchone()[0] == "CANCELLED"
        assert db.execute("SELECT status FROM chargers WHERE id=1").fetchone()[0] == "IDLE"

        print("PASS schema_and_seed")
        print("PASS admin_password_hash")
        print("PASS reservation_cancel_release")
    finally:
        db.close()
        path.unlink(missing_ok=True)


if __name__ == "__main__":
    main()

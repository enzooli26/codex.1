import sqlite3
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    tmp = tempfile.NamedTemporaryFile(suffix=".db", delete=False)
    tmp.close()
    db = sqlite3.connect(tmp.name)
    try:
        db.executescript((ROOT / "database" / "schema.sql").read_text(encoding="utf-8"))
        db.executescript((ROOT / "database" / "seed.sql").read_text(encoding="utf-8"))
        db.executescript((ROOT / "database" / "test_data.sql").read_text(encoding="utf-8"))

        stations = db.execute("SELECT s.name,COUNT(c.id) FROM stations s LEFT JOIN chargers c ON c.station_id=s.id GROUP BY s.id").fetchall()
        orders = db.execute("SELECT o.id,u.phone,c.code,o.amount FROM charge_orders o JOIN users u ON u.id=o.user_id JOIN chargers c ON c.id=o.charger_id ORDER BY o.id").fetchall()
        users = db.execute("SELECT phone,status FROM users ORDER BY id").fetchall()
        assert len(stations) == 2 and sum(row[1] for row in stations) == 3
        assert len(orders) == 2 and round(sum(row[3] for row in orders), 2) == 34.00
        assert len(users) == 3 and any(status == "FROZEN" for _, status in users)
        user_orders = db.execute("SELECT o.id,s.name,c.code,o.status FROM charge_orders o JOIN chargers c ON c.id=o.charger_id JOIN stations s ON s.id=c.station_id WHERE o.user_id=101 ORDER BY o.id DESC").fetchall()
        assert len(user_orders) == 1 and user_orders[0][3] == "COMPLETED"

        db.execute("UPDATE users SET status='FROZEN' WHERE id=101")
        db.execute("INSERT INTO operation_logs(actor_type,action,target_type,target_id,result,created_at) VALUES('ADMIN','FREEZE_USER','USER',101,'SUCCESS',datetime('now'))")
        db.execute("UPDATE chargers SET status='IDLE',last_seen=datetime('now') WHERE id=3 AND status!='CHARGING'")
        db.commit()
        assert db.execute("SELECT status FROM users WHERE id=101").fetchone()[0] == "FROZEN"
        assert db.execute("SELECT COUNT(*) FROM operation_logs").fetchone()[0] == 1

        print("PASS admin_station_charger_overview")
        print("PASS admin_order_user_lists")
        print("PASS mobile_user_order_list")
        print("PASS admin_user_status_and_audit")
    finally:
        db.close()
        Path(tmp.name).unlink(missing_ok=True)


if __name__ == "__main__":
    main()

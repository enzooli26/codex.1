import sqlite3
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class MaintenanceAlarmTests(unittest.TestCase):
    def test_schema_supports_alarm_and_maintenance_lifecycle(self):
        connection = sqlite3.connect(":memory:")
        connection.executescript((ROOT / "database" / "schema.sql").read_text(encoding="utf-8"))
        connection.execute(
            "INSERT INTO stations(name,address,longitude,latitude,base_price,status) "
            "VALUES('测试站','测试地址',121.1,38.9,1.2,'ONLINE')"
        )
        connection.execute(
            "INSERT INTO chargers(station_id,code,type,rated_power,status) "
            "VALUES(1,'TEST-001','FAST',120,'FAULT')"
        )
        connection.execute(
            "INSERT INTO alarms(charger_id,level,type,message,status,created_at) "
            "VALUES(1,'CRITICAL','DEVICE_FAULT','绝缘检测异常','OPEN','2026-09-07T10:00:00')"
        )
        connection.execute(
            "INSERT INTO maintenance_orders(alarm_id,charger_id,issue,assignee,scheduled_at,status,created_at) "
            "VALUES(1,1,'绝缘检测异常','张师傅','2026-09-07 11:00','DISPATCHED','2026-09-07T10:05:00')"
        )
        row = connection.execute(
            "SELECT a.status,m.assignee,m.status FROM alarms a "
            "JOIN maintenance_orders m ON m.alarm_id=a.id"
        ).fetchone()
        self.assertEqual(row, ("OPEN", "张师傅", "DISPATCHED"))

    def test_server_and_admin_ui_expose_integrated_operations(self):
        server = (ROOT / "server" / "serverapp.cpp").read_text(encoding="utf-8")
        database = (ROOT / "server" / "database.cpp").read_text(encoding="utf-8")
        ui = (ROOT / "admin_client" / "adminwindow.ui").read_text(encoding="utf-8")
        for route in (
            "admin.alarms", "admin.maintenance", "admin.inspection.run",
            "admin.alarm.ack", "admin.maintenance.create", "admin.maintenance.status",
        ):
            self.assertIn(route, server)
        self.assertIn("m_inspectionTimer.setInterval(10000)", server)
        self.assertIn("HEARTBEAT_TIMEOUT", database)
        self.assertIn('name="alarmsTable"', ui)
        self.assertIn('name="maintenanceTable"', ui)
        self.assertIn("实时告警与巡检", ui)


if __name__ == "__main__":
    unittest.main()

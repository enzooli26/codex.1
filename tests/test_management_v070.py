from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def require(path, *needles):
    text = (ROOT / path).read_text(encoding="utf-8")
    for needle in needles:
        assert needle in text, f"{needle!r} missing from {path}"


require("server/database.cpp", "Database::updateStation", "Database::deleteStation",
        "Database::addCharger", "Database::updateCharger", "Database::deleteCharger",
        "已有订单、预约或遥测记录")
require("server/serverapp.cpp", 'type=="admin.station.update"',
        'type=="admin.station.delete"', 'type=="admin.charger.add"',
        'type=="admin.charger.update"', 'type=="admin.charger.delete"')
require("server/main.cpp", "configuredDbPath", "Migrated legacy database", "projectDir.cdUp()")
require("admin_client/adminwindow.cpp", "QAbstractItemView::NoEditTriggers",
        "QHeaderView::Stretch", "updateStationChoices", "AdminWindow::addCharger",
        "AdminWindow::updateCharger", "AdminWindow::deleteCharger")
require("admin_client/adminwindow.ui", 'name="updateStationButton"',
        'name="deleteStationButton"', 'name="chargerStationCombo"',
        'name="addChargerButton"', 'name="updateChargerButton"',
        'name="deleteChargerButton"')

print("PASS stable_database_station_charger_crud_and_responsive_tables")


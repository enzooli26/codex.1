from pathlib import Path


root = Path(__file__).resolve().parents[1]
server = (root / "server" / "serverapp.cpp").read_text(encoding="utf-8")
database = (root / "server" / "database.cpp").read_text(encoding="utf-8")
edge = (root / "device_simulator" / "edgedatabase.cpp").read_text(encoding="utf-8")
simulator = (root / "device_simulator" / "simulator.cpp").read_text(encoding="utf-8")

for table in ("chargers", "charge_orders", "telemetry"):
    assert f"CREATE TABLE IF NOT EXISTS {table}" in edge

assert 'type=="device.register"' in server
assert 'type=="device.sync"' in server
assert '"device.order.start"' in server and '"device.order.stop"' in server
assert "connectedDevice(chargerCode)" in server
assert "充电桩与服务器连接断开，暂时无法停止订单" in server
assert 'socket->property("role").toString()!="device"' in server

assert 'mode=="TIME"' in database
assert "(target/60.0)*ratedPower*price" in database
assert "status='STARTING'" in database
assert "completeChargeFromDevice" in database

assert "SYNC_PENDING" in edge
assert 'mode=="AMOUNT"' in edge and 'mode=="ENERGY"' in edge and 'mode=="TIME"' in edge
assert "m_database.tick" in simulator
assert "m_database.pendingOrders" in simulator
assert "local charging and metering continue" in simulator

print("PASS charger_edge_sqlite_tls_three_party_billing_reconnect_architecture")

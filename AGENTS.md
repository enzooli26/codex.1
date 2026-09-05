# EV Charging Platform - Agent Guide

## Build

All build files go in `build/` at repository root:

```bash
cd build
qmake ../charging-platform/charging-platform.pro
make -j"$(nproc)"
```

Requires: Qt 5 (Core, Widgets, Network, SQL, Charts, QML, Quick Controls 2), SQLite Qt driver.

## Run

```bash
cp charging-platform/config/app.ini.example charging-platform/config/app.ini  # first time only
./build/server/ev_server --config charging-platform/config/app.ini    # must start first
./build/device_simulator/ev_device_simulator --code DL-SW-001
./build/admin_client/ev_admin_client                # admin/123456 initial
./build/user_client/ev_user_client
```

Server creates `build/data/charging.db` on first run. For demo data: `sqlite3 build/data/charging.db < charging-platform/database/test_data.sql` (stop server first).

## Test

```bash
cd charging-platform/tests
python3 -m pytest
```

Tests create temp SQLite DBs, run schema+seed+test_data SQL. No running server needed.

## Structure

- `server/` - TCP server (C++), port 9527, TLS 1.2+, SQLite DB
- `admin_client/` - Qt Widgets admin UI (station/order/user management)
- `user_client/` - Qt Widgets desktop user UI
- `mobile_client/` - Qt Quick/QML Android client
- `device_simulator/` - simulates charger device
- `common/` - shared TLS, frame codec, password utilities (`common.pri`)
- `database/` - `schema.sql`, `seed.sql`, `test_data.sql`
- `config/` - TLS certs, `app.ini.example` (never commit real map API keys)

## Conventions

- All components use TLS via built-in demo certs (`config/server-cert.pem`, `config/server-key.pem`)
- Passwords: 128-bit random salt + 50000-round SHA-256 derivation, constant-time comparison
- Account lockout after 5 consecutive failed attempts (login + recharge share counter)
- Build order enforced in `.pro`: server builds first, all clients depend on it
- Database auto-migrates on server start (adds salt/hash columns to old DBs)

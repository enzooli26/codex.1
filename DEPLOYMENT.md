# Linux 部署说明

## 环境要求

- Ubuntu 22.04 或更高版本
- Qt 5，包含 Core、Widgets、Network、SQL、Charts、QML、Quick Controls 2
- SQLite Qt 驱动
- GNU C++ 编译器和 make

Ubuntu 可使用以下命令安装常用依赖：

```bash
sudo apt update
sudo apt install build-essential qt5-qmake qtbase5-dev libqt5charts5-dev \
  libqt5sql5-sqlite sqlite3 qtdeclarative5-dev qtquickcontrols2-5-dev \
  qml-module-qtquick2 qml-module-qtquick-controls2 \
  qml-module-qtquick-layouts qml-module-qtquick-window2
```

## 编译

```bash
cd charging-platform
mkdir -p build
cd build
qmake ../charging-platform.pro
make -j"$(nproc)"
```

## 配置

```bash
cd ..
cp config/app.ini.example config/app.ini
```

编辑 `config/app.ini`。腾讯地图 Key 只填写在本地配置文件中，不提交到代码仓库。数据库路径的父目录必须可写。

## 启动顺序

```bash
./build/server/ev_server --config config/app.ini
./build/device_simulator/ev_device_simulator --code DL-SW-001 \
  --database data/charger-edge.db --token course-device-token
./build/user_client/ev_user_client
./build/mobile_client/ev_mobile_client
./build/admin_client/ev_admin_client
```

管理员初始账号为 `admin`，密码为 `123456`。首次完整演示前应修改初始密码。

## 测试数据

服务器首次启动会自动创建基础站点、电桩、电价和管理员。需要附加演示数据时，在停止服务器后执行：

```bash
sqlite3 data/charging.db < database/test_data.sql
```

测试数据包含正常用户、冻结用户、已完成订单、钱包流水和已解决告警。

## 常见问题

- 提示 `QSQLITE driver not loaded`：安装 `libqt5sql5-sqlite` 并重新启动。
- 客户端无法连接：确认服务器已启动，端口与 `config/app.ini` 一致，且防火墙允许该端口。
- 提示充电桩与服务器连接断开：确认充电桩端进程正在运行、`--code` 与中心数据库电桩编号一致，且 `--token` 与 `device/token` 一致。
- 地图不可用：检查 `map/api_key`，并确认虚拟机能够联网。
- 中文显示异常：安装常用中文字体，例如 `fonts-noto-cjk`。

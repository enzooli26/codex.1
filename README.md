# 电动汽车充电桩应用管理平台

第一阶段包含服务器、Qt Widgets 桌面用户端、Qt Quick/QML Android 手机端、管理端和设备模拟器。桌面固定界面保存在 `.ui` 文件中；手机端位于 `mobile_client`，采用适合触摸、自适应布局和 Android 打包的 QML。Web 大屏和机器学习不在本阶段。

管理端已包含运营总览、近 7 天营收折线图、电桩状态环形图、站点营收排行，以及电站、电桩、订单、用户、审计日志五个数据页面。可新增电站、冻结/解冻用户，并可重启非充电状态的模拟电桩。

V0.3 统一为明亮简洁的浅色视觉：管理端采用白色卡片、蓝灰层次和轻量数据图表；用户端采用约 420×720 的手机比例模拟窗口，包含移动端式品牌区、账户卡片、附近电站、预约与充电操作区。它仍是 Linux Qt Widgets 桌面程序，并非可直接安装到 Android/iOS 的原生 App。

V0.4 新增真正独立的 Android-ready 手机客户端：底部“充电 / 订单 / 我的”导航、手机网络配置、触摸式站点选择、预约充电、停止结算、余额和历史订单。Android 构建步骤见 `MOBILE_ANDROID.md`。

## Linux 构建

安装 Qt 5 Core、Widgets、Network、SQL、Charts 和 SQLite 驱动后执行：

```bash
cd charging-platform
qmake charging-platform.pro
make -j"$(nproc)"
```

## 运行

```bash
cp config/app.ini.example config/app.ini
./server/ev_server --config config/app.ini
./device_simulator/ev_device_simulator --code DL-SW-001
./user_client/ev_user_client
./admin_client/ev_admin_client
```

管理员初始账号为 `admin`，密码为 `123456`。腾讯地图 Key 填入 `config/app.ini` 的 `map/api_key`，不得提交真实 Key。

如需立即看到订单、用户和营收图表，请先停止服务器，再导入演示数据：

```bash
sqlite3 data/charging.db < database/test_data.sql
```

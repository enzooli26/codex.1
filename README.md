# 电动汽车充电桩应用管理平台

第一阶段包含服务器、Qt Widgets 桌面用户端、Qt Quick/QML Android 手机端、管理端和设备模拟器。桌面固定界面保存在 `.ui` 文件中；手机端位于 `mobile_client`，采用适合触摸、自适应布局和 Android 打包的 QML。Web 大屏和机器学习不在本阶段。

管理端已包含运营总览、近 7 天营收折线图、电桩状态环形图、站点营收排行，以及电站、电桩、订单、用户、审计日志五个数据页面。可新增电站、冻结/解冻用户，并可重启非充电状态的模拟电桩。

V0.6 完成安全认证与移动端导航：全部客户端、管理端和设备模拟器均改用 TLS 1.2+；用户密码仅在 TLS 通道中传输，数据库保存随机盐和 50000 轮 SHA-256 派生哈希。用户注册、登录和充值均执行前后端校验，登录/充值连续 5 次密码错误会锁定账户，管理员可在用户管理页解锁。手机端提供“首页 / 找桩 / 充电 / 我的”四个可跳转页面，站点和订单列表支持滚动。管理员登录后可安全创建其他管理员账号，禁止匿名注册管理员。

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

管理员初始账号为 `admin`，密码为 `123456`。首次成功登录会把旧版 SHA-256 哈希自动升级为加盐迭代哈希。腾讯地图 Key 填入 `config/app.ini` 的 `map/api_key`，不得提交真实 Key。

TLS 演示证书和私钥位于 `config`。课程验收可直接使用；公开部署必须更换私钥和受信任证书，详见 `SECURITY.md`。

如需立即看到订单、用户和营收图表，请先停止服务器，再导入演示数据：

```bash
sqlite3 data/charging.db < database/test_data.sql
```

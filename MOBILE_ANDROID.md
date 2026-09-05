# Android 手机端构建说明

`mobile_client` 是独立的 Qt Quick/QML 手机客户端，包含服务器连接、手机号登录、余额充值、站点选择、20 分钟预约、开始/停止充电和用户订单列表。Android 清单已声明网络权限并锁定竖屏。

## Linux 桌面预览

安装 Qt 5 QML 开发和运行模块：

```bash
sudo apt install qtdeclarative5-dev qtquickcontrols2-5-dev \
qml-module-qtquick2 qml-module-qtquick-controls2 \
qml-module-qtquick-layouts qml-module-qtquick-window2
```

在项目构建目录重新执行：

```bash
qmake ../charging-platform.pro
make -j"$(nproc)"
./mobile_client/ev_mobile_client
```

桌面预览连接本机服务器时使用 `127.0.0.1:9527`。

## 生成 Android APK

1. 在 Qt Maintenance Tool 中安装与桌面版本一致的 Qt 5 Android 组件。
2. 安装 Android SDK、NDK 和 JDK，并在 Qt Creator 的“设备/Android”页面完成检测。
3. 在 Qt Creator 中打开 `mobile_client/mobile_client.pro`。
4. 选择 Android Qt 5 Kit，而不是 Desktop Kit。
5. 使用 Debug 构建并连接手机或启动模拟器，点击运行；发布时选择 Release 并配置签名证书。

生成的 APK 通常位于 Android 构建目录的 `android-build/build/outputs/apk/` 下。

## 真机服务器地址

Android 真机中的 `127.0.0.1` 指手机自身，不能连接电脑服务器。手机和服务器需要在同一局域网，在手机端“我的 → 服务器设置”填写服务器电脑的局域网 IP，例如 `192.168.1.105`，端口为 `9527`。

服务器侧需要监听所有网卡并允许端口通过防火墙：

```bash
sudo ufw allow 9527/tcp
```

如果服务器运行在虚拟机内，还需要将虚拟机网络设为桥接模式，或配置 NAT 端口转发。

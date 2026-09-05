# 使用 VS Code 构建和预览

VS Code 可以在 Windows 直接编译并弹出 Qt 页面，但 VS Code 本身不包含 Qt。需要先安装 Qt 5 桌面 MinGW 工具链，并确保安装 Qt Charts、Qt Quick/QML、Qt Quick Controls 2 和对应的 MinGW 编译器。

## Windows 一次性设置

1. 安装 Qt 5，并选择 Desktop MinGW 64-bit、Qt Charts、Qt Quick Controls 2、Qt 的 MinGW Tools 和 OpenSSL 1.1.x 64-bit 运行库。
2. 在 VS Code 安装 `Qt Extension for VS Code` 和 `C/C++` 扩展。
3. 将项目中的 `charging-platform` 文件夹作为工作区根目录打开。
4. 确保 VS Code 终端可以执行 `qmake -v` 和 `mingw32-make -v`。

如果命令找不到，可以从 Qt 自带的 MinGW 命令行启动 VS Code，或者在 PowerShell 临时加入 PATH。以下路径只是示例，应替换为本机实际版本：

```powershell
$env:Path="D:\QtSDK\5.15.2\mingw81_64\bin;D:\QtSDK\Tools\mingw810_64\bin;$env:Path"
qmake -v
mingw32-make -v
code .
```

## 一键构建

项目已提供 `.vscode/tasks.json`：

- 按 `Ctrl+Shift+B` 执行 `Qt: Build`。
- 按 `Ctrl+Shift+P`，选择 `Tasks: Run Task`。
- 运行 `Run: Server` 启动服务器。
- 运行 `Run: Device Simulator` 启动设备模拟器。
- 再运行 `Run: Admin` 查看运营管理端。
- 运行 `Run: Desktop User` 查看 Qt Widgets 用户端。
- 运行 `Run: Mobile Preview` 查看 QML 手机端。

Windows 构建目录为 `build-vscode`。数据库相对于服务器启动目录创建在 `build-vscode/data/charging.db`。

## QML 实时预览

Qt 官方 VS Code 扩展支持 QML Preview。先成功构建 `mobile_client`，然后在命令面板执行 `Qt: Preview changes to QML code live in your application`，选择 `ev_mobile_client.exe`。若扩展找不到资源文件，把 `build-vscode` 加入 Qt QML Preview 的 Additional Build Dirs。

## 常见问题

- `qmake 不是内部或外部命令`：Qt 的 `bin` 没有加入 PATH。
- `mingw32-make 不是内部或外部命令`：Qt Tools 中 MinGW 的 `bin` 没有加入 PATH。
- `cannot find -lQt5Charts`：当前 Qt Kit 未安装 Charts，或 qmake 与 MinGW 来自不同 Qt 安装。
- `Unknown module(s) in QT: quickcontrols2`：当前 Qt Kit 未安装 Qt Quick Controls 2。
- `TLS initialization failed` 或 `QSslSocket::supportsSsl() == false`：缺少与 Qt 5.15.2 匹配的 OpenSSL 1.1.x 64-bit DLL；不要使用 OpenSSL 3.x DLL冒充。
- 服务器提示端口占用：Windows 和虚拟机不要同时启动占用同一个映射端口的服务器。

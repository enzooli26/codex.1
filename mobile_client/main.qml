import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.5

ApplicationWindow {
    id: window
    visible: true
    width: 420
    height: 820
    minimumWidth: 360
    minimumHeight: 640
    title: "悦充"
    color: theme.background
    Material.theme: theme.dark ? Material.Dark : Material.Light
    Material.primary: theme.primary
    Material.accent: theme.primary

    property int currentTab: 0
    property color primary: theme.primary
    property color ink: theme.ink
    property color muted: theme.muted
    property color sub: theme.sub
    property color green: theme.green
    property color greenText: theme.greenText
    property color red: theme.red
    property color softBg: theme.dark ? "#182235" : "#F1F5FB"
    property color accentSoft: theme.dark ? "#263356" : "#E9F0FF"
    property color fieldBg: theme.dark ? "#111927" : "#F3F7FD"
    property color pillActive: theme.dark ? "#313F63" : "#E7EEFD"
    property var pageTitles: ["首页", "附近充电站", "充电中心", "我的"]
    property var pageSubs: ["把每一度电用在路上", "附近空闲充电桩一目了然", "充电进度尽在掌握", "账户、订单与外观"]

    function selectedStationName() {
        if (mobileClient.selectedIndex < 0) return ""
        var stations = mobileClient.stations
        if (!stations || mobileClient.selectedIndex >= stations.length) return ""
        var s = stations[mobileClient.selectedIndex]
        return s ? String(s.name || "") : ""
    }

    function orderStatusText(s) {
        if (s === "COMPLETED") return "已完成"
        if (s === "CHARGING") return "充电中"
        if (s === "CANCELLED") return "已取消"
        if (s === "CREATED") return "待充电"
        return s
    }
    function orderStatusColor(s) {
        if (s === "COMPLETED") return greenText
        if (s === "CHARGING") return primary
        if (s === "CANCELLED") return muted
        return primary
    }

    Component.onCompleted: {
        if (mobileClient.savedHost !== "") hostField.text = mobileClient.savedHost
        if (mobileClient.savedPort > 0) portField.text = String(mobileClient.savedPort)
        phoneField.text = mobileClient.savedPhone
        if (mobileClient.savedHost !== "")
            mobileClient.connectServer(mobileClient.savedHost, mobileClient.savedPort > 0 ? mobileClient.savedPort : 9527)
    }

    Timer { id: liveTimer; interval: 3000; repeat: true; triggeredOnStart: true; running: mobileClient.charging; onTriggered: mobileClient.refreshChargeStatus() }
    Timer { id: stationTimer; interval: 3000; repeat: true; running: window.currentTab === 1 && mobileClient.connected && mobileClient.selectedIndex < 0; onTriggered: mobileClient.refreshStations() }
    Timer { id: orderTimer; interval: 3000; repeat: true; running: window.currentTab === 3 && mobileClient.loggedIn; onTriggered: mobileClient.refreshOrders() }

    header: Rectangle {
        height: 100
        gradient: Gradient {
            GradientStop { position: 0; color: theme.headerStart }
            GradientStop { position: 1; color: theme.headerEnd }
        }
        Rectangle { width: 150; height: 150; radius: 75; color: "#14FFFFFF"; x: parent.width - 55; y: -84 }
        Rectangle { width: 84; height: 84; radius: 42; color: "#0FFFFFFF"; x: parent.width - 92; y: -42 }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 16
            anchors.topMargin: 6
            ColumnLayout {
                spacing: 1
                Layout.fillWidth: true
                Label { text: pageTitles[window.currentTab]; color: "white"; font.pixelSize: 24; font.bold: true }
                Label { text: pageSubs[window.currentTab]; color: theme.headerSub; font.pixelSize: 12; Layout.fillWidth: true; elide: Text.ElideRight }
            }
            Rectangle {
                Layout.preferredWidth: connText.implicitWidth + 32
                Layout.preferredHeight: 32
                radius: 16
                color: mobileClient.connected ? "#2EFFFFFF" : "#1AFFFFFF"
                RowLayout {
                    anchors.centerIn: parent
                    spacing: 7
                    Rectangle { Layout.preferredWidth: 8; Layout.preferredHeight: 8; radius: 4; color: mobileClient.connected ? "#9AF6CD" : "#D9E3F0" }
                    Label { id: connText; text: mobileClient.connected ? "安全连接" : "未连接"; color: "white"; font.pixelSize: 12; font.bold: true }
                }
            }
        }
    }

    StackLayout {
        anchors.top: parent.top
        anchors.bottom: bottomBar.top
        anchors.left: parent.left
        anchors.right: parent.right
        currentIndex: window.currentTab

        /* ============ 首页 ============ */
        ScrollView {
            id: homeScroll
            clip: true
            contentWidth: availableWidth
            ColumnLayout {
                width: homeScroll.availableWidth
                spacing: 14
                Item { Layout.preferredHeight: 4 }

                /* 余额 / 欢迎主卡 */
                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 14
                    Layout.rightMargin: 14
                    Layout.preferredHeight: 178
                    radius: 22
                    clip: true
                    gradient: Gradient {
                        GradientStop { position: 0; color: theme.headerStart }
                        GradientStop { position: 1; color: theme.headerEnd }
                    }
                    Rectangle { width: 158; height: 158; radius: 79; color: "#16FFFFFF"; x: parent.width - 62; y: -86 }
                    Rectangle { width: 86; height: 86; radius: 43; color: "#10FFFFFF"; x: parent.width - 118; y: -30 }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 22
                        anchors.rightMargin: 22
                        anchors.topMargin: 20
                        anchors.bottomMargin: 18
                        spacing: 3
                        Label {
                            text: mobileClient.loggedIn ? "账户余额" : "欢迎使用悦充"
                            color: theme.headerSub
                            font.pixelSize: 13
                            font.bold: true
                        }
                        Label {
                            text: mobileClient.loggedIn ? "¥" + mobileClient.balance.toFixed(2) : "让充电更简单"
                            color: "white"
                            font.pixelSize: mobileClient.loggedIn ? 32 : 25
                            font.bold: true
                        }
                        Label {
                            Layout.fillWidth: true
                            text: mobileClient.loggedIn ? "你好，" + mobileClient.userText + " · 随时出发去充电"
                                : (mobileClient.connected ? "登录后即可预约附近的空闲充电桩" : "请先在「我的」连接服务器")
                            color: "#D8E6FF"
                            font.pixelSize: 13
                            elide: Text.ElideRight
                        }
                        Item { Layout.fillHeight: true }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12
                            Button {
                                Layout.preferredHeight: 44
                                text: mobileClient.loggedIn ? "去找桩" : "连接并登录"
                                contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 15; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                background: Rectangle { radius: 22; color: "#2AFFFFFF" }
                                onClicked: window.currentTab = mobileClient.loggedIn ? 1 : 3
                            }
                            Button {
                                Layout.preferredHeight: 44
                                visible: mobileClient.loggedIn
                                text: "去充值"
                                contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 15; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                background: Rectangle { radius: 22; color: "#16FFFFFF"; border.width: 1; border.color: "#52FFFFFF" }
                                onClicked: rechargeDialog.open()
                            }
                        }
                    }
                }

                /* 快捷入口 */
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 14
                    Layout.rightMargin: 14
                    spacing: 12
                    Card {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 104
                        MouseArea { anchors.fill: parent; onClicked: { window.currentTab = 1; if (mobileClient.connected) mobileClient.refreshStations() } }
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 16
                            anchors.rightMargin: 12
                            spacing: 12
                            Rectangle {
                                Layout.preferredWidth: 46
                                Layout.preferredHeight: 46
                                radius: 15
                                color: accentSoft
                                Layout.alignment: Qt.AlignVCenter
                                Label { anchors.centerIn: parent; text: "⌖"; color: primary; font.pixelSize: 24 }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                                spacing: 4
                                Label { text: "找充电站"; color: ink; font.pixelSize: 16; font.bold: true }
                                Label { text: mobileClient.stations.length + " 个站点可选"; color: muted; font.pixelSize: 12 }
                            }
                            Label { text: "›"; color: theme.dark ? "#5A6B85" : "#C6D0DD"; font.pixelSize: 28; Layout.alignment: Qt.AlignVCenter }
                        }
                    }
                    Card {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 104
                        MouseArea { anchors.fill: parent; onClicked: window.currentTab = 2 }
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 16
                            anchors.rightMargin: 12
                            spacing: 12
                            Rectangle {
                                Layout.preferredWidth: 46
                                Layout.preferredHeight: 46
                                radius: 15
                                color: theme.greenBg
                                Layout.alignment: Qt.AlignVCenter
                                Label { anchors.centerIn: parent; text: "⚡"; color: green; font.pixelSize: 24 }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                                spacing: 4
                                Label { text: "充电中心"; color: ink; font.pixelSize: 16; font.bold: true }
                                Label { text: mobileClient.charging ? "正在充电" : (mobileClient.reserved ? "预约有效" : "暂无进行中"); color: muted; font.pixelSize: 12 }
                            }
                            Label { text: "›"; color: theme.dark ? "#5A6B85" : "#C6D0DD"; font.pixelSize: 28; Layout.alignment: Qt.AlignVCenter }
                        }
                    }
                }

                /* 充电进行中 */
                Card {
                    Layout.fillWidth: true
                    Layout.leftMargin: 14
                    Layout.rightMargin: 14
                    Layout.preferredHeight: 92
                    visible: mobileClient.charging
                    MouseArea { anchors.fill: parent; onClicked: window.currentTab = 2 }
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 14
                        spacing: 14
                        Rectangle {
                            Layout.preferredWidth: 48
                            Layout.preferredHeight: 48
                            radius: 16
                            color: theme.greenBg
                            Layout.alignment: Qt.AlignVCenter
                            Label { anchors.centerIn: parent; text: "⚡"; color: greenText; font.pixelSize: 26 }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter
                            spacing: 3
                            Label { text: "充电进行中"; color: ink; font.pixelSize: 16; font.bold: true }
                            Label {
                                Layout.fillWidth: true
                                text: "功率 " + mobileClient.livePower.toFixed(1) + " kW · 已充 " + mobileClient.liveEnergy.toFixed(2) + " kWh"
                                color: muted; font.pixelSize: 12; elide: Text.ElideRight
                            }
                        }
                        Label { text: "查看 ›"; color: greenText; font.pixelSize: 14; font.bold: true; Layout.alignment: Qt.AlignVCenter }
                    }
                }

                /* 未充电时的引导 */
                Card {
                    Layout.fillWidth: true
                    Layout.leftMargin: 14
                    Layout.rightMargin: 14
                    Layout.preferredHeight: 78
                    visible: !mobileClient.charging && mobileClient.loggedIn
                    MouseArea { anchors.fill: parent; onClicked: window.currentTab = 1 }
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 14
                        spacing: 14
                        Rectangle {
                            Layout.preferredWidth: 44
                            Layout.preferredHeight: 44
                            radius: 15
                            color: softBg
                            Layout.alignment: Qt.AlignVCenter
                            Label { anchors.centerIn: parent; text: "◌"; color: muted; font.pixelSize: 22 }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter
                            spacing: 3
                            Label { text: mobileClient.reserved ? "已预约充电桩，可去充电中心开始" : "当前没有进行中的充电"; color: ink; font.pixelSize: 15; font.bold: true }
                            Label { text: "选择附近站点即可开始充电"; color: muted; font.pixelSize: 12 }
                        }
                        Label { text: "›"; color: theme.dark ? "#5A6B85" : "#C6D0DD"; font.pixelSize: 28; Layout.alignment: Qt.AlignVCenter }
                    }
                }

                Item { Layout.preferredHeight: 24 }
            }
        }

        /* ============ 找桩 ============ */
        Item {
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 10
                RowLayout {
                    Layout.fillWidth: true
                    visible: mobileClient.selectedIndex < 0
                    Label { text: "附近充电站"; color: ink; font.pixelSize: 20; font.bold: true }
                    Item { Layout.fillWidth: true }
                    Rectangle {
                        Layout.preferredHeight: 28
                        Layout.preferredWidth: subText.implicitWidth + 22
                        radius: 14
                        color: accentSoft
                        Label {
                            id: subText
                            anchors.centerIn: parent
                            text: mobileClient.connected ? mobileClient.stations.length + " 个站点" : "未连接"
                            color: primary
                            font.pixelSize: 12
                            font.bold: true
                        }
                    }
                    Button {
                        text: "刷新"
                        flat: true
                        font.pixelSize: 14
                        enabled: mobileClient.connected
                        onClicked: mobileClient.refreshStations()
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    visible: mobileClient.selectedIndex >= 0
                    Button {
                        text: "‹ 返回站点"
                        flat: true
                        font.pixelSize: 15
                        onClicked: mobileClient.backToStations()
                    }
                    Label {
                        Layout.fillWidth: true
                        text: selectedStationName()
                        color: ink
                        font.pixelSize: 18
                        font.bold: true
                        elide: Text.ElideRight
                    }
                    Rectangle {
                        Layout.preferredHeight: 28
                        Layout.preferredWidth: chargerCountText.implicitWidth + 22
                        radius: 14
                        color: accentSoft
                        Label {
                            id: chargerCountText
                            anchors.centerIn: parent
                            text: mobileClient.chargers.length + " 个充电桩"
                            color: primary
                            font.pixelSize: 12
                            font.bold: true
                        }
                    }
                }
                Label { text: mobileClient.selectedIndex < 0 ? "点击站点查看充电桩，选好站桩后可发起导航或充电" : "点击选择空闲充电桩，选好后即可前往充电中心"; color: muted; font.pixelSize: 12 }
                ListView {
                    id: stationList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: mobileClient.selectedIndex < 0
                    spacing: 12
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    model: mobileClient.stations
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    delegate: Rectangle {
                        id: stationCard
                        width: stationList.width
                        height: 130
                        radius: 20
                        color: theme.card
                        border.width: index === mobileClient.selectedIndex ? 2 : 1
                        border.color: index === mobileClient.selectedIndex ? primary : theme.border

                        MouseArea { anchors.fill: parent; onClicked: mobileClient.selectStation(index) }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 16
                            anchors.rightMargin: 14
                            anchors.topMargin: 16
                            anchors.bottomMargin: 16
                            spacing: 14
                            Rectangle {
                                Layout.preferredWidth: 44
                                Layout.preferredHeight: 44
                                radius: 14
                                color: index === mobileClient.selectedIndex ? accentSoft : softBg
                                Layout.alignment: Qt.AlignVCenter
                                Label { anchors.centerIn: parent; text: "⌖"; color: primary; font.pixelSize: 22 }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                                spacing: 5
                                Label { Layout.fillWidth: true; text: modelData.name; color: ink; font.pixelSize: 16; font.bold: true; elide: Text.ElideRight }
                                Label { Layout.fillWidth: true; text: modelData.address; color: muted; font.pixelSize: 12; elide: Text.ElideRight }
                                Rectangle {
                                    Layout.preferredWidth: priceText.implicitWidth + 14
                                    Layout.preferredHeight: 24
                                    radius: 12
                                    color: accentSoft
                                    Label {
                                        id: priceText
                                        anchors.centerIn: parent
                                        text: "¥" + Number(modelData.price).toFixed(2) + "/度"
                                        color: primary
                                        font.pixelSize: 12
                                        font.bold: true
                                    }
                                }
                            }
                            ColumnLayout {
                                Layout.alignment: Qt.AlignVCenter
                                spacing: 10
                                Rectangle {
                                    Layout.alignment: Qt.AlignHCenter
                                    Layout.preferredWidth: idleText.implicitWidth + 20
                                    Layout.preferredHeight: 26
                                    radius: 13
                                    color: modelData.idle > 0 ? theme.greenBg : theme.redBg
                                    Label {
                                        id: idleText
                                        anchors.centerIn: parent
                                        text: modelData.idle + "/" + modelData.total + " 空闲"
                                        color: modelData.idle > 0 ? greenText : red
                                        font.pixelSize: 12
                                        font.bold: true
                                    }
                                }
                                Rectangle {
                                    Layout.alignment: Qt.AlignHCenter
                                    Layout.preferredWidth: 62
                                    Layout.preferredHeight: 34
                                    radius: 17
                                    color: primary
                                    Label {
                                        anchors.centerIn: parent
                                        text: "导航"
                                        color: "white"
                                        font.pixelSize: 13
                                        font.bold: true
                                    }
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            if (modelData.latitude === 0 && modelData.longitude === 0) {
                                                toastText.text = "该站点暂无坐标，无法导航"
                                                toast.opacity = 1
                                                toastTimer.restart()
                                                return
                                            }
                                            toastText.text = "正在定位当前位置…"
                                            toast.opacity = 1
                                            toastTimer.restart()
                                            mobileClient.startNavigation(modelData.name, modelData.latitude, modelData.longitude)
                                        }
                                    }
                                }
                            }
                        }
                    }
                    Label {
                        anchors.centerIn: parent
                        visible: stationList.count === 0
                        text: mobileClient.connected ? "暂无站点，点击右上角刷新" : "请先在「我的」中连接服务器"
                        color: muted
                    }
                }
                ListView {
                    id: chargerList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: mobileClient.selectedIndex >= 0
                    spacing: 12
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    model: mobileClient.chargers
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    delegate: Rectangle {
                        id: chargerCard
                        width: chargerList.width
                        height: 104
                        radius: 20
                        color: theme.card
                        border.width: index === mobileClient.selectedChargerIndex ? 2 : 1
                        border.color: index === mobileClient.selectedChargerIndex ? primary : theme.border
                        opacity: modelData.available ? 1 : 0.6

                        MouseArea {
                            anchors.fill: parent
                            onClicked: mobileClient.selectCharger(index)
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 16
                            anchors.rightMargin: 14
                            anchors.topMargin: 14
                            anchors.bottomMargin: 14
                            spacing: 14
                            Rectangle {
                                Layout.preferredWidth: 44
                                Layout.preferredHeight: 44
                                radius: 14
                                color: index === mobileClient.selectedChargerIndex ? accentSoft : softBg
                                Layout.alignment: Qt.AlignVCenter
                                Label {
                                    anchors.centerIn: parent
                                    text: modelData.code ? String(modelData.code) : ("#" + (index + 1))
                                    color: primary
                                    font.pixelSize: 15
                                    font.bold: true
                                }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                                spacing: 5
                                Label {
                                    Layout.fillWidth: true
                                    text: (modelData.type || "充电桩") + " · 最大功率 " + modelData.rated_power + " kW"
                                    color: ink
                                    font.pixelSize: 15
                                    font.bold: true
                                    elide: Text.ElideRight
                                }
                                RowLayout {
                                    spacing: 8
                                    Rectangle {
                                        Layout.preferredWidth: chargerStatusText.implicitWidth + 12
                                        Layout.preferredHeight: 22
                                        radius: 11
                                        color: modelData.available ? theme.greenBg : theme.redBg
                                        Label {
                                            id: chargerStatusText
                                            anchors.centerIn: parent
                                            text: modelData.available ? "空闲" : (modelData.status === "RESERVED" ? "已被预约" : (modelData.status === "CHARGING" ? "使用中" : "不可用"))
                                            color: modelData.available ? greenText : red
                                            font.pixelSize: 11
                                            font.bold: true
                                        }
                                    }
                                    Label {
                                        text: "桩号 " + (index + 1)
                                        color: muted
                                        font.pixelSize: 12
                                    }
                                }
                            }
                            Rectangle {
                                Layout.preferredWidth: 64
                                Layout.preferredHeight: 34
                                radius: 17
                                color: index === mobileClient.selectedChargerIndex ? theme.greenBg : (modelData.available ? primary : theme.border)
                                Label {
                                    anchors.centerIn: parent
                                    text: index === mobileClient.selectedChargerIndex ? "✓ 已选" : (modelData.available ? "选择" : "不可选")
                                    color: index === mobileClient.selectedChargerIndex ? greenText : (modelData.available ? "white" : muted)
                                    font.pixelSize: 13
                                    font.bold: true
                                }
                            }
                        }
                    }
                    Label {
                        anchors.centerIn: parent
                        visible: chargerList.count === 0
                        text: "该站点暂无充电桩，请返回选择其他站点"
                        color: muted
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                    }
                }
                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 52
                    visible: mobileClient.selectedIndex < 0
                    text: "请先选择站点"
                    enabled: mobileClient.selectedIndex >= 0
                    highlighted: true
                    font.pixelSize: 16
                    font.bold: true
                    onClicked: window.currentTab = 2
                }
                Button {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 52
                    visible: mobileClient.selectedIndex >= 0
                    text: mobileClient.selectedChargerIndex >= 0 ? "已选好充电桩，前往充电" : "请先选择充电桩"
                    enabled: mobileClient.selectedChargerIndex >= 0
                    highlighted: true
                    font.pixelSize: 16
                    font.bold: true
                    onClicked: window.currentTab = 2
                }
            }
        }

        /* ============ 充电中心 ============ */
        ScrollView {
            id: chargeScroll
            clip: true
            contentWidth: availableWidth
            ColumnLayout {
                width: chargeScroll.availableWidth
                spacing: 14
                Item { Layout.preferredHeight: 4 }

                /* 状态卡 */
                Card {
                    Layout.fillWidth: true
                    Layout.leftMargin: 14
                    Layout.rightMargin: 14
                    Layout.preferredHeight: 92
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 18
                        anchors.rightMargin: 18
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 5
                            Label { text: "当前状态"; color: muted; font.pixelSize: 12 }
                            Label {
                                Layout.fillWidth: true
                                text: mobileClient.chargeStatus
                                color: mobileClient.charging ? greenText : ink
                                font.pixelSize: 18
                                font.bold: true
                                elide: Text.ElideRight
                            }
                        }
                        Rectangle {
                            Layout.preferredWidth: 18
                            Layout.preferredHeight: 18
                            radius: 9
                            color: mobileClient.charging ? green : theme.inactiveDot
                            Layout.alignment: Qt.AlignVCenter
                            Label {
                                anchors.centerIn: parent
                                text: mobileClient.charging ? "●" : "○"
                                color: mobileClient.charging ? "white" : theme.inactiveDot
                                font.pixelSize: 11
                            }
                        }
                    }
                }

                /* 实时数据 */
                Card {
                    Layout.fillWidth: true
                    Layout.leftMargin: 14
                    Layout.rightMargin: 14
                    visible: mobileClient.charging
                    Layout.preferredHeight: 242
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 18
                        spacing: 12
                        Label { text: "实时充电数据"; color: ink; font.pixelSize: 17; font.bold: true }
                        RowLayout { Layout.fillWidth: true; spacing: 10
                            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 78; radius: 16; color: theme.greenBg
                                Column { anchors.centerIn: parent; spacing: 4
                                    Label { anchors.horizontalCenter: parent.horizontalCenter; text: mobileClient.livePower.toFixed(1); color: greenText; font.pixelSize: 20; font.bold: true }
                                    Label { anchors.horizontalCenter: parent.horizontalCenter; text: "功率 kW"; color: muted; font.pixelSize: 11 } } }
                            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 78; radius: 16; color: theme.greenBg
                                Column { anchors.centerIn: parent; spacing: 4
                                    Label { anchors.horizontalCenter: parent.horizontalCenter; text: mobileClient.liveSoc.toFixed(0) + " %"; color: greenText; font.pixelSize: 20; font.bold: true }
                                    Label { anchors.horizontalCenter: parent.horizontalCenter; text: "SOC"; color: muted; font.pixelSize: 11 } } }
                            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 78; radius: 16; color: theme.greenBg
                                Column { anchors.centerIn: parent; spacing: 4
                                    Label { anchors.horizontalCenter: parent.horizontalCenter; text: mobileClient.liveEnergy.toFixed(2); color: greenText; font.pixelSize: 20; font.bold: true }
                                    Label { anchors.horizontalCenter: parent.horizontalCenter; text: "已充 kWh"; color: muted; font.pixelSize: 11 } } }
                        }
                        RowLayout { Layout.fillWidth: true; spacing: 10
                            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 78; radius: 16; color: theme.greenBg
                                Column { anchors.centerIn: parent; spacing: 4
                                    Label { anchors.horizontalCenter: parent.horizontalCenter; text: Math.floor(mobileClient.liveDuration / 60) + " 分"; color: greenText; font.pixelSize: 20; font.bold: true }
                                    Label { anchors.horizontalCenter: parent.horizontalCenter; text: "已充时长"; color: muted; font.pixelSize: 11 } } }
                            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 78; radius: 16; color: theme.greenBg
                                Column { anchors.centerIn: parent; spacing: 4
                                    Label { anchors.horizontalCenter: parent.horizontalCenter; text: "¥" + mobileClient.liveCost.toFixed(2); color: greenText; font.pixelSize: 20; font.bold: true }
                                    Label { anchors.horizontalCenter: parent.horizontalCenter; text: "预估费用"; color: muted; font.pixelSize: 11 } } }
                        }
                    }
                }

                /* 充电设置 */
                Card {
                    Layout.fillWidth: true
                    Layout.leftMargin: 14
                    Layout.rightMargin: 14
                    Layout.preferredHeight: 372
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 18
                        spacing: 12
                        RowLayout {
                            Layout.fillWidth: true
                            Label { text: "充电设置"; color: ink; font.pixelSize: 18; font.bold: true }
                            Item { Layout.fillWidth: true }
                            Label {
                                text: mobileClient.selectedChargerIndex >= 0 ? "已选桩 " + (mobileClient.selectedChargerIndex + 1) + " 号" : (mobileClient.selectedIndex >= 0 ? "请先选桩" : "未选站点")
                                color: mobileClient.selectedChargerIndex >= 0 ? greenText : (mobileClient.selectedIndex >= 0 ? primary : muted)
                                font.pixelSize: 13
                                font.bold: true
                            }
                        }
                        Label { text: "充电模式"; color: muted; font.pixelSize: 12 }
                        ComboBox {
                            id: modeBox
                            Layout.fillWidth: true
                            implicitHeight: 48
                            textRole: "text"
                            background: Rectangle { radius: 14; color: fieldBg; border.width: 1; border.color: theme.border }
                            model: ListModel {
                                ListElement { text: "按金额"; value: "AMOUNT" }
                                ListElement { text: "按电量"; value: "ENERGY" }
                                ListElement { text: "按时间"; value: "TIME" }
                            }
                        }
                        Label { text: "充电目标"; color: muted; font.pixelSize: 12 }
                        TextField {
                            id: targetField
                            Layout.fillWidth: true
                            implicitHeight: 48
                            text: "10"
                            placeholderText: "请输入充电目标值"
                            inputMethodHints: Qt.ImhFormattedNumbersOnly
                            validator: DoubleValidator { bottom: .01; top: 9999 }
                            background: Rectangle { radius: 14; color: fieldBg; border.width: 1; border.color: theme.border }
                        }
                        Button {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 50
                            visible: !mobileClient.charging && !mobileClient.reserved
                            text: mobileClient.selectedChargerIndex >= 0 ? "预约该充电桩" : (mobileClient.loggedIn ? "请先选择充电桩" : "登录后即可预约")
                            enabled: mobileClient.loggedIn && !mobileClient.reserved && !mobileClient.charging && mobileClient.selectedChargerIndex >= 0
                            font.pixelSize: 15
                            onClicked: mobileClient.reserve()
                        }
                        Button {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 50
                            visible: mobileClient.reserved
                            text: "取消当前预约"
                            font.pixelSize: 15
                            onClicked: mobileClient.cancelReservation()
                        }
                        Button {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 52
                            visible: !mobileClient.charging
                            text: mobileClient.selectedChargerIndex >= 0 || mobileClient.reserved ? "开始充电" : "请先选择充电桩"
                            enabled: mobileClient.loggedIn && !mobileClient.charging && (mobileClient.selectedChargerIndex >= 0 || mobileClient.reserved)
                            highlighted: true
                            font.pixelSize: 16
                            font.bold: true
                            onClicked: mobileClient.startCharge(modeBox.model.get(modeBox.currentIndex).value, Number(targetField.text))
                        }
                        Button {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 52
                            visible: mobileClient.charging
                            contentItem: Text { text: "停止并结算"; color: "white"; font.pixelSize: 16; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            background: Rectangle { radius: 16; color: red }
                            onClicked: mobileClient.stopCharge()
                        }
                    }
                }
                Label {
                    Layout.fillWidth: true
                    Layout.leftMargin: 22
                    Layout.rightMargin: 22
                    text: mobileClient.selectedChargerIndex >= 0 ? "已选择充电桩，可以预约或直接开始充电" : (mobileClient.reserved ? "已预约充电桩，可直接开始充电" : "请先到「找桩」页面选择站点与充电桩")
                    color: muted
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                }
                Item { Layout.preferredHeight: 24 }
            }
        }

        /* ============ 我的 ============ */
        ScrollView {
            id: profileScroll
            clip: true
            contentWidth: availableWidth
            ColumnLayout {
                width: profileScroll.availableWidth
                spacing: 14
                Item { Layout.preferredHeight: 4 }

                /* 服务器设置 */
                Card {
                    Layout.fillWidth: true
                    Layout.leftMargin: 14
                    Layout.rightMargin: 14
                    Layout.preferredHeight: 208
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 18
                        spacing: 10
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            ColumnLayout { Layout.fillWidth: true; spacing: 3
                                Label { text: "服务器设置"; color: ink; font.pixelSize: 17; font.bold: true }
                                Label { text: "真机需填写服务器电脑的局域网 IP"; color: muted; font.pixelSize: 12; wrapMode: Text.WordWrap } }
                            Rectangle {
                                Layout.preferredWidth: 46; Layout.preferredHeight: 46; radius: 15; color: accentSoft; Layout.alignment: Qt.AlignVCenter
                                Label { anchors.centerIn: parent; text: "⚙"; color: primary; font.pixelSize: 22 }
                            }
                        }
                        TextField {
                            id: hostField
                            Layout.fillWidth: true
                            implicitHeight: 48
                            text: "127.0.0.1"
                            placeholderText: "服务器地址"
                            background: Rectangle { radius: 14; color: fieldBg; border.width: 1; border.color: theme.border }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            TextField {
                                id: portField
                                Layout.fillWidth: true
                                implicitHeight: 48
                                text: "9527"
                                placeholderText: "端口"
                                inputMethodHints: Qt.ImhDigitsOnly
                                validator: IntValidator { bottom: 1; top: 65535 }
                                background: Rectangle { radius: 14; color: fieldBg; border.width: 1; border.color: theme.border }
                            }
                            Button {
                                Layout.preferredWidth: 120
                                Layout.preferredHeight: 48
                                text: mobileClient.connected ? "重新连接" : "TLS 连接"
                                highlighted: !mobileClient.connected
                                font.pixelSize: 14
                                onClicked: mobileClient.connectServer(hostField.text, Number(portField.text))
                            }
                        }
                    }
                }

                /* 账户 */
                Card {
                    Layout.fillWidth: true
                    Layout.leftMargin: 14
                    Layout.rightMargin: 14
                    Layout.preferredHeight: mobileClient.loggedIn ? 168 : 312
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 18
                        spacing: 12
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12
                            Rectangle {
                                Layout.preferredWidth: 48
                                Layout.preferredHeight: 48
                                radius: 24
                                color: mobileClient.loggedIn ? accentSoft : softBg
                                Layout.alignment: Qt.AlignVCenter
                                Label {
                                    anchors.centerIn: parent
                                    text: mobileClient.loggedIn ? mobileClient.userText.charAt(0) : "?"
                                    color: mobileClient.loggedIn ? primary : muted
                                    font.pixelSize: 20
                                    font.bold: true
                                }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 3
                                Label { text: mobileClient.loggedIn ? mobileClient.userText : "未登录"; color: ink; font.pixelSize: 17; font.bold: true }
                                Label { text: mobileClient.loggedIn ? "账户余额  ¥" + mobileClient.balance.toFixed(2) : "登录后可预约与充电"; color: mobileClient.loggedIn ? primary : muted; font.pixelSize: 14; font.bold: mobileClient.loggedIn }
                            }
                        }
                        TextField {
                            id: phoneField
                            Layout.fillWidth: true
                            implicitHeight: 48
                            visible: !mobileClient.loggedIn
                            placeholderText: "11 位手机号"
                            maximumLength: 11
                            inputMethodHints: Qt.ImhDigitsOnly
                            background: Rectangle { radius: 14; color: fieldBg; border.width: 1; border.color: theme.border }
                        }
                        TextField {
                            id: loginPassword
                            Layout.fillWidth: true
                            implicitHeight: 48
                            visible: !mobileClient.loggedIn
                            placeholderText: "密码（至少 6 位）"
                            echoMode: TextInput.Password
                            background: Rectangle { radius: 14; color: fieldBg; border.width: 1; border.color: theme.border }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            visible: !mobileClient.loggedIn
                            spacing: 10
                            Button {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 50
                                text: "登录"
                                enabled: mobileClient.connected
                                highlighted: true
                                font.pixelSize: 15
                                font.bold: true
                                onClicked: mobileClient.login(phoneField.text, loginPassword.text)
                            }
                            Button {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 50
                                text: "注册"
                                enabled: mobileClient.connected
                                font.pixelSize: 15
                                onClicked: registerDialog.open()
                            }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            visible: mobileClient.loggedIn
                            spacing: 10
                            Button {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 50
                                text: "账户充值"
                                font.pixelSize: 15
                                onClicked: rechargeDialog.open()
                            }
                            Button {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 50
                                text: "退出登录"
                                font.pixelSize: 15
                                onClicked: mobileClient.logout()
                            }
                        }
                        Label {
                            Layout.fillWidth: true
                            visible: !mobileClient.loggedIn
                            text: "连续 5 次密码错误将锁定账户"
                            color: theme.warning
                            font.pixelSize: 12
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                }

                /* 外观 */
                Card {
                    Layout.fillWidth: true
                    Layout.leftMargin: 14
                    Layout.rightMargin: 14
                    Layout.preferredHeight: 68
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 18
                        anchors.rightMargin: 12
                        Label { text: "深色模式"; color: ink; font.pixelSize: 16; font.bold: true }
                        Item { Layout.fillWidth: true }
                        Switch {
                            checked: theme.dark
                            onToggled: theme.dark = checked
                        }
                    }
                }

                /* 最近订单 */
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Label { text: "最近订单"; color: ink; font.pixelSize: 18; font.bold: true }
                    Item { Layout.fillWidth: true }
                    Button {
                        text: "刷新"
                        flat: true
                        font.pixelSize: 14
                        enabled: mobileClient.loggedIn
                        onClicked: mobileClient.refreshOrders()
                    }
                }
                ListView {
                    id: orderList
                    Layout.fillWidth: true
                    Layout.leftMargin: 14
                    Layout.rightMargin: 14
                    Layout.preferredHeight: Math.max(80, Math.min(420, contentHeight))
                    spacing: 12
                    clip: true
                    model: mobileClient.orders
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
                    delegate: Card {
                        width: orderList.width
                        height: 112
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 16
                            spacing: 6
                            RowLayout {
                                Layout.fillWidth: true
                                Label { text: modelData.station; color: ink; font.pixelSize: 15; font.bold: true; elide: Text.ElideRight; Layout.fillWidth: true }
                                Rectangle {
                                    Layout.preferredWidth: statusText.implicitWidth + 16
                                    Layout.preferredHeight: 24
                                    radius: 12
                                    color: orderStatusColor(modelData.status) === greenText ? theme.greenBg : (orderStatusColor(modelData.status) === muted ? softBg : accentSoft)
                                    Label {
                                        id: statusText
                                        anchors.centerIn: parent
                                        text: orderStatusText(modelData.status)
                                        color: orderStatusColor(modelData.status)
                                        font.pixelSize: 11
                                        font.bold: true
                                    }
                                }
                            }
                            Label { text: modelData.charger + " · 订单 #" + modelData.id; color: muted; font.pixelSize: 12 }
                            RowLayout {
                                Layout.fillWidth: true
                                Label { text: Number(modelData.energy).toFixed(2) + " kWh"; color: ink; font.pixelSize: 14 }
                                Item { Layout.fillWidth: true }
                                Label { text: "¥" + Number(modelData.amount).toFixed(2); color: primary; font.pixelSize: 16; font.bold: true }
                            }
                        }
                    }
                    Label {
                        anchors.centerIn: parent
                        visible: orderList.count === 0
                        text: mobileClient.loggedIn ? "暂无订单" : "登录后查看订单"
                        color: muted
                    }
                }
                Item { Layout.preferredHeight: 24 }
            }
        }
    }

    /* ============ 底部导航 ============ */
    Rectangle {
        id: bottomBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 76
        color: theme.card
        border.color: theme.border
        RowLayout {
            anchors.fill: parent
            anchors.topMargin: 6
            spacing: 0
            Repeater {
                model: [{ name: "首页", icon: "⌂" }, { name: "找桩", icon: "⌖" }, { name: "充电", icon: "⚡" }, { name: "我的", icon: "●" }]
                delegate: Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Rectangle {
                        id: tabPill
                        anchors.horizontalCenter: parent.horizontalCenter
                        y: 2
                        width: 58
                        height: 30
                        radius: 15
                        color: window.currentTab === index ? pillActive : "transparent"
                        Label {
                            anchors.centerIn: parent
                            text: modelData.icon
                            color: window.currentTab === index ? primary : theme.bottomInactive
                            font.pixelSize: 18
                            font.bold: window.currentTab === index
                        }
                    }
                    Label {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: tabPill.bottom
                        anchors.topMargin: 5
                        text: modelData.name
                        color: window.currentTab === index ? primary : theme.bottomInactiveText
                        font.pixelSize: 11
                        font.bold: window.currentTab === index
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            window.currentTab = index
                            if (index === 1 && mobileClient.connected) mobileClient.refreshStations()
                            if (index === 3 && mobileClient.loggedIn) mobileClient.refreshOrders()
                        }
                    }
                }
            }
        }
    }

    /* ============ 弹窗 ============ */
    Dialog {
        id: registerDialog
        title: "注册新账户"
        modal: true
        anchors.centerIn: parent
        width: Math.min(360, window.width - 34)
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: mobileClient.registerUser(phoneField.text, registerPassword.text, confirmPassword.text)
        ColumnLayout {
            width: parent.width
            spacing: 12
            Label { Layout.fillWidth: true; text: "手机号：" + phoneField.text; color: muted }
            TextField {
                id: registerPassword
                Layout.fillWidth: true
                implicitHeight: 48
                placeholderText: "设置密码（6～64 位）"
                echoMode: TextInput.Password
                background: Rectangle { radius: 14; color: fieldBg; border.width: 1; border.color: theme.border }
            }
            TextField {
                id: confirmPassword
                Layout.fillWidth: true
                implicitHeight: 48
                placeholderText: "再次输入密码"
                echoMode: TextInput.Password
                background: Rectangle { radius: 14; color: fieldBg; border.width: 1; border.color: theme.border }
            }
        }
    }

    Dialog {
        id: rechargeDialog
        title: "充值确认"
        modal: true
        anchors.centerIn: parent
        width: Math.min(360, window.width - 34)
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: mobileClient.recharge(Number(rechargeAmount.text), rechargePassword.text)
        ColumnLayout {
            width: parent.width
            spacing: 12
            TextField {
                id: rechargeAmount
                Layout.fillWidth: true
                implicitHeight: 48
                text: "100"
                placeholderText: "充值金额"
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                validator: DoubleValidator { bottom: .01; top: 10000 }
                background: Rectangle { radius: 14; color: fieldBg; border.width: 1; border.color: theme.border }
            }
            TextField {
                id: rechargePassword
                Layout.fillWidth: true
                implicitHeight: 48
                placeholderText: "输入登录密码确认"
                echoMode: TextInput.Password
                background: Rectangle { radius: 14; color: fieldBg; border.width: 1; border.color: theme.border }
            }
            Label {
                Layout.fillWidth: true
                text: "密码连续错误 5 次将锁定账户"
                color: theme.warning
                font.pixelSize: 12
                wrapMode: Text.WordWrap
            }
        }
    }

    Rectangle {
        id: toast
        visible: opacity > 0
        opacity: 0
        z: 20
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: bottomBar.top
        anchors.bottomMargin: 12
        width: Math.min(parent.width - 36, toastText.implicitWidth + 34)
        height: 48
        radius: 14
        color: theme.toastBg
        Label {
            id: toastText
            anchors.centerIn: parent
            color: "white"
            font.bold: true
        }
        Behavior on opacity { NumberAnimation { duration: 180 } }
        Timer {
            id: toastTimer
            interval: 2600
            onTriggered: toast.opacity = 0
        }
    }
    Connections {
        target: mobileClient
        onNotice: {
            toast.color = error ? theme.toastErrorBg : theme.toastBg
            toastText.text = text
            toast.opacity = 1
            toastTimer.restart()
        }
    }
}

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
    color: "#F3F6FA"
    Material.theme: Material.Light
    Material.primary: "#416FE3"
    Material.accent: "#416FE3"
    property int currentTab: 0
    property color primary: "#416FE3"
    property color ink: "#25324A"
    property color muted: "#7C8AA1"

    header: Rectangle {
        height: 104
        gradient: Gradient {
            GradientStop { position: 0; color: "#416FE3" }
            GradientStop { position: 1; color: "#6C91ED" }
        }
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 22
            anchors.rightMargin: 22
            ColumnLayout {
                spacing: 2
                Label { text: "悦充"; color: "white"; font.pixelSize: 28; font.bold: true }
                Label { text: "轻松找桩，安心充电"; color: "#E7EDFF"; font.pixelSize: 13 }
            }
            Item { Layout.fillWidth: true }
            Rectangle {
                implicitWidth: stateText.implicitWidth + 24
                implicitHeight: 34
                radius: 17
                color: mobileClient.connected ? "#2EFFFFFF" : "#25FFFFFF"
                Label { id: stateText; anchors.centerIn: parent; text: (mobileClient.connected ? "● " : "○ ") + mobileClient.connectionText; color: "white"; font.bold: true }
            }
        }
    }

    StackLayout {
        anchors.top: parent.top
        anchors.bottom: bottomBar.top
        anchors.left: parent.left
        anchors.right: parent.right
        currentIndex: window.currentTab

        ScrollView {
            id: chargePage
            clip: true
            contentWidth: availableWidth
            ColumnLayout {
                width: chargePage.availableWidth
                spacing: 14
                anchors.margins: 14

                Card {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 98
                    RowLayout {
                        anchors.fill: parent; anchors.margins: 18
                        ColumnLayout {
                            Label { text: mobileClient.loggedIn ? mobileClient.userText : "欢迎使用悦充"; color: ink; font.pixelSize: 17; font.bold: true }
                            Label { text: mobileClient.loggedIn ? "账户余额" : "请在“我的”页面登录"; color: muted; font.pixelSize: 13 }
                        }
                        Item { Layout.fillWidth: true }
                        Label { text: mobileClient.loggedIn ? "¥" + mobileClient.balance.toFixed(2) : "--"; color: primary; font.pixelSize: 25; font.bold: true }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Label { text: "附近充电站"; color: ink; font.pixelSize: 19; font.bold: true }
                    Item { Layout.fillWidth: true }
                    ToolButton { text: "刷新"; enabled: mobileClient.connected; onClicked: mobileClient.refreshStations() }
                }

                ListView {
                    id: stationList
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.max(148, Math.min(310, contentHeight))
                    spacing: 10
                    clip: true
                    model: mobileClient.stations
                    delegate: Card {
                        width: stationList.width
                        height: 104
                        border.width: index === mobileClient.selectedIndex ? 2 : 1
                        border.color: index === mobileClient.selectedIndex ? primary : "#E5EAF2"
                        RowLayout {
                            anchors.fill: parent; anchors.margins: 15
                            ColumnLayout {
                                Layout.fillWidth: true; spacing: 5
                                Label { text: modelData.name; color: ink; font.pixelSize: 16; font.bold: true; elide: Text.ElideRight; Layout.fillWidth: true }
                                Label { text: modelData.address; color: muted; font.pixelSize: 12; elide: Text.ElideRight; Layout.fillWidth: true }
                                Label { text: "¥" + Number(modelData.price).toFixed(2) + "/度"; color: primary; font.bold: true }
                            }
                            Rectangle {
                                implicitWidth: 66; implicitHeight: 34; radius: 17
                                color: modelData.idle > 0 ? "#E8F7F1" : "#FFF0F2"
                                Label { anchors.centerIn: parent; text: modelData.idle + "/" + modelData.total + " 空闲"; color: modelData.idle > 0 ? "#238765" : "#C74D5E"; font.pixelSize: 12; font.bold: true }
                            }
                        }
                        MouseArea { anchors.fill: parent; onClicked: mobileClient.selectStation(index) }
                    }
                    Label { anchors.centerIn: parent; visible: stationList.count === 0; text: mobileClient.connected ? "暂无可用站点，点击刷新" : "请先连接服务器"; color: muted }
                }

                Card {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 224
                    ColumnLayout {
                        anchors.fill: parent; anchors.margins: 16; spacing: 10
                        Label { text: "充电设置"; color: ink; font.pixelSize: 18; font.bold: true }
                        RowLayout {
                            Layout.fillWidth: true
                            ComboBox {
                                id: modeBox; Layout.fillWidth: true
                                textRole: "text"
                                model: ListModel { ListElement { text: "按金额"; value: "AMOUNT" } ListElement { text: "按电量"; value: "ENERGY" } ListElement { text: "按时间"; value: "TIME" } }
                            }
                            TextField { id: targetField; Layout.fillWidth: true; text: "10"; placeholderText: "充电目标"; inputMethodHints: Qt.ImhFormattedNumbersOnly; validator: DoubleValidator { bottom: 0.01; top: 9999 } }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            Button { Layout.fillWidth: true; text: mobileClient.reserved ? "已预约" : "预约 20 分钟"; enabled: mobileClient.loggedIn && !mobileClient.reserved && !mobileClient.charging; onClicked: mobileClient.reserve() }
                            Button { Layout.fillWidth: true; text: "取消预约"; enabled: mobileClient.reserved; flat: true; onClicked: mobileClient.cancelReservation() }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            Button { Layout.fillWidth: true; text: "开始充电"; enabled: mobileClient.loggedIn && !mobileClient.charging; highlighted: true; onClicked: mobileClient.startCharge(modeBox.model.get(modeBox.currentIndex).value, Number(targetField.text)) }
                            Button { Layout.fillWidth: true; text: "停止并结算"; enabled: mobileClient.charging; onClicked: mobileClient.stopCharge() }
                        }
                        Label { Layout.fillWidth: true; text: mobileClient.chargeStatus; color: mobileClient.charging ? "#238765" : muted; horizontalAlignment: Text.AlignHCenter; font.bold: mobileClient.charging }
                    }
                }
                Item { Layout.preferredHeight: 8 }
            }
        }

        Item {
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 14; spacing: 12
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: "我的订单"; color: ink; font.pixelSize: 21; font.bold: true }
                    Item { Layout.fillWidth: true }
                    ToolButton { text: "刷新"; enabled: mobileClient.loggedIn; onClicked: mobileClient.refreshOrders() }
                }
                ListView {
                    id: orderList
                    Layout.fillWidth: true; Layout.fillHeight: true; spacing: 10; clip: true
                    model: mobileClient.orders
                    delegate: Card {
                        width: orderList.width; height: 132
                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: 15; spacing: 4
                            RowLayout { Layout.fillWidth: true; Label { text: modelData.station; color: ink; font.pixelSize: 16; font.bold: true } Item { Layout.fillWidth: true } Label { text: modelData.status; color: modelData.status === "COMPLETED" ? "#238765" : primary; font.bold: true } }
                            Label { text: modelData.charger + "  ·  订单 #" + modelData.id; color: muted; font.pixelSize: 12 }
                            RowLayout { Layout.fillWidth: true; Label { text: Number(modelData.energy).toFixed(2) + " kWh"; color: ink } Item { Layout.fillWidth: true } Label { text: "¥" + Number(modelData.amount).toFixed(2); color: primary; font.pixelSize: 18; font.bold: true } }
                            Label { text: modelData.startAt; color: muted; font.pixelSize: 11 }
                        }
                    }
                    Label { anchors.centerIn: parent; visible: orderList.count === 0; text: mobileClient.loggedIn ? "暂无订单" : "登录后查看订单"; color: muted }
                }
            }
        }

        ScrollView {
            id: profilePage
            clip: true; contentWidth: availableWidth
            ColumnLayout {
                width: profilePage.availableWidth; spacing: 14; anchors.margins: 14
                Label { text: "我的"; color: ink; font.pixelSize: 22; font.bold: true }
                Card {
                    Layout.fillWidth: true; Layout.preferredHeight: 180
                    ColumnLayout {
                        anchors.fill: parent; anchors.margins: 16; spacing: 9
                        Label { text: "服务器设置"; color: ink; font.pixelSize: 17; font.bold: true }
                        Label { text: "真机请填写服务器所在电脑的局域网 IP"; color: muted; font.pixelSize: 12; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                        TextField { id: hostField; Layout.fillWidth: true; text: "127.0.0.1"; placeholderText: "服务器 IP" }
                        RowLayout { Layout.fillWidth: true; TextField { id: portField; Layout.fillWidth: true; text: "9527"; inputMethodHints: Qt.ImhDigitsOnly; validator: IntValidator { bottom: 1; top: 65535 } } Button { text: mobileClient.connected ? "重新连接" : "连接服务器"; onClicked: mobileClient.connectServer(hostField.text, Number(portField.text)) } }
                    }
                }
                Card {
                    Layout.fillWidth: true; Layout.preferredHeight: 210
                    ColumnLayout {
                        anchors.fill: parent; anchors.margins: 16; spacing: 10
                        Label { text: "账户"; color: ink; font.pixelSize: 17; font.bold: true }
                        TextField { id: phoneField; Layout.fillWidth: true; placeholderText: "11 位手机号"; maximumLength: 11; inputMethodHints: Qt.ImhDigitsOnly }
                        Button { Layout.fillWidth: true; text: mobileClient.loggedIn ? "已登录：" + mobileClient.userText : "登录 / 注册"; enabled: mobileClient.connected && !mobileClient.loggedIn; highlighted: true; onClicked: mobileClient.login(phoneField.text) }
                        RowLayout { Layout.fillWidth: true; TextField { id: rechargeField; Layout.fillWidth: true; text: "100"; placeholderText: "充值金额"; inputMethodHints: Qt.ImhFormattedNumbersOnly; validator: DoubleValidator { bottom: 0.01; top: 10000 } } Button { text: "模拟充值"; enabled: mobileClient.loggedIn; onClicked: mobileClient.recharge(Number(rechargeField.text)) } }
                        Label { Layout.fillWidth: true; text: mobileClient.loggedIn ? "余额 ¥" + mobileClient.balance.toFixed(2) : "尚未登录"; color: primary; font.pixelSize: 18; font.bold: true; horizontalAlignment: Text.AlignHCenter }
                    }
                }
                Card {
                    Layout.fillWidth: true; Layout.preferredHeight: 92
                    RowLayout { anchors.fill: parent; anchors.margins: 16; Label { text: "预约规则"; color: ink; font.bold: true } Item { Layout.fillWidth: true } Label { text: "有效期 20 分钟\n超时自动取消并提醒"; color: muted; horizontalAlignment: Text.AlignRight } }
                }
            }
        }
    }

    Rectangle {
        id: bottomBar
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
        height: 72; color: "white"; border.color: "#E3E8F0"
        RowLayout {
            anchors.fill: parent; anchors.leftMargin: 18; anchors.rightMargin: 18
            Repeater {
                model: ["充电", "订单", "我的"]
                delegate: Item {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    Column { anchors.centerIn: parent; spacing: 4; Label { anchors.horizontalCenter: parent.horizontalCenter; text: index === 0 ? "⚡" : index === 1 ? "▤" : "●"; color: window.currentTab === index ? primary : "#9AA6B8"; font.pixelSize: 20 } Label { anchors.horizontalCenter: parent.horizontalCenter; text: modelData; color: window.currentTab === index ? primary : "#7F8CA0"; font.bold: window.currentTab === index } }
                    MouseArea { anchors.fill: parent; onClicked: { window.currentTab=index; if(index===1 && mobileClient.loggedIn)mobileClient.refreshOrders() } }
                }
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
        width: Math.min(parent.width-36,toastText.implicitWidth+34)
        height: 48; radius: 12; color: "#E9344158"
        Label { id: toastText; anchors.centerIn: parent; color: "white"; font.bold: true }
        Behavior on opacity { NumberAnimation { duration: 180 } }
        Timer { id: toastTimer; interval: 2400; onTriggered: toast.opacity=0 }
    }
    Connections {
        target: mobileClient
        onNotice: { toast.color=error ? "#E9B94D5D" : "#E9344158"; toastText.text=text; toast.opacity=1; toastTimer.restart() }
    }
}

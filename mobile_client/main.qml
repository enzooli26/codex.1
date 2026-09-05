import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.5

ApplicationWindow {
    id: window; visible: true; width: 420; height: 820; minimumWidth: 360; minimumHeight: 640
    title: "悦充"; color: "#F4F7FB"; Material.theme: Material.Light; Material.primary: "#3F6FE5"; Material.accent: "#3F6FE5"
    property int currentTab: 0
    property color primary: "#3F6FE5"
    property color ink: "#25324A"
    property color muted: "#7C8AA1"

    header: Rectangle {
        height: 92; gradient: Gradient { GradientStop { position: 0; color: "#3567DE" } GradientStop { position: 1; color: "#6C91ED" } }
        RowLayout { anchors.fill: parent; anchors.margins: 20
            ColumnLayout { spacing: 1
                Label { text: ["悦充首页","附近充电站","充电中心","个人中心"][window.currentTab]; color: "white"; font.pixelSize: 23; font.bold: true }
                Label { text: "安全 · 便捷 · 低碳出行"; color: "#EAF0FF"; font.pixelSize: 12 }
            }
            Item { Layout.fillWidth: true }
            Rectangle { implicitWidth: secureText.implicitWidth+20; implicitHeight: 32; radius: 16; color: "#26FFFFFF"
                Label { id: secureText; anchors.centerIn: parent; text: mobileClient.connected ? "🔒 TLS 在线" : "○ 未连接"; color: "white"; font.pixelSize: 12; font.bold: true }
            }
        }
    }

    StackLayout {
        anchors.top: parent.top; anchors.bottom: bottomBar.top; anchors.left: parent.left; anchors.right: parent.right; currentIndex: window.currentTab

        ScrollView { id: homeScroll; clip: true; contentWidth: availableWidth
            ColumnLayout { width: homeScroll.availableWidth; spacing: 14
                Item { Layout.preferredHeight: 2 }
                Card { Layout.fillWidth: true; Layout.margins: 14; Layout.preferredHeight: 130
                    ColumnLayout { anchors.fill: parent; anchors.margins: 18; spacing: 7
                        Label { text: mobileClient.loggedIn ? "你好，"+mobileClient.userText : "欢迎使用悦充"; color: ink; font.pixelSize: 19; font.bold: true }
                        Label { text: mobileClient.loggedIn ? "账户余额  ¥"+mobileClient.balance.toFixed(2) : "登录后即可预约和充电"; color: mobileClient.loggedIn?primary:muted; font.pixelSize: 17; font.bold: mobileClient.loggedIn }
                        Button { Layout.fillWidth: true; text: mobileClient.loggedIn?"查看附近充电站":"连接并登录"; highlighted: true; onClicked: window.currentTab=mobileClient.loggedIn?1:3 }
                    }
                }
                RowLayout { Layout.fillWidth: true; Layout.leftMargin: 14; Layout.rightMargin: 14; spacing: 12
                    Card { Layout.fillWidth: true; Layout.preferredHeight: 116
                        Column { anchors.centerIn: parent; spacing: 7; Label{anchors.horizontalCenter:parent.horizontalCenter;text:"⌖";color:primary;font.pixelSize:28} Label{anchors.horizontalCenter:parent.horizontalCenter;text:"找充电站";color:ink;font.bold:true} Label{anchors.horizontalCenter:parent.horizontalCenter;text:mobileClient.stations.length+" 个站点";color:muted;font.pixelSize:12} }
                        MouseArea { anchors.fill: parent; onClicked: {window.currentTab=1;if(mobileClient.connected)mobileClient.refreshStations()} }
                    }
                    Card { Layout.fillWidth: true; Layout.preferredHeight: 116
                        Column { anchors.centerIn: parent; spacing: 7; Label{anchors.horizontalCenter:parent.horizontalCenter;text:"⚡";color:"#27A47A";font.pixelSize:27} Label{anchors.horizontalCenter:parent.horizontalCenter;text:"充电中心";color:ink;font.bold:true} Label{anchors.horizontalCenter:parent.horizontalCenter;text:mobileClient.charging?"正在充电":"暂无进行中";color:muted;font.pixelSize:12} }
                        MouseArea { anchors.fill: parent; onClicked: window.currentTab=2 }
                    }
                }
                Card { Layout.fillWidth: true; Layout.leftMargin: 14; Layout.rightMargin: 14; Layout.preferredHeight: 150
                    ColumnLayout { anchors.fill: parent; anchors.margins: 18; spacing: 8
                        Label{text:"服务保障";color:ink;font.pixelSize:17;font.bold:true} Label{text:"🔒  全程 TLS 加密通信";color:"#3F5574"} Label{text:"◷  预约保留 20 分钟";color:"#3F5574"} Label{text:"✓  服务端校验订单与账户";color:"#3F5574"}
                    }
                }
            }
        }

        Item { ColumnLayout { anchors.fill: parent; anchors.margins: 14; spacing: 10
            RowLayout { Layout.fillWidth: true; Label{text:"附近充电站";color:ink;font.pixelSize:20;font.bold:true} Item{Layout.fillWidth:true} Button{text:"刷新";flat:true;enabled:mobileClient.connected;onClicked:mobileClient.refreshStations()} }
            Label { text: "列表可上下滚动，点击卡片选择站点"; color: muted; font.pixelSize: 12 }
            ListView { id: stationList; Layout.fillWidth: true; Layout.fillHeight: true; spacing: 10; clip: true; boundsBehavior: Flickable.StopAtBounds; model: mobileClient.stations; ScrollBar.vertical: ScrollBar{policy:ScrollBar.AsNeeded}
                delegate: Card { width: stationList.width; height: 116; border.width:index===mobileClient.selectedIndex?2:1; border.color:index===mobileClient.selectedIndex?primary:"#E5EAF2"
                    RowLayout { anchors.fill: parent; anchors.margins: 15
                        ColumnLayout { Layout.fillWidth:true; spacing:5; Label{Layout.fillWidth:true;text:modelData.name;color:ink;font.pixelSize:16;font.bold:true;elide:Text.ElideRight} Label{Layout.fillWidth:true;text:modelData.address;color:muted;font.pixelSize:12;elide:Text.ElideRight} Label{text:"¥"+Number(modelData.price).toFixed(2)+"/度";color:primary;font.bold:true} }
                        Rectangle { implicitWidth:68;implicitHeight:36;radius:18;color:modelData.idle>0?"#E8F7F1":"#FFF0F2"; Label{anchors.centerIn:parent;text:modelData.idle+"/"+modelData.total+" 空闲";color:modelData.idle>0?"#238765":"#C74D5E";font.pixelSize:12;font.bold:true} }
                    }
                    MouseArea { anchors.fill: parent; onClicked: mobileClient.selectStation(index) }
                }
                Label { anchors.centerIn:parent;visible:stationList.count===0;text:mobileClient.connected?"暂无站点，点击刷新":"请先在“我的”中连接服务器";color:muted }
            }
            Button { Layout.fillWidth:true;text:mobileClient.selectedIndex>=0?"已选择，前往充电":"请先选择站点";enabled:mobileClient.selectedIndex>=0;highlighted:true;onClicked:window.currentTab=2 }
        } }

        ScrollView { id: chargeScroll; clip:true; contentWidth:availableWidth
            ColumnLayout { width:chargeScroll.availableWidth; spacing:14
                Item{Layout.preferredHeight:2}
                Card { Layout.fillWidth:true;Layout.margins:14;Layout.preferredHeight:92
                    RowLayout { anchors.fill:parent;anchors.margins:17; ColumnLayout{Label{text:"当前状态";color:muted} Label{text:mobileClient.chargeStatus;color:mobileClient.charging?"#238765":ink;font.bold:true;elide:Text.ElideRight;Layout.maximumWidth:280}} Item{Layout.fillWidth:true} Label{text:mobileClient.charging?"●":"○";color:mobileClient.charging?"#27A47A":"#A8B2C2";font.pixelSize:25} }
                }
                Card { Layout.fillWidth:true;Layout.leftMargin:14;Layout.rightMargin:14;Layout.preferredHeight:330
                    ColumnLayout { anchors.fill:parent;anchors.margins:17;spacing:10
                        Label{text:"充电设置";color:ink;font.pixelSize:18;font.bold:true}
                        ComboBox{id:modeBox;Layout.fillWidth:true;textRole:"text";model:ListModel{ListElement{text:"按金额";value:"AMOUNT"}ListElement{text:"按电量";value:"ENERGY"}ListElement{text:"按时间";value:"TIME"}}}
                        TextField{id:targetField;Layout.fillWidth:true;text:"10";placeholderText:"充电目标";inputMethodHints:Qt.ImhFormattedNumbersOnly;validator:DoubleValidator{bottom:.01;top:9999}}
                        Button{Layout.fillWidth:true;text:mobileClient.reserved?"已预约（20 分钟有效）":"预约充电桩";enabled:mobileClient.loggedIn&&!mobileClient.reserved&&!mobileClient.charging;onClicked:mobileClient.reserve()}
                        Button{Layout.fillWidth:true;text:"取消当前预约";visible:mobileClient.reserved;onClicked:mobileClient.cancelReservation()}
                        Button{Layout.fillWidth:true;text:"开始充电";enabled:mobileClient.loggedIn&&!mobileClient.charging&&mobileClient.selectedIndex>=0;highlighted:true;onClicked:mobileClient.startCharge(modeBox.model.get(modeBox.currentIndex).value,Number(targetField.text))}
                        Button{Layout.fillWidth:true;text:"停止并结算";visible:mobileClient.charging;onClicked:mobileClient.stopCharge()}
                    }
                }
                Label { Layout.fillWidth:true;Layout.margins:20;text:mobileClient.selectedIndex<0?"请先到“找桩”页面选择站点":"站点已选择，可以预约或开始充电";color:muted;wrapMode:Text.WordWrap }
            }
        }

        ScrollView { id: profileScroll;clip:true;contentWidth:availableWidth
            ColumnLayout { width:profileScroll.availableWidth;spacing:14
                Item{Layout.preferredHeight:2}
                Card { Layout.fillWidth:true;Layout.margins:14;Layout.preferredHeight:176
                    ColumnLayout { anchors.fill:parent;anchors.margins:16;spacing:9
                        Label{text:"服务器设置";color:ink;font.pixelSize:17;font.bold:true} Label{text:"真机需填写服务器电脑的局域网 IP";color:muted;font.pixelSize:12}
                        TextField{id:hostField;Layout.fillWidth:true;text:"127.0.0.1";placeholderText:"服务器地址"}
                        RowLayout{Layout.fillWidth:true;TextField{id:portField;Layout.fillWidth:true;text:"9527";inputMethodHints:Qt.ImhDigitsOnly;validator:IntValidator{bottom:1;top:65535}} Button{text:mobileClient.connected?"重新连接":"TLS 连接";onClicked:mobileClient.connectServer(hostField.text,Number(portField.text))}}
                    }
                }
                Card { Layout.fillWidth:true;Layout.leftMargin:14;Layout.rightMargin:14;Layout.preferredHeight:mobileClient.loggedIn?150:244
                    ColumnLayout { anchors.fill:parent;anchors.margins:16;spacing:9
                        Label{text:"账户";color:ink;font.pixelSize:17;font.bold:true}
                        Label{visible:mobileClient.loggedIn;text:mobileClient.userText+"    余额 ¥"+mobileClient.balance.toFixed(2);color:primary;font.pixelSize:18;font.bold:true}
                        TextField{id:phoneField;visible:!mobileClient.loggedIn;Layout.fillWidth:true;placeholderText:"11 位手机号";maximumLength:11;inputMethodHints:Qt.ImhDigitsOnly}
                        TextField{id:loginPassword;visible:!mobileClient.loggedIn;Layout.fillWidth:true;placeholderText:"密码（至少 6 位）";echoMode:TextInput.Password}
                        RowLayout{visible:!mobileClient.loggedIn;Layout.fillWidth:true;Button{Layout.fillWidth:true;text:"登录";enabled:mobileClient.connected;highlighted:true;onClicked:mobileClient.login(phoneField.text,loginPassword.text)} Button{Layout.fillWidth:true;text:"注册";enabled:mobileClient.connected;onClicked:registerDialog.open()}}
                        Button{visible:mobileClient.loggedIn;Layout.fillWidth:true;text:"账户充值";onClicked:rechargeDialog.open()}
                        Label{Layout.fillWidth:true;text:"连续 5 次密码错误将锁定账户";color:"#B16A32";font.pixelSize:12;horizontalAlignment:Text.AlignHCenter}
                    }
                }
                RowLayout { Layout.fillWidth:true;Layout.leftMargin:14;Layout.rightMargin:14;Label{text:"最近订单";color:ink;font.pixelSize:18;font.bold:true} Item{Layout.fillWidth:true} Button{text:"刷新";flat:true;enabled:mobileClient.loggedIn;onClicked:mobileClient.refreshOrders()} }
                ListView { id:orderList;Layout.fillWidth:true;Layout.leftMargin:14;Layout.rightMargin:14;Layout.preferredHeight:Math.max(160,Math.min(390,contentHeight));spacing:10;clip:true;model:mobileClient.orders;ScrollBar.vertical:ScrollBar{policy:ScrollBar.AsNeeded}
                    delegate:Card{width:orderList.width;height:110;ColumnLayout{anchors.fill:parent;anchors.margins:14;RowLayout{Layout.fillWidth:true;Label{text:modelData.station;color:ink;font.bold:true}Item{Layout.fillWidth:true}Label{text:modelData.status;color:modelData.status==="COMPLETED"?"#238765":primary;font.bold:true}}Label{text:modelData.charger+" · 订单 #"+modelData.id;color:muted;font.pixelSize:12}RowLayout{Layout.fillWidth:true;Label{text:Number(modelData.energy).toFixed(2)+" kWh";color:ink}Item{Layout.fillWidth:true}Label{text:"¥"+Number(modelData.amount).toFixed(2);color:primary;font.bold:true}}}}
                    Label{anchors.centerIn:parent;visible:orderList.count===0;text:mobileClient.loggedIn?"暂无订单":"登录后查看订单";color:muted}
                }
            }
        }
    }

    Rectangle { id:bottomBar;anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;height:72;color:"white";border.color:"#E3E8F0"
        RowLayout { anchors.fill:parent;anchors.leftMargin:10;anchors.rightMargin:10;spacing:0
            Repeater { model:[{name:"首页",icon:"⌂"},{name:"找桩",icon:"⌖"},{name:"充电",icon:"⚡"},{name:"我的",icon:"●"}]
                delegate:Item{Layout.fillWidth:true;Layout.fillHeight:true;Column{anchors.centerIn:parent;spacing:3;Label{anchors.horizontalCenter:parent.horizontalCenter;text:modelData.icon;color:window.currentTab===index?primary:"#9AA6B8";font.pixelSize:20}Label{anchors.horizontalCenter:parent.horizontalCenter;text:modelData.name;color:window.currentTab===index?primary:"#7F8CA0";font.bold:window.currentTab===index}}MouseArea{anchors.fill:parent;onClicked:{window.currentTab=index;if(index===1&&mobileClient.connected)mobileClient.refreshStations();if(index===3&&mobileClient.loggedIn)mobileClient.refreshOrders()}}}
            }
        }
    }

    Dialog { id:registerDialog;title:"注册新账户";modal:true;anchors.centerIn:parent;width:Math.min(360,window.width-34);standardButtons:Dialog.Ok|Dialog.Cancel;onAccepted:mobileClient.registerUser(phoneField.text,registerPassword.text,confirmPassword.text)
        ColumnLayout{width:parent.width;spacing:10;Label{Layout.fillWidth:true;text:"手机号："+phoneField.text;color:muted}TextField{id:registerPassword;Layout.fillWidth:true;placeholderText:"设置密码（6～64 位）";echoMode:TextInput.Password}TextField{id:confirmPassword;Layout.fillWidth:true;placeholderText:"再次输入密码";echoMode:TextInput.Password}}
    }
    Dialog { id:rechargeDialog;title:"充值确认";modal:true;anchors.centerIn:parent;width:Math.min(360,window.width-34);standardButtons:Dialog.Ok|Dialog.Cancel;onAccepted:mobileClient.recharge(Number(rechargeAmount.text),rechargePassword.text)
        ColumnLayout{width:parent.width;spacing:10;TextField{id:rechargeAmount;Layout.fillWidth:true;text:"100";placeholderText:"充值金额";inputMethodHints:Qt.ImhFormattedNumbersOnly;validator:DoubleValidator{bottom:.01;top:10000}}TextField{id:rechargePassword;Layout.fillWidth:true;placeholderText:"输入登录密码确认";echoMode:TextInput.Password}Label{Layout.fillWidth:true;text:"密码连续错误 5 次将锁定账户";color:"#B16A32";font.pixelSize:12;wrapMode:Text.WordWrap}}
    }
    Rectangle{id:toast;visible:opacity>0;opacity:0;z:20;anchors.horizontalCenter:parent.horizontalCenter;anchors.bottom:bottomBar.top;anchors.bottomMargin:12;width:Math.min(parent.width-36,toastText.implicitWidth+34);height:48;radius:12;color:"#E9344158";Label{id:toastText;anchors.centerIn:parent;color:"white";font.bold:true}Behavior on opacity{NumberAnimation{duration:180}}Timer{id:toastTimer;interval:2600;onTriggered:toast.opacity=0}}
    Connections{target:mobileClient;onNotice:{toast.color=error?"#E9B94D5D":"#E9344158";toastText.text=text;toast.opacity=1;toastTimer.restart()}}
}

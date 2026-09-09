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
    property bool authRegisterMode: false

    header: Rectangle {
        visible: mobileClient.loggedIn; height: visible ? 92 : 0; gradient: Gradient { GradientStop { position: 0; color: "#3567DE" } GradientStop { position: 1; color: "#6C91ED" } }
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
        visible: mobileClient.loggedIn; anchors.top: parent.top; anchors.bottom: bottomBar.top; anchors.left: parent.left; anchors.right: parent.right; currentIndex: window.currentTab

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
            TextField { Layout.fillWidth:true; placeholderText:"搜索站名、地址或充电桩编号"; onTextChanged:mobileClient.setStationSearch(text) }
            Label { text: "每张卡片对应一个具体充电桩，点击后选择"; color: muted; font.pixelSize: 12 }
            ListView { id: stationList; Layout.fillWidth: true; Layout.fillHeight: true; spacing: 10; clip: true; boundsBehavior: Flickable.StopAtBounds; model: mobileClient.stations; ScrollBar.vertical: ScrollBar{policy:ScrollBar.AsNeeded}
                delegate: Card { width: stationList.width; height: 116; border.width:index===mobileClient.selectedIndex?2:1; border.color:index===mobileClient.selectedIndex?primary:"#E5EAF2"
                    RowLayout { anchors.fill: parent; anchors.margins: 15
                        ColumnLayout { Layout.fillWidth:true; spacing:4; Label{Layout.fillWidth:true;text:modelData.name+" · "+modelData.chargerCode;color:ink;font.pixelSize:15;font.bold:true;elide:Text.ElideRight} Label{Layout.fillWidth:true;text:modelData.address;color:muted;font.pixelSize:12;elide:Text.ElideRight} Label{text:modelData.chargerType+" · "+Number(modelData.ratedPower).toFixed(0)+" kW · ¥"+Number(modelData.price).toFixed(2)+"/度";color:primary;font.bold:true;font.pixelSize:12} }
                        Rectangle { implicitWidth:68;implicitHeight:36;radius:18;color:modelData.chargerStatus==="IDLE"?"#E8F7F1":"#FFF0F2"; Label{anchors.centerIn:parent;text:modelData.chargerStatus==="IDLE"?"空闲":modelData.chargerStatus;color:modelData.chargerStatus==="IDLE"?"#238765":"#C74D5E";font.pixelSize:12;font.bold:true} }
                    }
                    MouseArea { anchors.fill: parent; onClicked: mobileClient.selectStation(index) }
                }
                Label { anchors.centerIn:parent;visible:stationList.count===0;text:mobileClient.connected?"暂无站点，点击刷新":"请先在“我的”中连接服务器";color:muted }
            }
            Button { Layout.fillWidth:true;text:mobileClient.selectedIndex>=0?"已选择充电桩，前往充电":"请先选择充电桩";enabled:mobileClient.selectedIndex>=0;highlighted:true;onClicked:window.currentTab=2 }
        } }

        ScrollView { id: chargeScroll; clip:true; contentWidth:availableWidth
            ColumnLayout { width:chargeScroll.availableWidth; spacing:14
                Item{Layout.preferredHeight:2}
                Card { Layout.fillWidth:true;Layout.margins:14;Layout.preferredHeight:92
                    RowLayout { anchors.fill:parent;anchors.margins:17; ColumnLayout{Label{text:"当前状态";color:muted} Label{text:mobileClient.chargeStatus;color:mobileClient.charging?"#238765":ink;font.bold:true;elide:Text.ElideRight;Layout.maximumWidth:280}} Item{Layout.fillWidth:true} Label{text:mobileClient.charging?"●":"○";color:mobileClient.charging?"#27A47A":"#A8B2C2";font.pixelSize:25} }
                }
                Card { Layout.fillWidth:true;Layout.leftMargin:14;Layout.rightMargin:14;Layout.preferredHeight:340
                    ColumnLayout { anchors.fill:parent;anchors.margins:17;spacing:10
                        Label{text:"充电设置";color:ink;font.pixelSize:18;font.bold:true}
                        ComboBox{id:modeBox;Layout.fillWidth:true;textRole:"text";model:ListModel{ListElement{text:"按金额";value:"AMOUNT"}ListElement{text:"按电量";value:"ENERGY"}ListElement{text:"按时间";value:"TIME"}}}
                        RowLayout { Layout.fillWidth:true
                            TextField{id:targetField;Layout.fillWidth:true;text:"10";placeholderText:modeBox.currentIndex===0?"充值金额":modeBox.currentIndex===1?"目标电量":"充电时间";inputMethodHints:Qt.ImhFormattedNumbersOnly;validator:DoubleValidator{bottom:.01;top:9999}}
                            Label{text:modeBox.currentIndex===0?"元":modeBox.currentIndex===1?"kWh":"分钟";color:primary;font.bold:true;Layout.preferredWidth:42}
                        }
                        Button{Layout.fillWidth:true;text:mobileClient.reserved?"已预约（20 分钟有效）":"预约充电桩";enabled:mobileClient.loggedIn&&!mobileClient.reserved&&!mobileClient.charging&&mobileClient.selectedIndex>=0&&mobileClient.stations[mobileClient.selectedIndex].chargerStatus==="IDLE";onClicked:mobileClient.reserve()}
                        Button{Layout.fillWidth:true;text:"取消当前预约";visible:mobileClient.reserved;onClicked:mobileClient.cancelReservation()}
                        Button{Layout.fillWidth:true;text:"开始充电";enabled:mobileClient.loggedIn&&!mobileClient.charging&&mobileClient.selectedIndex>=0&&mobileClient.stations[mobileClient.selectedIndex].chargerStatus==="IDLE";highlighted:true;onClicked:mobileClient.startCharge(modeBox.model.get(modeBox.currentIndex).value,Number(targetField.text))}
                        Button{Layout.fillWidth:true;text:"停止并结算";visible:mobileClient.charging;onClicked:mobileClient.stopCharge()}
                    }
                }
                Card { visible:mobileClient.charging;Layout.fillWidth:true;Layout.leftMargin:14;Layout.rightMargin:14;Layout.preferredHeight:330
                    ColumnLayout { anchors.fill:parent;anchors.margins:16;spacing:8
                        RowLayout { Layout.fillWidth:true;Label{text:"实时充电数据";color:ink;font.pixelSize:18;font.bold:true}Item{Layout.fillWidth:true}Label{text:"预计剩余 "+mobileClient.remainingText;color:primary;font.bold:true} }
                        GridLayout { Layout.fillWidth:true;columns:2;columnSpacing:10;rowSpacing:8
                            Repeater { model:[{n:"电压",v:mobileClient.voltage,u:"V"},{n:"电流",v:mobileClient.current,u:"A"},{n:"功率",v:mobileClient.power,u:"kW"},{n:"SOC",v:mobileClient.soc,u:"%"}]
                                delegate:Rectangle { Layout.fillWidth:true;implicitHeight:55;radius:9;color:"#F3F6FC";Column{anchors.centerIn:parent;Label{anchors.horizontalCenter:parent.horizontalCenter;text:modelData.n;color:muted;font.pixelSize:11}Label{anchors.horizontalCenter:parent.horizontalCenter;text:Number(modelData.v).toFixed(1)+" "+modelData.u;color:ink;font.pixelSize:17;font.bold:true}}}
                            }
                        }
                        Canvas { id:liveChart;Layout.fillWidth:true;Layout.fillHeight:true
                            onPaint:{var c=getContext("2d");c.clearRect(0,0,width,height);c.fillStyle="#F8FAFE";c.fillRect(0,0,width,height);var a=mobileClient.liveSamples;if(a.length<2)return;var colors=["#3F6FE5","#27A47A","#F29B38","#B85FD3"],keys=["voltage","current","power","soc"],maxs=[420,400,200,100];for(var k=0;k<4;k++){c.beginPath();c.strokeStyle=colors[k];c.lineWidth=2;for(var i=0;i<a.length;i++){var x=i*width/Math.max(1,a.length-1),y=height-(Number(a[i][keys[k]])/maxs[k])*height;if(i===0)c.moveTo(x,y);else c.lineTo(x,y)}c.stroke()}}
                            Connections{target:mobileClient;onLiveChanged:liveChart.requestPaint()}
                        }
                        Label{text:"蓝 电压   绿 电流   橙 功率   紫 SOC（归一化曲线）";color:muted;font.pixelSize:11}
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
                Card { Layout.fillWidth:true;Layout.leftMargin:14;Layout.rightMargin:14;Layout.preferredHeight:mobileClient.loggedIn?205:170
                    ColumnLayout { anchors.fill:parent;anchors.margins:16;spacing:9
                        Label{text:"账户";color:ink;font.pixelSize:17;font.bold:true}
                        Label{visible:mobileClient.loggedIn;text:mobileClient.userText+"    余额 ¥"+mobileClient.balance.toFixed(2);color:primary;font.pixelSize:18;font.bold:true}
                        RowLayout{visible:!mobileClient.loggedIn;Layout.fillWidth:true;Button{Layout.fillWidth:true;text:"登录";enabled:mobileClient.connected;highlighted:true;onClicked:loginDialog.open()} Button{Layout.fillWidth:true;text:"注册";enabled:mobileClient.connected;onClicked:registerDialog.open()}}
                        Button{visible:mobileClient.loggedIn;Layout.fillWidth:true;text:"账户充值";onClicked:rechargeDialog.open()}
                        Button{visible:mobileClient.loggedIn;Layout.fillWidth:true;text:"退出登录";flat:true;onClicked:mobileClient.logout()}
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

    Rectangle { id:bottomBar;visible:mobileClient.loggedIn;anchors.left:parent.left;anchors.right:parent.right;anchors.bottom:parent.bottom;height:visible?72:0;color:"white";border.color:"#E3E8F0"
        RowLayout { anchors.fill:parent;anchors.leftMargin:10;anchors.rightMargin:10;spacing:0
            Repeater { model:[{name:"首页",icon:"⌂"},{name:"找桩",icon:"⌖"},{name:"充电",icon:"⚡"},{name:"我的",icon:"●"}]
                delegate:Item{Layout.fillWidth:true;Layout.fillHeight:true;Column{anchors.centerIn:parent;spacing:3;Label{anchors.horizontalCenter:parent.horizontalCenter;text:modelData.icon;color:window.currentTab===index?primary:"#9AA6B8";font.pixelSize:20}Label{anchors.horizontalCenter:parent.horizontalCenter;text:modelData.name;color:window.currentTab===index?primary:"#7F8CA0";font.bold:window.currentTab===index}}MouseArea{anchors.fill:parent;onClicked:{window.currentTab=index;if(index===1&&mobileClient.connected)mobileClient.refreshStations();if(index===3&&mobileClient.loggedIn)mobileClient.refreshOrders()}}}
            }
        }
    }

    Rectangle {
        id: authPage; anchors.fill: parent; z: 30; visible: !mobileClient.loggedIn; color: "#F2F5FA"
        ScrollView { anchors.fill: parent; contentWidth: availableWidth; clip: true
            ColumnLayout { width: parent.width; spacing: 14
                Item { Layout.preferredHeight: 22 }
                ColumnLayout { Layout.fillWidth: true; Layout.leftMargin: 26; Layout.rightMargin: 26; spacing: 3
                    Rectangle { Layout.preferredWidth: 54; Layout.preferredHeight: 54; radius: 17; color: primary
                        Label { anchors.centerIn: parent; text: "⚡"; color: "white"; font.pixelSize: 27 }
                    }
                    Label { text: "悦充"; color: ink; font.pixelSize: 30; font.bold: true }
                    Label { text: "安全连接，轻松开启每一次充电"; color: muted; font.pixelSize: 13 }
                }
                Card { Layout.fillWidth: true; Layout.leftMargin: 18; Layout.rightMargin: 18; Layout.preferredHeight: 168
                    ColumnLayout { anchors.fill: parent; anchors.margins: 16; spacing: 9
                        RowLayout { Layout.fillWidth: true; Label { text: "服务器"; color: ink; font.pixelSize: 16; font.bold: true } Item { Layout.fillWidth: true } Label { text: mobileClient.connected ? "● TLS 已连接" : "○ 未连接"; color: mobileClient.connected ? "#23906C" : "#C45667"; font.bold: true } }
                        TextField { id: authHostField; Layout.fillWidth: true; text: "127.0.0.1"; placeholderText: "服务器 IP" }
                        RowLayout { Layout.fillWidth: true
                            TextField { id: authPortField; Layout.fillWidth: true; text: "9527"; placeholderText: "端口"; inputMethodHints: Qt.ImhDigitsOnly; validator: IntValidator { bottom: 1; top: 65535 } }
                            Button { text: mobileClient.connected ? "重新连接" : "TLS 连接"; highlighted: true; onClicked: mobileClient.connectServer(authHostField.text, Number(authPortField.text)) }
                        }
                    }
                }
                Card { Layout.fillWidth: true; Layout.leftMargin: 18; Layout.rightMargin: 18; Layout.preferredHeight: authRegisterMode ? 330 : 270
                    ColumnLayout { anchors.fill: parent; anchors.margins: 18; spacing: 11
                        RowLayout { Layout.fillWidth: true
                            Button { Layout.fillWidth: true; text: "登录"; flat: authRegisterMode; highlighted: !authRegisterMode; onClicked: authRegisterMode=false }
                            Button { Layout.fillWidth: true; text: "注册"; flat: !authRegisterMode; highlighted: authRegisterMode; onClicked: authRegisterMode=true }
                        }
                        Label { text: authRegisterMode ? "创建新账户" : "欢迎回来"; color: ink; font.pixelSize: 20; font.bold: true }
                        TextField { id: authPhone; Layout.fillWidth: true; placeholderText: "11 位手机号"; maximumLength: 11; inputMethodHints: Qt.ImhDigitsOnly }
                        TextField { id: authPassword; Layout.fillWidth: true; placeholderText: authRegisterMode ? "设置密码（6～64 位）" : "密码"; echoMode: TextInput.Password }
                        TextField { id: authConfirmPassword; visible: authRegisterMode; Layout.fillWidth: true; placeholderText: "再次输入密码"; echoMode: TextInput.Password }
                        Button { Layout.fillWidth: true; highlighted: true; enabled: mobileClient.connected; text: authRegisterMode ? "注册并进入" : "登录并进入"; onClicked: { if(authRegisterMode) mobileClient.registerUser(authPhone.text,authPassword.text,authConfirmPassword.text); else mobileClient.login(authPhone.text,authPassword.text) } }
                        Label { Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; text: mobileClient.connected ? "密码将通过 TLS 加密传输" : "请先建立 TLS 安全连接"; color: mobileClient.connected ? "#23906C" : muted; font.pixelSize: 12 }
                    }
                }
                Item { Layout.preferredHeight: 18 }
            }
        }
    }

    Dialog { id:loginDialog;title:"用户登录";modal:true;anchors.centerIn:parent;width:Math.min(360,window.width-34);standardButtons:Dialog.Ok|Dialog.Cancel;onAccepted:mobileClient.login(loginPhone.text,loginPassword.text)
        ColumnLayout{width:parent.width;spacing:10;TextField{id:loginPhone;Layout.fillWidth:true;placeholderText:"11 位手机号";maximumLength:11;inputMethodHints:Qt.ImhDigitsOnly}TextField{id:loginPassword;Layout.fillWidth:true;placeholderText:"密码";echoMode:TextInput.Password}}
    }
    Dialog { id:registerDialog;title:"注册新账户";modal:true;anchors.centerIn:parent;width:Math.min(360,window.width-34);standardButtons:Dialog.Ok|Dialog.Cancel;onAccepted:mobileClient.registerUser(registerPhone.text,registerPassword.text,confirmPassword.text)
        ColumnLayout{width:parent.width;spacing:10;TextField{id:registerPhone;Layout.fillWidth:true;placeholderText:"11 位手机号";maximumLength:11;inputMethodHints:Qt.ImhDigitsOnly}TextField{id:registerPassword;Layout.fillWidth:true;placeholderText:"设置密码（6～64 位）";echoMode:TextInput.Password}TextField{id:confirmPassword;Layout.fillWidth:true;placeholderText:"再次输入密码";echoMode:TextInput.Password}}
    }
    Dialog { id:rechargeDialog;title:"充值确认";modal:true;anchors.centerIn:parent;width:Math.min(360,window.width-34);standardButtons:Dialog.Ok|Dialog.Cancel;onAccepted:mobileClient.recharge(Number(rechargeAmount.text),rechargePassword.text)
        ColumnLayout{width:parent.width;spacing:10;TextField{id:rechargeAmount;Layout.fillWidth:true;text:"100";placeholderText:"充值金额";inputMethodHints:Qt.ImhFormattedNumbersOnly;validator:DoubleValidator{bottom:.01;top:10000}}TextField{id:rechargePassword;Layout.fillWidth:true;placeholderText:"输入登录密码确认";echoMode:TextInput.Password}Label{Layout.fillWidth:true;text:"密码连续错误 5 次将锁定账户";color:"#B16A32";font.pixelSize:12;wrapMode:Text.WordWrap}}
    }
    Rectangle{id:toast;visible:opacity>0;opacity:0;z:20;anchors.horizontalCenter:parent.horizontalCenter;anchors.bottom:bottomBar.top;anchors.bottomMargin:12;width:Math.min(parent.width-36,toastText.implicitWidth+34);height:48;radius:12;color:"#E9344158";Label{id:toastText;anchors.centerIn:parent;color:"white";font.bold:true}Behavior on opacity{NumberAnimation{duration:180}}Timer{id:toastTimer;interval:2600;onTriggered:toast.opacity=0}}
    Connections{target:mobileClient;onNotice:{toast.color=error?"#E9B94D5D":"#E9344158";toastText.text=text;toast.opacity=1;toastTimer.restart()}}
}

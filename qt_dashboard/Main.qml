import QtQuick 2.15
import QtQuick.Window 2.15

Window {
    width: 480
    height: 360
    visible: true
    title: "边缘网关监控面板"
    color: "#1e1e1e"

    Column {
        anchors.centerIn: parent
        spacing: 30

        Text {
            text: "Edge Gateway Dashboard"
            color: "#00d2ff"
            font.pixelSize: 28
            font.bold: true
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Row {
            spacing: 20
            anchors.horizontalCenter: parent.horizontalCenter

            // 温度卡片
            Rectangle {
                width: 160
                height: 120
                color: "#2d2d2d"
                radius: 10
                border.color: "#00d2ff"
                border.width: 1
                Column {
                    anchors.centerIn: parent
                    spacing: 10
                    Text { text: "当前温度"; color: "#aaaaaa"; font.pixelSize: 16; anchors.horizontalCenter: parent.horizontalCenter }
                    Text { text: backend.currentTemp.toFixed(1) + " ℃"; color: "#ffffff"; font.pixelSize: 36; font.bold: true; anchors.horizontalCenter: parent.horizontalCenter }
                }
            }

            // 湿度卡片
            Rectangle {
                width: 160
                height: 120
                color: "#2d2d2d"
                radius: 10
                border.color: "#00ff00"
                border.width: 1
                Column {
                    anchors.centerIn: parent
                    spacing: 10
                    Text { text: "当前湿度"; color: "#aaaaaa"; font.pixelSize: 16; anchors.horizontalCenter: parent.horizontalCenter }
                    Text { text: backend.currentHum.toFixed(1) + " %"; color: "#ffffff"; font.pixelSize: 36; font.bold: true; anchors.horizontalCenter: parent.horizontalCenter }
                }
            }
        }

        Text {
            text: "系统状态: 正常运行"
            color: "#00ff00"
            font.pixelSize: 14
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
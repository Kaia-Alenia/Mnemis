import QtQuick
import QtQuick.Controls

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Mnemis")

    Text {
        anchors.centerIn: parent
        text: qsTr("Welcome to Mnemis (Fase 1)")
        font.pixelSize: 24
    }
}

import QtQuick 2.15
import QtQuick.Controls 2.15

ApplicationWindow {
    visible: true
    width: 400
    height: 500
    title: "WiFi Scanner"

    Column {
        anchors.centerIn: parent
        spacing: 10

        Button {
            text: "Scan WiFi"
            onClicked: {
                console.log(" Scanning WiFi...");
                wifiManager.scanNetworks();
            }
        }

        ListView {
            width: 300
            height: 400
            model: wifiManager.wifiNetworks
            delegate: Text {
                text: modelData
                font.pixelSize: 16
            }
        }
    }
}

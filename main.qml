import QtQuick 2.15
import QtQuick.Controls 2.15



ApplicationWindow {
    visible: true
    width: 400
    height: 600
    title: "WiFi Manager"

    Column {
        anchors.centerIn: parent
        spacing: 10

        // Scan WiFi Button
        Button {
            text: "Scan WiFi"
            onClicked: wifiManager.scanNetworks()
        }

        // WiFi List View
        ListView {
            id: wifiListView
            width: parent.width
            height: 300
            model: wifiManager.wifiNetworks
            delegate: Item {
                width: parent.width
                height: 50

                Row {
                    spacing: 10
                    anchors.fill: parent

                    Rectangle {
                        width: 80
                        height: 30
                        color: "#f0f0f0"
                        border.color: "#888"
                        radius: 5

                        Text {
                            anchors.centerIn: parent
                            text: modelData
                        }
                    }

                    Button {
                        text: "Connect"
                        enabled: connectedSSID !== modelData  // Disable if already connected
                        onClicked: {
                            selectedSSID = modelData;
                            wifiPasswordDialog.visible = true;
                        }
                    }

                    Button {
                        text: "Disconnect"
                        visible: connectedSSID === modelData  // Show only if connected
                        onClicked: {
                            wifiManager.disconnectFromWiFi();
                        }
                    }
                }
            }
        }
    }

    // Password Input Dialog
    Dialog {
        id: wifiPasswordDialog
        title: "Enter WiFi Password"
        modal: true
        width: 300
        standardButtons: Dialog.Ok | Dialog.Cancel

        Column {
            spacing: 10
            width: parent.width

            Text {
                text: "Connecting to: " + selectedSSID
            }

            TextField {
                id: passwordField
                width: parent.width
                placeholderText: "Enter WiFi Password"
                echoMode: TextInput.Password
            }
        }

        onAccepted: {
            wifiManager.connectToWiFi(selectedSSID, passwordField.text);
            wifiPasswordDialog.visible = false;
        }
    }

    Dialog {
        id: messageDialog
        title: "WiFi Connection Status"
        width: 300
        modal: true
        standardButtons: Dialog.Ok

        contentItem: Text {
            id: messageText
            text: ""
            wrapMode: Text.WordWrap
        }
    }

    // Update the message correctly in Connections
    Connections {
        target: wifiManager
        function onConnectionStatusChanged(success, message) {
            messageText.text = message;
            messageDialog.visible = true;

            if (success) {
                connectedSSID = selectedSSID; // Store the connected WiFi SSID
            } else {
                connectedSSID = ""; //Clear if connection failed
            }
        }
    }

    property string selectedSSID: ""
    property string connectedSSID: ""  // Track connected WiFi SSID
}

#include "wifi_manager.h"
#include <windows.h>
#include <wlanapi.h>
#include <objbase.h>
#include <wtypes.h>
#include <QDebug>
#include <QProcess>
#include <QFile>
#include <QDir>


WiFiManager::WiFiManager(QObject *parent) : QObject(parent) {}

QStringList WiFiManager::wifiNetworks() const {
    return m_wifiNetworks;
}

void WiFiManager::scanNetworks() {
    HANDLE hClient = NULL;
    DWORD dwMaxClient = 2;
    DWORD dwCurVersion = 0;
    DWORD dwResult = WlanOpenHandle(dwMaxClient, NULL, &dwCurVersion, &hClient);

    if (dwResult != ERROR_SUCCESS) {
        qDebug() << " WlanOpenHandle failed!";
        return;
    }

    PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
    PWLAN_AVAILABLE_NETWORK_LIST pBssList = NULL;

    if (WlanEnumInterfaces(hClient, NULL, &pIfList) != ERROR_SUCCESS) {
        qDebug() << " WlanEnumInterfaces failed!";
        WlanCloseHandle(hClient, NULL);
        return;
    }

    m_wifiNetworks.clear();

    for (int i = 0; i < (int)pIfList->dwNumberOfItems; i++) {
        PWLAN_INTERFACE_INFO pIfInfo = (WLAN_INTERFACE_INFO *)&pIfList->InterfaceInfo[i];

        if (WlanGetAvailableNetworkList(hClient, &pIfInfo->InterfaceGuid, 0, NULL, &pBssList) != ERROR_SUCCESS) {
            qDebug() << " WlanGetAvailableNetworkList failed!";
            continue;
        }

        for (int j = 0; j < (int)pBssList->dwNumberOfItems; j++) {
            WLAN_AVAILABLE_NETWORK pBssEntry = pBssList->Network[j];
            QString ssid = QString::fromUtf8((char*)pBssEntry.dot11Ssid.ucSSID, pBssEntry.dot11Ssid.uSSIDLength);
            if (!ssid.isEmpty()) {
                m_wifiNetworks.append(ssid);
            }
        }

        WlanFreeMemory(pBssList);
    }

    WlanFreeMemory(pIfList);
    WlanCloseHandle(hClient, NULL);

    emit wifiNetworksChanged();
}

// void WiFiManager::connectToWiFi(const QString &ssid, const QString &password) {

//     QProcess process;
//     QString command = QString("netsh wlan connect name=\"%1\" ssid=\"%1\" key=\"%2\"")
//                           .arg(ssid, password);

//     qDebug() << "Backend:" << command;
//     process.start(command);
//     process.waitForFinished();

//     if (process.exitCode() == 0) {
//         emit connectionStatusChanged(true, "Connected to " + ssid);
//     } else {
//         emit connectionStatusChanged(false, "Failed to connect to " + ssid);
//     }
// }
void WiFiManager::connectToWiFi(const QString &ssid, const QString &password) {
    QProcess process;
    QString profileName = "CustomProfile";  // Temporary profile name

    // Step 1: Create a WiFi profile XML
    QString xmlContent = QString(
                             "<?xml version=\"1.0\"?>"
                             "<WLANProfile xmlns=\"http://www.microsoft.com/networking/WLAN/profile/v1\">"
                             "<name>%1</name>"
                             "<SSIDConfig><SSID><name>%1</name></SSID></SSIDConfig>"
                             "<connectionType>ESS</connectionType>"
                             "<connectionMode>auto</connectionMode>"
                             "<MSM><security><authEncryption>"
                             "<authentication>WPA2PSK</authentication>"
                             "<encryption>AES</encryption>"
                             "<useOneX>false</useOneX>"
                             "</authEncryption>"
                             "<sharedKey><keyType>passPhrase</keyType>"
                             "<protected>false</protected>"
                             "<keyMaterial>%2</keyMaterial>"
                             "</sharedKey></security></MSM></WLANProfile>"
                             ).arg(ssid, password);

    // Step 2: Save to a temporary file
    QString tempFilePath = QDir::temp().filePath("wifi_profile.xml");
    QFile file(tempFilePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << xmlContent;
        file.close();
    } else {
        emit connectionStatusChanged(false, "Failed to create WiFi profile file.");
        return;
    }

    // Step 3: Add the profile
    process.start("netsh", QStringList() << "wlan" << "add" << "profile" << "filename=" + tempFilePath << "user=current");
    process.waitForFinished();

    if (process.exitCode() != 0) {
        emit connectionStatusChanged(false, "Failed to add WiFi profile.");
        return;
    }

    // Step 4: Try to connect to WiFi
    process.start("netsh", QStringList() << "wlan" << "connect" << "name=" + ssid);
    process.waitForFinished();

    QString output = process.readAllStandardOutput();
    qDebug() << "WiFi Connection Output:" << output;

    if (output.contains("completed successfully", Qt::CaseInsensitive)) {
        emit connectionStatusChanged(true, "Connected to " + ssid);
    } else {
        emit connectionStatusChanged(false, "Incorrect password or failed to connect.");
    }
}



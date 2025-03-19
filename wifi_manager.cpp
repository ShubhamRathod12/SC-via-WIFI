#include "wifi_manager.h"
#include <windows.h>
#include <wlanapi.h>
#include <objbase.h>
#include <wtypes.h>
#include <QDebug>
#include <QProcess>
#include <QFile>
#include <QDir>
#include <QRegularExpression>


#pragma comment(lib, "wlanapi.lib")


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
        qDebug() << " Please Turn On your Wifi!";
        WlanCloseHandle(hClient, NULL);
        return;
    }

    m_wifiNetworks.clear();

    for (int i = 0; i < (int)pIfList->dwNumberOfItems; i++) {
        PWLAN_INTERFACE_INFO pIfInfo = (WLAN_INTERFACE_INFO *)&pIfList->InterfaceInfo[i];

        if (WlanGetAvailableNetworkList(hClient, &pIfInfo->InterfaceGuid, 0, NULL, &pBssList) != ERROR_SUCCESS) {
            qDebug() << " Please Turn On your Wifi!!";
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


void WiFiManager::connectToWiFi(const QString &ssid, const QString &password) {
    QProcess process;

    // 🔹 Step 1: Disconnect from WiFi (prevents old credentials from being reused)
    process.start("netsh", QStringList() << "wlan" << "disconnect");
    process.waitForFinished();

    // 🔹 Step 2: Fully remove the existing profile
    process.start("cmd", QStringList() << "/c" << "netsh wlan delete profile name=\"" + ssid + "\"");
    process.waitForFinished();

    // 🔹 Step 3: Verify if profile was deleted
    process.start("netsh", QStringList() << "wlan" << "show" << "profiles");
    process.waitForFinished();
    QString profileList = process.readAllStandardOutput();
    if (profileList.contains(ssid, Qt::CaseInsensitive)) {
        qDebug() << "Profile still exists! Trying to remove again.";
        process.start("cmd", QStringList() << "/c" << "netsh wlan delete profile name=\"" + ssid + "\"");
        process.waitForFinished();
    }

    // 🔹 Step 4: Create a WiFi profile XML with new password
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
                             "<sharedKey>"
                             "<keyType>passPhrase</keyType>"
                             "<protected>false</protected>"
                             "<keyMaterial>%2</keyMaterial>"
                             "</sharedKey>"
                             "</security></MSM>"
                             "</WLANProfile>"
                             ).arg(ssid, password);

    // 🔹 Step 5: Save XML to a temporary file
    QString tempFilePath = QDir::temp().filePath("wifi_profile.xml");
    QFile file(tempFilePath);
    if (file.exists()) {
        QFile::remove(tempFilePath);
    }
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << xmlContent;
        file.close();
    } else {
        emit connectionStatusChanged(false, "Failed to create WiFi profile file.");
        return;
    }

    // 🔹 Step 6: Add the profile with the new password
    process.start("netsh", QStringList() << "wlan" << "add" << "profile" << "filename=" + tempFilePath << "user=current");
    process.waitForFinished();

    // Delete the temporary XML file
    if (QFile::remove(tempFilePath)) {
        qDebug() << "Temporary WiFi profile file removed successfully.";
    } else {
        qDebug() << "Failed to remove WiFi profile file.";
    }

    // 🔹 Step 7: Try to connect using the new password
    process.start("netsh", QStringList() << "wlan" << "connect" << "name=" + ssid);
    process.waitForFinished();

    // 🔹 Step 8: Handle connection success/failure
    QString output = process.readAllStandardOutput();
    QString errorOutput = process.readAllStandardError();
    qDebug() << "WiFi Connection Output:" << output;
    qDebug() << "WiFi Connection Errors:" << errorOutput;

    if (output.contains("completed successfully", Qt::CaseInsensitive)) {
        emit connectionStatusChanged(true, "Connected to " + ssid);
    } else {
        emit connectionStatusChanged(false, "Incorrect password or failed to connect.\nError: " + errorOutput);
    }
}

// void WiFiManager::disconnectFromWiFi() {
//     QProcess process;


//     process.start("netsh wlan show interfaces");
//     process.waitForFinished();
//     QString output = process.readAllStandardOutput();
//     qDebug() << "Raw netsh Output:\n" << output;



//     QString interfaces;
//     QRegularExpression regex(R"(Name\s*:\s*([^\r\n]+))");
//     QRegularExpressionMatch match = regex.match(output);

//     if (match.hasMatch()) {
//         interfaces = match.captured(1).trimmed();
//         qDebug() << "Extracted Interface Name:" << interfaces;
//     }

//     if (interfaces.isEmpty()) {
//         qDebug() << " Failed to detect WiFi interface!";
//         emit connectionStatusChanged(false, "Failed to detect WiFi interface.");
//         return;
//     }


//     process.start("netsh", QStringList() << "wlan" << "disconnect" << "interface=" + interfaces);
//     process.waitForFinished();

//     QString disconnectOutput = process.readAllStandardOutput();
//     QString errorOutput = process.readAllStandardError();
//     qDebug() << "WiFi Disconnect Output:\n" << disconnectOutput;
//     qDebug() << "WiFi Disconnect Errors:\n" << errorOutput;

//     if (disconnectOutput.contains("disconnected", Qt::CaseInsensitive)) {
//         emit connectionStatusChanged(false, "Disconnected from WiFi");
//     } else {
//         emit connectionStatusChanged(false, "Failed to disconnect");
//     }
// }

void WiFiManager::disconnectFromWiFi() {
    HANDLE hClient = NULL;
    DWORD dwMaxClient = 2;
    DWORD dwCurVersion = 0;
    DWORD dwResult = WlanOpenHandle(dwMaxClient, NULL, &dwCurVersion, &hClient);

    if (dwResult != ERROR_SUCCESS) {
        qDebug() << " WlanOpenHandle failed!";
        emit connectionStatusChanged(false, "Failed to open WLAN handle.");
        return;
    }

    PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
    dwResult = WlanEnumInterfaces(hClient, NULL, &pIfList);

    if (dwResult != ERROR_SUCCESS) {
        qDebug() << " WlanEnumInterfaces failed!";
        WlanCloseHandle(hClient, NULL);
        emit connectionStatusChanged(false, "Failed to detect WiFi interfaces.");
        return;
    }

    if (pIfList->dwNumberOfItems == 0) {
        qDebug() << " No WiFi interfaces found!";
        WlanFreeMemory(pIfList);
        WlanCloseHandle(hClient, NULL);
        emit connectionStatusChanged(false, "No WiFi interfaces found.");
        return;
    }

    // Use the first available WiFi interface
    PWLAN_INTERFACE_INFO pIfInfo = &pIfList->InterfaceInfo[0];
    GUID interfaceGuid = pIfInfo->InterfaceGuid;

    // 🔹 Disconnect from WiFi
    dwResult = WlanDisconnect(hClient, &interfaceGuid, NULL);

    if (dwResult == ERROR_SUCCESS) {
        qDebug() << "Successfully disconnected from WiFi.";
        emit connectionStatusChanged(false, "Disconnected from WiFi.");
    } else {
        qDebug() << "WlanDisconnect failed!";
        emit connectionStatusChanged(false, "Failed to disconnect from WiFi.");
    }

    // Free resources
    WlanFreeMemory(pIfList);
    WlanCloseHandle(hClient, NULL);
}
// nXon@0987

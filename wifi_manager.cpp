
#include "wifi_manager.h"
#include <QDebug>
#include <QProcess>
#include <QFile>
#include <QDir>

#ifdef Q_OS_WIN
#include <windows.h>
#include <wlanapi.h>
#include <objbase.h>
#include <wtypes.h>
//#pragma comment(lib, "wlanapi.lib")
#endif

WiFiManager::WiFiManager(QObject *parent) : QObject(parent) {
     scanNetworks();
}

QStringList WiFiManager::wifiNetworks() const {
    return m_wifiNetworks;
}

void WiFiManager::scanNetworks() {
    m_wifiNetworks.clear(); // Clear previous scan results
    QProcess process;

#ifdef _WIN32
    HANDLE hClient = NULL;
    DWORD dwMaxClient = 2;
    DWORD dwCurVersion = 0;
    DWORD dwResult = WlanOpenHandle(dwMaxClient, NULL, &dwCurVersion, &hClient);

    if (dwResult != ERROR_SUCCESS) {
        qDebug() << "WlanOpenHandle failed!";
        emit wifiNetworksChanged();
        return;
    }

    PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
    PWLAN_AVAILABLE_NETWORK_LIST pBssList = NULL;

    if (WlanEnumInterfaces(hClient, NULL, &pIfList) != ERROR_SUCCESS) {
        qDebug() << "Please Turn On your Wifi!";
        WlanCloseHandle(hClient, NULL);
        emit wifiNetworksChanged();
        return;
    }

    for (int i = 0; i < (int)pIfList->dwNumberOfItems; i++) {
        PWLAN_INTERFACE_INFO pIfInfo = &pIfList->InterfaceInfo[i];

        if (WlanGetAvailableNetworkList(hClient, &pIfInfo->InterfaceGuid, 0, NULL, &pBssList) != ERROR_SUCCESS) {
            qDebug() << "Please Turn On your Wifi!";
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

#elif defined(__linux__)

    process.start("nmcli", QStringList() << "-t" << "-f" << "SSID" << "dev" << "wifi" << "list");
    process.waitForFinished();

    QString output = process.readAllStandardOutput();
    QStringList ssidList = output.split("\n", Qt::SkipEmptyParts);

    for (const QString &ssid : ssidList) {
        QString trimmedSSID = ssid.trimmed();
        if (!trimmedSSID.isEmpty()) {
            m_wifiNetworks.append(trimmedSSID);
        }
    }

#endif

    emit wifiNetworksChanged();
}



void WiFiManager::connectToWiFi(const QString &ssid, const QString &password) {
    QProcess process;

#ifdef _WIN32
    // Windows: Create XML WiFi profile
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

    // Save XML to a temporary file
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

    // Add profile
    process.start("netsh", QStringList() << "wlan" << "add" << "profile" << "filename=" + tempFilePath << "user=current");
    process.waitForFinished();

    // Delete the temporary XML file
    QFile::remove(tempFilePath);

    // Connect to WiFi
    process.start("netsh", QStringList() << "wlan" << "connect" << "name=" + ssid);
    process.waitForFinished();

#elif defined(__linux__)
    // Linux: Use nmcli to connect
    process.start("nmcli", QStringList() << "device" << "wifi" << "connect" << ssid << "password" << password);
    process.waitForFinished();
#endif

    // Handle connection result
    QString output = process.readAllStandardOutput();
    QString errorOutput = process.readAllStandardError();
    qDebug() << "WiFi Connection Output:" << output;
    qDebug() << "WiFi Connection Errors:" << errorOutput;

    if (output.contains("successfully", Qt::CaseInsensitive) || output.contains("completed successfully", Qt::CaseInsensitive)) {
        emit connectionStatusChanged(true, "Connected to " + ssid);
    } else {
        emit connectionStatusChanged(false, "Failed to connect to WiFi.\nError: " + errorOutput);
    }
}





// nXon@0987


void WiFiManager::disconnectFromWiFi() {
#ifdef _WIN32

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

    //  Disconnect from WiFi
    dwResult = WlanDisconnect(hClient, &interfaceGuid, NULL);

    if (dwResult == ERROR_SUCCESS) {
        qDebug() << "Successfully disconnected from WiFi.";
        emit connectionStatusChanged(false, "Disconnected from WiFi.");
    } else {
        qDebug() << "WlanDisconnect failed!";
        emit connectionStatusChanged(false, "Failed to disconnect from WiFi.");
    }

    // Free resourc
    WlanFreeMemory(pIfList);
    WlanCloseHandle(hClient, NULL);

#else
    QProcess process;
    process.start("nmcli device disconnect wlpls0");  // Change wlan0 to your interface
    process.waitForFinished();

    QString output = process.readAllStandardOutput();
    QString errorOutput = process.readAllStandardError();

    if (output.contains("successfully disconnected", Qt::CaseInsensitive)) {
        emit connectionStatusChanged(false, "Disconnected from WiFi.");
    } else {
        emit connectionStatusChanged(false, "Failed to disconnect.");
    }
#endif
}


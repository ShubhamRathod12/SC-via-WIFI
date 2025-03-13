#include "wifi_manager.h"
#include <windows.h>
#include <wlanapi.h>
#include <objbase.h>
#include <wtypes.h>
#include <QDebug>

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

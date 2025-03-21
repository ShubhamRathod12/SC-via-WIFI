// #ifndef WIFI_MANAGER_H
// #define WIFI_MANAGER_H

// #include <QObject>
// #include <QTimer>
// #include <QStringList>

// class WiFiManager : public QObject {
//     Q_OBJECT
//     Q_PROPERTY(QStringList wifiNetworks READ wifiNetworks NOTIFY wifiNetworksChanged)

// public:
//     explicit WiFiManager(QObject *parent = nullptr);
//     QStringList wifiNetworks() const;
//     Q_INVOKABLE void scanNetworks();
//     Q_INVOKABLE void connectToWiFi(const QString &ssid, const QString &password);
//     Q_INVOKABLE void disconnectFromWiFi();


// signals:
//     void wifiNetworksChanged();
//     void connectionStatusChanged(bool success, QString message);
//     void wifiDisconnected();



// private:
//     QStringList m_wifiNetworks;
//     QTimer *timer;
// };

// #endif // WIFI_MANAGER_H

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <QObject>
#include <QTimer>
#include <QStringList>

class WiFiManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList wifiNetworks READ wifiNetworks NOTIFY wifiNetworksChanged)
    Q_PROPERTY(QString connectedSSID READ getConnectedSSID NOTIFY connectedSSIDChanged)

public:
    explicit WiFiManager(QObject *parent = nullptr);
    QStringList wifiNetworks() const;
    Q_INVOKABLE void scanNetworks();
    Q_INVOKABLE void connectToWiFi(const QString &ssid, const QString &password);
    Q_INVOKABLE void disconnectFromWiFi();
    Q_INVOKABLE QString getConnectedSSID();  // New function

signals:
    void wifiNetworksChanged();
    void connectedSSIDChanged();
    void connectionStatusChanged(bool success, QString message);
    void wifiDisconnected();

private:
    QStringList m_wifiNetworks;
    QString m_connectedSSID;  // Store connected SSID
    QTimer *timer;
};

#endif // WIFI_MANAGER_H

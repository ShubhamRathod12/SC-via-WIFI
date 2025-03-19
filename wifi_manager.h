#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <QObject>
#include <QStringList>

class WiFiManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList wifiNetworks READ wifiNetworks NOTIFY wifiNetworksChanged)

public:
    explicit WiFiManager(QObject *parent = nullptr);
    QStringList wifiNetworks() const;
    Q_INVOKABLE void scanNetworks();
    Q_INVOKABLE void connectToWiFi(const QString &ssid, const QString &password);
    Q_INVOKABLE void disconnectFromWiFi();

signals:
    void wifiNetworksChanged();
    void connectionStatusChanged(bool success, QString message);



private:
    QStringList m_wifiNetworks;
};

#endif // WIFI_MANAGER_H

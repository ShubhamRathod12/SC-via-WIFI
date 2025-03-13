#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <QObject>
#include <QStringList>

class WiFiManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList wifiNetworks READ wifiNetworks NOTIFY wifiNetworksChanged)

public:
    explicit WiFiManager(QObject *parent = nullptr);
    QStringList wifiNetworks() const;

public slots:
    void scanNetworks();

signals:
    void wifiNetworksChanged();

private:
    QStringList m_wifiNetworks;
};

#endif // WIFIMANAGER_H

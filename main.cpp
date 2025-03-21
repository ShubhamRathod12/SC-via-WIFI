
// #include <QGuiApplication>
// #include <QQmlApplicationEngine>
// #include <QQmlContext>
// #include "wifi_manager.h"

// int main(int argc, char *argv[]) {
//     QGuiApplication app(argc, argv);
//     QQmlApplicationEngine engine;
//     WiFiManager wifiManager;


//     engine.rootContext()->setContextProperty("wifiManager", &wifiManager);

//     engine.load(QUrl(QStringLiteral("qrc:/bluetooth/main.qml")));

//     if (engine.rootObjects().isEmpty())
//         return -1;

//     return app.exec();
// }

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "wifi_manager.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;
    WiFiManager wifiManager;

    engine.rootContext()->setContextProperty("wifiManager", &wifiManager);

    // Check if WiFi is already connected
    QString connectedSSID = wifiManager.getConnectedSSID();
    if (!connectedSSID.isEmpty()) {
        qDebug() << "WiFi is already connected to:" << connectedSSID;
    } else {
        qDebug() << "No WiFi connection detected.";
    }

    engine.load(QUrl(QStringLiteral("qrc:/bluetooth/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QNetworkRequest>
#include "backend.h"

Backend::Backend(QObject *parent) : QObject(parent) {
    m_manager = new QNetworkAccessManager(this);
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &Backend::fetchMetrics);
    timer->start(5000); // 5秒刷新
}

void Backend::fetchMetrics() {
    // 请求网关的 /api/data 接口
    QNetworkReply *reply = m_manager->get(QNetworkRequest(QUrl("http://127.0.0.1:8080/api/data")));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QString response = QString::fromUtf8(reply->readAll()).trimmed();
            QStringList parts = response.split(",");
            if (parts.size() == 2) {
                m_currentTemp = parts[0].toDouble();
                m_currentHum = parts[1].toDouble();
                emit dataChanged(); // 通知界面刷新
            }
        }
        reply->deleteLater();
    });
}

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    Backend backend;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("backend", &backend);
    engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));
    if (engine.rootObjects().isEmpty()) return -1;
    return app.exec();
}
#ifndef BACKEND_H
#define BACKEND_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>

class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(double currentTemp READ currentTemp NOTIFY dataChanged)
    Q_PROPERTY(double currentHum READ currentHum NOTIFY dataChanged)

public:
    explicit Backend(QObject *parent = nullptr);
    double currentTemp() const { return m_currentTemp; }
    double currentHum() const { return m_currentHum; }

public slots:
    void fetchMetrics();

signals:
    void dataChanged();

private:
    QNetworkAccessManager *m_manager;
    double m_currentTemp = 0.0;
    double m_currentHum = 0.0;
};
#endif
#ifndef NETHELPER_H
#define NETHELPER_H

#include <QObject>
#include <QtNetwork>
class Nethelper : public QObject
{
    Q_OBJECT
public:
    explicit Nethelper(QObject *parent = nullptr);
private:
    QNetworkAccessManager *manager;
    QNetworkReply *reply;
Q_SIGNALS:
	
	
private Q_SLOTS:

    void replyFinished(QNetworkReply *);
public Q_SLOTS:
};

#endif // NETHELPER_H

#ifndef ERPCBASE_H
#define ERPCBASE_H

#include <QObject>
#include <QtNetwork>
class QNetworkAccessManager;
class QNetworkReply;
//class QUrl;
class ERpcbase : public QObject
{
    Q_OBJECT
public:
    explicit ERpcbase(QObject *parent = nullptr);
    QByteArray RpcResult;

private:
    QNetworkAccessManager *manager;
    QNetworkReply *reply;
    QUrl url;//访问目标ip和端口
    QString RpcUser;
    QString RpcPasswd;
    QString RpcMethod;
    QString RpcParams;


Q_SIGNALS:

//signals:
    void finished();

private Q_SLOTS:
    void replyFinished(QNetworkReply *);
//public slots:
public Q_SLOTS:

    int setUrl(QString turl);
    int setRpcUser(QString name);
    int setRpcPasswd(QString passwd);
    int setParams(QString params);
    int setMethod(QString method);
    int sendPost();
};

#endif // ERPCBASE_H

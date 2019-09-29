#ifndef BTCCONFIRM_H
#define BTCCONFIRM_H

#include <QObject>
#include <QFile>
class QTextStream;
class ERpcbase;
class BTCconfirm : public QObject
{
    Q_OBJECT
public:
    explicit BTCconfirm(QObject *parent = nullptr);
    int index = 0;
    int nowindex = 0;
    void getblock(QString hash);
    void getblockhash(int i);
    ERpcbase * rpcreq;
    //QTextStream out;
    QByteArray tt;
    QFile file;

Q_SIGNALS:


public Q_SLOTS:

    void cal();
    void cal2();
};

#endif // BTCCONFIRM_H

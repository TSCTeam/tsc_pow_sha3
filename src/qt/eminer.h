#ifndef EMINER_H
#define EMINER_H

#include <QObject>
#include <QFile>
#include "../uint256.h"
#include <iostream>
#include <QRunnable>
#include "../primitives/block.h"
#include <QMutex>
class ERpcbase;
#include <QTimer>
class EMiner : public QObject
{
    Q_OBJECT
public:
    explicit EMiner(QObject *parent = nullptr);
    bool ECheckProofOfWork(uint256 hash, unsigned int nBits);
    ERpcbase * rpcreq;
    ERpcbase * rpcsub;
    int minheight;
    CBlock nblock;
    QMutex mutex;
	QTimer * timer;
    int getwork();
    int submitblock(std::string hex);
    void subcaler();
	void buyticket();

Q_SIGNALS:
    void calsuccess();


public Q_SLOTS:
    void calheader();
    void submitfinished();
    void calsubmit(uint64_t num);
	void updatetime();
};

class subminer : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit subminer(CBlock blk,int index, int cores_count);
    int subindex;
    uint64_t sublength;
    CBlock nblk;
    void run();
    ~subminer();
    bool subCheckProofOfWork(uint256 hash, unsigned int nBits);
Q_SIGNALS:
    void calsuccess(uint64_t num);


public Q_SLOTS:



};

#endif // EMINER_H

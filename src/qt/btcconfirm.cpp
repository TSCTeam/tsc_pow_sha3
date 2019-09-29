#include "btcconfirm.h"
//#include "uint128_t.h"
#include "erpcbase.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QByteArray>
BTCconfirm::BTCconfirm(QObject *parent) : QObject(parent)
{

    file.setFileName("err1.txt");
    //方式：Append为追加，WriteOnly，ReadOnly
    if (!file.open(QIODevice::WriteOnly|QIODevice::Text)) {

        //return -1;
    }

    //file.close();


}
QByteArray stoe(QByteArray exam)
{  QByteArray res;
    int lenth = exam.length();
    for(int i = 0;i < lenth;i++)
    {
       res.append(exam.at(lenth - 1- i));

    }
    return res;
}
QByteArray cal_hash(uint32_t nVersion,uint32_t nTime,uint32_t nNonce,uint32_t nBits,QByteArray prev_block,QByteArray mrkl_root)
{
    QByteArray result;
    result.append( QByteArray::fromHex(QByteArray::number(qToBigEndian(nVersion),16)));
    result.append(stoe(QByteArray::fromHex(prev_block)));
    result.append(stoe(QByteArray::fromHex(mrkl_root)));
    result.append( QByteArray::fromHex(QByteArray::number(qToBigEndian(nTime),16)));
    result.append( QByteArray::fromHex(QByteArray::number(qToBigEndian(nBits),16)));
    result.append( QByteArray::fromHex(QByteArray::number(qToBigEndian(nNonce),16)));
    QByteArray hash_result;
    hash_result = QCryptographicHash::hash(result,QCryptographicHash::Sha256 );
    hash_result = QCryptographicHash::hash(hash_result,QCryptographicHash::Sha256 );
    return hash_result;
}
void BTCconfirm::cal()
{

      // qDebug()<<rpcreq->RpcResult;
       QByteArray test = rpcreq->RpcResult;
       QJsonParseError json_error;
       QJsonDocument parse_doucment = QJsonDocument::fromJson(test, &json_error);
       if(json_error.error == QJsonParseError::NoError)
       {
           if(parse_doucment.isObject())
           {
               QJsonObject obj = parse_doucment.object();
               if(obj.contains("result"))
               {
                   QJsonValue stake_value = obj.take("result");
                   if(stake_value.isString())
                   {
                       QString name = stake_value.toString();
                       //qDebug()<<"gethash"<<QString::number(index)<<"  "<< name;
                       ERpcbase * temp = rpcreq;
                       temp->deleteLater();
                       QString hash;
                       hash.append("\"");
                       hash.append(name);
                       hash.append("\"");
                       getblock(hash);



                   }
               }
               else
               {
                    qDebug()<<"json_parse_result_error";

               }
           }
       }
       else
       {
           qDebug()<<"json_parse_error";

       }



}

void BTCconfirm::cal2()
{
    uint32_t nTime = 0;
    uint32_t nNonce = 0;
    uint32_t nBits = 0;
    int32_t nVersion = 0;
    QByteArray prev_block = "" ;
    QByteArray mrkl_root = "";
    QString nowhash ;
   // qDebug()<<"start prase";
   // qDebug()<<rpcreq->RpcResult;
    tt = rpcreq->RpcResult;;
    //qDebug()<<tt;
    QByteArray test = rpcreq->RpcResult;
    QJsonParseError json_error;
    QJsonDocument parse_doucment = QJsonDocument::fromJson(test, &json_error);
    if(json_error.error == QJsonParseError::NoError)
    {
        if(parse_doucment.isObject())
        {
            QJsonObject obj = parse_doucment.object();
            if(obj.contains("result"))
            {
                QJsonValue result_value = obj.take("result");
                if(result_value.isObject())
                {
                    QJsonObject result = result_value.toObject();
                    if(result.contains("version"))
                    {
                        QJsonValue version_value = result.take("version");
                       // qDebug()<<"version :  "<<version_value.toInt();
                        nVersion = version_value.toInt();
                    }
                    if(result.contains("nonce"))
                    {
                        QJsonValue nonce_value = result.take("nonce");
                        //qDebug()<<"nouce :  "<<quint32(nonce_value.toDouble());
                        nNonce = quint32(nonce_value.toDouble());


                       // nNonce = nonce_value.toString();
                    }
                    if(result.contains("previousblockhash"))
                    {
                        QJsonValue previousblockhash_value = result.take("previousblockhash");
                       // qDebug()<<"previousblockhash :  "<<previousblockhash_value.toString();
                        prev_block = previousblockhash_value.toString().toLatin1();
                    }
                    if(result.contains("merkleroot"))
                    {
                        QJsonValue merkleroot_value = result.take("merkleroot");
                       // qDebug()<<"merkleroot :  "<<merkleroot_value.toString();
                        mrkl_root = merkleroot_value.toString().toLatin1();
                    }
                    if(result.contains("bits"))
                    {
                        QJsonValue bits_value = result.take("bits");
                        //QByteArray::fromHex(bits_value.toString()).toInt();
                        //qDebug()<<"bits :  "<<QByteArray::fromHex(bits_value.toString()).toInt();
                        //bits_value.toString();
                        QByteArray n0 ,n1,n2,n3,n4,n5,n6,n7;
                        QByteArray temp = bits_value.toString().toLatin1();
                        n0[0] = temp.at(0);
                        n1[0] = temp.at(1);
                        n2[0] = temp.at(2);
                        n3[0] = temp.at(3);
                        n4[0] = temp.at(4);
                        n5[0] = temp.at(5);
                        n6[0] = temp.at(6);
                        n7[0] = temp.at(7);
                        int bits = n7.toInt(NULL,16) + n6.toInt(NULL,16)*16 + n5.toInt(NULL,16) *16 *16 +n4.toInt(NULL,16)*16*16*16+n3.toInt(NULL,16)*16*16*16*16+n2.toInt(NULL,16)*16*16*16*16*16+n1.toInt(NULL,16)*16*16*16*16*16*16+n0.toInt(NULL,16)*16*16*16*16*16*16*16;
                        nBits = bits;

                       // qDebug()<<"bits :  "<<bits;
                    }
                    if(result.contains("time"))
                    {
                        QJsonValue time_value = result.take("time");
                       // qDebug()<<"time :  "<<time_value.toInt();
                        nTime = time_value.toInt();
                    }
                    if(result.contains("hash"))
                    {
                        QJsonValue hash_value = result.take("hash");
                       // qDebug()<<"hash :  "<<hash_value.toString();
                        nowhash = hash_value.toString();
                    }






                }
            }
        }
    }


QByteArray ans ;
QString calhash;

ans = cal_hash(nVersion,nTime,nNonce,nBits,prev_block,mrkl_root);

ans = stoe(ans);
calhash.append(ans.toHex());


if(calhash.compare(nowhash) == 0)
{qDebug() << "校验通过number--  "<<nowindex;
}
else
{
    qDebug()<<"errnumber--  "<<nowindex;
//    qDebug()<<"cal"<<calhash;
//    qDebug()<<"now"<<nowhash;
//    qDebug()<<"nTime"<<nTime;
//    qDebug()<<"nNonce"<<nNonce;
//    qDebug()<<"nBits"<<nBits;
//    qDebug()<<"nVersion"<<nVersion;
//    qDebug()<<"prev_block"<<prev_block;
//    qDebug()<<"mrkl_root"<<mrkl_root;
    QTextStream out(&file);
    out<<"errnumber--  "<<nowindex<<endl;
    out<<"/****************************************************"<<endl;
    out<<tt<<endl;
    out<<"****************************************************/"<<endl;
    out<<"cal"<<"       "<<calhash<<endl;
    out<<"now"<<"       "<<nowhash<<endl;
    out<<"nTime"<<"       "<<nTime<<endl;
    out<<"nNonce"<<"       "<<nNonce<<endl;
    out<<"nBits"<<"       "<<nBits<<endl;
    out<<"nVersion"<<"       "<<nVersion<<endl;
    out<<"prev_block"<<"       "<<prev_block<<endl;
    out<<"mrkl_root "<<"       "<<mrkl_root<<endl;
    out.flush();






              //return;
}
    ERpcbase * temp = rpcreq;
    temp->deleteLater();
        index++;
        if(index > 552000)
        {

            qDebug()<<"readend";
        }
        else
        {
            getblockhash(index);
        }
}

void BTCconfirm::getblock(QString hash)
{
    rpcreq = new ERpcbase();
    rpcreq->setMethod("getblock");
    rpcreq->setParams(hash);
    rpcreq->setRpcUser("123456");
    rpcreq->setRpcPasswd("654321");
    //rpcreq->setUrl("http://35.201.227.245:8332");
     rpcreq->setUrl("http://106.15.192.229:8332");
    rpcreq->sendPost();
    connect(rpcreq, SIGNAL(finished()), this, SLOT(cal2()));


}

void BTCconfirm::getblockhash(int i)
{      nowindex = i;
       rpcreq = new ERpcbase();
       rpcreq->setMethod("getblockhash");
       rpcreq->setParams(QString::number(i));
       rpcreq->setRpcUser("123456");
       rpcreq->setRpcPasswd("654321");
       //rpcreq->setUrl("http://35.201.227.245:8332");
          rpcreq->setUrl("http://106.15.192.229:8332");
       rpcreq->sendPost();
       connect(rpcreq, SIGNAL(finished()), this, SLOT(cal()));
}

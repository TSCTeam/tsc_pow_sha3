#include "erpcbase.h"

ERpcbase::ERpcbase(QObject *parent) : QObject(parent)
{



//    //"127.0.0.1:23412"http://191.167.2.1:8369
//    //manager->get(QNetworkRequest(QUrl("http://191.167.2.1:8369")));

}
void ERpcbase::replyFinished(QNetworkReply *reply)
{
    //QTextCodec *codec = QTextCodec::codecForName("utf8");
    //QString all = codec->toUnicode(reply->readAll());
    RpcResult = reply->readAll().trimmed();
    //qDebug()<<RpcResult;
    //qDebug()<<"hello";
    Q_EMIT finished();
    reply->deleteLater();
}
int ERpcbase::setRpcUser(QString name)
{
    RpcUser = name;
    return 0;
}

int ERpcbase::setRpcPasswd(QString passwd)
{
    RpcPasswd = passwd;
    return 0;
}

int ERpcbase::setUrl(QString turl)
{
    url.setUrl(turl);
    return 0;
}

int ERpcbase::setMethod(QString method)
{
    RpcMethod = method;
    return 0;
}

int ERpcbase::setParams(QString params)
{
    RpcParams = params;
    return 0;
}

int ERpcbase::sendPost()
{
    if(RpcUser.isEmpty()||RpcPasswd.isEmpty()||url.isEmpty()||RpcMethod.isEmpty())
    {
        qDebug()<<"ERR!: at least one of [RpcUser|RpcPasswd|url|RpcMethod] is Empty,check your code before "
                  "where use ERpcbase::sendPost() ";
        return 1;
    }
    else
    {
        manager = new QNetworkAccessManager(this);
        connect(manager, SIGNAL(finished(QNetworkReply*)),
        this,SLOT(replyFinished(QNetworkReply*)));
        QNetworkRequest request;


        QByteArray post_data;//上传的数据可以是QByteArray或者file类型
//        post_data.append("{\"jsonrpc\": \"1.0\", \"id\":\"curltest\", \"method\": \"getinfo\", \"params\": [] }");
        post_data.append("{");
        post_data.append("\"jsonrpc\": \"1.0\"");
        post_data.append(",");
        post_data.append(" \"id\":\"curltest\"");
        post_data.append(",");
        post_data.append(" \"method\": \"");
        post_data.append(RpcMethod);
        post_data.append("\"");
        post_data.append(", \"params\": [");
        post_data.append(RpcParams);
        post_data.append("] }");
        request.setUrl(url);
        request.setRawHeader("Content-Type","text/plain");
        QString auth;
        QByteArray authEncode;
        authEncode.append(RpcUser);
        authEncode.append(":");
        authEncode.append(RpcPasswd);
        auth.append("Basic ");
        auth.append(authEncode.toBase64());

        request.setRawHeader("Authorization",auth.toLocal8Bit());//用户名和密码的Basic 加密
        reply=manager->post(request,post_data);
        return 0;
    }
}

/*
 *demo
 *  ERpcbase * rpcreq = new ERpcbase();
    rpcreq->setMethod("getinfo");
    rpcreq->setRpcUser("user");
    rpcreq->setRpcPasswd("123456");
    rpcreq->setUrl("http://191.167.2.1:8369");
    rpcreq->sendPost();
 *
 *
*/




















//curl --user myusername --data-binary '{"jsonrpc": "1.0", "id":"curltest", "method": "getinfo", "params": [] }' -H 'content-type: text/plain;' http://127.0.0.1:3889/

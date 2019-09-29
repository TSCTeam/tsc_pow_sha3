#include "nethelper.h"
#include <QDebug>
Nethelper::Nethelper(QObject *parent) : QObject(parent)
{
    manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)),
    this,SLOT(replyFinished(QNetworkReply*)));
    QNetworkRequest request;
    QUrl url("http://191.167.2.1:8369");
    QByteArray post_data;//上传的数据可以是QByteArray或者file类型
    post_data.append("{\"jsonrpc\": \"1.0\", \"id\":\"curltest\", \"method\": \"getinfo\", \"params\": [] }");
    request.setUrl(url);
    //request.setHeader(QNetworkRequest::ContentTypeHeader,"text/plain");
    request.setRawHeader("Content-Type","text/plain");
    request.setRawHeader("Authorization","Basic dXNlcjoxMjM0NTY=");//服务器要求的数据头部
    reply=manager->post(request,post_data);
    //"127.0.0.1:23412"http://191.167.2.1:8369
    //manager->get(QNetworkRequest(QUrl("http://191.167.2.1:8369")));

}
void Nethelper::replyFinished(QNetworkReply *reply)
{
    QTextCodec *codec = QTextCodec::codecForName("utf8");
    QString all = codec->toUnicode(reply->readAll());
    qDebug()<<all;
    qDebug()<<"hello";
    reply->deleteLater();
}
//curl --user myusername --data-binary '{"jsonrpc": "1.0", "id":"curltest", "method": "getinfo", "params": [] }' -H 'content-type: text/plain;' http://127.0.0.1:3889/

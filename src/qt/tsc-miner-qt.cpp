#include <QCoreApplication>
#include <stdio.h>
#include <QDebug>
#include "erpcbase.h"
#include "eminer.h"
#include <QSettings>
#include <iostream>
#include <QTextStream>
#include <QThread>
#include <QCommandLineOption>
#include <QCommandLineParser>



bool writeInit(QString path, QString user_key, QString user_value)
{
    if(path.isEmpty() || user_key.isEmpty())
    {
    return false;
    }
    else
    {
    //创建配置文件操作对象
    QSettings *config = new QSettings(path, QSettings::IniFormat);

    //将信息写入配置文件
    config->beginGroup("config");
    config->setValue(user_key, user_value);
    config->endGroup();
    config->sync();
    config->deleteLater();

    return true;
    }
}

bool readInit(QString path, QString user_key, QString &user_value)
{
    user_value = QString("");
    if(path.isEmpty() || user_key.isEmpty())
    {
    return false;
    }
    else
    {
    //创建配置文件操作对象
    QSettings *config = new QSettings(path, QSettings::IniFormat);

    //读取用户配置信息
    user_value = config->value(QString("config/") + user_key,"error").toString();
    config->deleteLater();
    return true;
    }
}
//将debug信息输出到日志
void outputMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
 static QMutex mutex;
 mutex.lock();
 QString text;
 switch (type)
 {
 case QtDebugMsg:
  text = QString("Debug:");
  break;
 case QtWarningMsg:
  text = QString("Warning:");
  break;
 case QtCriticalMsg:
  text = QString("Critical:");
  break;
 case QtFatalMsg:
  text = QString("Fatal:");
 default: break;
 }
 QString context_info = QString("File:(%1) Line:(%2)").arg(QString(context.file)).arg(context.line);
 QString current_date_time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss ddd");
 QString current_date = QString("(%1)").arg(current_date_time);
 //QString message = QString("%1 %2 %3 %4").arg(text).arg(context_info).arg(current_date).arg(msg);
 QString message = QString("%1 %2 %3").arg(text).arg(current_date).arg(msg);
 QFile file("log.txt");
 file.open(QIODevice::WriteOnly | QIODevice::Append);
 QTextStream text_stream(&file);
 text_stream << message << "\r\n";
 file.flush();
 file.close();
 mutex.unlock();
}
//全局配置rpc信息
QString gloabl_USER;
QString gloabl_PASSWD;
QString gloabl_URL;
QString gloabl_Params;
bool is_buyticket = false;
int cpu_core = 2;
bool is_submode = false ;
int ticketsnum = 0;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QCoreApplication::setOrganizationName("EB-tech");
    QCoreApplication::setOrganizationDomain("www.tschain.top");
    QCoreApplication::setApplicationName("TSC-MINER");
	QCoreApplication::setApplicationVersion("1.2.0");
    //qDebug()<<QThread::idealThreadCount();

	QCommandLineOption op1(QStringList()<<"o"<<"output","output the debug info to the screen");
	QCommandLineOption op2(QStringList()<<"t"<<"thread","define the number of threads to join the calculate","Threads number","1");
	QCommandLineOption op3(QStringList()<<"b"<<"buyticket","buyticket","tickets number","1");
    QCommandLineParser parser;
    parser.setApplicationDescription("a  Multithread cpu miners");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption(op1);
	parser.addOption(op2);
	parser.addOption(op3);
    parser.process(a);

    //读取配置信息
   bool is_USER = readInit(QString("./miner.ini"), "rpcuser", gloabl_USER);
   bool is_PASSWD = readInit(QString("./miner.ini"), "rpcpassword", gloabl_PASSWD);
   bool is_URL = readInit(QString("./miner.ini"), "rpcurl", gloabl_URL);
   bool is_Params = readInit(QString("./miner.ini"), "Params", gloabl_Params);
   if(is_USER==false || is_PASSWD==false || is_URL==false || is_Params==false || gloabl_USER.compare("error") == 0 || gloabl_PASSWD.compare("error") == 0|| gloabl_URL.compare("error") == 0 || gloabl_Params.compare("error")==0 || gloabl_USER.isEmpty()||gloabl_PASSWD.isEmpty()||gloabl_URL.isEmpty()||gloabl_Params.isEmpty())
   {
      QTextStream qin(stdin);
      std::cout<<"setting info is not correct!!!"<<std::endl;
      std::cout<<"Please input the minerpoll ip:port and press Enter to continnue"<<std::endl;
      gloabl_URL.clear();
      gloabl_PASSWD.clear();
      gloabl_USER.clear();
	  gloabl_Params.clear();
      qin >> gloabl_URL;
      std::cout<<"Please input the minerpoll rpcuser and press Enter to continnue"<<std::endl;
      qin >> gloabl_USER;
      std::cout<<"Please input the minerpoll rpcpassword and press Enter to continnue"<<std::endl;
      qin >> gloabl_PASSWD;
      std::cout<<"Please input the params Enter to continnue"<<std::endl;
	  qin >> gloabl_Params;


      writeInit(QString("./miner.ini"), "rpcuser", gloabl_USER);
      writeInit(QString("./miner.ini"), "rpcpassword", gloabl_PASSWD);
      writeInit(QString("./miner.ini"), "rpcurl", gloabl_URL);
	  writeInit(QString("./miner.ini"), "Params", gloabl_Params);
	  //writeInit(QString("./miner.ini"), "corenumber", QThread::idealThreadCount());

      std::cout<<"config is saved, Please restart the software"<<std::endl;
      return 0;

   }

    if(parser.isSet(op1))
    {

    }
    else
    {
       qInstallMessageHandler(outputMessage);
    }

    if(parser.isSet(op2))
    {
        cpu_core = parser.value(op2).toInt();
        is_submode = true;
        qDebug()<<"Start submode";
    }
    else
    {
        qDebug()<<"Start solomode";
    }
	 if(parser.isSet(op3))
    {
        qDebug()<<"autobuyticketmode";
		is_buyticket = true;
		ticketsnum = parser.value(op3).toInt();
    }



    printf("the program is running!!!see the detail in the log.txt or restart the program with param 'output'\n");
    qDebug()<<"tsc-miner start!";



    EMiner *miner = new EMiner();

    miner->getwork();

    return a.exec();
}

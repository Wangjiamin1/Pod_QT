#include "tcpbroad.h"
#include <unistd.h>

TcpBroad::TcpBroad(QObject *parent) : QObject(parent)
{
   qDebug() << "ThreadRecvData start ok.";
   tcpsocket=nullptr;
   mtcpRunFlag=true;
}



void TcpBroad::setMai(QObject *qMain, const QString ip, quint16 port){
    m_ip=ip;
    m_port = port;
    this->connectToMain(qMain);
//    this->start(QThread::TimeCriticalPriority);
}


void TcpBroad::connectToMain(QObject *Main_Obj){
    QObject::connect(Main_Obj,SIGNAL(sendtcpdata(int *)),this,SLOT(gettcpdata(int *)),Qt::QueuedConnection);
}


TcpBroad::~TcpBroad()
{
    delete tcpsocket;
}

void TcpBroad::set_Rec_Buffer_ptr(uchar *x_Rec_Buffer)
{
    m_Rec_Buffer = x_Rec_Buffer;
}

void TcpBroad::set_freeBytes_ptr(QSemaphore *x_freeBytes)
{
    m_freeBytes = x_freeBytes;
}

void TcpBroad::set_usedBytes_ptr(QSemaphore *x_usedBytes)
{
    m_usedBytes = x_usedBytes;
}

void TcpBroad::gettcpdata(int *pdata)
{
    for (int i = 0;i<4;i++) {
        data[i] = pdata[i]&0XFF;
        data[i+1] = pdata[i]>>8;
    }
    Tcp_send = true;

    tcpsocket->write((char*)data,8);
    tcpsocket->flush();
}

void TcpBroad::run(){
    char *Rec_Temp = new char[65535];
    int Rec_len = 0;
    Run_stopped = false;
    tcpsocket = new QTcpSocket(this);
    tcpsocket->connectToHost(m_ip,m_port);
    qDebug()<<"run connectToHost";
    if(tcpsocket->waitForConnected(3000))
    {
        qDebug()<<"run if";

        while (1) {
            //qDebug()<< QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")<<"start r1";
            /*接收数据*/
            if(tcpsocket->waitForReadyRead(1)&&mtcpRunFlag)
            {
                Rec_len = tcpsocket->read(Rec_Temp, 65535);

                qDebug()<<"rec_len:"<<Rec_len;

                for (int i=0; i<Rec_len;)
                {
                    if (m_freeBytes->tryAcquire(1))
                    {
                        m_Rec_Buffer[Writer_Ptr++] = (uchar)Rec_Temp[i];

                        if (Writer_Ptr == BUFFER_SIZE)
                            Writer_Ptr = 0;

                        m_usedBytes->release(1);
                        qDebug()<< QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")<<m_usedBytes->available();

                        i++;
                    }
                    else
                    {
                        usleep(1);
                    }
                }
            }
            else{
                qDebug()<<"exit";
                return;
            }

        }

    }
    else
    {
        qDebug()<<"连接失败";
        emit TCPConnectError();
        //            tcpsocket->close();
        //            delete tcpsocket;
        //            delete [] Rec_Temp;
        //        tcpsocket->abort();
        //        tcpsocket->deleteLater();
        //        tcpsocket=nullptr;
    }
}



#ifndef TCPBROAD_H
#define TCPBROAD_H


#define BUFFER_SIZE 65536
#include <QThread>
#include <QTcpSocket>
#include <QTcpServer>
#include <QDebug>
#include <QSemaphore>
#include <QTime>

class TcpBroad : public QObject
{
    Q_OBJECT
public:
    explicit TcpBroad(QObject *parent = nullptr);
    ~TcpBroad();


    void set_Rec_Buffer_ptr(uchar *x_Rec_Buffer);
    void set_freeBytes_ptr(QSemaphore *x_freeBytes);
    void set_usedBytes_ptr(QSemaphore *x_usedBytes);

    void setMai(QObject *qMain, const QString ip, quint16 port);
    void connectToMain(QObject *Main_Obj);

    QSemaphore *m_freeBytes;
    QSemaphore *m_usedBytes;
    uchar *m_Rec_Buffer;
    bool mtcpRunFlag;

    quint16 Writer_Ptr;
    quint16 Reader_Ptr;
    quint16 Pre_reader_Ptr;

    bool Run_stopped;
    volatile bool Run;
    volatile bool Tcp_send;
    volatile bool ctr_send;

    uchar *data = new uchar[8];
    QTcpSocket *tcpsocket;
    QString ctrcmd;
    QString m_ip;
    quint16 m_port;
    QString m_PortName;

signals:
    void sendmsgtomain(uchar num,qint16 x, qint16 y ,quint16 dis,quint8 dis1);
    void TCPConnectError();
//    void sendshowbuff(uchar* showbuff,int len);


private slots:
    void gettcpdata(int *pdata);
    void run();
};

#endif // TCPBROAD_H

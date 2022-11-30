#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include "ViewLink.h"
#include "VideoObjNetwork.h"
#include "VideoObjUSBCamera.h"
#include "tcpbroad.h"

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();
    int *pdata = new int[4];
    TcpBroad* tcpbroad;
    bool mTCPConnectFlag;

private slots:
    void on_btnConnectTCP_clicked();

private:
    QThread * thread;
    static int VLK_ConnStatusCallback(int iConnStatus, const char* szMessage, int iMsgLen, void* pUserParam);
    static int VLK_DevStatusCallback(int iType, const char* szBuffer, int iBufLen, void* pUserParam);
    void initApplication(void);


    void on_btnUp_pressed();

    void on_btnLeft_pressed();

    void on_btnHome_clicked();

    void on_btnRight_pressed();

    void on_btnDown_pressed();

    void on_btnLeftDown_pressed();

    void on_btnLeftUp_pressed();

    void on_btnRightUp_pressed();

    void on_btnRightDown_pressed();



signals:
    void SignalConnectionStatus(int iConnStatus, const QString& strMessage);
    void SignalDeviceModel(VLK_DEV_MODEL model);
    void SignalDeviceConfig(VLK_DEV_CONFIG config);
    void SignalDeviceTelemetry(VLK_DEV_TELEMETRY telemetry);
    void sendtcpdata(int *pdata);
    void broadTCPThreadSignal();
private slots:
    void onSlotConnectionStatus(int iConnStatus, const QString& strMessage);
    void onSlotDeviceModel(VLK_DEV_MODEL model);
    void onSlotDeviceConfig(VLK_DEV_CONFIG config);
    void onSlotDeviceTelemetry(VLK_DEV_TELEMETRY telemetry);

    void TCPConnectErrorDo();

    void onPTZPressed(int,const QString&);
    void onPTZReleased();

    void on_cmbImageSensor_activated(int index);

    void on_cmbIRColor_activated(int index);

    void on_checkBoxPIP_clicked(bool checked);
    void on_btnOpenNetworkVideo_clicked();

    void on_btnOpenUSBVideo_clicked();

    void on_btnZoomIn_pressed();

    void on_btnZoomIn_released();

    void on_btnZoomOut_pressed();

    void on_btnZoomOut_released();

    void on_btnGimbalTakePhoto_clicked();

    void on_btnStartRecord_clicked();

    void on_btnStopRecord_clicked();

    void on_angle_enter_clicked();

    void on_btnConnectTCPBroad_clicked();

private:
    // initialize UI control
    void InitUI();

private:
    Ui::Widget *ui;

    CVideoObjNetwork m_VideoObjNetwork;
    CVideoObjUSBCamera m_VideoObjUSBCamera;
};

#endif // WIDGET_H

#include "widget.h"
#include "ui_widget.h"
#include <QDebug>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QCamera>
#include <cstring>

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    qRegisterMetaType<VLK_DEV_MODEL>("VLK_DEV_MODEL");
    qRegisterMetaType<VLK_DEV_CONFIG>("VLK_DEV_CONFIG");
    qRegisterMetaType<VLK_DEV_TELEMETRY>("VLK_DEV_TELEMETRY");
    thread = new QThread;
    tcpbroad = new TcpBroad();
    tcpbroad->moveToThread(thread);
    thread->start();

    connect(this, SIGNAL(SignalConnectionStatus(int, const QString&)), this, SLOT(onSlotConnectionStatus(int, const QString&)));
    connect(this, SIGNAL(SignalDeviceModel(VLK_DEV_MODEL)), this, SLOT(onSlotDeviceModel(VLK_DEV_MODEL)));
    connect(this, SIGNAL(SignalDeviceConfig(VLK_DEV_CONFIG)), this, SLOT(onSlotDeviceConfig(VLK_DEV_CONFIG)));
    connect(this, SIGNAL(SignalDeviceTelemetry(VLK_DEV_TELEMETRY)), this, SLOT(onSlotDeviceTelemetry(VLK_DEV_TELEMETRY)));
    connect(ui->gaugeCloud, SIGNAL(mousePressed(int,const QString&)), this, SLOT(onPTZPressed(int,const QString&)));
    connect(ui->gaugeCloud, SIGNAL(mouseReleased(int,const QString&)), this, SLOT(onPTZReleased()));
    connect(this,SIGNAL(broadTCPThreadSignal()),tcpbroad,SLOT(run()));
    connect(tcpbroad, SIGNAL(TCPConnectError()),this,SLOT(TCPConnectErrorDo()));

    InitUI();

    VLK_RegisterDevStatusCB(VLK_DevStatusCallback, this);
}

Widget::~Widget()
{
    m_VideoObjNetwork.Close();
    m_VideoObjUSBCamera.Close();

    delete ui;
}

void Widget::InitUI()
{

    // initialize tcp gimbal list
    ui->cmbIPAddr->addItem("192.168.2.119");
    ui->lineEditPort->setText("2000");

    // initialize network url list
    ui->cmbNetworkVideoURL->addItem("rtsp://192.168.2.119:554");

    // initialize USB Camera list
    QList<QByteArray> listAvailableCamera = QCamera::availableDevices();
    foreach (const QByteArray &camera, listAvailableCamera)
    {
        ui->cmbUSBCamera->addItem(QCamera::deviceDescription(camera));
    }

    // initialize image sensor combobox items
    ui->cmbImageSensor->addItem(tr("Visible1"), VLK_IMAGE_TYPE_VISIBLE1);
    ui->cmbImageSensor->addItem(tr("IR"), VLK_IMAGE_TYPE_IR1);
    // ui->cmbImageSensor->addItem(tr("Visible2"), VLK_IMAGE_TYPE_VISIBLE2);
    // ui->cmbImageSensor->addItem(tr("IR2"), VLK_IMAGE_TYPE_IR2);
    // ui->cmbImageSensor->addItem(tr("Fusion"), VLK_IMAGE_TYPE_FUSION);

    // initialize IR color combobox items
    ui->cmbIRColor->addItem(tr("White hot"), VLK_IR_COLOR_WHITEHOT);
    ui->cmbIRColor->addItem(tr("Black hot"), VLK_IR_COLOR_BLACKHOT);
    ui->cmbIRColor->addItem(tr("Pseudo hot"), VLK_IR_COLOR_PSEUDOHOT);

}

int Widget::VLK_ConnStatusCallback(int iConnStatus, const char* szMessage, int iMsgLen, void* pUserParam)
{
    Widget* pThis = (Widget*)pUserParam;
    if (NULL == pThis)
    {
        return 0;
    }

    QString strMessage;
    if (szMessage != NULL && iMsgLen > 0 )
    {
        strMessage = QString::fromUtf8(szMessage);
    }

    emit pThis->SignalConnectionStatus(iConnStatus, strMessage);

    return 0;
}

int Widget::VLK_DevStatusCallback(int iType, const char* szBuffer, int iBufLen, void* pUserParam)
{
    Widget* pThis = (Widget*)pUserParam;
    if (NULL == pThis)
    {
        return 0;
    }

    if (iType == VLK_DEV_STATUS_TYPE_MODEL)
    {
        if (iBufLen != sizeof(VLK_DEV_MODEL))
        {
            qCritical("################## bad device model info\n");
            return 0;
        }

        VLK_DEV_MODEL* pModel = (VLK_DEV_MODEL*)szBuffer;
        qDebug("model code: %02x, model name: %s\n", pModel->cModelCode, pModel->szModelName);

        emit pThis->SignalDeviceModel(*pModel);
    }
    else if (iType == VLK_DEV_STATUS_TYPE_CONFIG)
    {
        if (iBufLen != sizeof(VLK_DEV_CONFIG))
        {
            qCritical("################## bad device configure\n");
            return 0;
        }

        VLK_DEV_CONFIG* pDevConfig = (VLK_DEV_CONFIG*)szBuffer;
        qDebug("VersionNO: %s, DeviceID: %s, SerialNO: %s\n",
            pDevConfig->cVersionNO, pDevConfig->cDeviceID, pDevConfig->cSerialNO);

        emit pThis->SignalDeviceConfig(*pDevConfig);
    }
    else if (iType == VLK_DEV_STATUS_TYPE_TELEMETRY)
    {
        if (iBufLen != sizeof(VLK_DEV_TELEMETRY))
        {
            qCritical("################## bad telemetry data\n");
            return 0;
        }

        VLK_DEV_TELEMETRY* pTelemetry = (VLK_DEV_TELEMETRY*)szBuffer;
        // qDebug("Yaw: %lf, Pitch: %lf, Sensor type: %02x, Zoom mag times: %d\n",
        //    pTelemetry->dYaw, pTelemetry->dPitch, pTelemetry->emSensorType, pTelemetry->sZoomMagTimes);

        emit pThis->SignalDeviceTelemetry(*pTelemetry);
    }

    return 0;
}

void Widget::on_btnConnectTCP_clicked()
{
    // check if tcp connected
    if (!!VLK_IsTCPConnected())
    {
       VLK_DisconnectTCP();
       ui->btnConnectTCP->setText(tr("Connect"));
    }
    else
    {
        // connect TCP Gimbal
        VLK_CONN_PARAM Param;
        memset(&Param, 0, sizeof(VLK_CONN_PARAM));
        Param.emType = VLK_CONN_TYPE_TCP;

        QString strIPAddr = ui->cmbIPAddr->currentText();
        std::string stdstrIPAddr = strIPAddr.toStdString();
        std::strcpy(Param.ConnParam.IPAddr.szIPV4, stdstrIPAddr.c_str());
        Param.ConnParam.IPAddr.iPort = ui->lineEditPort->text().toInt();

        if (VLK_ERROR_NO_ERROR != VLK_Connect(&Param, VLK_ConnStatusCallback, this))
        {
            qCritical() << "create tcp connection failed!";
            ui->btnConnectTCP->setText(tr("Connect"));
        }
        else
        {
            ui->btnConnectTCP->setText(tr("Connecting"));
        }
    }
}


void Widget::onSlotConnectionStatus(int iConnStatus, const QString& strMessage)
{
    Q_UNUSED(strMessage);
    if (iConnStatus == VLK_CONN_STATUS_TCP_CONNECTED)
    {
       qDebug() << "TCP Gimbal connected !!!";
       ui->btnConnectTCP->setText(tr("Disconnect"));
    }
    else if (iConnStatus == VLK_CONN_STATUS_TCP_DISCONNECTED)
    {
        qDebug() << "TCP Gimbal disconnected !!!";
        ui->btnConnectTCP->setText("Connect");
    }
    else
    {
        qCritical() << "unknown connection status: " << iConnStatus;
    }
}

void Widget::onSlotDeviceModel(VLK_DEV_MODEL model)
{
    Q_UNUSED(model);
}

void Widget::onSlotDeviceConfig(VLK_DEV_CONFIG config)
{
    Q_UNUSED(config);
}

void Widget::onSlotDeviceTelemetry(VLK_DEV_TELEMETRY telemetry)
{
    QString::number(telemetry.dPitch);
    ui->lbYaw->setText(QString::number(telemetry.dYaw));
    ui->lbPitch->setText(QString::number(telemetry.dPitch));
    ui->lbTargetLat->setText(QString::number(telemetry.dTargetLat));
    ui->lbTargetLng->setText(QString::number(telemetry.dTargetLng));
    ui->lbTargetAlt->setText(QString::number(telemetry.dTargetAlt));
//    ui->lbZoom->setText(QString::number(telemetry.sZoomMagTimes));
    ui->lbLaserDistance->setText(QString::number(telemetry.sLaserDistance));
}

void Widget::on_btnOpenNetworkVideo_clicked()
{
    m_VideoObjUSBCamera.Close();

    if (m_VideoObjNetwork.IsOpen())
    {
       m_VideoObjNetwork.Close();
       ui->widgetMainVideo->Clear();
       ui->btnOpenNetworkVideo->setText(tr("Open"));
    }
    else
    {
       QString strURL = ui->cmbNetworkVideoURL->currentText();
       m_VideoObjNetwork.Open(strURL.toStdString(), ui->widgetMainVideo);
       ui->btnOpenNetworkVideo->setText(tr("Close"));
    }
}

void Widget::on_btnOpenUSBVideo_clicked()
{
    m_VideoObjNetwork.Close();

    if (m_VideoObjUSBCamera.IsOpen())
    {
        m_VideoObjUSBCamera.Close();
        ui->widgetMainVideo->Clear();
        ui->btnOpenUSBVideo->setText(tr("Open"));
    }
    else
    {
        QString strUSBCamera = ui->cmbUSBCamera->currentText();
        m_VideoObjUSBCamera.Open(strUSBCamera, ui->widgetMainVideo);
        ui->btnOpenUSBVideo->setText(tr("Close"));
    }
}


void Widget::onPTZPressed(int direction,const QString &string){
    switch(direction){
    case 0:
        on_btnDown_pressed();
        break;
    case 2:
        on_btnLeft_pressed();
        break;
    case 4:
        on_btnUp_pressed();
        break;
    case 6:
        on_btnRight_pressed();
        break;
    case 8:
        on_btnHome_clicked();
        break;
    case 1:
        on_btnLeftDown_pressed();
        break;
    case 3:
        on_btnLeftUp_pressed();
        break;
    case 5:
        on_btnRightUp_pressed();
        break;
    case 7:
        on_btnRightDown_pressed();
        break;

    }
}

void Widget::initApplication()
{
    pdata[0] = uchar(0x00);
    pdata[1] = uchar(0x00);
    pdata[2] = uchar(0x00);
    pdata[3] = uchar(0x00);
    pdata[4] = uchar(0x00);
    pdata[5] = uchar(0x00);
    pdata[6] = uchar(0x00);
    pdata[7] = uchar(0x00);
}




void Widget::on_btnUp_pressed()
{
    qDebug()<<"on_btnUp_pressed";
    double dSpeedRate = (double)ui->SliderMoveSpeed->value() / (double)ui->SliderMoveSpeed->maximum();
    double dScaleSpeed = (double)VLK_MAX_PITCH_SPEED * dSpeedRate;
    VLK_Move(0, (short)dScaleSpeed);
}

void Widget::onPTZReleased()
{
    qDebug()<<"released";
    VLK_Stop();
}

void Widget::on_btnLeft_pressed()
{
    qDebug()<<"on_btnLeft_pressed";
    double dSpeedRate = (double)ui->SliderMoveSpeed->value() / (double)ui->SliderMoveSpeed->maximum();
    double dScaleSpeed = (double)VLK_MAX_YAW_SPEED * dSpeedRate * -1;
    VLK_Move(dScaleSpeed, 0);
}

void Widget::on_btnHome_clicked()
{
    qDebug()<<"on_btnHome_clicked";
    VLK_Home();
}

void Widget::on_btnRight_pressed()
{
    qDebug()<<"on_btnRight_pressed";
    double dSpeedRate = (double)ui->SliderMoveSpeed->value() / (double)ui->SliderMoveSpeed->maximum();
    double dScaleSpeed = (double)VLK_MAX_YAW_SPEED * dSpeedRate;
    VLK_Move(dScaleSpeed, 0);
}

void Widget::on_btnDown_pressed()
{
    qDebug()<<"on_btnDown_pressed";
    double dSpeedRate = (double)ui->SliderMoveSpeed->value() / (double)ui->SliderMoveSpeed->maximum();
    double dScaleSpeed = (double)VLK_MAX_PITCH_SPEED * dSpeedRate * -1;
    VLK_Move(0, dScaleSpeed);
}

void Widget::on_btnLeftDown_pressed()
{
    qDebug()<<"on_btnLeftDown_pressed";
    double dSpeedRate = (double)ui->SliderMoveSpeed->value() / (double)ui->SliderMoveSpeed->maximum();
    double dScaleSpeed = (double)VLK_MAX_PITCH_SPEED * dSpeedRate * -1;
    VLK_Move(dScaleSpeed, dScaleSpeed);
}

void Widget::on_btnLeftUp_pressed()
{
    qDebug()<<"on_btnLeftUp_pressed";
    double dSpeedRate = (double)ui->SliderMoveSpeed->value() / (double)ui->SliderMoveSpeed->maximum();
    double dScaleSpeed = (double)VLK_MAX_PITCH_SPEED * dSpeedRate * -1;
    VLK_Move(dScaleSpeed, -1*dScaleSpeed);
}

void Widget::on_btnRightUp_pressed()
{
    qDebug()<<"on_btnRightUp_pressed";
    double dSpeedRate = (double)ui->SliderMoveSpeed->value() / (double)ui->SliderMoveSpeed->maximum();
    double dScaleSpeed = (double)VLK_MAX_PITCH_SPEED * dSpeedRate;
    VLK_Move(dScaleSpeed, dScaleSpeed);
}

void Widget::on_btnRightDown_pressed()
{
    qDebug()<<"on_btnRightDown_pressed";
    double dSpeedRate = (double)ui->SliderMoveSpeed->value() / (double)ui->SliderMoveSpeed->maximum();
    double dScaleSpeed = (double)VLK_MAX_PITCH_SPEED * dSpeedRate * -1;
    VLK_Move(-1*dScaleSpeed, dScaleSpeed);
}





void Widget::on_cmbImageSensor_activated(int index)
{
    VLK_IMAGE_TYPE emType = (VLK_IMAGE_TYPE)ui->cmbImageSensor->itemData(index).toInt();
    VLK_IR_COLOR emColor = (VLK_IR_COLOR)ui->cmbIRColor->itemData(ui->cmbIRColor->currentIndex()).toInt();
    VLK_SetImageColor(emType, ui->checkBoxPIP->isChecked() ? 1 : 0, emColor);
}

void Widget::on_cmbIRColor_activated(int index)
{
    VLK_IMAGE_TYPE emType = (VLK_IMAGE_TYPE)ui->cmbImageSensor->itemData(ui->cmbImageSensor->currentIndex()).toInt();
    VLK_IR_COLOR emColor = (VLK_IR_COLOR)ui->cmbIRColor->itemData(index).toInt();
    VLK_SetImageColor(emType, ui->checkBoxPIP->isChecked() ? 1 : 0, emColor);
}

void Widget::on_checkBoxPIP_clicked(bool checked)
{
    VLK_IMAGE_TYPE emType = (VLK_IMAGE_TYPE)ui->cmbImageSensor->itemData(ui->cmbImageSensor->currentIndex()).toInt();
    VLK_IR_COLOR emColor = (VLK_IR_COLOR)ui->cmbIRColor->itemData(ui->cmbIRColor->currentIndex()).toInt();
    VLK_SetImageColor(emType, checked ? 1 : 0, emColor);
}

void Widget::on_btnZoomIn_pressed()
{
    VLK_ZoomIn(4);
}

void Widget::on_btnZoomIn_released()
{
    VLK_StopZoom();
}

void Widget::on_btnZoomOut_pressed()
{
    VLK_ZoomOut(4);
}

void Widget::on_btnZoomOut_released()
{
    VLK_StopZoom();
}

void Widget::on_btnGimbalTakePhoto_clicked()
{
    // please make sure there is SD card in the Gimbal
    VLK_Photograph();
}

void Widget::on_btnStartRecord_clicked()
{
    // please make sure there is SD card in the Gimbal
    VLK_SwitchRecord(1);
}

void Widget::on_btnStopRecord_clicked()
{
    // please make sure there is SD card in the Gimbal
    VLK_SwitchRecord(0);
}

void Widget::on_angle_enter_clicked()
{
    double angle_temp = ui->angle_input->text().toDouble();
    if(angle_temp>180 || angle_temp<-180){
        qDebug()<<"angle is invalid!";
        return ;
    }
    VLK_TurnTo(ui->angle_input->text().toDouble(),0.0);
}

void Widget::TCPConnectErrorDo(){
    tcpbroad->mtcpRunFlag=false;
    ui->textBrowser->insertPlainText("TCP connect failed.\nPlease check ip and port!\n");
    ui->btnConnectTCPBroad->setText("connect");
}

void Widget::on_btnConnectTCPBroad_clicked()
{
    if(ui->btnConnectTCPBroad->text() =="connect")
    {
        QString strIPAddr = ui->cmbIPAddr->currentText();
        std::string stdstrIPAddr = strIPAddr.toStdString().c_str();
        int iport = ui->lineEditPort->text().toInt();
        tcpbroad->mtcpRunFlag = true;
        tcpbroad->setMai(ui->widgetMainVideo,strIPAddr,iport);
//        tcpbroad->start(QThread::TimeCriticalPriority);
        ui->btnConnectTCPBroad->setText("disconnect");
        emit broadTCPThreadSignal();
    }
    else{
        tcpbroad->mtcpRunFlag=false;
        ui->btnConnectTCPBroad->setText("connect");
    }


}


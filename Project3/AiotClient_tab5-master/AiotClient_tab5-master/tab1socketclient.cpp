#include "tab1socketclient.h"
#include "ui_tab1socketclient.h"

Tab1SocketClient::Tab1SocketClient(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Tab1SocketClient)
{
    ui->setupUi(this);
    ui->pPBSendButton->setEnabled(false);
    pSocketClient = new SocketClient(this);
    connect(pSocketClient,SIGNAL(sigSocketRecv(QString)),this, SLOT(slotRecvUpdate(QString)));

}

Tab1SocketClient::~Tab1SocketClient()
{
    delete ui;
}

void Tab1SocketClient::on_pPBServerConnect_clicked(bool checked)
{
    bool bOk;
    if(checked)
    {
        pSocketClient->slotConnectToServer(bOk);
        if(bOk)
        {
            ui->pPBServerConnect->setText("서버 해제");
            ui->pPBSendButton->setEnabled(true);
            ui->pLERecvId->setText("ROS");
        }
    }
    else
    {
        pSocketClient->slotClosedByServer();
        ui->pPBServerConnect->setText("서버 연결");
        ui->pPBSendButton->setEnabled(false);
        ui->pLERecvId->setText("");
        ui->pLESendData->setText("");
    }
}

void Tab1SocketClient::slotRecvUpdate(QString strRecvData)
{
    QTime time = QTime::currentTime();
    QString strTime = time.toString();

    strRecvData.chop(1);    //'\n' 제거

    strTime = strTime  + " " + strRecvData;
    ui->pTERecvData->append(strTime);
    strRecvData.replace("[","@");
    strRecvData.replace("]","@");
    QStringList qList = strRecvData.split("@");
    //[KSH_LIN]LED@0x0f ==> @KSH_LIN@LED@0x0f
    //qList[0] => Null, qList[1] => KSH_LIN, qList[2] =>LED, qList[3] => 0x0f
}
void Tab1SocketClient::on_pPBSendButton_clicked()
{
    QTime time = QTime::currentTime();
    QString strTime = time.toString();
    QString strRecvId = ui->pLERecvId->text();
    QString strSendData = ui->pLESendData->text();
    if(!strSendData.isEmpty())
    {
        if(strRecvId.isEmpty())
            strSendData = "[ALLMSG]"+strSendData;
        else
            strSendData = "["+strRecvId+"]"+strSendData;

        pSocketClient->slotSocketSendData(strSendData);
        strTime = strTime  + " [USER2]->" + strSendData;
        ui->pTESendData->append(strSendData);
        ui->pLESendData->clear();
    }
}

void Tab1SocketClient::on_pPBPointSet_clicked()
{
    QString strId = "ROS";
    QString strOder = "GO@x좌표@y좌표@각도";
    ui->pLERecvId->setText(strId);
    ui->pLESendData->setText(strOder);
}

void Tab1SocketClient::on_pPBHome_clicked()
{
    QTime time = QTime::currentTime();
    QString strTime = time.toString();
    QString strHome ="[ROS]GOHOME";
    strTime = strTime  + " [USER2]->" + strHome;
    ui->pTESendData->append(strTime);
    pSocketClient->slotSocketSendData(strHome);
}

void Tab1SocketClient::on_pPBSpray_clicked()
{
     QTime time = QTime::currentTime();
     QString strTime = time.toString();
     QString strSpray ="[ROS]SPRAYIC";
     strTime = strTime  + " [USER2]->" + strSpray;
     ui->pTESendData->append(strTime);
     pSocketClient->slotSocketSendData(strSpray);
}


void Tab1SocketClient::on_pPBSendClear_clicked()
{
    ui->pTESendData->setText("");
}

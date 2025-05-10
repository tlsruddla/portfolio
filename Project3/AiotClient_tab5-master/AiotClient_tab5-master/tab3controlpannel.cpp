#include "tab3controlpannel.h"
#include "ui_tab3controlpannel.h"

Tab3ControlPannel::Tab3ControlPannel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Tab3ControlPannel)
{
    ui->setupUi(this);
    QUrl httpUrl1 = QStringLiteral("http://10.10.141.133/phpmyadmin");
    httpUrl1.setUserName("user");
    httpUrl1.setPassword("passwd");
    pQWebEngineViewCam1 = new QWebEngineView(this);
    pQWebEngineViewCam1->load(QUrl(httpUrl1));
    ui->verticalLayout_Cam2->addWidget(pQWebEngineViewCam1);
}


Tab3ControlPannel::~Tab3ControlPannel()
{
    delete ui;
}


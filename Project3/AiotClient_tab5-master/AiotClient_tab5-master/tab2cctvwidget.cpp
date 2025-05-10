#include "tab2cctvwidget.h"
#include "ui_tab2cctvwidget.h"

Tab2CctvWidget::Tab2CctvWidget(QWidget *parent):
      QWidget(parent),
      ui(new Ui::Tab2CctvWidget)
{
    ui->setupUi(this);
    QUrl httpUrl1 = QStringLiteral("http://10.10.141.133:8080");
    httpUrl1.setUserName("user");
    httpUrl1.setPassword("passwd");
    pQWebEngineViewCam1 = new QWebEngineView(this);
    pQWebEngineViewCam1->load(QUrl(httpUrl1));
    ui->verticalLayout_Cam1->addWidget(pQWebEngineViewCam1);

}

Tab2CctvWidget::~Tab2CctvWidget()
{
    delete ui;
}

#include "mainwidget.h"
#include "ui_mainwidget.h"

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWidget)
{
    ui->setupUi(this);
    pTab1SocketClient = new Tab1SocketClient(ui->pTab1);
    ui->pTab1->setLayout(pTab1SocketClient->layout());

    pTab2CctvWidget = new Tab2CctvWidget(ui->pTab2);
    ui->pTab2->setLayout(pTab2CctvWidget->layout());

    pTab3ControlPannel = new Tab3ControlPannel(ui->pTab3);
    ui->pTab3->setLayout(pTab3ControlPannel->layout());



    ui->tabWidget->setCurrentIndex(2);



}

MainWidget::~MainWidget()
{
    delete ui;
}


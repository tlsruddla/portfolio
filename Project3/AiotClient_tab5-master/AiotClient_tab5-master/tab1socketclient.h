#ifndef TAB1SOCKETCLIENT_H
#define TAB1SOCKETCLIENT_H

#include <QWidget>
#include <QTime>
#include "socketclient.h"

namespace Ui {
class Tab1SocketClient;
}

class Tab1SocketClient : public QWidget
{
    Q_OBJECT

public:
    explicit Tab1SocketClient(QWidget *parent = nullptr);
    ~Tab1SocketClient();

private slots:
    void on_pPBServerConnect_clicked(bool checked);

private:
    Ui::Tab1SocketClient *ui;

public:
    SocketClient *pSocketClient;

private slots:
    void slotRecvUpdate(QString);
    void on_pPBSendButton_clicked();

    void on_pPBPointSet_clicked();

    void on_pPBHome_clicked();

    void on_pPBSpray_clicked();

    void on_pPBSendClear_clicked();
};

#endif // TAB1SOCKETCLIENT_H

#ifndef TAB3CONTROLPANNEL_H
#define TAB3CONTROLPANNEL_H

#include <QWidget>
#include <QPalette>
#include <QUrl>
#include <QStringLiteral>
#include <QWebEngineView>
namespace Ui {
class Tab3ControlPannel;
}

class Tab3ControlPannel : public QWidget
{
    Q_OBJECT

public:
    explicit Tab3ControlPannel(QWidget *parent = nullptr);
    ~Tab3ControlPannel();

private:
    Ui::Tab3ControlPannel *ui;
    QWebEngineView *pQWebEngineViewCam1;


};

#endif // TAB3CONTROLPANNEL_H

#ifndef TAB2CCTVWIDGET_H
#define TAB2CCTVWIDGET_H

#include <QWidget>
#include <QUrl>
#include <QStringLiteral>
#include <QWebEngineView>

//QT_BEGIN_NAMESPACE
namespace Ui {
class Tab2CctvWidget;
}
//QT_END_NAMESPACE

class Tab2CctvWidget : public QWidget
{
    Q_OBJECT

public:
    Tab2CctvWidget(QWidget *parent = nullptr);
    ~Tab2CctvWidget();

private:
    Ui::Tab2CctvWidget *ui;
    QWebEngineView *pQWebEngineViewCam1;
//    QWebEngineView *pQWebEngineViewCam2;
//    QWebEngineView *pQWebEngineViewCam3;
//    QWebEngineView *pQWebEngineViewCam4;
};



#endif // TAB2CCTVWIDGET_H

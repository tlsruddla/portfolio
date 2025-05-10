#ifndef TAB2CCTV_H
#define TAB2CCTV_H


#include <QWidget>
#include <QUrl>
#include <QStringLiteral>
#include <QWebEngineView>

QT_BEGIN_NAMESPACE
namespace Ui { class CctvWidget; }
QT_END_NAMESPACE

class CctvWidget : public QWidget
{
    Q_OBJECT

public:
    CctvWidget(QWidget *parent = nullptr);
    ~CctvWidget();

private:
    Ui::CctvWidget *ui;
    QWebEngineView *pQWebEngineViewCam1;
    QWebEngineView *pQWebEngineViewCam2;
    QWebEngineView *pQWebEngineViewCam3;
    QWebEngineView *pQWebEngineViewCam4;
};



















#endif // TAB2CCTV_H

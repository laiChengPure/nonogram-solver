#include <QApplication>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QWidget>

#ifndef _ASPECTRATIO
#define _ASPECTRATIO
// reference: https://forum.qt.io/topic/112394/how-to-maintain-qwidget-aspect-ratio
class AspectRatioWidget : public QWidget
{
    Q_OBJECT
public:
    AspectRatioWidget(QWidget *parent = 0);

    // the widget we want to keep the ratio
    void setAspectWidget(QWidget *widget);
    void setRatio(const double ratio);
    double getRatio();

protected:
    void resizeEvent(QResizeEvent *event);

public slots:
    void applyAspectRatio();

private:
    QHBoxLayout *m_layout;
    QWidget *m_aspect_widget;
    double m_ratio;

signals:
    void redawCanvasSignal();
};
#endif
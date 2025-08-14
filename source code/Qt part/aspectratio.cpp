#include "aspectratio.h"

AspectRatioWidget::AspectRatioWidget(QWidget *parent) : QWidget(parent)
{
    m_layout = new QHBoxLayout();
    m_layout->setSpacing(0);
    m_layout->setContentsMargins(0, 0, 0, 0);
    setLayout(m_layout);
}

void AspectRatioWidget::setRatio(const double ratio)
{
    m_ratio = ratio;
    applyAspectRatio();
}

double AspectRatioWidget::getRatio()
{
    return m_ratio;
}

void AspectRatioWidget::setAspectWidget(QWidget *widget)
{
    m_aspect_widget = widget;
    m_layout->addWidget(widget);
    applyAspectRatio();
}

void AspectRatioWidget::resizeEvent(QResizeEvent *event)
{
    (void)event;
    applyAspectRatio();
    emit redawCanvasSignal();
}

void AspectRatioWidget::applyAspectRatio()
{
    int w = this->width();
    int h = this->height();
    double aspect = static_cast<double>(h) / static_cast<double>(w);

    if (aspect < m_ratio) // parent is too wide
    {
        int target_width = static_cast<int>(static_cast<double>(h) / m_ratio);
        m_aspect_widget->setMaximumWidth(target_width);
        m_aspect_widget->setMaximumHeight(h);
    }
    else // parent is too high
    {
        int target_heigth = static_cast<int>(static_cast<double>(w) * m_ratio);
        m_aspect_widget->setMaximumHeight(target_heigth);
        m_aspect_widget->setMaximumWidth(w);
    }
}
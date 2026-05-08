#pragma once

#include <QSlider>
#include <QMouseEvent>
#include <QToolTip>
#include <QStyle>

class SeekSlider : public QSlider {
public:
    explicit SeekSlider(Qt::Orientation o, QWidget *parent = nullptr)
        : QSlider(o, parent) {
        setMouseTracking(true);
    }

    void setDuration(double duration) { m_duration = duration; }

protected:
    void mousePressEvent(QMouseEvent *e) override {
        if (e->button() == Qt::LeftButton) {
            int val = QStyle::sliderValueFromPosition(
                minimum(), maximum(), e->pos().x(), width());
            setValue(val);
            emit sliderMoved(val);
        }
        showTooltip(e->pos());
        QSlider::mousePressEvent(e);
    }

    void mouseMoveEvent(QMouseEvent *e) override {
        showTooltip(e->pos());
        QSlider::mouseMoveEvent(e);
    }

    void leaveEvent(QEvent *e) override {
        QToolTip::hideText();
        QSlider::leaveEvent(e);
    }

private:
    double m_duration = 0.0;

    // void showTooltip(const QPoint &pos) {
    //     if (m_duration <= 0) return;
    //     int val = QStyle::sliderValueFromPosition(
    //         minimum(), maximum(), pos.x(), width());
    //     double t = (val / 1000.0) * m_duration;
    //     int secs = static_cast<int>(t);
    //     QString text = QString("%1:%2")
    //         .arg(secs / 60)
    //         .arg(secs % 60, 2, 10, QChar('0'));
    //     QToolTip::showText(mapToGlobal(pos) + QPoint(0, -60), text, this);
    // }
    void showTooltip(const QPoint &pos) {
        if (m_duration <= 0) return;
        int val = QStyle::sliderValueFromPosition(
            minimum(), maximum(), pos.x(), width());
        double t = (val / 1000.0) * m_duration;
        int secs = static_cast<int>(t);
        QString text = QString("%1:%2")
            .arg(secs / 60)
            .arg(secs % 60, 2, 10, QChar('0'));
        // fixed vertical position above the seekbar, only x follows cursor
        QPoint fixed(pos.x(), 0);
        QToolTip::showText(mapToGlobal(fixed) + QPoint(0, -50), text, this);
    }
};

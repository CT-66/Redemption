#pragma once

#include <QLabel>
#include <QTimer>
#include <QPainter>
#include <QFontMetrics>
#include <QMouseEvent>

class ScrollingLabel : public QWidget
{
    Q_OBJECT

public:
    explicit ScrollingLabel(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setFixedHeight(fontMetrics().height() + 4);
        setMouseTracking(true);

        m_timer = new QTimer(this);
        m_timer->setInterval(30);
        connect(m_timer, &QTimer::timeout, this, &ScrollingLabel::scroll);

        setCursor(Qt::PointingHandCursor);
    }

    void setText(const QString &text)
    {
        m_text = text;
        m_offset = 0;
        m_timer->stop();
        m_waiting = true;

        update();

        QFontMetrics fm(font());
        if (fm.horizontalAdvance(text) > width()) {
            // wait 1.5 seconds before starting scroll
            QTimer::singleShot(1500, this, [this]() {
                m_waiting = false;
                m_timer->start();
            });
        }
        update();
    }

    void setFont(const QFont &font)
    {
        QWidget::setFont(font);
        setFixedHeight(fontMetrics().height() + 4);
    }

    void setAlignment(Qt::Alignment) {} // kept for compatibility
    QString text() const { return m_text; }

    void showEvent(QShowEvent *e) override {
        QWidget::showEvent(e);
        update(); // force repaint when shown
    }

    signals:
        void clicked();

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setFont(font());
        p.setPen(palette().windowText().color());

        QFontMetrics fm(font());
        int textWidth = fm.horizontalAdvance(m_text);

        if (textWidth <= width()) {
            p.drawText(rect(), Qt::AlignLeft | Qt::AlignVCenter, m_text);
        } else {
            p.setClipRect(rect());
            p.drawText(-m_offset, 0, textWidth + 40, height(),
                       Qt::AlignLeft | Qt::AlignVCenter, m_text);
        }
    }

    void resizeEvent(QResizeEvent *e) override
    {
        QWidget::resizeEvent(e);
        setText(m_text); // re-evaluate scrolling need
    }
    void enterEvent(QEnterEvent *) override {
        m_hovered = true;
        m_timer->stop();
        m_offset = 0;
        update();
    }

    void leaveEvent(QEvent *) override {
        m_hovered = false;
        QTimer::singleShot(1500, this, [this]() {
            QFontMetrics fm(font());
            if (fm.horizontalAdvance(m_text) > width())
                m_timer->start();
        });
    }

    void mousePressEvent(QMouseEvent *e) override {
        QFontMetrics fm(font());
        int textWidth = fm.horizontalAdvance(m_text);
        if (e->pos().x() <= textWidth)
            emit clicked();
    }

    void mouseMoveEvent(QMouseEvent *e) override {
        QFontMetrics fm(font());
        int textWidth = fm.horizontalAdvance(m_text);
        setCursor(e->pos().x() <= textWidth
            ? Qt::PointingHandCursor
            : Qt::ArrowCursor);
    }

private:
    QString m_text;
    int m_offset = 0;
    bool m_waiting = false;
    QTimer *m_timer = nullptr;
    bool m_hovered = false;

    void scroll()
    {
        QFontMetrics fm(font());
        int textWidth = fm.horizontalAdvance(m_text);
        m_offset += 1;
        // reset after text scrolls just past the visible area + small gap
        if (m_offset > textWidth - width() + 40) {
            m_offset = 0;
            m_timer->stop();
            QTimer::singleShot(1500, this, [this]() {
                if (!m_hovered)
                    m_timer->start();
            });
        }
        update();
    }
};

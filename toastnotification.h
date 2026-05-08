/*
#pragma once

#include <QLabel>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QWidget>

class ToastNotification : public QLabel
{
    Q_OBJECT

public:
    static void show(QWidget *parent, const QString &message)
    {
        ToastNotification *toast = new ToastNotification(parent, message);
        toast->popup();
    }

private:
    explicit ToastNotification(QWidget *parent, const QString &message)
        : QLabel(parent)
    {
        setText(message);
        setAlignment(Qt::AlignCenter);
        setStyleSheet(
            "QLabel {"
            "  background: palette(tooltip-base);"
            "  color: palette(tooltip-text);"
            "  border-radius: 6px;"
            "  padding: 8px 16px;"
            "  font-size: 13px;"
            "}"
        );

        adjustSize();

        // position at upper middle of parent
        int x = (parent->width() - width()) / 2;
        int y = 24;
        move(x, y);

        m_effect = new QGraphicsOpacityEffect(this);
        m_effect->setOpacity(1.0);
        setGraphicsEffect(m_effect);

        raise();
        QLabel::show();
    }

    void popup()
    {
        // stay visible for 1.5 seconds then fade out
        QTimer::singleShot(1500, this, [this]() {
            auto *anim = new QPropertyAnimation(m_effect, "opacity", this);
            anim->setDuration(400);
            anim->setStartValue(1.0);
            anim->setEndValue(0.0);
            connect(anim, &QPropertyAnimation::finished, this, &QObject::deleteLater);
            anim->start(QAbstractAnimation::DeleteWhenStopped);
        });
    }

    QGraphicsOpacityEffect *m_effect = nullptr;
};

*/

#pragma once

#include <QLabel>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QWidget>
#include <QHBoxLayout>
#include <QIcon>

class ToastNotification : public QWidget
{
    Q_OBJECT

public:
    static void show(QWidget *parent, const QString &message,
                     const QString &iconName = QString())
    {
        ToastNotification *toast = new ToastNotification(parent, message, iconName);
        toast->popup();
    }

private:
    explicit ToastNotification(QWidget *parent, const QString &message,
                                const QString &iconName)
        : QWidget(parent)
    {
        setStyleSheet(
            "QWidget {"
            "  background: palette(tooltip-base);"
            "  border-radius: 8px;"
            "  padding: 4px;"
            "}"
        );

        QHBoxLayout *layout = new QHBoxLayout(this);
        layout->setContentsMargins(12, 8, 16, 8);
        layout->setSpacing(10);

        if (!iconName.isEmpty()) {
            QLabel *iconLabel = new QLabel();
            QIcon icon = QIcon::fromTheme(iconName);
            if (!icon.isNull())
                iconLabel->setPixmap(icon.pixmap(22, 22));
            layout->addWidget(iconLabel);
        }

        QLabel *textLabel = new QLabel(message);
        textLabel->setStyleSheet(
            "QLabel {"
            "  color: palette(tooltip-text);"
            "  font-size: 13px;"
            "  background: transparent;"
            "  padding: 0px;"
            "}"
        );
        layout->addWidget(textLabel);

        adjustSize();

        int x = (parent->width() - width()) / 2;
        int y = 24;
        move(x, y);

        m_effect = new QGraphicsOpacityEffect(this);
        m_effect->setOpacity(1.0);
        setGraphicsEffect(m_effect);

        setAttribute(Qt::WA_StyledBackground, true);

        raise();
        QWidget::show();
    }

    void popup()
    {
        QTimer::singleShot(1500, this, [this]() {
            auto *anim = new QPropertyAnimation(m_effect, "opacity", this);
            anim->setDuration(400);
            anim->setStartValue(1.0);
            anim->setEndValue(0.0);
            connect(anim, &QPropertyAnimation::finished,
                    this, &QObject::deleteLater);
            anim->start(QAbstractAnimation::DeleteWhenStopped);
        });
    }

    QGraphicsOpacityEffect *m_effect = nullptr;
};

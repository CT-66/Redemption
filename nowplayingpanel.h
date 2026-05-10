#pragma once

#include <QWidget>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QGraphicsDropShadowEffect>
#include "seekslider.h"
#include "ambientbar.h"

class NowPlayingPanel : public QWidget
{
    Q_OBJECT

public:
    explicit NowPlayingPanel(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        // cover art
        m_coverLabel = new QLabel();
        m_coverLabel->setFixedSize(250, 250);
        m_coverLabel->setAlignment(Qt::AlignCenter);
        m_coverLabel->setStyleSheet("background: transparent;");

        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(30);
        shadow->setOffset(0, 6);
        shadow->setColor(QColor(0, 0, 0, 180));
        m_coverLabel->setGraphicsEffect(shadow);

        // title
        m_titleLabel = new QLabel("No track playing");
        m_titleLabel->setAlignment(Qt::AlignCenter);
        m_titleLabel->setStyleSheet(
            "color: white; font-size: 18px; font-weight: bold;");
        m_titleLabel->setWordWrap(true);

        // artist
        m_artistLabel = new QLabel("");
        m_artistLabel->setAlignment(Qt::AlignCenter);
        m_artistLabel->setStyleSheet(
            "color: rgba(255,255,255,180); font-size: 13px;");

        // seekbar
        m_seekBar = new SeekSlider(Qt::Horizontal);
        m_seekBar->setRange(0, 1000);
        m_seekBar->setValue(0);
        m_seekBar->setStyleSheet(
            "QSlider::groove:horizontal {"
            "  height: 4px;"
            "  background: rgba(255,255,255,60);"
            "  border-radius: 2px;"
            "}"
            "QSlider::sub-page:horizontal {"
            "  background: white;"
            "  border-radius: 2px;"
            "}"
            "QSlider::handle:horizontal {"
            "  width: 12px; height: 12px;"
            "  background: white;"
            "  border-radius: 6px;"
            "  margin: -4px 0;"
            "}"
        );

        m_elapsedLabel = new QLabel("0:00");
        m_elapsedLabel->setStyleSheet("color: rgba(255,255,255,180); font-size: 10px;");

        m_remainingLabel = new QLabel("0:00");
        m_remainingLabel->setStyleSheet("color: rgba(255,255,255,180); font-size: 10px;");
        m_remainingLabel->setAlignment(Qt::AlignRight);

        QHBoxLayout *seekRow = new QHBoxLayout();
        seekRow->addWidget(m_elapsedLabel);
        seekRow->addWidget(m_seekBar, 1);
        seekRow->addWidget(m_remainingLabel);

        // controls — created externally, added via setControls
        m_controlsLayout = new QHBoxLayout();
        m_controlsLayout->setAlignment(Qt::AlignCenter);

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setAlignment(Qt::AlignCenter);
        layout->addStretch();
        layout->addWidget(m_coverLabel, 0, Qt::AlignCenter);
        layout->addSpacing(16);
        layout->addWidget(m_titleLabel);
        layout->addWidget(m_artistLabel);
        layout->addSpacing(16);
        layout->addLayout(seekRow);
        layout->addSpacing(8);
        layout->addLayout(m_controlsLayout);
        layout->addStretch();
    }

    void setCover(const QPixmap &pixmap, int radius = 16)
    {
        m_lastCover = pixmap;
        int sz = qMin(m_coverLabel->width(), m_coverLabel->height());
        if (sz < 50) sz = 250;

        if (pixmap.isNull()) {
            QPixmap fallback(250, 250);
            fallback.fill(Qt::transparent);
            QPainter p(&fallback);
            p.setRenderHint(QPainter::Antialiasing);
            p.setBrush(QColor(255, 255, 255, 30));
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(0, 0, 250, 250, radius, radius);
            QIcon::fromTheme("media-album-cover").paint(&p, 50, 50, 150, 150);
            m_coverLabel->setPixmap(fallback);
        } else {
            QPixmap scaled = pixmap.scaled(250, 250,
                Qt::KeepAspectRatio, Qt::SmoothTransformation);
            QPixmap rounded(scaled.size());
            rounded.fill(Qt::transparent);
            QPainter p(&rounded);
            p.setRenderHint(QPainter::Antialiasing);
            QPainterPath path;
            path.addRoundedRect(rounded.rect(), radius, radius);
            p.setClipPath(path);
            p.drawPixmap(0, 0, scaled);
            m_coverLabel->setPixmap(rounded);
        }
    }

    void setTitle(const QString &title) { m_titleLabel->setText(title); }
    void setArtist(const QString &artist) { m_artistLabel->setText(artist); }

    SeekSlider *seekBar() const { return m_seekBar; }
    QLabel *elapsedLabel() const { return m_elapsedLabel; }
    QLabel *remainingLabel() const { return m_remainingLabel; }
    QHBoxLayout *controlsLayout() const { return m_controlsLayout; }

private:
    QLabel *m_coverLabel = nullptr;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_artistLabel = nullptr;
    SeekSlider *m_seekBar = nullptr;
    QLabel *m_elapsedLabel = nullptr;
    QLabel *m_remainingLabel = nullptr;
    QHBoxLayout *m_controlsLayout = nullptr;
    QPixmap m_lastCover;


};

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
#include <QTimer>
#include "scrollinglabel.h"

class NowPlayingPanel : public QWidget
{
    Q_OBJECT

public:
    explicit NowPlayingPanel(QWidget *parent = nullptr)
        : QWidget(parent)
    {

        // cover art
        m_coverLabel = new QLabel();
        // m_coverLabel->setFixedSize(250, 250);
        m_coverLabel->setAlignment(Qt::AlignCenter);
        m_coverLabel->setStyleSheet("background: transparent;");
    // m_coverLabel->setMinimumSize(150, 150);
    // m_coverLabel->setMaximumSize(280, 280);

        // m_coverLabel->setMinimumSize(150, 150);
        // m_coverLabel->setMaximumSize(500, 500);
        // m_coverLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        // m_coverLabel->setScaledContents(false);
        // m_coverLabel->setAlignment(Qt::AlignCenter);
        m_coverLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_coverLabel->setMinimumSize(150, 150);
        m_coverLabel->setMaximumSize(500, 500);
        m_coverLabel->setAlignment(Qt::AlignCenter);

        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(30);
        shadow->setOffset(0, 6);
        shadow->setColor(QColor(0, 0, 0, 180));
        m_coverLabel->setGraphicsEffect(shadow);

        // // title
        // m_titleLabel = new QLabel("No track playing");
        // m_titleLabel->setAlignment(Qt::AlignCenter);
        // m_titleLabel->setStyleSheet(
        //     "color: white; font-size: 18px; font-weight: bold;");
        // m_titleLabel->setWordWrap(true);

        // // artist
        // m_artistLabel = new QLabel("");
        // m_artistLabel->setAlignment(Qt::AlignCenter);
        // m_artistLabel->setStyleSheet(
        //     "color: rgba(255,255,255,180); font-size: 13px;");
        m_titleLabel = new ScrollingLabel();
        QFont titleFont = m_titleLabel->font();
        titleFont.setPointSize(18);
        titleFont.setBold(true);
        m_titleLabel->setFont(titleFont);
        m_titleLabel->setFixedHeight(QFontMetrics(titleFont).height() + 4);
        QPalette titlePal = m_titleLabel->palette();
        titlePal.setColor(QPalette::WindowText, Qt::white);
         m_titleLabel->setPalette(titlePal);
        // m_titleLabel->setStyleSheet("color: white;");
        m_titleLabel->setText("No track playing");
        qDebug() << "titleLabel palette windowText:" << m_titleLabel->palette().windowText().color();
        qDebug() << "titleLabel text:" << m_titleLabel->text();
        qDebug() << "titleLabel size:" << m_titleLabel->size();


        m_artistLabel = new ScrollingLabel();
        QFont artistFont = m_artistLabel->font();
        artistFont.setPointSize(13);
        m_artistLabel->setFont(artistFont);
        m_artistLabel->setFixedHeight(QFontMetrics(artistFont).height() + 4);
        QPalette artistPal = m_artistLabel->palette();
        artistPal.setColor(QPalette::WindowText, QColor(255, 255, 255, 180));
        m_artistLabel->setPalette(artistPal);
        // m_artistLabel->setStyleSheet("color: rgba(255,255,255,180);");
        m_artistLabel->setText("");

        m_titleLabel->setFixedWidth(800);
        m_artistLabel->setFixedWidth(800);

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

    QWidget *seekContainer = new QWidget();
    seekContainer->setFixedWidth(320);
        QHBoxLayout *seekRow = new QHBoxLayout(seekContainer);
    seekRow->setContentsMargins(0, 0, 0, 0);
        seekRow->addWidget(m_elapsedLabel);
        seekRow->addWidget(m_seekBar, 1);
        seekRow->addWidget(m_remainingLabel);

        // controls — created externally, added via setControls
        m_controlsLayout = new QHBoxLayout();
        m_controlsLayout->setAlignment(Qt::AlignCenter);

        // volume controls
        m_volumeLayout = new QHBoxLayout();
        m_volumeLayout->setAlignment(Qt::AlignCenter);

        // QVBoxLayout *layout = new QVBoxLayout(this);
        // layout->setAlignment(Qt::AlignCenter);
        // layout->addStretch();
        // layout->addWidget(m_coverLabel, 0, Qt::AlignCenter);
        // layout->addSpacing(16);
        // layout->addWidget(m_titleLabel);
        // layout->addWidget(m_artistLabel);
        // layout->addSpacing(16);
        // layout->addLayout(seekRow);
        // layout->addSpacing(8);
        // layout->addLayout(m_controlsLayout);
        // layout->addStretch();

        QVBoxLayout *layout = new QVBoxLayout(this);
        // layout->setAlignment(Qt::AlignCenter);
        // layout->addStretch(2);
        // layout->addWidget(m_coverLabel, 0, Qt::AlignCenter);
        // layout->addSpacing(12);
        // layout->addWidget(m_titleLabel);
        // layout->addWidget(m_artistLabel);
        // layout->addSpacing(12);
        // // layout->addLayout(seekRow);
        // layout->addWidget(seekContainer, 0, Qt::AlignCenter);
        // layout->addSpacing(4);
        // layout->addLayout(m_controlsLayout);
        // layout->addSpacing(4);
        // layout->addLayout(m_volumeLayout);
        // layout->addStretch(1);

        // layout->addStretch(1);
        // layout->addWidget(m_coverLabel, 0, Qt::AlignCenter);
        // layout->addSpacing(12);
        // layout->addWidget(m_titleLabel);
        // layout->addWidget(m_artistLabel);
        // layout->addSpacing(12);
        // layout->addWidget(seekContainer, 0, Qt::AlignCenter);
        // layout->addSpacing(4);
        // layout->addLayout(m_controlsLayout);
        // layout->addSpacing(4);
        // layout->addLayout(m_volumeLayout);
        // layout->addStretch(1);

        // layout->addStretch(1);
        // layout->addWidget(m_coverLabel, 0, Qt::AlignCenter);
        // layout->addSpacing(8);
        // layout->addWidget(m_titleLabel, 0, Qt::AlignCenter);
        // layout->addWidget(m_artistLabel, 0, Qt::AlignCenter);
        // layout->addSpacing(8);
        // layout->addWidget(seekContainer, 0, Qt::AlignCenter);
        // layout->addSpacing(4);
        // layout->addLayout(m_controlsLayout);
        // layout->addSpacing(4);
        // layout->addLayout(m_volumeLayout);
        // layout->addStretch(1);

        QHBoxLayout *titleRow = new QHBoxLayout();
        titleRow->addStretch();
        titleRow->addWidget(m_titleLabel);
        titleRow->addStretch();

        QHBoxLayout *artistRow = new QHBoxLayout();
        artistRow->addStretch();
        artistRow->addWidget(m_artistLabel);
        artistRow->addStretch();

        layout->addStretch(2);  // more space above pushes content up
        layout->addWidget(m_coverLabel, 0, Qt::AlignCenter);
        layout->addSpacing(12);
        // layout->addWidget(m_titleLabel, 0, Qt::AlignCenter);
        // layout->addWidget(m_artistLabel, 0, Qt::AlignCenter);
        // layout->addWidget(m_titleLabel);
        // layout->addWidget(m_artistLabel);
        layout->addLayout(titleRow);
        layout->addLayout(artistRow);
        layout->addSpacing(16);  // spacing between artist and seekbar
        layout->addWidget(seekContainer, 0, Qt::AlignCenter);
        layout->addSpacing(4);
        layout->addLayout(m_controlsLayout);
        layout->addSpacing(4);
        layout->addLayout(m_volumeLayout);
        layout->addStretch(1);  // less space below

    }

    // void setCover(const QPixmap &pixmap, int radius = 16)
    // {

    //     // m_lastCover = pixmap;
    //     // int sz = qMin(m_coverLabel->width(), m_coverLabel->height());
    //     // if (sz < 50) sz = 250;
    //     m_lastCover = pixmap;
    //     // int sz = qMin(width() / 2, height() / 2);
    //     // if (sz < 50) sz = 250;
    //     // int sz = qMin(m_coverLabel->width(), m_coverLabel->height());
    //     // if (sz < 50) sz = 250;
    //     int available = qMin(width(), height()) - 150; // leave room for controls
    //     int sz = qBound(150, available, 500);
    //     qDebug() << "setCover - panel:" << size() << "available:" << available << "sz:" << sz;

    //     if (!pixmap.isNull()) {
    //         QPixmap scaled = pixmap.scaled(sz, sz,
    //             Qt::KeepAspectRatio, Qt::SmoothTransformation);
    //         // round it
    //         QPixmap rounded(scaled.size());
    //         rounded.fill(Qt::transparent);
    //         QPainter p(&rounded);
    //         p.setRenderHint(QPainter::Antialiasing);
    //         QPainterPath path;
    //         path.addRoundedRect(rounded.rect(), radius, radius);
    //         p.setClipPath(path);
    //         p.drawPixmap(0, 0, scaled);
    //         m_coverLabel->setPixmap(rounded);

    //     }

    //     if (pixmap.isNull()) {
    //         QPixmap fallback(250, 250);
    //         fallback.fill(Qt::transparent);
    //         QPainter p(&fallback);
    //         p.setRenderHint(QPainter::Antialiasing);
    //         p.setBrush(QColor(255, 255, 255, 30));
    //         p.setPen(Qt::NoPen);
    //         p.drawRoundedRect(0, 0, 250, 250, radius, radius);
    //         QIcon::fromTheme("media-album-cover").paint(&p, 50, 50, 150, 150);
    //         m_coverLabel->setPixmap(fallback);
    //     } else {
    //         QPixmap scaled = pixmap.scaled(250, 250,
    //             Qt::KeepAspectRatio, Qt::SmoothTransformation);
    //         QPixmap rounded(scaled.size());
    //         rounded.fill(Qt::transparent);
    //         QPainter p(&rounded);
    //         p.setRenderHint(QPainter::Antialiasing);
    //         QPainterPath path;
    //         path.addRoundedRect(rounded.rect(), radius, radius);
    //         p.setClipPath(path);
    //         p.drawPixmap(0, 0, scaled);
    //         m_coverLabel->setPixmap(rounded);
    //     }
    // }
    void setCover(const QPixmap &pixmap, int radius = 16)
    {
        m_lastCover = pixmap;
        // int available = qMin(width(), height()) - 150;
        // int sz = qBound(150, available, 500);
        int available = qMin(width(), height()) - 280; // more room for controls
        // int sz = qBound(150, available, 350); // cap at 350 max
        int sz = qBound(150, available, 500);

        if (!pixmap.isNull()) {
            QPixmap scaled = pixmap.scaled(sz, sz,
                Qt::KeepAspectRatio, Qt::SmoothTransformation);
            QPixmap rounded(scaled.size());
            rounded.fill(Qt::transparent);
            QPainter p(&rounded);
            p.setRenderHint(QPainter::Antialiasing);
            QPainterPath path;
            path.addRoundedRect(rounded.rect(), radius, radius);
            p.setClipPath(path);
            p.drawPixmap(0, 0, scaled);
            m_coverLabel->setFixedSize(sz, sz);
            m_coverLabel->setPixmap(rounded);
        } else {
            QPixmap fallback(sz, sz);
            fallback.fill(Qt::transparent);
            QPainter p(&fallback);
            p.setRenderHint(QPainter::Antialiasing);
            p.setBrush(QColor(255, 255, 255, 30));
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(0, 0, sz, sz, radius, radius);
            // QIcon::fromTheme("media-album-cover").paint(&p, sz/4, sz/4, sz/2, sz/2);
            // QIcon::fromTheme("audio-x-generic").paint(&p, sz/4, sz/4, sz/2, sz/2);
            QIcon::fromTheme("library-music-symbolic").paint(&p, sz/4, sz/4, sz/2, sz/2);
            m_coverLabel->setFixedSize(sz, sz);
            m_coverLabel->setPixmap(fallback);
        }
    }

    // void setTitle(const QString &title) { m_titleLabel->setText(title); }
    // void setArtist(const QString &artist) { m_artistLabel->setText(artist); }
    void setTitle(const QString &title) { m_titleLabel->setText(title); }
    void setArtist(const QString &artist) { m_artistLabel->setText(artist); }


        // QLabel *titleLabel() const { return m_titleLabel; }
        // QLabel *artistLabel() const { return m_artistLabel; }
        ScrollingLabel *titleLabel() const { return m_titleLabel; }
        ScrollingLabel *artistLabel() const { return m_artistLabel; }
        QLabel *coverLabel() const { return m_coverLabel; }
        QHBoxLayout *controlsLayout() const { return m_controlsLayout; }
        QHBoxLayout *volumeLayout() const { return m_volumeLayout; }
        SeekSlider *seekBar() const { return m_seekBar; }
        QLabel *elapsedLabel() const { return m_elapsedLabel; }
        QLabel *remainingLabel() const { return m_remainingLabel; }

private:
    QLabel *m_coverLabel = nullptr;
    // QLabel *m_titleLabel = nullptr;
    // QLabel *m_artistLabel = nullptr;
    ScrollingLabel *m_titleLabel = nullptr;
    ScrollingLabel *m_artistLabel = nullptr;
    SeekSlider *m_seekBar = nullptr;
    QLabel *m_elapsedLabel = nullptr;
    QLabel *m_remainingLabel = nullptr;
    QHBoxLayout *m_controlsLayout = nullptr;
    QPixmap m_lastCover;
    QHBoxLayout *m_volumeLayout = nullptr;

protected:
    void resizeEvent(QResizeEvent *e) override {
        QWidget::resizeEvent(e);
        if (!m_lastCover.isNull()) {
            QTimer::singleShot(10, this, [this]() {
                setCover(m_lastCover);
            });
        }
    }


};

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

class CenteredScrollingLabel : public ScrollingLabel
{
public:
    explicit CenteredScrollingLabel(QWidget *parent = nullptr)
        : ScrollingLabel(parent) {}

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setFont(font());
        p.setPen(palette().windowText().color());
        QFontMetrics fm(font());
        int textWidth = fm.horizontalAdvance(m_text);
        if (textWidth <= width()) {
            p.drawText(rect(), Qt::AlignCenter, m_text);
        } else {
            p.setClipRect(rect());
            p.drawText(-m_offset, 0, textWidth + 40, height(),
                       Qt::AlignLeft | Qt::AlignVCenter, m_text);
        }
    }
};

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

        // title
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
        // m_artistLabel->setWordWrap(true);
        m_titleLabel = new CenteredScrollingLabel();
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
        // qDebug() << "titleLabel palette windowText:" << m_titleLabel->palette().windowText().color();
        // qDebug() << "titleLabel text:" << m_titleLabel->text();
        // qDebug() << "titleLabel size:" << m_titleLabel->size();


        m_artistLabel = new CenteredScrollingLabel();
        QFont artistFont = m_artistLabel->font();
        artistFont.setPointSize(13);
        m_artistLabel->setFont(artistFont);
        m_artistLabel->setFixedHeight(QFontMetrics(artistFont).height() + 4);
        QPalette artistPal = m_artistLabel->palette();
        artistPal.setColor(QPalette::WindowText, QColor(255, 255, 255, 180));
        m_artistLabel->setPalette(artistPal);
        // m_artistLabel->setStyleSheet("color: rgba(255,255,255,180);");
        m_artistLabel->setText("");

        m_titleLabel->setFixedWidth(400);
        m_artistLabel->setFixedWidth(400);

        // seekbar
        m_seekBar = new SeekSlider(Qt::Horizontal);
        m_seekBar->setRange(0, 1000);
        m_seekBar->setValue(0);
        // m_seekBar->setStyleSheet(
        //     "QSlider::groove:horizontal {"
        //     "  height: 4px;"
        //     "  background: rgba(255,255,255,60);"
        //     "  border-radius: 2px;"
        //     "}"
        //     "QSlider::sub-page:horizontal {"
        //     "  background: white;"
        //     "  border-radius: 2px;"
        //     "}"
        //     "QSlider::handle:horizontal {"
        //     "  width: 12px; height: 12px;"
        //     "  background: white;"
        //     "  border-radius: 6px;"
        //     "  margin: -4px 0;"
        //     "}"
        // );
        m_seekBar->setStyleSheet(
            "QSlider::groove:horizontal {"
            "  height: 6px;"
            "  background: rgba(255,255,255,40);"
            "  border-radius: 3px;"
            "}"
            "QSlider::sub-page:horizontal {"
            "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
            // "    stop:0 rgba(255,255,255,200),"
                "    stop:0 rgba(180,140,255,200),"
            "    stop:1 white);"
            "  border-radius: 3px;"
            "  border: 0px solid rgba(255,255,255,60);"
            "}"
            // "QSlider::handle:horizontal {"
            // "  width: 14px; height: 14px;"
            // "  background: white;"
            // "  border-radius: 7px;"
            // "  margin: -4px 0;"
            // "  border: 2px solid rgba(255,255,255,180);"
            // "}"
            "QSlider::handle:horizontal {"
            "  width: 14px; height: 14px;"
            "  background: transparent;"
            "  border-radius: 7px;"
            "  margin: -4px 0;"
            "}"
            "QSlider::handle:horizontal:hover {"
            "  background: white;"
            "  border: 2px solid rgba(255,255,255,180);"
            "}"
            // hide handle when not hovered
            // "QSlider::handle:horizontal:!hover {"
            // "  background: transparent;"
            // "  border: none;"
            // "  width: 0px;"
            // "  margin: 0px;"
            // "}"
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

        m_extraLayout = new QHBoxLayout();
        m_extraLayout->setAlignment(Qt::AlignCenter);

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

        /*
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
        */
    // layout->addStretch(1);  // more above = higher position
    // layout->addSpacing(50);
    // layout->addSpacing(isMaximized() ? 100 : 30);
    // layout->setContentsMargins(20, 50, 20, 0);
    // layout->setContentsMargins(20, 30, 20, 0);
    layout->setContentsMargins(20, 30, 20, 20);
        layout->addWidget(m_coverLabel, 0, Qt::AlignCenter);
        layout->addSpacing(12);
        layout->addLayout(titleRow);  // or however your title is added
        layout->addLayout(artistRow);
        layout->addSpacing(16);
        layout->addWidget(seekContainer, 0, Qt::AlignCenter);
        layout->addSpacing(4);
        layout->addLayout(m_controlsLayout);
        layout->addSpacing(8);
layout->addLayout(m_extraLayout);  // loop/shuffle go here
layout->addSpacing(4);
        layout->addLayout(m_volumeLayout);
        layout->addStretch(1);

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
        // int available = qMin(width(), height()) - 280; // more room for controls
        // int available = qMin(width(), height()) - 380; // more room for controls
        // int sz = qBound(150, available, 350); // cap at 350 max
        // int sz = qBound(150, available, 500);
        // int sz = qBound(150, available, 320);
        // int available = qMin(width(), height()) - 280;
        // int sz = qBound(200, available, 380);
        int available = qMin(width(), height()) - 320;
        int sz = qBound(200, available, 320);

        // qDebug() << "setCover sz:" << sz << "panel:" << size();

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
            // QPixmap fallbackBlur(":/images/fallback.jpg");
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

            void setMaximized(bool maximized) {
                m_isMaximized = maximized;
            }
        // void setTitle(const QString &title) { m_titleLabel->setText(title); }
        // void setArtist(const QString &artist) { m_artistLabel->setText(artist); }
            void setTitle(const QString &title) { m_titleLabel->setText(title);
        }
        void setArtist(const QString &artist) { m_artistLabel->setText(artist);
        }

        void setLoopShuffleButtons(QToolButton *loop, QToolButton *shuffle) {
            m_nowPlayingLoopButton = loop;
            m_nowPlayingShuffleButton = shuffle;
        }


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

        QHBoxLayout *extraLayout() const { return m_extraLayout; }

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

    QHBoxLayout *m_extraLayout = nullptr;
    bool m_isMaximized = false;

    bool m_compact = false;
    QToolButton *m_nowPlayingLoopButton = nullptr;
    QToolButton *m_nowPlayingShuffleButton = nullptr;

protected:
    // void resizeEvent(QResizeEvent *e) override {
    //     QWidget::resizeEvent(e);
    //     if (!m_lastCover.isNull()) {
    //         QTimer::singleShot(10, this, [this]() {
    //             setCover(m_lastCover);
    //         });
    //     }
    // }
    // void resizeEvent(QResizeEvent *e) override {
    //     QWidget::resizeEvent(e);
    //     int labelWidth = qMin(width() - 100, 1000);
    //     if (m_titleLabel) m_titleLabel->setFixedWidth(labelWidth);
    //     if (m_artistLabel) m_artistLabel->setFixedWidth(labelWidth);
    //     if (!m_lastCover.isNull()) {
    //         QTimer::singleShot(10, this, [this]() {
    //             setCover(m_lastCover);
    //         });
    //     }
    // }
    void resizeEvent(QResizeEvent *e) override {
        QWidget::resizeEvent(e);
        setCompactControls(height() <= 700);
        // int topMargin = (height() > 700) ? 100 : 30;
        // if (auto *l = qobject_cast<QVBoxLayout*>(layout()))
        //     l->setContentsMargins(20, topMargin, 20, 0);
        int topMargin = (height() > 700) ? 100 : 50;
        if (auto *l = qobject_cast<QVBoxLayout*>(layout()))
            l->setContentsMargins(20, topMargin, 20, 20);
        int labelWidth = qMin(width() - 100, 1000);
        if (m_titleLabel) m_titleLabel->setFixedWidth(labelWidth);
        if (m_artistLabel) m_artistLabel->setFixedWidth(labelWidth);
        if (!m_lastCover.isNull()) {
            QTimer::singleShot(10, this, [this]() {
                setCover(m_lastCover);
            });
        }
    }
    void setCompactControls(bool compact)
    {
        if (compact == m_compact) return;
        m_compact = compact;

        // remove loop/shuffle from extraLayout
        m_extraLayout->removeWidget(m_nowPlayingLoopButton);
        m_extraLayout->removeWidget(m_nowPlayingShuffleButton);
        m_controlsLayout->removeWidget(m_nowPlayingLoopButton);
        m_controlsLayout->removeWidget(m_nowPlayingShuffleButton);

        if (compact) {
            // insert loop left of prev, shuffle right of next
            m_controlsLayout->insertWidget(0, m_nowPlayingLoopButton);
            // m_controlsLayout->insertSpacing(1, 8);
            // m_controlsLayout->addSpacing(8);
            m_controlsLayout->addWidget(m_nowPlayingShuffleButton);
        } else {
            // put them back in extraLayout
            m_extraLayout->addWidget(m_nowPlayingLoopButton);
            m_extraLayout->addWidget(m_nowPlayingShuffleButton);
        }
    }


};

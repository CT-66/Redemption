#pragma once

#include <QWidget>
#include <QImage>
#include <QColor>
#include <QPixmap>
#include <QTimer>
#include <QPainter>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QResizeEvent>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <vector>
#include <cstring>
#include <QPointer>

static void blurH(float *buf, int w, int h, int r)
{
    const float inv = 1.0f / float(r * 2 + 1);
    std::vector<float> row(size_t(w + r * 2) * 3);

    for (int y = 0; y < h; y++) {
        float *src = buf + y * w * 3;
        for (int x = 0; x < r; x++) {
            int sx = r - 1 - x;
            row[x*3+0] = src[sx*3+0];
            row[x*3+1] = src[sx*3+1];
            row[x*3+2] = src[sx*3+2];
        }
        std::memcpy(row.data() + r*3, src, size_t(w) * 3 * sizeof(float));
        for (int x = 0; x < r; x++) {
            int sx = w - 1 - x;
            row[(r+w+x)*3+0] = src[sx*3+0];
            row[(r+w+x)*3+1] = src[sx*3+1];
            row[(r+w+x)*3+2] = src[sx*3+2];
        }
        float s0 = 0, s1 = 0, s2 = 0;
        int d = r * 2 + 1;
        for (int x = 0; x < d; x++) { s0+=row[x*3]; s1+=row[x*3+1]; s2+=row[x*3+2]; }
        for (int x = 0; x < w; x++) {
            src[x*3+0] = s0 * inv;
            src[x*3+1] = s1 * inv;
            src[x*3+2] = s2 * inv;
            s0 += row[(x+d)*3+0] - row[x*3+0];
            s1 += row[(x+d)*3+1] - row[x*3+1];
            s2 += row[(x+d)*3+2] - row[x*3+2];
        }
    }
}

static void transposeBuffer(float *buf, int w, int h)
{
    std::vector<float> tmp(size_t(w) * h * 3);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            tmp[(x*h+y)*3+0] = buf[(y*w+x)*3+0];
            tmp[(x*h+y)*3+1] = buf[(y*w+x)*3+1];
            tmp[(x*h+y)*3+2] = buf[(y*w+x)*3+2];
        }
    std::memcpy(buf, tmp.data(), tmp.size() * sizeof(float));
}

static QImage separableGaussianBlur(const QImage &input, int radius)
{
    const QImage src = input.convertToFormat(QImage::Format_RGB32);
    const int w = src.width(), h = src.height();

    std::vector<float> buf(size_t(w) * h * 3);
    for (int y = 0; y < h; y++) {
        const auto *line = reinterpret_cast<const QRgb*>(src.constScanLine(y));
        for (int x = 0; x < w; x++) {
            buf[(y*w+x)*3+0] = float(qRed  (line[x]));
            buf[(y*w+x)*3+1] = float(qGreen(line[x]));
            buf[(y*w+x)*3+2] = float(qBlue (line[x]));
        }
    }

    for (int pass = 0; pass < 3; pass++) {
        blurH(buf.data(), w, h, radius);
        transposeBuffer(buf.data(), w, h);
        blurH(buf.data(), h, w, radius);
        transposeBuffer(buf.data(), h, w);
    }

    QImage out(w, h, QImage::Format_RGB32);
    for (int y = 0; y < h; y++) {
        auto *line = reinterpret_cast<QRgb*>(out.scanLine(y));
        for (int x = 0; x < w; x++) {
            int r2 = qBound(0, int(buf[(y*w+x)*3+0]), 255);
            int g  = qBound(0, int(buf[(y*w+x)*3+1]), 255);
            int b  = qBound(0, int(buf[(y*w+x)*3+2]), 255);
            line[x] = qRgb(r2, g, b);
        }
    }
    return out;
}

class AmbientBar : public QWidget
{
    Q_OBJECT

public:
    explicit AmbientBar(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_StyledBackground, false);
        m_watcher = new QFutureWatcher<QPair<QImage, QColor>>(this);
        connect(m_watcher, &QFutureWatcher<QPair<QImage, QColor>>::finished,
                this, &AmbientBar::onBlurFinished);
    }

    void updateFromCover(const QPixmap &cover)
    {
        if (m_fadeTimer) {
            m_fadeTimer->stop();
            m_fadeTimer->deleteLater();
            m_fadeTimer = nullptr;
        }

        if (cover.isNull()) {
            m_previousImage = m_blurredImage;
            m_previousDominant = m_dominant;
            m_blurredImage = QImage();
            m_dominant = QColor(30, 30, 40);
            startFade();
            return;
        }

        m_previousImage = m_blurredImage;
        m_previousDominant = m_dominant;
        m_fadeOpacity = 0.0f;

        const QImage src = cover.toImage().convertToFormat(QImage::Format_RGB32);
        const int targetW = width() > 0 ? width() : 900;
        const int targetH = height() > 0 ? height() : 175;

        auto future = QtConcurrent::run([src, targetW, targetH]() {
            const QImage half = src.scaled(
                src.width() / 2, src.height() / 2,
                Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            const QImage blurred = separableGaussianBlur(half, 30);
            QImage result = blurred.scaled(
                targetW, targetH,
                Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

            // dominant color — original "cheesy" values
            const int w = src.width(), h = src.height();
            constexpr int samples = 16;
            const int stepX = qMax(1, w / samples);
            const int stepY = qMax(1, h / samples);
            qint64 rSum = 0, gSum = 0, bSum = 0, count = 0;
            for (int y = 0; y < h; y += stepY) {
                const auto *line = reinterpret_cast<const QRgb*>(src.constScanLine(y));
                for (int x = 0; x < w; x += stepX) {
                    rSum += qRed(line[x]);
                    gSum += qGreen(line[x]);
                    bSum += qBlue(line[x]);
                    ++count;
                }
            }
            QColor avg(int(rSum/count), int(gSum/count), int(bSum/count));
            QColor hsv = avg.toHsv();
            int s = qMin(255, int(hsv.saturation() * 2.2));
            int v = qMin(255, int(hsv.value() * 0.75));
            hsv.setHsv(hsv.hsvHue(), s, v);

            return QPair<QImage, QColor>(result, hsv);
        });

        m_watcher->setFuture(future);
    }

    void setAmbientEnabled(bool enabled)
    {
        m_enabled = enabled;
        update();
    }
        void setDarkOverlay(int alpha) { m_darkOverlay = alpha; update(); }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        const QRectF r = rect();

        if (!m_enabled) {
            p.fillRect(r, palette().window().color());
            return;
        }

        if (m_blurredImage.isNull() && m_previousImage.isNull()) {
            p.fillRect(r, palette().window().color());
            return;
        }

        // previous fading out
        if (!m_previousImage.isNull() && m_fadeOpacity < 1.0f) {
            p.setOpacity(1.0f - m_fadeOpacity);
            p.drawImage(r, m_previousImage);
            p.fillRect(r, QColor(0, 0, 0, m_darkOverlay));
            drawGlows(p, r, m_previousDominant);
        }

        // current fading in
        if (!m_blurredImage.isNull()) {
            p.setOpacity(m_fadeOpacity);
            p.drawImage(r, m_blurredImage);
            p.fillRect(r, QColor(0, 0, 0, m_darkOverlay));
            drawGlows(p, r, m_dominant);
        }

        p.setOpacity(1.0f);
    }

    void resizeEvent(QResizeEvent *e) override
    {
        QWidget::resizeEvent(e);
        if (!m_blurredImage.isNull())
            m_blurredImage = m_blurredImage.scaled(
                width(), height(),
                Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

private slots:
    void onBlurFinished()
    {
        auto result = m_watcher->result();
        m_blurredImage = result.first;
        m_previousDominant = m_dominant;
        m_dominant = result.second;
        startFade();
    }

private:
    QImage m_blurredImage;
    QImage m_previousImage;
    QColor m_dominant{30, 30, 40};
    QColor m_previousDominant{30, 30, 40};
    float m_fadeOpacity = 1.0f;
    bool m_enabled = true;
    QTimer *m_fadeTimer = nullptr;
    QFutureWatcher<QPair<QImage, QColor>> *m_watcher = nullptr;
    int m_darkOverlay = 110;

    // void startFade()
    // {
    //     m_fadeOpacity = 0.0f;
    //     m_fadeTimer = new QTimer(this);
    //     m_fadeTimer->setInterval(16);
    //     connect(m_fadeTimer, &QTimer::timeout, this, [this]() {
    //         m_fadeOpacity += 0.04f;
    //         if (m_fadeOpacity >= 1.0f) {
    //             m_fadeOpacity = 1.0f;
    //             m_fadeTimer->stop();
    //             m_fadeTimer->deleteLater();
    //             m_fadeTimer = nullptr;
    //             m_previousImage = QImage();
    //         }
    //         update();
    //     });
    //     m_fadeTimer->start();
    // }
    void startFade()
    {
        m_fadeOpacity = 0.0f;
        if (m_fadeTimer) {
            m_fadeTimer->stop();
            delete m_fadeTimer;  // delete immediately instead of deleteLater
            m_fadeTimer = nullptr;
        }
        m_fadeTimer = new QTimer(this);
        m_fadeTimer->setInterval(16);
        QPointer<AmbientBar> guard(this);  // guard against deletion
        connect(m_fadeTimer, &QTimer::timeout, this, [this, guard]() {
            if (!guard) return;  // widget was deleted
            m_fadeOpacity += 0.04f;
            if (m_fadeOpacity >= 1.0f) {
                m_fadeOpacity = 1.0f;
                if (m_fadeTimer) {
                    m_fadeTimer->stop();
                    delete m_fadeTimer;
                    m_fadeTimer = nullptr;
                }
                m_previousImage = QImage();
            }
            update();
        });
        m_fadeTimer->start();
    }

    void drawGlows(QPainter &p, const QRectF &r, const QColor &dominant)
    {
        // primary glow — upper right
        QColor gc = dominant;
        gc.setAlpha(160);
        QColor tr = gc; tr.setAlpha(0);
        QRadialGradient glow(r.width() * 0.72, r.height() * 0.22, r.width() * 0.65);
        glow.setColorAt(0.0, gc);
        glow.setColorAt(1.0, tr);
        p.fillRect(r, glow);

        // secondary glow — lower left
        gc.setAlpha(80);
        tr.setAlpha(0);
        QRadialGradient glow2(r.width() * 0.15, r.height() * 0.85, r.width() * 0.5);
        glow2.setColorAt(0.0, gc);
        glow2.setColorAt(1.0, tr);
        p.fillRect(r, glow2);
    }
};

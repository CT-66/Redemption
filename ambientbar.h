#pragma once

#include <QPainter>
#include <QRadialGradient>
#include <QLinearGradient>
#include <QResizeEvent>
#include <QWidget>
#include <QImage>
#include <QColor>
#include <QPixmap>
#include <vector>
#include <cstring>
#include <QTimer>

// Separable box blur implementation (pure Qt, no deps)
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
    }

    void updateFromCover(const QPixmap &cover)
    {

        if (m_fadeTimer) {
            m_fadeTimer->stop();
            m_fadeTimer->deleteLater();
            m_fadeTimer = nullptr;
        }

        m_previousImage = m_blurredImage;
        m_fadeOpacity = 0.0;

        if (cover.isNull()) {
            m_blurredImage = QImage();
            m_dominant = QColor(30, 30, 40);
            update();
            return;
        }

        const QImage src = cover.toImage().convertToFormat(QImage::Format_RGB32);

        // build blurred background
        const QImage half = src.scaled(
            src.width() / 2, src.height() / 2,
            Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        const QImage blurred = separableGaussianBlur(half, 30);
        m_blurredImage = blurred.scaled(
            width() > 0 ? width() : 900,
            height() > 0 ? height() : 175,
            Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        // extract dominant color
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
        m_dominant = hsv;

    if (!m_previousImage.isNull()) {
        m_fadeTimer = new QTimer(this);
        m_fadeTimer->setInterval(16); // ~60fps
        connect(m_fadeTimer, &QTimer::timeout, this, [this]() {
            m_fadeOpacity += 0.05f;
            if (m_fadeOpacity >= 1.0f) {
                m_fadeOpacity = 1.0f;
                m_fadeTimer->stop();
                m_fadeTimer->deleteLater();
                m_fadeTimer = nullptr;
                m_previousImage = QImage();
            }
            update();
        });
        m_fadeTimer->start();
        } else {
            m_fadeOpacity = 1.0f;
            update();
        }

    }
    void setAmbientEnabled(bool enabled) {
        m_enabled = enabled;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {

        QPainter p(this);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        const QRectF r = rect();

        // if (m_blurredImage.isNull()) {
        //     // fallback — dark gradient
        //     QLinearGradient grad(0, 0, 0, r.height());
        //     grad.setColorAt(0.0, QColor(30, 30, 40));
        //     grad.setColorAt(1.0, QColor(20, 20, 30));
        //     p.fillRect(r, grad);
        // } else {
        if (!m_enabled) {
            p.fillRect(r, palette().window().color());
            return;
        }
        if (m_blurredImage.isNull()) {
            p.fillRect(r, palette().window().color());
            return;
        } else {

    if (!m_previousImage.isNull() && m_fadeOpacity < 1.0) {
    // draw previous
    p.setOpacity(1.0 - m_fadeOpacity);
    p.drawImage(r, m_previousImage);
    p.setOpacity(1.0);
}

// draw current with fade in
p.setOpacity(m_fadeOpacity);
p.drawImage(r, m_blurredImage);
p.setOpacity(1.0);

// overlays at full opacity
p.fillRect(r, QColor(0, 0, 0, 180));
// ... rest of glows etc

            // blurred cover fills bar
            p.drawImage(r, m_blurredImage);

            // dark tint
            p.fillRect(r, QColor(0, 0, 0, 140));

            // dominant color radial glow — upper right
            {
                QColor gc = m_dominant;
                gc.setAlpha(120);
                QColor tr = gc; tr.setAlpha(0);
                QRadialGradient glow(r.width() * 0.75, 0, r.width() * 0.6);
                glow.setColorAt(0.0, gc);
                glow.setColorAt(1.0, tr);
                p.fillRect(r, glow);
            }

            // secondary glow — lower left
            {
                QColor gc = m_dominant;
                gc.setAlpha(60);
                QColor tr = gc; tr.setAlpha(0);
                QRadialGradient glow2(r.width() * 0.1, r.height(), r.width() * 0.4);
                glow2.setColorAt(0.0, gc);
                glow2.setColorAt(1.0, tr);
                p.fillRect(r, glow2);
            }

            // top border fade
            {
                QLinearGradient vt(0, 0, 0, 20);
                vt.setColorAt(0.0, QColor(0, 0, 0, 60));
                vt.setColorAt(1.0, QColor(0, 0, 0, 0));
                p.fillRect(r, vt);
            }
        }
    }

    void resizeEvent(QResizeEvent *e) override
    {
        QWidget::resizeEvent(e);
        // rescale blurred image to new size if available
        if (!m_blurredImage.isNull())
            m_blurredImage = m_blurredImage.scaled(
                width(), height(),
                Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

private:
    QImage m_blurredImage;
    QColor m_dominant{30, 30, 40};
    bool m_enabled = true;
    QImage m_previousImage;
    qreal m_fadeOpacity = 1.0;
    QTimer *m_fadeTimer = nullptr;
};



#pragma once

#include <QWidget>
#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QDir>
#include <QFileInfo>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>

#include "metadatareader.h"

class PlaylistView : public QWidget
{
    Q_OBJECT

public:
    explicit PlaylistView(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setObjectName("playlistView");

                // setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        // top bar
        m_dirLabel = new QLabel("No directory");
        m_dirLabel->setStyleSheet(
            "color: white; font-size: 13px; font-weight: bold;");

        m_searchButton = new QToolButton();
        m_searchButton->setIcon(QIcon::fromTheme("system-search"));
        m_searchButton->setAutoRaise(true);
        m_searchButton->setToolTip("Search");
        m_searchButton->setStyleSheet("color: white;");

        m_switchButton = new QToolButton();
        m_switchButton->setIcon(QIcon::fromTheme("view-list-tree"));
        m_switchButton->setAutoRaise(true);
        m_switchButton->setToolTip("Switch to tree view");
        m_switchButton->setStyleSheet("color: white;");

        QHBoxLayout *topBar = new QHBoxLayout();
        topBar->addWidget(m_dirLabel);
        topBar->addStretch();
        topBar->addWidget(m_searchButton);
        topBar->addWidget(m_switchButton);

        // song list
        m_listWidget = new QListWidget();
        m_listWidget->setStyleSheet(
            "QListWidget {"
            "  background: transparent;"
            "  border: none;"
            "  color: white;"
            "}"
            "QListWidget::item {"
            "  padding: 4px;"
            "  border-radius: 4px;"
            "}"
            "QListWidget::item:selected {"
            "  background: rgba(255,255,255,40);"
            "}"
            "QListWidget::item:hover:!selected {"
            "  background: rgba(255,255,255,20);"
            "}"
        );
        m_listWidget->setIconSize(QSize(40, 40));
        m_listWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setContentsMargins(12, 12, 12, 12);
        layout->addLayout(topBar);
        layout->addSpacing(8);
        layout->addWidget(m_listWidget);

        m_listWidget->setFocusPolicy(Qt::ClickFocus);
        m_searchButton->setFocusPolicy(Qt::NoFocus);
        m_switchButton->setFocusPolicy(Qt::NoFocus);

    }

    void loadDirectory(const QString &dirPath, const QString &currentTrack)
    {
        m_listWidget->clear();
        m_currentDir = dirPath;
        m_dirLabel->setText(QFileInfo(dirPath).fileName());

        QDir dir(dirPath);
        dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);

        static const QStringList AUDIO_EXTENSIONS = {
            "mp3", "flac", "ogg", "opus", "wav", "aac",
            "m4a", "wma", "ape", "mpc", "aiff", "tta"
        };

        QFileInfoList entries = dir.entryInfoList();
        // natural sort
        // std::sort(entries.begin(), entries.end(),
        //     [](const QFileInfo &a, const QFileInfo &b) {
        //         return a.fileName().toLower() < b.fileName().toLower();
        //     });
        std::sort(entries.begin(), entries.end(),
            [](const QFileInfo &a, const QFileInfo &b) {
                QString an = a.fileName().toLower();
                QString bn = b.fileName().toLower();
                int i = 0, j = 0;
                while (i < an.size() && j < bn.size()) {
                    if (an[i].isDigit() && bn[j].isDigit()) {
                        QString na, nb;
                        while (i < an.size() && an[i].isDigit()) na += an[i++];
                        while (j < bn.size() && bn[j].isDigit()) nb += bn[j++];
                        if (na != nb) return na.toInt() < nb.toInt();
                    } else {
                        if (an[i] != bn[j]) return an[i] < bn[j];
                        ++i; ++j;
                    }
                }
                return an.size() < bn.size();
            });

        for (const QFileInfo &info : entries) {
            if (!AUDIO_EXTENSIONS.contains(info.suffix().toLower()))
                continue;

            TrackMetadata meta = MetadataReader::read(info.filePath());
            QString title = meta.hasTitle ? meta.title : info.fileName();

            QListWidgetItem *item = new QListWidgetItem(title);
            item->setData(Qt::UserRole, info.filePath());

            // cover thumbnail
            if (meta.hasCover) {
                QPixmap thumb = meta.cover.scaled(40, 40,
                    Qt::KeepAspectRatio, Qt::SmoothTransformation);
                // round the thumbnail
                QPixmap rounded(40, 40);
                rounded.fill(Qt::transparent);
                QPainter p(&rounded);
                p.setRenderHint(QPainter::Antialiasing);
                QPainterPath path;
                path.addRoundedRect(0, 0, 40, 40, 6, 6);
                p.setClipPath(path);
                p.drawPixmap(0, 0, thumb);
                item->setIcon(QIcon(rounded));
            } else {
                // item->setIcon(QIcon::fromTheme("media-album-cover"));
                QPixmap placeholder(40, 40);
                placeholder.fill(Qt::transparent);
                QPainter p(&placeholder);
                p.setRenderHint(QPainter::Antialiasing);
                p.setBrush(QColor(255, 255, 255, 30));
                p.setPen(Qt::NoPen);
                p.drawRoundedRect(0, 0, 40, 40, 6, 6);
                // QIcon::fromTheme("media-album-cover").paint(&p, 8, 8, 24, 24);
                // QIcon::fromTheme("audio-x-generic").paint(&p, 8, 8, 24, 24);
                // QIcon::fromTheme("library-music-symbolic").paint(&p, 8, 8, 24, 24);
                QPixmap iconPx = QIcon::fromTheme("library-music-symbolic")
                    .pixmap(QSize(24, 24));
                p.drawPixmap(8, 8, iconPx);
                item->setIcon(QIcon(placeholder));
            }

            m_listWidget->addItem(item);
        }

        // highlight current track
        updateCurrentTrack(currentTrack);
    }

    void updateCurrentTrack(const QString &trackPath)
    {
        m_currentTrack = trackPath;
        for (int i = 0; i < m_listWidget->count(); ++i) {
            QListWidgetItem *item = m_listWidget->item(i);
            bool isCurrent = item->data(Qt::UserRole).toString() == trackPath;
            QFont f = item->font();
            f.setBold(isCurrent);
            item->setFont(f);
            if (isCurrent) {
                m_listWidget->scrollToItem(item, QAbstractItemView::PositionAtCenter);
                m_listWidget->setCurrentItem(item);
            }
        }
    }

    /*
    void loadQueue(const QStringList &queue, const QString &currentTrack)
    {
        m_listWidget->clear();
        m_currentDir = ""; // not a directory view
        m_dirLabel->setText("Queue");

        for (const QString &path : queue) {
            TrackMetadata meta = MetadataReader::read(path);
            QString title = meta.hasTitle ? meta.title : QFileInfo(path).fileName();

            QListWidgetItem *item = new QListWidgetItem(title);
            item->setData(Qt::UserRole, path);

            if (meta.hasCover) {
                QPixmap thumb = meta.cover.scaled(40, 40,
                    Qt::KeepAspectRatio, Qt::SmoothTransformation);
                QPixmap rounded(40, 40);
                rounded.fill(Qt::transparent);
                QPainter p(&rounded);
                p.setRenderHint(QPainter::Antialiasing);
                QPainterPath path2;
                path2.addRoundedRect(0, 0, 40, 40, 6, 6);
                p.setClipPath(path2);
                p.drawPixmap(0, 0, thumb);
                item->setIcon(QIcon(rounded));
            } else {
                QPixmap placeholder(40, 40);
                placeholder.fill(Qt::transparent);
                QPainter p(&placeholder);
                p.setRenderHint(QPainter::Antialiasing);
                p.setBrush(QColor(255, 255, 255, 30));
                p.setPen(Qt::NoPen);
                p.drawRoundedRect(0, 0, 40, 40, 6, 6);
                QPixmap iconPx = QIcon::fromTheme("audio-x-generic")
                    .pixmap(QSize(24, 24));
                p.drawPixmap(8, 8, iconPx);
                item->setIcon(QIcon(placeholder));
            }

            m_listWidget->addItem(item);
        }
        updateCurrentTrack(currentTrack);
    }
    */
   void loadQueue(const QStringList &queue, const QString &currentTrack,
                   int windowSize = 20)
    {
        m_listWidget->clear();
        m_currentDir = "";
        m_dirLabel->setText("Queue");

        // find current track position
        int currentIdx = queue.indexOf(currentTrack);
        if (currentIdx < 0) currentIdx = 0;

        // show windowSize items centered around current
        int start = qMax(0, currentIdx - 2); // show 2 before current
        int end = qMin(queue.size(), start + windowSize);
        // adjust start if end hit the limit
        start = qMax(0, end - windowSize);

        for (int i = start; i < end; i++) {
            const QString &path = queue[i];
            TrackMetadata meta = MetadataReader::read(path);
            QString title = meta.hasTitle
                ? meta.title : QFileInfo(path).fileName();

            QListWidgetItem *item = new QListWidgetItem(title);
            item->setData(Qt::UserRole, path);

            // position indicator
            if (i == currentIdx) {
                QFont f = item->font();
                f.setBold(true);
                item->setFont(f);
            }

            if (meta.hasCover) {
                QPixmap thumb = meta.cover.scaled(40, 40,
                    Qt::KeepAspectRatio, Qt::SmoothTransformation);
                QPixmap rounded(40, 40);
                rounded.fill(Qt::transparent);
                QPainter p(&rounded);
                p.setRenderHint(QPainter::Antialiasing);
                QPainterPath path2;
                path2.addRoundedRect(0, 0, 40, 40, 6, 6);
                p.setClipPath(path2);
                p.drawPixmap(0, 0, thumb);
                item->setIcon(QIcon(rounded));
            } else {
                QPixmap placeholder(40, 40);
                placeholder.fill(Qt::transparent);
                QPainter p(&placeholder);
                p.setRenderHint(QPainter::Antialiasing);
                p.setBrush(QColor(255, 255, 255, 30));
                p.setPen(Qt::NoPen);
                p.drawRoundedRect(0, 0, 40, 40, 6, 6);
                QPixmap iconPx = QIcon::fromTheme("audio-x-generic")
                    .pixmap(QSize(24, 24));
                p.drawPixmap(8, 8, iconPx);
                item->setIcon(QIcon(placeholder));
            }

            m_listWidget->addItem(item);
        }

        updateCurrentTrack(currentTrack);
    }

    QToolButton *switchButton() const { return m_switchButton; }
    QToolButton *searchButton() const { return m_searchButton; }
    QListWidget *listWidget() const { return m_listWidget; }
    QString currentDir() const { return m_currentDir; }

private:
    QLabel *m_dirLabel = nullptr;
    QToolButton *m_searchButton = nullptr;
    QToolButton *m_switchButton = nullptr;
    QListWidget *m_listWidget = nullptr;
    QString m_currentDir;
    QString m_currentTrack;
    QHash<QString, QIcon> m_thumbnailCache;
};

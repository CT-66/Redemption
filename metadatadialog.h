#pragma once

#include <QDialog>
#include <QTabWidget>
#include <QLabel>
#include <QLineEdit>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QTreeWidget>
#include <QPixmap>
#include <QFileInfo>
#include "metadatareader.h"
#include <QHeaderView>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QEvent>
#include <QMouseEvent>
#include "toastnotification.h"
#include <QTimer>

#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tpropertymap.h>

#include "coverartdialog.h"

class MetadataDialog : public QDialog
{
public:
    explicit MetadataDialog(const QString &filePath, QWidget *parent = nullptr)
        : QDialog(parent), m_filePath(filePath)
    {
        setWindowTitle("Track Information");
        setMinimumSize(500, 400);

        QTabWidget *tabs = new QTabWidget(this);

        // --- Tab 1: Basic ---
        QWidget *basicTab = new QWidget();
        QHBoxLayout *basicMain = new QHBoxLayout(basicTab);

        // cover art on the left
        QLabel *coverLabel = new QLabel();
        coverLabel->setFixedSize(150, 150);
        coverLabel->setAlignment(Qt::AlignCenter);
        coverLabel->setStyleSheet("background: palette(mid); border-radius: 4px;");

        coverLabel->setCursor(Qt::PointingHandCursor);
        coverLabel->setToolTip("Click to view full size");
        coverLabel->installEventFilter(this);
        m_coverLabel = coverLabel;


        TrackMetadata meta = MetadataReader::read(filePath);
        if (meta.hasCover) {
            m_coverPixmap = meta.cover;
            QPixmap scaled = meta.cover.scaled(150, 150,
                Qt::KeepAspectRatio, Qt::SmoothTransformation);
            coverLabel->setPixmap(scaled);
        }

        // fields on the right
        m_titleEdit = new QLineEdit();
        m_artistEdit = new QLineEdit();
        m_albumEdit = new QLineEdit();

        if (meta.hasTitle) m_titleEdit->setText(meta.title);
        if (meta.hasArtist) m_artistEdit->setText(meta.artist);

        // read album via taglib
        TagLib::FileRef f(filePath.toLocal8Bit().constData());
        if (!f.isNull() && f.tag()) {
            QString album = QString::fromStdString(f.tag()->album().to8Bit(true));
            if (!album.isEmpty()) m_albumEdit->setText(album);
        }

        QFormLayout *formLayout = new QFormLayout();
        formLayout->addRow("Title:", m_titleEdit);
        formLayout->addRow("Artist:", m_artistEdit);
        formLayout->addRow("Album:", m_albumEdit);

        // QLabel *fileLabel = new QLabel(QFileInfo(filePath).fileName());
        // fileLabel->setStyleSheet("color: palette(windowtext); font-size: 14px;");
        // formLayout->addRow("File:", fileLabel);

        // clickable filename with file manager button
        QLabel *fileLabel = new QLabel(QFileInfo(filePath).fileName());
        fileLabel->setStyleSheet("color: palette(windowtext); font-size: 14px;");
        fileLabel->setCursor(Qt::PointingHandCursor);
        fileLabel->setToolTip("Click to copy path");
        fileLabel->installEventFilter(this);
        // ToastNotification::show(this, "Copied to clipboard", "edit-copy");

        QPushButton *openDirBtn = new QPushButton();
        openDirBtn->setIcon(QIcon::fromTheme("folder-open"));
        openDirBtn->setFixedSize(24, 24);
        openDirBtn->setFlat(true);
        openDirBtn->setToolTip("Open containing folder");
        connect(openDirBtn, &QPushButton::clicked, this, [filePath]() {
            QDesktopServices::openUrl(QUrl::fromLocalFile(
                QFileInfo(filePath).absolutePath()));
        });

        QHBoxLayout *fileRow = new QHBoxLayout();
        fileRow->addWidget(fileLabel);
        fileRow->addWidget(openDirBtn);
        fileRow->addStretch();

        formLayout->addRow("File:", fileRow);

        QVBoxLayout *rightLayout = new QVBoxLayout();
        rightLayout->addLayout(formLayout);
        rightLayout->addStretch();

        basicMain->addWidget(coverLabel, 0, Qt::AlignTop);
        basicMain->addSpacing(12);
        basicMain->addLayout(rightLayout, 1);

        // --- Tab 2: Raw Metadata ---
        QWidget *rawTab = new QWidget();
        QVBoxLayout *rawLayout = new QVBoxLayout(rawTab);

        m_rawTree = new QTreeWidget();
        m_rawTree->setHeaderLabels({"Field", "Value"});
        m_rawTree->setRootIsDecorated(false);
        m_rawTree->header()->setStretchLastSection(true);
        m_rawTree->setAlternatingRowColors(true);

        if (!f.isNull()) {
            TagLib::PropertyMap props = f.file()->properties();
            for (auto it = props.begin(); it != props.end(); ++it) {
                QString key = QString::fromStdString(it->first.to8Bit(true));
                QStringList values;
                for (const auto &v : it->second)
                    values << QString::fromStdString(v.to8Bit(true));
                QTreeWidgetItem *item = new QTreeWidgetItem(m_rawTree);
                item->setText(0, key);
                item->setText(1, values.join(", "));
            }
        }

        rawLayout->addWidget(m_rawTree);

        tabs->addTab(basicTab, "Basic");
        tabs->addTab(rawTab, "Raw Metadata");

        // --- buttons ---
        QPushButton *saveButton = new QPushButton("Save");
        QPushButton *closeButton = new QPushButton("Close");

        connect(saveButton, &QPushButton::clicked, this, &MetadataDialog::saveMetadata);
        connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

        QHBoxLayout *btnLayout = new QHBoxLayout();
        btnLayout->addStretch();
        btnLayout->addWidget(saveButton);
        btnLayout->addWidget(closeButton);

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->addWidget(tabs);
        mainLayout->addLayout(btnLayout);

        m_fileLabel = fileLabel;
        m_fullPath = filePath;
    }

private:
    QString m_filePath;
    QLineEdit *m_titleEdit = nullptr;
    QLineEdit *m_artistEdit = nullptr;
    QLineEdit *m_albumEdit = nullptr;
    QTreeWidget *m_rawTree = nullptr;
    QLabel *m_fileLabel = nullptr;
    QString m_fullPath;
    QPixmap m_coverPixmap;
    QLabel *m_coverLabel = nullptr;

    void saveMetadata()
    {
        TagLib::FileRef f(m_filePath.toLocal8Bit().constData());
        if (f.isNull() || !f.tag())
            return;

        f.tag()->setTitle(TagLib::String(m_titleEdit->text().toStdString(),
                                         TagLib::String::UTF8));
        f.tag()->setArtist(TagLib::String(m_artistEdit->text().toStdString(),
                                          TagLib::String::UTF8));
        f.tag()->setAlbum(TagLib::String(m_albumEdit->text().toStdString(),
                                         TagLib::String::UTF8));
        f.save();
        accept();
    }
protected:
    bool eventFilter(QObject *obj, QEvent *event) override
    {
        if (obj == m_coverLabel && event->type() == QEvent::MouseButtonPress) {
            if (!m_coverPixmap.isNull()) {
                CoverArtDialog dialog(m_coverPixmap, this);
                dialog.exec();
            }
            return true;
        }

        if (obj == m_fileLabel) {
            if (event->type() == QEvent::MouseButtonPress) {
                QApplication::clipboard()->setText(m_fullPath);
                // flash highlight
                m_fileLabel->setStyleSheet(
                    "color: palette(highlight); font-size: 14; font-weight: bold;");
                QTimer::singleShot(300, this, [this]() {
                    m_fileLabel->setStyleSheet(
                        "color: palette(windowtext); font-size: 14px;");
                });
                ToastNotification::show(this, "Copied to clipboard", "edit-copy");
                return true;
            }
        }
        return QDialog::eventFilter(obj, event);
    }
};

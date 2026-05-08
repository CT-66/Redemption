#pragma once

#include <QDialog>
#include <QCheckBox>
#include <QFontComboBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSettings>
#include <QApplication>
#include <QSpinBox>
#include <QComboBox>
#include <QStandardPaths>

class SettingsDialog : public QDialog
{

public:
    explicit SettingsDialog(QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("Redemption — Settings");
        setMinimumWidth(300);

        // QSettings settings("settings.ini", QSettings::IniFormat);
        // QSettings settings(
            // QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/settings.ini",
            // QSettings::IniFormat);
            QSettings settings(
                QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                + "/Redemption/settings.ini",
                QSettings::IniFormat);

        m_cleanTree = new QCheckBox("Clean tree view (no branch lines)");
        m_cleanTree->setChecked(settings.value("treeview/clean", true).toBool());

        m_singleClickExpand = new QCheckBox("Expand folders on single click");
        m_singleClickExpand->setChecked(settings.value("treeview/singleclick", false).toBool());

        m_showQueue = new QCheckBox("Show queue button");
        m_showQueue->setChecked(settings.value("ui/showQueue", true).toBool());

        m_gapless = new QCheckBox("Gapless playback");
        m_gapless->setChecked(settings.value("playback/gapless", true).toBool());

        m_replaygain = new QComboBox();
        m_replaygain->addItems({"Off", "Track", "Album"});
        QString rg = settings.value("playback/replaygain", "track").toString();
        m_replaygain->setCurrentIndex(rg == "off" ? 0 : rg == "track" ? 1 : 2);

        QLabel *rgLabel = new QLabel("Replay gain:");

        m_fontCombo = new QFontComboBox();
        m_fontCombo->setCurrentFont(QFont(settings.value("font/family",
            QApplication::font().family()).toString()));

        m_fontSize = new QSpinBox();
        m_fontSize->setRange(6, 24);
        m_fontSize->setValue(settings.value("font/size",
            QApplication::font().pointSize()).toInt());

        QDialogButtonBox *buttons = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->addWidget(new QLabel("Tree view style:"));
        layout->addWidget(m_cleanTree);
        layout->addSpacing(12);
        layout->addWidget(new QLabel("Behavior:"));
        layout->addWidget(m_singleClickExpand);
        layout->addSpacing(12);
        layout->addWidget(new QLabel("Queue Button:"));
        layout->addWidget(m_showQueue);
        layout->addSpacing(12);
        layout->addWidget(new QLabel("Playback:"));
        layout->addWidget(m_gapless);
        layout->addSpacing(12);
        layout->addWidget(rgLabel);
        layout->addWidget(m_replaygain);
        layout->addSpacing(12);
        layout->addWidget(new QLabel("Font:"));
        layout->addWidget(m_fontCombo);
        layout->addWidget(new QLabel("Font size:"));
        layout->addWidget(m_fontSize);
        layout->addSpacing(12);
        layout->addWidget(buttons);

    }

    bool cleanTree() const { return m_cleanTree->isChecked(); }
    bool singleClickExpand() const { return m_singleClickExpand->isChecked(); }
    QFont selectedFont() const { return m_fontCombo->currentFont(); }
    int selectedFontSize() const { return m_fontSize->value(); }
    bool gapless() const { return m_gapless->isChecked(); }
    QString replaygain() const {
        switch (m_replaygain->currentIndex()) {
        case 0: return "no";
        case 1: return "track";
        case 2: return "album";
        default: return "no";
        }
    }
    bool showQueue() const { return m_showQueue->isChecked(); }

private:
    QCheckBox *m_checkBox = nullptr;
    QCheckBox *m_cleanTree = nullptr;
    QCheckBox *m_singleClickExpand = nullptr;
    QFontComboBox *m_fontCombo = nullptr;
    QSpinBox *m_fontSize = nullptr;
    QCheckBox *m_gapless = nullptr;
    QComboBox *m_replaygain = nullptr;
    QCheckBox *m_showQueue = nullptr;
};


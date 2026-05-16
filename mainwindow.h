#pragma once

#include <QMainWindow>
#include <QTreeView>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <QSlider>
#include <QToolButton>
#include <QKeyEvent>
#include <QFuture>
#include <QFutureWatcher>

#include "playbackengine.h"
#include "filetreemodel.h"
#include "metadatareader.h"
#include "seekslider.h"
#include "coverartdialog.h"
#include "settingsdialog.h"
#include <QSettings>
#include "searchdialog.h"
#include "mprisplayer.h"
#include <QSystemTrayIcon>
#include <QMenu>
#include <QCloseEvent>
#include "toastnotification.h"
#include <QDesktopServices>
#include <QClipboard>
#include <QUrl>
#include "metadatadialog.h"
#include "scrollinglabel.h"
#include "ambientbar.h"
#include <QStandardPaths>
#include <QProcess>
#include "playlistview.h"
#include "nowplayingpanel.h"
#include <QStackedWidget>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onTreeItemActivated(const QModelIndex &index);
    void onTrackChanged(const QString &filePath);
    void onPlayPauseClicked();
    void onNextClicked();
    void onPreviousClicked();
    void onSeekBarMoved(int value);
    void updateSeekBar();
    void onVolumeChanged(int value);
    void onMuteClicked();
    void onCoverClicked();
    void onSettingsClicked();
    void onLoopClicked();
    void onShuffleClicked();
    void updateLoopIcon();
    void updateShuffleIcon();
    void onSearchClicked();
    void buildSearchIndex();
    void onTreeContextMenu(const QPoint &pos);
    void onTitleClicked();
    void onArtistClicked();
    void onCavaClicked();
    void updateButtonStates();
    void onQueueClicked();
    void onViewToggled();
    void refreshPlaylistView(const QString &dirPath);
    void showLoopMenu(QToolButton *sourceBtn);
    void showShuffleMenu(QToolButton *sourceBtn);

private:
    // engine
    PlaybackEngine *m_engine = nullptr;
    FileTreeModel  *m_model  = nullptr;

    // ui
    QTreeView   *m_treeView     = nullptr;
    QLabel      *m_coverLabel   = nullptr;
    // QLabel      *m_titleLabel   = nullptr;
    ScrollingLabel *m_titleLabel = nullptr;
    // QLabel      *m_artistLabel  = nullptr;
    ScrollingLabel *m_artistLabel = nullptr;
    QToolButton *m_prevButton   = nullptr;
    QToolButton *m_playButton   = nullptr;
    QToolButton *m_nextButton   = nullptr;
    QToolButton *m_muteButton  = nullptr;
    QToolButton *m_settingsButton = nullptr;

    SeekSlider *m_seekBar = nullptr;
    QLabel *m_elapsedLabel  = nullptr;
    QLabel *m_remainingLabel = nullptr;
    bool m_showRemaining = false;
    QTimer  *m_seekTimer  = nullptr;
    bool     m_seeking    = false;

    QToolButton *m_loopButton    = nullptr;
    QToolButton *m_shuffleButton = nullptr;
    QToolButton *m_queueButton = nullptr;

    QSystemTrayIcon *m_trayIcon = nullptr;
    QMenu           *m_trayMenu = nullptr;

    AmbientBar *m_ambientBar = nullptr;

    QPixmap m_currentCover;
    QPixmap roundedPixmap(const QPixmap &src, int radius);

    void setupUi();
    void updateNowPlaying(const QString &filePath);

    void setControlsEnabled(bool enabled);

    QSlider *m_volumeSlider  = nullptr;
    QLabel  *m_volumeLabel   = nullptr;
    bool m_muted = false;

    QToolButton *m_searchButton = nullptr;
    QList<SearchResult> m_searchIndex;

    QToolButton *m_cavaButton = nullptr;
    QProcess    *m_cavaProcess = nullptr;

    QFutureWatcher<void> *m_indexWatcher = nullptr;
    bool m_indexReady = false;

    void applySettings();
    static QString configPath() {
        return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
               + "/Redemption/settings.ini";
    }

    void flashLabel(QWidget *label, const QString &text);

    void playFromPath(const QString &path);

    MprisPlayer *m_mpris = nullptr;

    void setupTray();

    PlaylistView    *m_playlistView   = nullptr;
    NowPlayingPanel *m_nowPlayingPanel = nullptr;
    QWidget         *m_playlistModeWidget = nullptr;
    bool             m_playlistMode   = false;
    QToolButton     *m_viewToggleButton = nullptr;
    AmbientBar *m_nowPlayingAmbient = nullptr;
    // QGraphicsOpacityEffect *m_playlistEffect = nullptr;
    QWidget *m_fadeOverlay = nullptr;
    AmbientBar *m_playlistAmbient = nullptr;

    void setupNowPlayingControls();
    QToolButton *m_nowPlayingPlayButton  = nullptr;
    QToolButton *m_nowPlayingMuteButton  = nullptr;
    QSlider     *m_nowPlayingVolume      = nullptr;
    QLabel      *m_nowPlayingVolumeLabel = nullptr;

    QToolButton *m_nowPlayingLoopButton   = nullptr;
    QToolButton *m_nowPlayingShuffleButton = nullptr;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;
};

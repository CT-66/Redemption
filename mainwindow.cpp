#include "mainwindow.h"
#include <QApplication>
#include <QScreen>
#include <QSizePolicy>
#include <QFont>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QSettings>
#include <QFile>
#include <QToolTip>
#include <QHeaderView>
#include <QPainter>
#include <QtConcurrent>
#include <QHoverEvent>
#include <QPainterPath>
#include <QDebug>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_engine = new PlaybackEngine(this);
    m_model  = new FileTreeModel(QDir::homePath() + "/Music", this);

    setupUi();

    m_mpris = new MprisPlayer(m_engine, this);


    QSettings kdeglobals(QDir::homePath() + "/.config/kdeglobals", QSettings::IniFormat);
    QString iconTheme = kdeglobals.value("Icons/Theme", "breeze").toString();
    QString iconBase = "/usr/share/icons/" + iconTheme + "/actions/22/";

    auto themeIcon = [&](const QString &name) {
        QString path = iconBase + name + ".svg";
        if (QFile::exists(path))
            return QIcon(path);
        return QIcon::fromTheme(name);
    };

    connect(m_titleLabel, &ScrollingLabel::clicked,
            this, &MainWindow::onTitleClicked);

    connect(m_artistLabel, &ScrollingLabel::clicked,
            this, &MainWindow::onArtistClicked);

    // m_artistLabel->setCursor(Qt::PointingHandCursor);
    // m_artistLabel->installEventFilter(this);
    // m_artistLabel->setAttribute(Qt::WA_Hover);

    m_prevButton->setIcon(themeIcon("media-skip-backward"));
    m_playButton->setIcon(themeIcon("media-playback-pause"));
    m_nextButton->setIcon(themeIcon("media-skip-forward"));

    connect(m_engine, &PlaybackEngine::trackChanged,
            this, &MainWindow::onTrackChanged);
    connect(m_engine, &PlaybackEngine::pauseStateChanged,
            this, [this](bool paused) {
        m_playButton->setIcon(QIcon::fromTheme(
            paused ? "media-playback-start" : "media-playback-pause"));
        m_mpris->updatePlaybackStatus();
        if (m_nowPlayingPlayButton)
            m_nowPlayingPlayButton->setIcon(QIcon::fromTheme(
                paused ? "media-playback-start" : "media-playback-pause"));
    });
    connect(m_engine, &PlaybackEngine::durationChanged,
            this, [this](double duration) {
        if (m_mpris)
            m_mpris->updateDuration(duration);
    });

    applySettings();
    m_engine->setRootPath(QDir::homePath() + "/Music");
    updateLoopIcon();
    updateShuffleIcon();
    updateButtonStates();
setupNowPlayingControls();
    setupTray();
    // buildSearchIndex();
    // QTimer::singleShot(100, this, [this]() {
    //     QtConcurrent::run([this]() {
    //         buildSearchIndex();
    //     });
    // });
    // QTimer::singleShot(100, this, [this]() {
    //     auto future = QtConcurrent::run([this]() {
    //         buildSearchIndex();
    //     });
    //     Q_UNUSED(future)
    // });
    m_indexWatcher = new QFutureWatcher<void>(this);
    connect(m_indexWatcher, &QFutureWatcher<void>::finished, this, [this]() {
        m_indexReady = true;
    });

    auto future = QtConcurrent::run([this]() {
        buildSearchIndex();
    });
    m_indexWatcher->setFuture(future);

    QTimer::singleShot(0, this, [this]() {
        m_model->startMetadataLoading();
    });
        setControlsEnabled(false);

}

MainWindow::~MainWindow() {}

void MainWindow::setupUi()
{
    setWindowTitle("Redemption");
    resize(900, 600);

    // --- file browser (top) ---
    m_treeView = new QTreeView();
    m_treeView->setModel(m_model);
        m_model->setTreeView(m_treeView);

    connect(m_treeView, &QTreeView::expanded, this, [this](const QModelIndex &idx) {
        emit m_model->dataChanged(idx, idx);
    });
    connect(m_treeView, &QTreeView::collapsed, this, [this](const QModelIndex &idx) {
        emit m_model->dataChanged(idx, idx);
    });

    m_treeView->setHeaderHidden(true);
    m_treeView->setAnimated(true);
    m_treeView->setIndentation(16);

    m_treeView->installEventFilter(this);

    m_treeView->setColumnWidth(0, 300);
    // m_treeView->header()->setStretchLastSection(true);
    m_treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_treeView->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_treeView->header()->resizeSection(1, 50);
    m_treeView->header()->setStretchLastSection(false);
    m_treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_treeView->setAllColumnsShowFocus(true);
    m_treeView->setRootIsDecorated(true);
    m_treeView->setUniformRowHeights(true);
    m_treeView->header()->setMinimumSectionSize(0);
    m_treeView->setStyleSheet("QTreeView::branch { border: none; }");

    connect(m_treeView, &QTreeView::doubleClicked,
            this, &MainWindow::onTreeItemActivated);

    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_treeView, &QTreeView::customContextMenuRequested,
            this, &MainWindow::onTreeContextMenu);

    connect(m_treeView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, [this](const QModelIndex &current) {
        FileNode *node = m_model->nodeFromIndex(current);
        if (!node) return;
        if (node->isDir)
            m_engine->setCurrentDirPath(node->path);
        else
            m_engine->setCurrentDirPath(QFileInfo(node->path).absolutePath());
    });

    // --- cover art ---
    m_coverLabel = new QLabel();
    m_coverLabel->setFixedSize(150, 150);
    m_coverLabel->setAlignment(Qt::AlignCenter);
    // m_coverLabel->setStyleSheet(
    //     "background: palette(mid);"
    //     "border-radius: 4px;"
    // );
    m_coverLabel->setStyleSheet("background: transparent");

    m_coverLabel->setCursor(Qt::PointingHandCursor);
    m_coverLabel->installEventFilter(this);

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(20);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 160));
    m_coverLabel->setGraphicsEffect(shadow);

    // --- title and artist ---
    // m_titleLabel = new QLabel("No track playing");
    // m_titleLabel->setWordWrap(false);
    // QFont titleFont = m_titleLabel->font();
    // titleFont.setPointSize(16);
    // titleFont.setBold(true);
    // m_titleLabel->setFont(titleFont);
    m_titleLabel = new ScrollingLabel();
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setText("No track playing");

    // m_artistLabel = new QLabel("");
    // QFont artistFont = m_artistLabel->font();
    // artistFont.setPointSize(11);
    // artistFont.setBold(true);
    // m_artistLabel->setFont(artistFont);
    // QPalette artistPalette = m_artistLabel->palette();
    // artistPalette.setColor(QPalette::WindowText, QColor(128, 128, 128));
    // m_artistLabel->setPalette(artistPalette);

    m_artistLabel = new ScrollingLabel();
    QFont artistFont = m_artistLabel->font();
    artistFont.setPointSize(11);
    artistFont.setBold(true);
    m_artistLabel->setFont(artistFont);
    QPalette artistPalette = m_artistLabel->palette();
    artistPalette.setColor(QPalette::WindowText, QColor(128, 128, 128));
    m_artistLabel->setPalette(artistPalette);
    m_artistLabel->setText("");

    // --- seekbar ---
    m_seekBar = new SeekSlider(Qt::Horizontal);
    m_seekBar->setRange(0, 1000);
    m_seekBar->setValue(0);
    m_seekBar->setFixedHeight(20);

    m_elapsedLabel = new QLabel("0:00");
    m_elapsedLabel->setFixedWidth(40);
    m_elapsedLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_elapsedLabel->setStyleSheet("color: palette(windowtext); font-size: 10px;");

    m_remainingLabel = new QLabel("0:00");
    m_remainingLabel->setFixedWidth(40);
    m_remainingLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_remainingLabel->setStyleSheet("color: palette(windowtext); font-size: 10px;");
    m_remainingLabel->setCursor(Qt::PointingHandCursor);
    m_remainingLabel->installEventFilter(this);

    connect(m_seekBar, &QSlider::sliderPressed, this, [this]() {
        m_seeking = true;
    });
    connect(m_seekBar, &QSlider::sliderReleased, this, [this]() {
        double duration = 0;
        mpv_get_property(m_engine->mpvHandle(), "duration", MPV_FORMAT_DOUBLE, &duration);
        double seekTo = (m_seekBar->value() / 1000.0) * duration;
        const QString cmd = QString::number(seekTo, 'f', 2);
        QByteArray ba = cmd.toUtf8();
        const char *args[] = {"seek", ba.constData(), "absolute", nullptr};
        mpv_command(m_engine->mpvHandle(), args);
        m_seeking = false;
    });
    connect(m_seekBar, &QSlider::sliderMoved, this, [this](int) {
        double duration = 0;
        mpv_get_property(m_engine->mpvHandle(), "duration", MPV_FORMAT_DOUBLE, &duration);
        double seekTo = (m_seekBar->value() / 1000.0) * duration;
        const QString cmd = QString::number(seekTo, 'f', 2);
        QByteArray ba = cmd.toUtf8();
        const char *args[] = {"seek", ba.constData(), "absolute", nullptr};
        mpv_command(m_engine->mpvHandle(), args);
    });

    m_seekBar->installEventFilter(this);

    m_seekTimer = new QTimer(this);
    m_seekTimer->setInterval(500);
    connect(m_seekTimer, &QTimer::timeout, this, &MainWindow::updateSeekBar);
    m_seekTimer->start();

    // --- buttons ---
    m_prevButton = new QToolButton();
    m_playButton = new QToolButton();
    m_nextButton = new QToolButton();
    m_prevButton->setToolTip("Previous ([)");
    m_playButton->setToolTip("Play/Pause (Space)");
    m_nextButton->setToolTip("Next (])");

    for (QToolButton *btn : {m_prevButton, m_playButton, m_nextButton}) {
        btn->setFixedSize(44, 44);
        btn->setIconSize(QSize(22, 22));
        btn->setAutoRaise(true);
    }

    connect(m_prevButton, &QToolButton::clicked, this, &MainWindow::onPreviousClicked);
    connect(m_playButton, &QToolButton::clicked, this, &MainWindow::onPlayPauseClicked);
    connect(m_nextButton, &QToolButton::clicked, this, &MainWindow::onNextClicked);


    // --- volume ---
    m_volumeSlider = new QSlider(Qt::Horizontal);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(100);
    m_volumeSlider->setFixedWidth(100);
    m_volumeSlider->setFixedHeight(20);

    m_volumeLabel = new QLabel("100%");
    m_volumeLabel->setFixedWidth(40);
    m_volumeLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    connect(m_volumeSlider, &QSlider::valueChanged, this, &MainWindow::onVolumeChanged);
    m_volumeSlider->installEventFilter(this);

    m_muteButton = new QToolButton();
    m_muteButton->setFixedSize(44, 44);
    m_muteButton->setIconSize(QSize(22, 22));
    m_muteButton->setAutoRaise(true);
    m_muteButton->setIcon(QIcon::fromTheme("audio-volume-high"));
    m_muteButton->setToolTip("Mute (Ctrl+M)");
    connect(m_muteButton, &QToolButton::clicked, this, &MainWindow::onMuteClicked);

    // --- settings button ---
    m_settingsButton = new QToolButton();
    m_settingsButton->setFixedSize(44, 44);
    m_settingsButton->setIconSize(QSize(22, 22));
    m_settingsButton->setAutoRaise(true);
    m_settingsButton->setIcon(QIcon::fromTheme("configure"));
    m_settingsButton->setToolTip("Settings");
    connect(m_settingsButton, &QToolButton::clicked, this, &MainWindow::onSettingsClicked);

    // --- loop and shuffle buttons ---
    m_loopButton = new QToolButton();
    m_loopButton->setFixedSize(44, 44);
    m_loopButton->setIconSize(QSize(22, 22));
    m_loopButton->setAutoRaise(true);
    connect(m_loopButton, &QToolButton::clicked, this, &MainWindow::onLoopClicked);

    m_shuffleButton = new QToolButton();
    m_shuffleButton->setFixedSize(44, 44);
    m_shuffleButton->setIconSize(QSize(22, 22));
    m_shuffleButton->setAutoRaise(true);
    connect(m_shuffleButton, &QToolButton::clicked, this, &MainWindow::onShuffleClicked);

/*
m_loopButton->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_loopButton, &QToolButton::customContextMenuRequested,
            this, [this](const QPoint &pos) {
        QMenu menu(this);
                if (!m_loopButton->isEnabled()) return;
        menu.addAction("No loop", this, [this]() {
            m_engine->setLoopMode(LoopMode::None);
            m_engine->restoreQueue();
            updateLoopIcon();
            updateButtonStates();
            m_model->setLoopState((int)LoopMode::None, "");
            m_treeView->viewport()->update();
            ToastNotification::show(this, "Loop off", "media-playlist-repeat");
        });
        menu.addAction("Loop track", this, [this]() {
            m_engine->setShuffleMode(ShuffleMode::Off);
            m_engine->setLoopMode(LoopMode::Track);
                m_engine->setLoopTargetPath(m_engine->currentTrack());
            updateLoopIcon();
            updateShuffleIcon();
            updateButtonStates();
            m_model->setLoopState((int)LoopMode::Track, m_engine->currentTrack());
            m_treeView->viewport()->update();
            ToastNotification::show(this, "Looping track", "media-playlist-repeat-song");
        });
        menu.addAction("Loop directory", this, [this]() {
            m_engine->setShuffleMode(ShuffleMode::Off);
            m_engine->setLoopMode(LoopMode::Directory);
                m_engine->setLoopTargetPath(m_engine->currentDirPath());
            updateLoopIcon();
            updateShuffleIcon();
            updateButtonStates();
            m_model->setLoopState((int)LoopMode::Directory, m_engine->currentDirPath());
            m_treeView->viewport()->update();
            ToastNotification::show(this, "Looping directory", "media-playlist-repeat");
        });
        menu.addAction("Loop queue", this, [this]() {
            m_engine->setShuffleMode(ShuffleMode::Off);
            m_engine->setLoopMode(LoopMode::Queue);
            m_engine->restoreQueue();
            updateLoopIcon();
            updateShuffleIcon();
            updateButtonStates();
            m_model->setLoopState((int)LoopMode::Queue, "");
            m_treeView->viewport()->update();
            ToastNotification::show(this, "Looping queue", "media-playlist-repeat");
        });


        menu.exec(m_loopButton->mapToGlobal(pos));
    });

    m_shuffleButton->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_shuffleButton, &QToolButton::customContextMenuRequested,
            this, [this](const QPoint &pos) {
        QMenu menu(this);
            if (!m_shuffleButton->isEnabled()) return;
        menu.addAction("Shuffle off", this, [this]() {
            m_engine->setShuffleMode(ShuffleMode::Off);
            m_engine->restoreQueue();
            updateShuffleIcon();
            updateButtonStates();
            m_model->setShuffleState((int)ShuffleMode::Off, "");
            m_treeView->viewport()->update();
            ToastNotification::show(this, "Shuffle off", "media-playlist-shuffle");
        });
        menu.addAction("Shuffle directory", this, [this]() {
            m_engine->setLoopMode(LoopMode::None);
            m_engine->setShuffleMode(ShuffleMode::Directory);
                m_engine->setShuffleTargetPath(m_engine->currentDirPath());
            QStringList fullQueue = m_model->collectAudioFiles(
                QDir::homePath() + "/Music");
            QString current = m_engine->currentTrack();
            int idx = fullQueue.indexOf(current);
            m_engine->playFrom(fullQueue, idx >= 0 ? idx : 0);
            updateLoopIcon();
            updateShuffleIcon();
            updateButtonStates();
            m_model->setShuffleState((int)ShuffleMode::Directory,
                m_engine->currentDirPath());
            m_treeView->viewport()->update();
            ToastNotification::show(this, "Shuffling directory",
                "media-playlist-shuffle");
        });
        menu.addAction("Shuffle all", this, [this]() {
            m_engine->setLoopMode(LoopMode::None);
            m_engine->setShuffleMode(ShuffleMode::All);
            QStringList fullQueue = m_model->collectAudioFiles(
                QDir::homePath() + "/Music");
            QString current = m_engine->currentTrack();
            int idx = fullQueue.indexOf(current);
            m_engine->playFrom(fullQueue, idx >= 0 ? idx : 0);
            updateLoopIcon();
            updateShuffleIcon();
            updateButtonStates();
            m_model->setShuffleState((int)ShuffleMode::All, "");
            m_treeView->viewport()->update();
            ToastNotification::show(this, "Shuffling all", "media-playlist-shuffle");
        });
        menu.exec(m_shuffleButton->mapToGlobal(pos));
    });
*/

    m_loopButton->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_loopButton, &QToolButton::customContextMenuRequested,
            this, [this](const QPoint &pos) {
        if (!m_loopButton->isEnabled()) return;
        QMenu menu(this);

        QAction *noLoop = menu.addAction("No loop");
        noLoop->setCheckable(true);
        noLoop->setChecked(m_engine->loopMode() == LoopMode::None);
        connect(noLoop, &QAction::triggered, this, [this]() {
            m_engine->setLoopMode(LoopMode::None);
            m_engine->setLoopTargetPath("");
            m_engine->restoreQueue();
            updateLoopIcon();
            updateButtonStates();
            m_model->setLoopState((int)LoopMode::None, "");
            m_treeView->viewport()->update();
            ToastNotification::show(this, "Loop off", "media-playlist-repeat");
        });

        QAction *loopTrack = menu.addAction("Loop track");
        loopTrack->setCheckable(true);
        loopTrack->setChecked(m_engine->loopMode() == LoopMode::Track);
        connect(loopTrack, &QAction::triggered, this, [this]() {
            m_engine->setShuffleMode(ShuffleMode::Off);
            m_engine->setLoopMode(LoopMode::Track);
            m_engine->setLoopTargetPath(m_engine->currentTrack());
            updateLoopIcon();
            updateShuffleIcon();
            updateButtonStates();
            m_model->setLoopState((int)LoopMode::Track, m_engine->currentTrack());
            m_treeView->viewport()->update();
            ToastNotification::show(this, "Looping track", "media-playlist-repeat-song");
        });

        QAction *loopDir = menu.addAction("Loop directory");
        loopDir->setCheckable(true);
        loopDir->setChecked(m_engine->loopMode() == LoopMode::Directory);
        connect(loopDir, &QAction::triggered, this, [this]() {
            m_engine->setShuffleMode(ShuffleMode::Off);
            m_engine->setLoopMode(LoopMode::Directory);
            m_engine->setLoopTargetPath(m_engine->currentDirPath());
            updateLoopIcon();
            updateShuffleIcon();
            updateButtonStates();
            m_model->setLoopState((int)LoopMode::Directory, m_engine->currentDirPath());
            m_treeView->viewport()->update();
            ToastNotification::show(this, "Looping directory", "media-playlist-repeat");
        });

        QAction *loopQueue = menu.addAction("Loop queue");
        loopQueue->setCheckable(true);
        loopQueue->setChecked(m_engine->loopMode() == LoopMode::Queue);
        connect(loopQueue, &QAction::triggered, this, [this]() {
            m_engine->setShuffleMode(ShuffleMode::Off);
            m_engine->setLoopMode(LoopMode::Queue);
            m_engine->setLoopTargetPath("");
            m_engine->restoreQueue();
            updateLoopIcon();
            updateShuffleIcon();
            updateButtonStates();
            m_model->setLoopState((int)LoopMode::Queue, "");
            m_treeView->viewport()->update();
            ToastNotification::show(this, "Looping queue", "media-playlist-repeat");
        });

        menu.exec(m_loopButton->mapToGlobal(pos));
    });

    m_shuffleButton->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_shuffleButton, &QToolButton::customContextMenuRequested,
            this, [this](const QPoint &pos) {
        if (!m_shuffleButton->isEnabled()) return;
        QMenu menu(this);

        QAction *shuffleOff = menu.addAction("Shuffle off");
        shuffleOff->setCheckable(true);
        shuffleOff->setChecked(m_engine->shuffleMode() == ShuffleMode::Off);
        connect(shuffleOff, &QAction::triggered, this, [this]() {
            m_engine->setShuffleMode(ShuffleMode::Off);
            m_engine->setShuffleTargetPath("");
            m_engine->restoreQueue();
            updateShuffleIcon();
            updateButtonStates();
            m_model->setShuffleState((int)ShuffleMode::Off, "");
            m_treeView->viewport()->update();
            ToastNotification::show(this, "Shuffle off", "media-playlist-shuffle");
        });

        QAction *shuffleDir = menu.addAction("Shuffle directory");
        shuffleDir->setCheckable(true);
        shuffleDir->setChecked(m_engine->shuffleMode() == ShuffleMode::Directory);
        connect(shuffleDir, &QAction::triggered, this, [this]() {
            m_engine->setLoopMode(LoopMode::None);
            m_engine->setLoopTargetPath("");
            m_engine->setShuffleMode(ShuffleMode::Directory);
            m_engine->setShuffleTargetPath(m_engine->currentDirPath());
            QStringList fullQueue = m_model->collectAudioFiles(
                QDir::homePath() + "/Music");
            QString current = m_engine->currentTrack();
            int idx = fullQueue.indexOf(current);
            m_engine->playFrom(fullQueue, idx >= 0 ? idx : 0);
            updateLoopIcon();
            updateShuffleIcon();
            updateButtonStates();
            m_model->setShuffleState((int)ShuffleMode::Directory,
                m_engine->currentDirPath());
            m_treeView->viewport()->update();
            ToastNotification::show(this, "Shuffling directory",
                "media-playlist-shuffle");
        });

        QAction *shuffleAll = menu.addAction("Shuffle all");
        shuffleAll->setCheckable(true);
        shuffleAll->setChecked(m_engine->shuffleMode() == ShuffleMode::All);
        connect(shuffleAll, &QAction::triggered, this, [this]() {
            m_engine->setLoopMode(LoopMode::None);
            m_engine->setLoopTargetPath("");
            m_engine->setShuffleMode(ShuffleMode::All);
            m_engine->setShuffleTargetPath("");
            QStringList fullQueue = m_model->collectAudioFiles(
                QDir::homePath() + "/Music");
            QString current = m_engine->currentTrack();
            int idx = fullQueue.indexOf(current);
            m_engine->playFrom(fullQueue, idx >= 0 ? idx : 0);
            updateLoopIcon();
            updateShuffleIcon();
            updateButtonStates();
            m_model->setShuffleState((int)ShuffleMode::All, "");
            m_treeView->viewport()->update();
            ToastNotification::show(this, "Shuffling all", "media-playlist-shuffle");
        });

        menu.exec(m_shuffleButton->mapToGlobal(pos));
    });


    // -- search button --
    m_searchButton = new QToolButton();
    m_searchButton->setFixedSize(44, 44);
    m_searchButton->setIconSize(QSize(22, 22));
    m_searchButton->setAutoRaise(true);
    m_searchButton->setIcon(QIcon::fromTheme("system-search"));
    m_searchButton->setToolTip("Search (Ctrl+F or /)");
    connect(m_searchButton, &QToolButton::clicked, this, &MainWindow::onSearchClicked);

    // -- visualizer --
    m_cavaButton = new QToolButton();
    m_cavaButton->setIcon(QIcon::fromTheme("view-media-visualization"));
    m_cavaButton->setToolTip("Open Visualizer");
    m_cavaButton->setAutoRaise(true);
    m_cavaButton->setFixedSize(44, 44);
    m_cavaButton->setIconSize(QSize(22, 22));
    connect(m_cavaButton, &QToolButton::clicked, this, &MainWindow::onCavaClicked);

    // -- view queue --
    m_queueButton = new QToolButton();
    m_queueButton->setFixedSize(44, 44);
    m_queueButton->setIconSize(QSize(22, 22));
    m_queueButton->setAutoRaise(true);
    m_queueButton->setIcon(QIcon::fromTheme("view-media-playlist"));
    m_queueButton->setToolTip("View queue");
    connect(m_queueButton, &QToolButton::clicked, this, &MainWindow::onQueueClicked);

    // -- view toggle  --
    m_viewToggleButton = new QToolButton();
    m_viewToggleButton->setFixedSize(44, 44);
    m_viewToggleButton->setIconSize(QSize(22, 22));
    m_viewToggleButton->setAutoRaise(true);
    m_viewToggleButton->setIcon(QIcon::fromTheme("view-fullscreen"));
    m_viewToggleButton->setToolTip("Toggle playlist view");
    connect(m_viewToggleButton, &QToolButton::clicked,
            this, &MainWindow::onViewToggled);

    // --- right section: title/artist + seekbar + buttons ---
    QHBoxLayout *seekRowLayout = new QHBoxLayout();
    seekRowLayout->addWidget(m_elapsedLabel);
    seekRowLayout->addWidget(m_seekBar);
    seekRowLayout->addWidget(m_remainingLabel);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addWidget(m_settingsButton);
    // btnLayout->addWidget(m_searchButton);
    btnLayout->addWidget(m_cavaButton);
    btnLayout->addWidget(m_queueButton);
    btnLayout->addWidget(m_viewToggleButton);
    btnLayout->addStretch();
    btnLayout->addWidget(m_loopButton);
    btnLayout->addWidget(m_prevButton);
    btnLayout->addWidget(m_playButton);
    btnLayout->addWidget(m_nextButton);
    btnLayout->addWidget(m_shuffleButton);
    // btnLayout->addWidget(m_searchButton);
    // btnLayout->addWidget(m_cavaButton);
    btnLayout->addStretch();
    btnLayout->addWidget(m_muteButton);
    btnLayout->addWidget(m_volumeSlider);
    btnLayout->addSpacing(4);
    btnLayout->addWidget(m_volumeLabel);

    QVBoxLayout *rightSection = new QVBoxLayout();
    rightSection->setContentsMargins(0, 8, 8, 8);
    rightSection->setAlignment(Qt::AlignVCenter);
    rightSection->addWidget(m_titleLabel);
    // rightSection->addSpacing(2);
    rightSection->setSpacing(0);
    rightSection->addWidget(m_artistLabel);
    rightSection->addSpacing(6);
        rightSection->addSpacing(15);
    rightSection->addLayout(seekRowLayout);
    rightSection->addSpacing(4);
    rightSection->addLayout(btnLayout);

    // --- bottom bar ---
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->setContentsMargins(8, 8, 8, 8);
    bottomLayout->setAlignment(Qt::AlignVCenter);
    bottomLayout->addWidget(m_coverLabel, 0, Qt::AlignVCenter);
    bottomLayout->addSpacing(12);
    bottomLayout->addLayout(rightSection, 1);

    /*
    QWidget *bottomBar = new QWidget();
    bottomBar->setFixedHeight(175);
    bottomBar->setObjectName("bottomBar");
    bottomBar->setStyleSheet(
        "#bottomBar { border-top: 1px solid palette(mid); }"
    );
    bottomBar->setLayout(bottomLayout);
    */
    m_ambientBar = new AmbientBar();
    m_ambientBar->setFixedHeight(175);
    m_ambientBar->setObjectName("bottomBar");
    m_ambientBar->setLayout(bottomLayout);

    // --- main layout ---
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(m_treeView, 1);
    // mainLayout->addWidget(bottomBar);
    mainLayout->addWidget(m_ambientBar);

    // playlist mode widget (hidden by default)
    m_playlistView = new PlaylistView();
    m_nowPlayingPanel = new NowPlayingPanel();

    // give nowplaying panel the ambient background
    m_nowPlayingPanel->setAutoFillBackground(false);

    m_nowPlayingAmbient = new AmbientBar(m_nowPlayingPanel);
    m_nowPlayingAmbient->setGeometry(m_nowPlayingPanel->rect());
    m_nowPlayingAmbient->lower(); // behind everything
    m_nowPlayingAmbient->setGeometry(m_nowPlayingPanel->rect());
    m_nowPlayingAmbient->updateFromCover(m_currentCover);


    QWidget *central = new QWidget();
    central->setLayout(mainLayout);
    setCentralWidget(central);

        m_playlistModeWidget = new QWidget(central);
    m_playlistModeWidget->setParent(centralWidget());
    m_playlistModeWidget->setObjectName("playlistModeWidget");
    QHBoxLayout *playlistLayout = new QHBoxLayout(m_playlistModeWidget);
    playlistLayout->setContentsMargins(0, 0, 0, 0);
    playlistLayout->setSpacing(0);
    playlistLayout->addWidget(m_playlistView, 35);
    playlistLayout->addWidget(m_nowPlayingPanel, 65);
    //     playlistLayout->addWidget(m_playlistView, 40);
    // playlistLayout->addWidget(m_nowPlayingPanel, 60);

    // m_playlistModeWidget->setAutoFillBackground(true);

// m_playlistEffect = new QGraphicsOpacityEffect(m_playlistModeWidget);
// m_playlistEffect->setOpacity(0.0);
// m_playlistModeWidget->setGraphicsEffect(m_playlistEffect);

    m_fadeOverlay = new QWidget(this);
    m_fadeOverlay->setStyleSheet("background: palette(window);");
    m_fadeOverlay->hide();

    m_playlistModeWidget->hide();

m_playlistAmbient = new AmbientBar(m_playlistView);
m_playlistAmbient->setGeometry(m_playlistView->rect());
m_playlistAmbient->raise();
m_playlistAmbient->lower();
m_playlistAmbient->setDarkOverlay(160);
// m_playlistAmbient->setDarkOverlay(200);
    // m_playlistModeWidget->setGeometry(central->rect());

m_playlistView->installEventFilter(this);
m_playlistView->listWidget()->installEventFilter(this);
m_nowPlayingPanel->installEventFilter(this);

m_playlistView->setMaximumWidth(300);


    // connect playlist view signals
    connect(m_playlistView->switchButton(), &QToolButton::clicked,
            this, &MainWindow::onViewToggled);
    connect(m_playlistView->searchButton(), &QToolButton::clicked,
            this, &MainWindow::onSearchClicked);
    connect(m_playlistView->listWidget(), &QListWidget::itemActivated,
            this, [this](QListWidgetItem *item) {
        QString path = item->data(Qt::UserRole).toString();
        if (!path.isEmpty())
            playFromPath(path);
    });

    m_nowPlayingPanel->installEventFilter(this);
    m_nowPlayingPanel->seekBar()->installEventFilter(this);

    // setCentralWidget(central);

    // prevent focus on buttons when pressing tab
    for (auto *btn : {m_playButton, m_prevButton, m_nextButton,
                      m_searchButton, m_viewToggleButton, m_muteButton,
                      m_loopButton, m_shuffleButton, m_queueButton,
                      m_cavaButton, m_settingsButton}) {
        btn->setFocusPolicy(Qt::NoFocus);
    }

    updateNowPlaying("");
}

void MainWindow::onTreeItemActivated(const QModelIndex &index)
{
    FileNode *node = m_model->nodeFromIndex(index);
    if (!node)
        return;

    if (node->isDir) {
        if (m_treeView->isExpanded(index))
            m_treeView->collapse(index);
        else
            m_treeView->expand(index);
        return;
    }

    // check if clicking outside loop/shuffle directory
    QString clickedDir = QFileInfo(node->path).absolutePath();
    // QString loopDir = m_engine->currentDirPath();
    QString loopDir = m_engine->loopTargetPath();      // was currentDirPath()
    QString shuffleDir = m_engine->shuffleTargetPath(); // separate


    if (m_engine->loopMode() == LoopMode::Directory ||
        m_engine->loopMode() == LoopMode::DirectoryRecursive) {
        bool inside = m_engine->loopMode() == LoopMode::Directory
            ? clickedDir == loopDir
            : node->path.startsWith(loopDir);
        if (!inside) {
            m_engine->setLoopMode(LoopMode::None);
            m_engine->setLoopTargetPath("");
            updateLoopIcon();
            updateButtonStates();
            m_model->setLoopState((int)LoopMode::None, "");
            m_treeView->viewport()->update();
            ToastNotification::show(this, "Loop removed", "media-playlist-repeat");
        }
    }

    if (m_engine->shuffleMode() == ShuffleMode::Directory) {
        if (clickedDir != loopDir) {
            m_engine->setShuffleMode(ShuffleMode::Off);
            m_engine->setShuffleTargetPath("");
            updateShuffleIcon();
            updateButtonStates();
            m_model->setShuffleState((int)ShuffleMode::Off, "");
            m_treeView->viewport()->update();
            ToastNotification::show(this, "Shuffle removed", "media-playlist-shuffle");
        }
    }

    // set current dir BEFORE building queue so shuffle/loop dir works
    m_engine->setCurrentDirPath(QFileInfo(node->path).absolutePath());
    m_engine->setManualChange(true);

    QStringList fullQueue = m_model->collectAudioFiles(QDir::homePath() + "/Music");
    int startIndex = fullQueue.indexOf(node->path);
    if (startIndex < 0)
        startIndex = 0;


    m_engine->playFrom(fullQueue, startIndex);
}


void MainWindow::onTrackChanged(const QString &filePath)
{
    m_engine->setCurrentDirPath(QFileInfo(filePath).absolutePath());

    setControlsEnabled(true);
    updateNowPlaying(filePath);
    m_mpris->updateMetadata(
            m_titleLabel->text(),
            m_artistLabel->text(),
            filePath,
            m_currentCover
        );
    m_playButton->setIcon(QIcon::fromTheme("media-playback-pause"));

    // QModelIndex idx = m_model->indexForPath(filePath);
    // if (idx.isValid()) {
    //     m_treeView->scrollTo(idx, QAbstractItemView::PositionAtCenter);
    //     m_treeView->selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    //     m_treeView->expand(idx.parent());
    // }
    QModelIndex idx = m_model->indexForPath(filePath);
    if (idx.isValid()) {
        m_treeView->scrollTo(idx, QAbstractItemView::PositionAtCenter);
        m_treeView->setCurrentIndex(idx);
        m_treeView->selectionModel()->select(idx,
            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        m_treeView->expand(idx.parent());
    }

    m_trayIcon->setToolTip("Redemption\n" +
        m_titleLabel->text() + " — " + m_artistLabel->text());

    if (m_engine->loopMode() == LoopMode::Track)
        m_model->setLoopState((int)LoopMode::Track, filePath);

    if (m_engine->loopMode() == LoopMode::Track && m_engine->wasManualChange()) {
        m_engine->setLoopMode(LoopMode::None);
        updateLoopIcon();
        updateButtonStates();
        m_model->setLoopState((int)LoopMode::None, "");
        m_treeView->viewport()->update();
        ToastNotification::show(this, "Loop removed", "media-playlist-repeat");
    }
    m_engine->clearManualChange();

    m_treeView->viewport()->update();

    // playlist mode stuff
    if (m_playlistMode) {
        QString newDir = QFileInfo(filePath).absolutePath();
        if (newDir != m_playlistView->currentDir())
            refreshPlaylistView(newDir);
        else
            m_playlistView->updateCurrentTrack(filePath);

        m_nowPlayingPanel->setTitle(m_titleLabel->text());
        m_nowPlayingPanel->setArtist(m_artistLabel->text());
        m_nowPlayingPanel->setCover(m_currentCover);
        m_nowPlayingAmbient->updateFromCover(m_currentCover);
        m_playlistAmbient->updateFromCover(m_currentCover);
    }
}

void MainWindow::updateNowPlaying(const QString &filePath)
{
    if (filePath.isEmpty()) {
        m_titleLabel->setText("No track playing");
        m_artistLabel->setText("");
        /*
        QPixmap fallback = QIcon::fromTheme("media-album-cover").pixmap(m_coverLabel->size());
        // QPixmap fallback = QIcon::fromTheme("media-album-cover").pixmap(QSize(150, 150));
        // m_coverLabel->setPixmap(fallback.pixmap(m_coverLabel->size()));
        // m_coverLabel->setStyleSheet("background: palette(mid); border-radius: 4px;");
         m_coverLabel->setPixmap(roundedPixmap(fallback, 12));
        m_coverLabel->setStyleSheet("background: transparent;");
        m_ambientBar->updateFromCover(QPixmap());
        */

        // QPixmap fallback(150, 150);
        // fallback.fill(Qt::transparent);
        // QPainter painter(&fallback);
        // QIcon::fromTheme("media-album-cover").paint(&painter, 0, 0, 150, 150);
        // painter.end();
        // m_coverLabel->setPixmap(roundedPixmap(fallback, 12));
        // return;

         QPixmap fallbackBlur(":/images/fallback.jpg");
          qDebug() << "fallbackBlur loaded:" << !fallbackBlur.isNull()
                 << "size:" << fallbackBlur.size();
        if (!fallbackBlur.isNull())
            m_ambientBar->updateFromCover(fallbackBlur);
        else
            m_ambientBar->updateFromCover(QPixmap());

        QPixmap fallback(150, 150);
        fallback.fill(Qt::transparent);
        QPainter painter(&fallback);
        painter.setRenderHint(QPainter::Antialiasing);
        // draw rounded background
        painter.setBrush(palette().mid());
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(0, 0, 150, 150, 12, 12);
        // draw icon centered
        // QIcon::fromTheme("media-album-cover").paint(&painter, 25, 25, 100, 100);
        // QIcon::fromTheme("audio-x-generic").paint(&painter, 25, 25, 100, 100);
        QIcon::fromTheme("library-music-symbolic").paint(&painter, 25, 25, 100, 100);
        painter.end();
        m_coverLabel->setPixmap(roundedPixmap(fallback, 12));
        m_ambientBar->updateFromCover(QPixmap());
        return;
    }

    TrackMetadata meta = MetadataReader::read(filePath);

    if (meta.hasTitle)
        m_titleLabel->setText(meta.title);
    else
        m_titleLabel->setText(QFileInfo(filePath).fileName());

    if (meta.hasArtist)
        m_artistLabel->setText(meta.artist);
    else
        m_artistLabel->setText("");

    if (meta.hasCover) {
        m_currentCover = meta.cover;
        QPixmap scaled = meta.cover.scaled(
            m_coverLabel->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );
        // m_coverLabel->setPixmap(scaled);
        m_coverLabel->setPixmap(roundedPixmap(scaled, 12));
        m_ambientBar->updateFromCover(meta.cover);
    // } else {
    //     m_currentCover = QPixmap();
    //     QIcon fallback = QIcon::fromTheme("media-album-cover");
    //     if (fallback.isNull())
    //         fallback = QIcon::fromTheme("audio-x-generic");
    //     m_coverLabel->setPixmap(fallback.pixmap(m_coverLabel->size()));
    //     m_coverLabel->setStyleSheet("background: palette(mid); border-radius: 4px;");
    //     m_ambientBar->updateFromCover(QPixmap());
    // }
    // } else {
    //     QPixmap fallback = QIcon::fromTheme("media-album-cover")
    //         .pixmap(m_coverLabel->size());
    //     if (fallback.isNull())
    //         fallback = QIcon::fromTheme("audio-x-generic")
    //             .pixmap(m_coverLabel->size());
    //     m_coverLabel->setPixmap(roundedPixmap(fallback, 12));
    //     m_ambientBar->updateFromCover(QPixmap());
    // }
    } else {
        m_currentCover = QPixmap();
        // QPixmap fallback(150, 150);
        // fallback.fill(Qt::transparent);
        // QPainter painter(&fallback);
        // QIcon::fromTheme("media-album-cover").paint(&painter, 0, 0, 150, 150);
        // painter.end();
        // m_coverLabel->setPixmap(roundedPixmap(fallback, 12));
        // m_ambientBar->updateFromCover(QPixmap());

// use fallback image for blur if available
QPixmap fallbackBlur(":/images/fallback.jpg");
if (!fallbackBlur.isNull())
    m_ambientBar->updateFromCover(fallbackBlur);
else
    m_ambientBar->updateFromCover(QPixmap());

if (m_playlistMode) {
    if (!fallbackBlur.isNull())
        m_nowPlayingAmbient->updateFromCover(fallbackBlur);
    else
        m_nowPlayingAmbient->updateFromCover(QPixmap());
    m_nowPlayingPanel->setCover(QPixmap());
}

        QPixmap fallback(150, 150);
        fallback.fill(Qt::transparent);
        QPainter painter(&fallback);
        painter.setRenderHint(QPainter::Antialiasing);
        // draw rounded background
        painter.setBrush(palette().mid());
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(0, 0, 150, 150, 12, 12);
        // draw icon centered
        // QIcon::fromTheme("media-album-cover").paint(&painter, 25, 25, 100, 100);
        // QIcon::fromTheme("audio-x-generic").paint(&painter, 25, 25, 100, 100);
        QIcon::fromTheme("library-music-symbolic").paint(&painter, 25, 25, 100, 100);
        painter.end();
        m_coverLabel->setPixmap(roundedPixmap(fallback, 12));
        // m_ambientBar->updateFromCover(QPixmap());
    }

}

void MainWindow::onPlayPauseClicked()
{
    if (m_engine->isPlaying()) {
        m_engine->pause();
        m_playButton->setIcon(QIcon::fromTheme("media-playback-start"));
    } else {
        m_engine->resume();
        m_playButton->setIcon(QIcon::fromTheme("media-playback-pause"));
    }
    m_mpris->updatePlaybackStatus();
}

void MainWindow::onNextClicked()
{
    m_engine->next();
}

void MainWindow::onPreviousClicked()
{
    m_engine->previous();
}

void MainWindow::updateSeekBar()
{
    if (m_seeking)
        return;

    double pos = 0, duration = 0;
    mpv_get_property(m_engine->mpvHandle(), "time-pos", MPV_FORMAT_DOUBLE, &pos);
    mpv_get_property(m_engine->mpvHandle(), "duration", MPV_FORMAT_DOUBLE, &duration);

    if (duration > 0) {
        m_seekBar->setDuration(duration);
        m_seekBar->setValue(static_cast<int>((pos / duration) * 1000));

        int p = static_cast<int>(pos);
        int d = static_cast<int>(duration);

        m_elapsedLabel->setText(QString("%1:%2")
            .arg(p / 60).arg(p % 60, 2, 10, QChar('0')));

        if (m_showRemaining) {
            int remaining = d - p;
            m_remainingLabel->setText(QString("-%1:%2")
                .arg(remaining / 60).arg(remaining % 60, 2, 10, QChar('0')));
        } else {
            m_remainingLabel->setText(QString("%1:%2")
                .arg(d / 60).arg(d % 60, 2, 10, QChar('0')));
        }

        // m_nowPlayingPanel->seekBar()->setValue(val);
        // m_nowPlayingPanel->seekBar()->setDuration(duration);
        // m_nowPlayingPanel->elapsedLabel()->setText(elapsed);
        // m_nowPlayingPanel->remainingLabel()->setText(remaining);
        int val = static_cast<int>((pos / duration) * 1000);
        m_nowPlayingPanel->seekBar()->setValue(val);
        m_nowPlayingPanel->seekBar()->setDuration(duration);
        m_nowPlayingPanel->elapsedLabel()->setText(QString("%1:%2")
            .arg(p / 60).arg(p % 60, 2, 10, QChar('0')));
        m_nowPlayingPanel->remainingLabel()->setText(
            m_showRemaining
            ? QString("-%1:%2").arg((d-p)/60).arg((d-p)%60, 2, 10, QChar('0'))
            : QString("%1:%2").arg(d/60).arg(d%60, 2, 10, QChar('0')));
    }

}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{

    if (event->type() == QEvent::MouseButtonPress && obj == m_coverLabel) {
        if (!m_currentCover.isNull())
            onCoverClicked();
        return true;
    }

    // if (event->type() == QEvent::MouseButtonPress && obj == m_remainingLabel) {
    //     m_showRemaining = !m_showRemaining;
    //     updateSeekBar();
    //     return true;
    // }
    if (event->type() == QEvent::MouseButtonPress && obj == m_remainingLabel) {
        m_showRemaining = !m_showRemaining;
        QSettings settings(configPath(), QSettings::IniFormat);
        settings.setValue("playback/showRemaining", m_showRemaining);
        updateSeekBar();

        // highlight flash
        QPalette highlight = m_remainingLabel->palette();
        highlight.setColor(QPalette::WindowText,
            m_remainingLabel->palette().color(QPalette::Highlight));
        m_remainingLabel->setPalette(highlight);
        QTimer::singleShot(200, this, [this]() {
            m_remainingLabel->setPalette(QPalette());
        });

        return true;
    }
    if (event->type() == QEvent::MouseButtonRelease && obj == m_volumeSlider) {
        m_treeView->setFocus();
        return false;
    }
    if (event->type() == QEvent::MouseButtonPress && obj == m_volumeSlider) {
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        int val = QStyle::sliderValueFromPosition(
            m_volumeSlider->minimum(), m_volumeSlider->maximum(),
            me->pos().x(), m_volumeSlider->width());
        m_volumeSlider->setValue(val);
        onVolumeChanged(val);
        // m_treeView->setFocus();
        return false;
    }
    if (event->type() == QEvent::MouseButtonRelease && obj == m_seekBar) {
        m_treeView->setFocus();
        return false;
    }
    if (event->type() == QEvent::MouseButtonPress && obj == m_nowPlayingVolume) {
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        int val = QStyle::sliderValueFromPosition(
            m_nowPlayingVolume->minimum(), m_nowPlayingVolume->maximum(),
            me->pos().x(), m_nowPlayingVolume->width());
        m_nowPlayingVolume->setValue(val);
        onVolumeChanged(val);
        return false;
    }
    if (obj == m_artistLabel) {
        QFontMetrics fm(m_artistLabel->font());
        int textWidth = fm.horizontalAdvance(m_artistLabel->text());

        if (event->type() == QEvent::HoverMove) {
            QHoverEvent *he = static_cast<QHoverEvent*>(event);
            m_artistLabel->setCursor(he->position().x() <= textWidth
                ? Qt::PointingHandCursor
                : Qt::ArrowCursor);
            return false;
        }
        if (event->type() == QEvent::HoverLeave) {
            m_artistLabel->setCursor(Qt::ArrowCursor);
            return false;
        }
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *me = static_cast<QMouseEvent*>(event);
            if (me->position().x() <= textWidth)
                onArtistClicked();
            return true;
        }
    }
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *key = static_cast<QKeyEvent*>(event);
        if (key->key() == Qt::Key_F && key->modifiers() & Qt::ControlModifier) {
            onSearchClicked();
            return true;
        }
        if (key->key() == Qt::Key_Q && key->modifiers() & Qt::ControlModifier) {
            qApp->quit();
            return true;
        }
        if (key->key() == Qt::Key_M && key->modifiers() & Qt::ControlModifier) {
            onMuteClicked();
            return true;
        }
        if (key->key() == Qt::Key_BracketLeft && key->modifiers() & Qt::ControlModifier) {
            int vol = qMax(0, m_volumeSlider->value() - 5);
            m_volumeSlider->setValue(vol);
            onVolumeChanged(vol);
            return true;
        }
        if (key->key() == Qt::Key_BracketRight && key->modifiers() & Qt::ControlModifier) {
            int vol = qMin(100, m_volumeSlider->value() + 5);
            m_volumeSlider->setValue(vol);
            onVolumeChanged(vol);
            return true;
        }
        if (key->key() == Qt::Key_J && key->modifiers() & Qt::ControlModifier) {
            QModelIndex cur = m_treeView->currentIndex();
            QModelIndex next = m_treeView->indexBelow(cur);
            if (next.isValid()) {
                m_treeView->setCurrentIndex(next);
                m_treeView->scrollTo(next);
            }
            return true;
        }
        if (key->key() == Qt::Key_K && key->modifiers() & Qt::ControlModifier) {
            QModelIndex cur = m_treeView->currentIndex();
            QModelIndex prev = m_treeView->indexAbove(cur);
            if (prev.isValid()) {
                m_treeView->setCurrentIndex(prev);
                m_treeView->scrollTo(prev);
            }
            return true;
        }

        if ((key->key() == Qt::Key_I || key->key() == Qt::Key_V)
            && key->modifiers() & Qt::ControlModifier) {
            onViewToggled();
            return true;
        }

        switch (key->key()) {
        case Qt::Key_Space:
            onPlayPauseClicked();
            return true;
        case Qt::Key_Return:
        case Qt::Key_Enter: {
            QModelIndex idx = m_treeView->currentIndex();
            if (idx.isValid())
                onTreeItemActivated(idx);
            return true;
        }
        case Qt::Key_Left: {
            if (key->modifiers() & Qt::ShiftModifier) {
                double pos = 0;
                mpv_get_property(m_engine->mpvHandle(), "time-pos", MPV_FORMAT_DOUBLE, &pos);
                pos = qMax(0.0, pos - 5.0);
                const QString cmd = QString::number(pos, 'f', 2);
                QByteArray ba = cmd.toUtf8();
                const char *args[] = {"seek", ba.constData(), "absolute", nullptr};
                mpv_command(m_engine->mpvHandle(), args);
                return true;
            }
            return false;
        }
        case Qt::Key_Right: {
            if (key->modifiers() & Qt::ShiftModifier) {
                double pos = 0, duration = 0;
                mpv_get_property(m_engine->mpvHandle(), "time-pos", MPV_FORMAT_DOUBLE, &pos);
                mpv_get_property(m_engine->mpvHandle(), "duration", MPV_FORMAT_DOUBLE, &duration);
                pos = qMin(duration, pos + 5.0);
                const QString cmd = QString::number(pos, 'f', 2);
                QByteArray ba = cmd.toUtf8();
                const char *args[] = {"seek", ba.constData(), "absolute", nullptr};
                mpv_command(m_engine->mpvHandle(), args);
                return true;
            }
            return false;
        }
        case Qt::Key_BracketLeft:
            m_engine->previous();
            return true;
        case Qt::Key_BracketRight:
            m_engine->next();
            return true;

        case Qt::Key_Slash:
            onSearchClicked();
            return true;
        default:
            break;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::setControlsEnabled(bool enabled)
{
    m_prevButton->setEnabled(enabled);
    m_playButton->setEnabled(enabled);
    m_nextButton->setEnabled(enabled);
    m_seekBar->setEnabled(enabled);
    m_loopButton->setEnabled(enabled);
    m_shuffleButton->setEnabled(enabled);
    m_cavaButton->setEnabled(enabled);
    // m_queueButton->setEnabled(enabled);
    if (enabled) updateButtonStates();
    else m_queueButton->setEnabled(false);
}

void MainWindow::onSeekBarMoved(int value)
{
    Q_UNUSED(value)
}


void MainWindow::onVolumeChanged(int value)
{
    m_volumeLabel->setText(QString("%1%").arg(value));
    double vol = static_cast<double>(value);
    mpv_set_property(m_engine->mpvHandle(), "volume", MPV_FORMAT_DOUBLE, &vol);
}

void MainWindow::onMuteClicked()
{
    m_muted = !m_muted;
    int mute = m_muted ? 1 : 0;
    mpv_set_property(m_engine->mpvHandle(), "mute", MPV_FORMAT_FLAG, &mute);
    m_muteButton->setIcon(QIcon::fromTheme(
        m_muted ? "audio-volume-muted" : "audio-volume-high"));
    ToastNotification::show(this, m_muted ? "Muted" : "Unmuted",
    m_muted ? "audio-volume-muted" : "audio-volume-high");
    m_muteButton->setToolTip(m_muted ? "Unmute (Ctrl+M)" : "Mute (Ctrl+M)");

    if (m_nowPlayingMuteButton)
        m_nowPlayingMuteButton->setIcon(QIcon::fromTheme(
            m_muted ? "audio-volume-muted" : "audio-volume-high"));
}


void MainWindow::onCoverClicked()
{
    if (m_currentCover.isNull())
        return;

    CoverArtDialog dialog(m_currentCover, this);
    dialog.exec();
}

void MainWindow::applySettings()
{
    QSettings settings(configPath(), QSettings::IniFormat);

    bool cleanTree = settings.value("treeview/clean", true).toBool();
    if (cleanTree)
        m_treeView->setStyleSheet("QTreeView::branch { border: none; }");
    else
        m_treeView->setStyleSheet("");

    bool singleClick = settings.value("treeview/singleclick", false).toBool();
    if (singleClick) {
        connect(m_treeView, &QTreeView::clicked,
                this, &MainWindow::onTreeItemActivated,
                Qt::UniqueConnection);
    } else {
        disconnect(m_treeView, &QTreeView::clicked,
                   this, &MainWindow::onTreeItemActivated);
    }

    bool gapless = settings.value("playback/gapless", true).toBool();
    mpv_set_option_string(m_engine->mpvHandle(),
        "gapless-audio", gapless ? "yes" : "no");

    QString replaygain = settings.value("playback/replaygain", "track").toString();
    mpv_set_option_string(m_engine->mpvHandle(),
        "replaygain", replaygain.toUtf8().constData());

    bool showQueue = settings.value("ui/showQueue", true).toBool();
    m_queueButton->setVisible(showQueue);

    bool ambient = settings.value("ui/ambientMode", true).toBool();
    m_ambientBar->setAmbientEnabled(ambient);

    QFont font = QFont(settings.value("font/family",
    QApplication::font().family()).toString());
    font.setPointSize(settings.value("font/size",
    QApplication::font().pointSize()).toInt());
    QApplication::setFont(font);

    m_showRemaining = settings.value("playback/showRemaining", false).toBool();
}

void MainWindow::onSettingsClicked()
{
    SettingsDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QSettings settings(configPath(), QSettings::IniFormat);
        settings.setValue("treeview/clean", dialog.cleanTree());
        settings.setValue("treeview/singleclick", dialog.singleClickExpand());
        settings.setValue("playback/gapless", dialog.gapless());
        settings.setValue("playback/replaygain", dialog.replaygain());
        settings.setValue("ui/showQueue", dialog.showQueue());
        settings.setValue("ui/ambientMode", dialog.ambientMode());
        settings.setValue("font/family", dialog.selectedFont().family());
        settings.setValue("font/size", dialog.selectedFontSize());
        applySettings();
    }
}

void MainWindow::updateLoopIcon()
{
    QString iconName;
    QString tooltip;

    switch (m_engine->loopMode()) {
    case LoopMode::None:
        iconName = "media-playlist-repeat";
        tooltip = "No loop";
        break;
    case LoopMode::Track:
        iconName = "media-playlist-repeat-song";
        tooltip = "Loop track";
        break;
    case LoopMode::Directory:
        iconName = "media-playlist-repeat";
        tooltip = "Loop directory";
        break;
    case LoopMode::DirectoryRecursive:
        iconName = "media-playlist-repeat";
        tooltip = "Loop directory (recursive)";
        break;
    case LoopMode::Queue:
        iconName = "media-playlist-repeat";
        tooltip = "Loop queue";
        break;
    }

    QIcon icon = QIcon::fromTheme(iconName);
    if (m_engine->loopMode() == LoopMode::None) {
        QPixmap px = icon.pixmap(QSize(22, 22));
        QPixmap dimmed(QSize(22, 22));
        dimmed.fill(Qt::transparent);
        QPainter p(&dimmed);
        p.setOpacity(0.3);
        p.drawPixmap(0, 0, px);
        p.end();
        m_loopButton->setIcon(QIcon(dimmed));
    } else {
        m_loopButton->setIcon(icon);
    }
    m_loopButton->setToolTip(tooltip);

    if (m_nowPlayingLoopButton) {
        m_nowPlayingLoopButton->setIcon(m_loopButton->icon());
        m_nowPlayingLoopButton->setToolTip(tooltip);
    }

}

void MainWindow::updateShuffleIcon()
{
    QString tooltip;

    switch (m_engine->shuffleMode()) {
    case ShuffleMode::Off:
        tooltip = "Shuffle off";
        break;
    case ShuffleMode::Directory:
        tooltip = "Shuffle directory";
        break;
    case ShuffleMode::All:
        tooltip = "Shuffle all";
        break;
    }

    QIcon icon = QIcon::fromTheme("media-playlist-shuffle");
    if (m_engine->shuffleMode() == ShuffleMode::Off) {
        QPixmap px = icon.pixmap(QSize(22, 22));
        QPixmap dimmed(QSize(22, 22));
        dimmed.fill(Qt::transparent);
        QPainter p(&dimmed);
        p.setOpacity(0.3);
        p.drawPixmap(0, 0, px);
        p.end();
        m_shuffleButton->setIcon(QIcon(dimmed));
    } else {
        m_shuffleButton->setIcon(icon);
    }
    m_shuffleButton->setToolTip(tooltip);

    if (m_nowPlayingShuffleButton) {
        m_nowPlayingShuffleButton->setIcon(m_shuffleButton->icon());
        m_nowPlayingShuffleButton->setToolTip(tooltip);
    }
}

void MainWindow::onLoopClicked()
{
    switch (m_engine->loopMode()) {
    case LoopMode::None:
        m_engine->setLoopMode(LoopMode::Track);
        m_engine->setLoopTargetPath(m_engine->currentTrack());
        if (m_engine->shuffleMode() != ShuffleMode::Off) {
            m_engine->setShuffleMode(ShuffleMode::Off);
            m_engine->restoreQueue();
            updateShuffleIcon();
            ToastNotification::show(this, "Shuffle disabled", "media-playlist-shuffle");
        }
        ToastNotification::show(this, "Looping track", "media-playlist-repeat-song");
        break;
    case LoopMode::Track:
        m_engine->setLoopMode(LoopMode::Directory);
        m_engine->setLoopTargetPath(m_engine->currentDirPath());
        ToastNotification::show(this, "Looping directory", "media-playlist-repeat");
        break;
    case LoopMode::Directory:
        m_engine->setLoopMode(LoopMode::Queue);
        m_engine->setLoopTargetPath("");
        m_engine->restoreQueue();
        ToastNotification::show(this, "Looping queue", "media-playlist-repeat");
        break;
    case LoopMode::DirectoryRecursive:
        m_engine->setLoopMode(LoopMode::Queue);
        m_engine->setLoopTargetPath("");
        m_engine->restoreQueue();
        ToastNotification::show(this, "Looping queue", "media-playlist-repeat");
        break;
    case LoopMode::Queue:
        m_engine->setLoopMode(LoopMode::None);
        m_engine->setLoopTargetPath("");
        ToastNotification::show(this, "Loop off", "media-playlist-repeat");
        break;
    }

    updateLoopIcon();
    m_model->setLoopState((int)m_engine->loopMode(),
        m_engine->loopMode() == LoopMode::Track
            ? m_engine->currentTrack()
            : m_engine->loopTargetPath());
    m_treeView->viewport()->update();
    updateButtonStates();
}

void MainWindow::onShuffleClicked()
{
    switch (m_engine->shuffleMode()) {
    case ShuffleMode::Off:
        if (m_engine->loopMode() != LoopMode::None) {
            m_engine->setLoopMode(LoopMode::None);
            m_engine->setLoopTargetPath("");
            updateLoopIcon();
            ToastNotification::show(this, "Loop disabled", "media-playlist-repeat");
        }
        m_engine->setShuffleMode(ShuffleMode::Directory);
        m_engine->setShuffleTargetPath(m_engine->currentDirPath());
        {
            QStringList fullQueue = m_model->collectAudioFiles(
                QDir::homePath() + "/Music");
            QString current = m_engine->currentTrack();
            int idx = fullQueue.indexOf(current);
            m_engine->playFrom(fullQueue, idx >= 0 ? idx : 0);
        }
        ToastNotification::show(this, "Shuffling directory", "media-playlist-shuffle");
        break;
    case ShuffleMode::Directory:
        m_engine->setShuffleMode(ShuffleMode::All);
        m_engine->setShuffleTargetPath("");
        {
            QStringList fullQueue = m_model->collectAudioFiles(
                QDir::homePath() + "/Music");
            QString current = m_engine->currentTrack();
            int idx = fullQueue.indexOf(current);
            m_engine->playFrom(fullQueue, idx >= 0 ? idx : 0);
        }
        ToastNotification::show(this, "Shuffling all", "media-playlist-shuffle");
        break;
    case ShuffleMode::All:
        m_engine->setShuffleMode(ShuffleMode::Off);
        m_engine->setShuffleTargetPath("");
        m_engine->restoreQueue();
        ToastNotification::show(this, "Shuffle off", "media-playlist-shuffle");
        break;
    }

    updateShuffleIcon();
    m_model->setShuffleState((int)m_engine->shuffleMode(),
        m_engine->shuffleTargetPath());
    m_treeView->viewport()->update();
    updateButtonStates();
}


void MainWindow::buildSearchIndex()
{
    m_searchIndex.clear();
    QString root = QDir::homePath() + "/Music";
    QStringList files = m_model->collectAudioFiles(root);
    for (const QString &path : files) {
        TrackMetadata meta = MetadataReader::read(path);
        SearchResult r;
        r.path = path;
        r.displayName = meta.hasTitle ? meta.title : QFileInfo(path).fileName();
        r.artist = meta.hasArtist ? meta.artist : QString();

        // relative path from ~/Music
        QString relative = path;
        if (relative.startsWith(root + "/"))
            relative = relative.mid(root.length() + 1);
        r.relativePath = relative;

        m_searchIndex.append(r);
    }
}

// void MainWindow::onSearchClicked()
// {
//     if (m_searchIndex.isEmpty())
//         buildSearchIndex();

//     SearchDialog dialog(m_searchIndex, this);
//     if (dialog.exec() == QDialog::Accepted) {
//         QString path = dialog.selectedPath();
//         if (path.isEmpty())
//             return;

//         m_engine->setCurrentDirPath(QFileInfo(path).absolutePath());
//         QStringList fullQueue = m_model->collectAudioFiles(QDir::homePath() + "/Music");
//         int startIndex = fullQueue.indexOf(path);
//         if (startIndex < 0) startIndex = 0;
//         m_engine->playFrom(fullQueue, startIndex);

//         QModelIndex treeIdx = m_model->indexForPath(path);
//         if (treeIdx.isValid()) {
//             m_treeView->setCurrentIndex(treeIdx);
//             m_treeView->scrollTo(treeIdx, QAbstractItemView::PositionAtCenter);
//             m_treeView->expand(treeIdx.parent());
//             m_treeView->selectionModel()->select(treeIdx,
//                 QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
//         }
//     }
// }
// void MainWindow::onSearchClicked()
// {
//     SearchDialog dialog(m_searchIndex, this, !m_indexReady);
//     if (dialog.exec() == QDialog::Accepted)
//         playFromPath(dialog.selectedPath());
//     m_treeView->setFocus();
//     // clear focus from all buttons
//     m_playButton->clearFocus();
//     m_prevButton->clearFocus();
//     m_nextButton->clearFocus();
//     m_searchButton->clearFocus();
//     m_viewToggleButton->clearFocus();
// }
void MainWindow::onSearchClicked()
{
    SearchDialog dialog(m_searchIndex, this, !m_indexReady);
    if (dialog.exec() == QDialog::Accepted) {
        QString path = dialog.selectedPath();
        QString clickedDir = QFileInfo(path).absolutePath();

        // reset loop directory if outside
        if ((m_engine->loopMode() == LoopMode::Directory ||
             m_engine->loopMode() == LoopMode::DirectoryRecursive) &&
            !clickedDir.startsWith(m_engine->loopTargetPath())) {
            m_engine->setLoopMode(LoopMode::None);
            m_engine->setLoopTargetPath("");
            updateLoopIcon();
            updateButtonStates();
            m_model->setLoopState((int)LoopMode::None, "");
            m_treeView->viewport()->update();
    ToastNotification::show(this, "Loop removed", "media-playlist-repeat");
        }

        // reset shuffle directory if outside
        if (m_engine->shuffleMode() == ShuffleMode::Directory &&
            clickedDir != m_engine->shuffleTargetPath()) {
            m_engine->setShuffleMode(ShuffleMode::Off);
            m_engine->setShuffleTargetPath("");
            m_engine->restoreQueue();
            updateShuffleIcon();
            updateButtonStates();
            m_model->setShuffleState((int)ShuffleMode::Off, "");
            m_treeView->viewport()->update();
    ToastNotification::show(this, "Shuffle removed", "media-playlist-shuffle");
        }

        playFromPath(path);
    }
    m_treeView->setFocus();
    // clear focus from all buttons
    m_playButton->clearFocus();
    m_prevButton->clearFocus();
    m_nextButton->clearFocus();
    m_searchButton->clearFocus();
    m_viewToggleButton->clearFocus();
}

void MainWindow::setupTray()
{
    m_trayMenu = new QMenu(this);
    QAction *showAction = m_trayMenu->addAction(
        QIcon::fromTheme("window-restore-pip"), "Show/Hide");
    m_trayMenu->addSeparator();
    QAction *playAction = m_trayMenu->addAction(
        QIcon::fromTheme("media-playback-start"),"Play/Pause");
    QAction *nextAction = m_trayMenu->addAction(
         QIcon::fromTheme("media-skip-forward"), "Next");
    QAction *prevAction = m_trayMenu->addAction(
        QIcon::fromTheme("media-skip-backward"), "Previous");
    m_trayMenu->addSeparator();
    QAction *quitAction = m_trayMenu->addAction(
        QIcon::fromTheme("application-exit"), "Quit");

    connect(showAction, &QAction::triggered, this, [this]() {
        if (isVisible()) hide();
        else { show(); raise(); activateWindow(); }
    });
    connect(playAction, &QAction::triggered, this, &MainWindow::onPlayPauseClicked);
    connect(nextAction, &QAction::triggered, this, &MainWindow::onNextClicked);
    connect(prevAction, &QAction::triggered, this, &MainWindow::onPreviousClicked);
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    m_trayIcon = new QSystemTrayIcon(QIcon::fromTheme("redemption"), this);
    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->setToolTip("Redemption");
    // m_trayIcon->setToolTip("Redemption\n" + title + " — " + artist);
    m_trayIcon->show();

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            if (isVisible()) hide();
            else { show(); raise(); activateWindow(); }
        }
    });
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_trayIcon && m_trayIcon->isVisible()) {
        hide();
        event->ignore();
    } else {
        event->accept();
    }
}


/*
void MainWindow::onTreeContextMenu(const QPoint &pos)
{
    QModelIndex index = m_treeView->indexAt(pos);
    if (!index.isValid())
        return;

    FileNode *node = m_model->nodeFromIndex(index);
    if (!node)
        return;

    QMenu menu(this);

    // if (node->isDir) {
    //     QAction *openDir = menu.addAction(QIcon::fromTheme("folder-open"), "Open in File Manager");
    //     connect(openDir, &QAction::triggered, this, [node]() {
    //         QDesktopServices::openUrl(QUrl::fromLocalFile(node->path));
    //     });

    if (node->isDir) {
        QAction *loopDir = menu.addAction(
            QIcon::fromTheme("media-playlist-repeat"), "Loop directory");
        QAction *loopDirRecursive = menu.addAction(
            QIcon::fromTheme("media-playlist-repeat"), "Loop directory (recursive)");
        QAction *shuffleDir = menu.addAction(
            QIcon::fromTheme("media-playlist-shuffle"), "Shuffle directory");
        menu.addSeparator();
        QAction *openDir = menu.addAction(
            QIcon::fromTheme("folder-open"), "Open in File Manager");

        connect(loopDir, &QAction::triggered, this, [this, node]() {
            m_engine->setShuffleMode(ShuffleMode::Off);
            m_engine->setLoopMode(LoopMode::Directory);
            m_engine->setLoopTargetPath(node->path);
            m_engine->setCurrentDirPath(node->path);
            updateLoopIcon();
            updateShuffleIcon();
            updateButtonStates();
            m_model->setLoopState((int)LoopMode::Directory, node->path);
            m_treeView->viewport()->update();
            ToastNotification::show(this, "Looping directory", "media-playlist-repeat");
        });

        connect(loopDirRecursive, &QAction::triggered, this, [this, node]() {
            m_engine->setShuffleMode(ShuffleMode::Off);
            m_engine->setLoopMode(LoopMode::DirectoryRecursive);
            m_engine->setLoopTargetPath(node->path);
            m_engine->setCurrentDirPath(node->path);
            updateLoopIcon();
            updateShuffleIcon();
            updateButtonStates();
            m_model->setLoopState((int)LoopMode::DirectoryRecursive, node->path);
            m_treeView->viewport()->update();
            ToastNotification::show(this, "Looping directory (recursive)",
                "media-playlist-repeat");
        });

        connect(shuffleDir, &QAction::triggered, this, [this, node]() {
            m_engine->setLoopMode(LoopMode::None);
            m_engine->setShuffleMode(ShuffleMode::Directory);
            m_engine->setShuffleTargetPath(node->path);
            m_engine->setCurrentDirPath(node->path);
            QStringList fullQueue = m_model->collectAudioFiles(
                QDir::homePath() + "/Music");
            QString current = m_engine->currentTrack();
            int idx = fullQueue.indexOf(current);
            m_engine->playFrom(fullQueue, idx >= 0 ? idx : 0);
            updateLoopIcon();
            updateShuffleIcon();
            updateButtonStates();
            m_model->setShuffleState((int)ShuffleMode::Directory, node->path);
            m_treeView->viewport()->update();
            ToastNotification::show(this, "Shuffling directory",
                "media-playlist-shuffle");
        });

        connect(openDir, &QAction::triggered, this, [node]() {
            QDesktopServices::openUrl(QUrl::fromLocalFile(node->path));
        });

    } else {
        QAction *infoAction = menu.addAction(QIcon::fromTheme("document-properties"), "Information");
        QAction *openDirAction = menu.addAction(QIcon::fromTheme("folder-open"), "Show Containing Directory");
        menu.addSeparator();
        QAction *copyTitle = menu.addAction(QIcon::fromTheme("edit-copy"), "Copy Title");
        QAction *copyTitleArtist = menu.addAction(QIcon::fromTheme("edit-copy"), "Copy Title and Artist");
        QAction *copyFilename = menu.addAction(QIcon::fromTheme("edit-copy"), "Copy Filename");

        connect(infoAction, &QAction::triggered, this, [this, node]() {
            MetadataDialog dialog(node->path, this);
            dialog.exec();
        });

        connect(openDirAction, &QAction::triggered, this, [node]() {
            QDesktopServices::openUrl(QUrl::fromLocalFile(
                QFileInfo(node->path).absolutePath()));
        });

        connect(copyTitle, &QAction::triggered, this, [this, node]() {
            TrackMetadata meta = MetadataReader::read(node->path);
            QString title = meta.hasTitle ? meta.title : QFileInfo(node->path).fileName();
            QApplication::clipboard()->setText(title);
            ToastNotification::show(this, "Copied to clipboard", "edit-copy");
        });

        connect(copyTitleArtist, &QAction::triggered, this, [this, node]() {
            TrackMetadata meta = MetadataReader::read(node->path);
            QString title = meta.hasTitle ? meta.title : QFileInfo(node->path).fileName();
            QString artist = meta.hasArtist ? meta.artist : QString();
            QString text = artist.isEmpty() ? title : title + " - " + artist;
            QApplication::clipboard()->setText(text);
            ToastNotification::show(this, "Copied to clipboard", "edit-copy");
        });

        connect(copyFilename, &QAction::triggered, this, [this, node]() {
            QApplication::clipboard()->setText(node->path);
            ToastNotification::show(this, "Copied to clipboard", "edit-copy");
        });
    }

    menu.exec(m_treeView->viewport()->mapToGlobal(pos));
}
*/
void MainWindow::onTreeContextMenu(const QPoint &pos)
{
    QModelIndex index = m_treeView->indexAt(pos);
    if (!index.isValid())
        return;

    FileNode *node = m_model->nodeFromIndex(index);
    if (!node)
        return;

    QMenu menu(this);

    if (node->isDir) {
        bool isLoopedDir = (m_engine->loopMode() == LoopMode::Directory ||
                           m_engine->loopMode() == LoopMode::DirectoryRecursive) &&
                           m_engine->loopTargetPath() == node->path;
        bool isShuffledDir = m_engine->shuffleMode() == ShuffleMode::Directory &&
                             m_engine->shuffleTargetPath() == node->path;

        if (isLoopedDir) {
            QAction *removeLoop = menu.addAction(
                QIcon::fromTheme("media-playlist-repeat"), "Remove loop");
            connect(removeLoop, &QAction::triggered, this, [this]() {
                m_engine->setLoopMode(LoopMode::None);
                m_engine->setLoopTargetPath("");
                updateLoopIcon();
                updateButtonStates();
                m_model->setLoopState((int)LoopMode::None, "");
                m_treeView->viewport()->update();
                ToastNotification::show(this, "Loop removed", "media-playlist-repeat");
            });
            menu.addSeparator();
        } else if (isShuffledDir) {
            QAction *removeShuffle = menu.addAction(
                QIcon::fromTheme("media-playlist-shuffle"), "Remove shuffle");
            connect(removeShuffle, &QAction::triggered, this, [this]() {
                m_engine->setShuffleMode(ShuffleMode::Off);
                m_engine->setShuffleTargetPath("");
                m_engine->restoreQueue();
                updateShuffleIcon();
                updateButtonStates();
                m_model->setShuffleState((int)ShuffleMode::Off, "");
                m_treeView->viewport()->update();
                ToastNotification::show(this, "Shuffle removed", "media-playlist-shuffle");
            });
            menu.addSeparator();
        } else {
            QAction *loopDir = menu.addAction(
                QIcon::fromTheme("media-playlist-repeat"), "Loop directory");
            QAction *loopDirRecursive = menu.addAction(
                QIcon::fromTheme("media-playlist-repeat"), "Loop directory (recursive)");
            QAction *shuffleDir = menu.addAction(
                QIcon::fromTheme("media-playlist-shuffle"), "Shuffle directory");
            menu.addSeparator();

            connect(loopDir, &QAction::triggered, this, [this, node]() {
                m_engine->setShuffleMode(ShuffleMode::Off);
                m_engine->setLoopMode(LoopMode::Directory);
                m_engine->setLoopTargetPath(node->path);
                m_engine->setCurrentDirPath(node->path);
                updateLoopIcon();
                updateShuffleIcon();
                updateButtonStates();
                m_model->setLoopState((int)LoopMode::Directory, node->path);
                m_treeView->viewport()->update();
                ToastNotification::show(this, "Looping directory", "media-playlist-repeat");
            });

            connect(loopDirRecursive, &QAction::triggered, this, [this, node]() {
                m_engine->setShuffleMode(ShuffleMode::Off);
                m_engine->setLoopMode(LoopMode::DirectoryRecursive);
                m_engine->setLoopTargetPath(node->path);
                m_engine->setCurrentDirPath(node->path);
                updateLoopIcon();
                updateShuffleIcon();
                updateButtonStates();
                m_model->setLoopState((int)LoopMode::DirectoryRecursive, node->path);
                m_treeView->viewport()->update();
                ToastNotification::show(this, "Looping directory (recursive)",
                    "media-playlist-repeat");
            });

            connect(shuffleDir, &QAction::triggered, this, [this, node]() {
                m_engine->setLoopMode(LoopMode::None);
                m_engine->setShuffleMode(ShuffleMode::Directory);
                m_engine->setShuffleTargetPath(node->path);
                m_engine->setCurrentDirPath(node->path);
                QStringList fullQueue = m_model->collectAudioFiles(
                    QDir::homePath() + "/Music");
                QString current = m_engine->currentTrack();
                int idx = fullQueue.indexOf(current);
                m_engine->playFrom(fullQueue, idx >= 0 ? idx : 0);
                updateLoopIcon();
                updateShuffleIcon();
                updateButtonStates();
                m_model->setShuffleState((int)ShuffleMode::Directory, node->path);
                m_treeView->viewport()->update();
                ToastNotification::show(this, "Shuffling directory",
                    "media-playlist-shuffle");
            });
        }

        QAction *openDir = menu.addAction(
            QIcon::fromTheme("folder-open"), "Open in File Manager");
        connect(openDir, &QAction::triggered, this, [node]() {
            QDesktopServices::openUrl(QUrl::fromLocalFile(node->path));
        });

    } else {
        bool isLoopedTrack = m_engine->loopMode() == LoopMode::Track &&
                             m_engine->loopTargetPath() == node->path;

        if (isLoopedTrack) {
            QAction *removeLoop = menu.addAction(
                QIcon::fromTheme("media-playlist-repeat-song"), "Remove loop");
            connect(removeLoop, &QAction::triggered, this, [this]() {
                m_engine->setLoopMode(LoopMode::None);
                m_engine->setLoopTargetPath("");
                updateLoopIcon();
                updateButtonStates();
                m_model->setLoopState((int)LoopMode::None, "");
                m_treeView->viewport()->update();
                ToastNotification::show(this, "Loop removed",
                    "media-playlist-repeat-song");
            });
            menu.addSeparator();
        } else {
            QAction *loopTrack = menu.addAction(
                QIcon::fromTheme("media-playlist-repeat-song"), "Loop this track");
            connect(loopTrack, &QAction::triggered, this, [this, node]() {
                m_engine->setShuffleMode(ShuffleMode::Off);
                m_engine->setLoopMode(LoopMode::Track);
                m_engine->setLoopTargetPath(node->path);
                updateLoopIcon();
                updateShuffleIcon();
                updateButtonStates();
                m_model->setLoopState((int)LoopMode::Track, node->path);
                m_treeView->viewport()->update();
                ToastNotification::show(this, "Looping track",
                    "media-playlist-repeat-song");
            });
            menu.addSeparator();
        }

        QAction *infoAction = menu.addAction(
            QIcon::fromTheme("document-properties"), "Information");
        QAction *openDirAction = menu.addAction(
            QIcon::fromTheme("folder-open"), "Show Containing Directory");
        menu.addSeparator();
        QAction *copyTitle = menu.addAction(
            QIcon::fromTheme("edit-copy"), "Copy Title");
        QAction *copyTitleArtist = menu.addAction(
            QIcon::fromTheme("edit-copy"), "Copy Title and Artist");
        QAction *copyFilename = menu.addAction(
            QIcon::fromTheme("edit-copy"), "Copy Filename");

        connect(infoAction, &QAction::triggered, this, [this, node]() {
            MetadataDialog dialog(node->path, this);
            dialog.exec();
        });

        connect(openDirAction, &QAction::triggered, this, [node]() {
            QDesktopServices::openUrl(QUrl::fromLocalFile(
                QFileInfo(node->path).absolutePath()));
        });

        connect(copyTitle, &QAction::triggered, this, [this, node]() {
            TrackMetadata meta = MetadataReader::read(node->path);
            QString title = meta.hasTitle ? meta.title : QFileInfo(node->path).fileName();
            QApplication::clipboard()->setText(title);
            ToastNotification::show(this, "Copied to clipboard", "edit-copy");
        });

        connect(copyTitleArtist, &QAction::triggered, this, [this, node]() {
            TrackMetadata meta = MetadataReader::read(node->path);
            QString title = meta.hasTitle ? meta.title : QFileInfo(node->path).fileName();
            QString artist = meta.hasArtist ? meta.artist : QString();
            QString text = artist.isEmpty() ? title : title + " - " + artist;
            QApplication::clipboard()->setText(text);
            ToastNotification::show(this, "Copied to clipboard", "edit-copy");
        });

        connect(copyFilename, &QAction::triggered, this, [this, node]() {
            QApplication::clipboard()->setText(node->path);
            ToastNotification::show(this, "Copied to clipboard", "edit-copy");
        });
    }

    menu.exec(m_treeView->viewport()->mapToGlobal(pos));
}

void MainWindow::flashLabel(QWidget *widget, const QString &text)
{
    QApplication::clipboard()->setText(text);
    ToastNotification::show(this, "Copied to clipboard", "edit-copy");

    // scale effect — briefly increase font size then restore
    // QFont original = widget->font();
    // QFont bigger = original;
    // bigger.setPointSize(original.pointSize() + 3);
    // widget->setFont(bigger);
    // QTimer::singleShot(150, this, [widget, original]() {
    //     widget->setFont(original);
    // });

    // flash highlight color then restore
    /*QPalette highlight = widget->palette();
    highlight.setColor(QPalette::WindowText,
        widget->palette().color(QPalette::Highlight));
    widget->setPalette(highlight);
    QTimer::singleShot(200, this, [widget]() {
        widget->setPalette(QPalette());
    });*/
    QPalette original = widget->palette();  // save before changing
    QPalette highlight = original;
    highlight.setColor(QPalette::WindowText,
        widget->palette().color(QPalette::Highlight));
    widget->setPalette(highlight);
    QTimer::singleShot(200, this, [widget, original]() {
        widget->setPalette(original);  // restore exact original
    });
}

void MainWindow::onTitleClicked()
{
    flashLabel(m_titleLabel, m_titleLabel->text());
}

void MainWindow::onArtistClicked()
{
    flashLabel(m_artistLabel, m_artistLabel->text());
}

void MainWindow::playFromPath(const QString &path)
{
    if (path.isEmpty()) return;

    m_engine->setCurrentDirPath(QFileInfo(path).absolutePath());
    QStringList fullQueue = m_model->collectAudioFiles(QDir::homePath() + "/Music");
    int startIndex = fullQueue.indexOf(path);
    if (startIndex < 0) startIndex = 0;

    QString clickedDir = QFileInfo(path).absolutePath();
    if (m_engine->shuffleMode() == ShuffleMode::Directory &&
        clickedDir != m_engine->shuffleTargetPath()) {
        m_engine->setShuffleMode(ShuffleMode::Off);
        m_engine->setShuffleTargetPath("");
        m_engine->restoreQueue();
        updateShuffleIcon();
        updateButtonStates();
        m_model->setShuffleState((int)ShuffleMode::Off, "");
        m_treeView->viewport()->update();
    }

    m_engine->setManualChange(true);
    m_engine->setCurrentDirPath(clickedDir);
    m_engine->playFrom(fullQueue, startIndex);
    QModelIndex treeIdx = m_model->indexForPath(path);
    if (treeIdx.isValid()) {
        m_treeView->setCurrentIndex(treeIdx);
        m_treeView->scrollTo(treeIdx, QAbstractItemView::PositionAtCenter);
        m_treeView->expand(treeIdx.parent());
        m_treeView->selectionModel()->select(treeIdx,
            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }
}


void MainWindow::onCavaClicked()
{
    if (m_cavaProcess && m_cavaProcess->state() == QProcess::Running) {
        m_cavaProcess->terminate();
        m_cavaProcess->waitForFinished(1000);
        m_cavaProcess = nullptr;
        m_cavaButton->setDown(false);
        return;
    }

    m_cavaProcess = new QProcess(this);
    connect(m_cavaProcess, &QProcess::finished, this, [this]() {
        m_cavaProcess = nullptr;
        m_cavaButton->setDown(false);
    });

    m_cavaProcess->start("kitty", {
        "--override", "initial_window_width=85c",
        "--override", "initial_window_height=20c",
        "-e", "cava"
    });

    m_cavaButton->setDown(true);
}

void MainWindow::updateButtonStates()
{
    bool loopActive = m_engine->loopMode() != LoopMode::None;
    bool shuffleActive = m_engine->shuffleMode() != ShuffleMode::Off;

    m_shuffleButton->setEnabled(!loopActive);
    m_loopButton->setEnabled(!shuffleActive);
    m_queueButton->setEnabled(shuffleActive);

    if (m_nowPlayingLoopButton)
        m_nowPlayingLoopButton->setEnabled(!shuffleActive);
    if (m_nowPlayingShuffleButton)
        m_nowPlayingShuffleButton->setEnabled(!loopActive);

}

void MainWindow::onQueueClicked()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Current Queue");
    dialog.resize(500, 600);

    QListWidget *list = new QListWidget(&dialog);
    QStringList queue = m_engine->queue();
    QString current = m_engine->currentTrack();
    int currentIdx = m_engine->currentIndex();

    for (int i = 0; i < queue.size(); ++i) {
        QString path = queue[i];
        // use display name from search index if available
        QString display;
        for (const SearchResult &r : m_searchIndex) {
            if (r.path == path) {
                display = r.displayName;
                break;
            }
        }
        if (display.isEmpty())
            display = QFileInfo(path).fileName();

        QListWidgetItem *item = new QListWidgetItem(
            QString("%1. %2").arg(i + 1).arg(display));
        item->setData(Qt::UserRole, path);

        if (i == currentIdx) {
            QFont f = item->font();
            f.setBold(true);
            item->setFont(f);
            item->setForeground(list->palette().highlight());
        }

        list->addItem(item);
    }

    // scroll to current
    if (currentIdx >= 0 && currentIdx < list->count())
        list->scrollToItem(list->item(currentIdx),
            QAbstractItemView::PositionAtCenter);

    // clicking an item plays it
    connect(list, &QListWidget::itemActivated, this, [this, &dialog](QListWidgetItem *item) {
        QString path = item->data(Qt::UserRole).toString();
        int idx = m_engine->queue().indexOf(path);
        if (idx >= 0) {
            m_engine->setCurrentDirPath(QFileInfo(path).absolutePath());
            m_engine->jumpTo(idx);
        }
        dialog.accept();
    });

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->addWidget(list);
    dialog.exec();
}


QPixmap MainWindow::roundedPixmap(const QPixmap &src, int radius)
{
    QPixmap result(src.size());
    result.fill(Qt::transparent);
    QPainter p(&result);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addRoundedRect(result.rect(), radius, radius);
    p.setClipPath(path);
    p.drawPixmap(0, 0, src);
    return result;
}

/*
void MainWindow::onViewToggled()
{
    m_playlistMode = !m_playlistMode;

    if (m_playlistMode) {
        // refresh playlist with current directory
        refreshPlaylistView(m_engine->currentDirPath());

        // sync now playing panel
        m_nowPlayingPanel->setTitle(m_titleLabel->text());
        m_nowPlayingPanel->setArtist(m_artistLabel->text());
        m_nowPlayingPanel->setCover(m_currentCover);

        // animate: tree+bottombar out, playlist mode in
        m_playlistModeWidget->setGeometry(centralWidget()->rect());
        m_playlistModeWidget->show();
        m_playlistModeWidget->raise();

        // QPropertyAnimation *fadeIn = new QPropertyAnimation(m_playlistModeWidget, "windowOpacity");
        // use graphics effect for opacity since it's not a window
        QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(m_playlistModeWidget);
        m_playlistModeWidget->setGraphicsEffect(effect);
        effect->setOpacity(0.0);

        QPropertyAnimation *anim = new QPropertyAnimation(effect, "opacity");
        anim->setDuration(300);
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setEasingCurve(QEasingCurve::InOutQuad);
        anim->start(QAbstractAnimation::DeleteWhenStopped);

        m_viewToggleButton->setIcon(QIcon::fromTheme("view-list-tree"));
    } else {
        // animate out
        QGraphicsOpacityEffect *effect =
            qobject_cast<QGraphicsOpacityEffect*>(m_playlistModeWidget->graphicsEffect());
        if (effect) {
            QPropertyAnimation *anim = new QPropertyAnimation(effect, "opacity");
            anim->setDuration(300);
            anim->setStartValue(1.0);
            anim->setEndValue(0.0);
            anim->setEasingCurve(QEasingCurve::InOutQuad);
            connect(anim, &QPropertyAnimation::finished, this, [this]() {
                m_playlistModeWidget->hide();
                m_playlistModeWidget->setGraphicsEffect(nullptr);
            });
            anim->start(QAbstractAnimation::DeleteWhenStopped);
        } else {
            m_playlistModeWidget->hide();
        }

        m_viewToggleButton->setIcon(QIcon::fromTheme("view-fullscreen"));
    }
}
*/
void MainWindow::onViewToggled()
{
    m_playlistMode = !m_playlistMode;

    /*
    if (m_playlistMode) {
        refreshPlaylistView(m_engine->currentDirPath());
        m_nowPlayingPanel->setTitle(m_titleLabel->text());
        m_nowPlayingPanel->setArtist(m_artistLabel->text());
        m_nowPlayingPanel->setCover(m_currentCover);
        m_nowPlayingAmbient->setGeometry(m_nowPlayingPanel->rect());
        m_nowPlayingAmbient->updateFromCover(m_currentCover);

        m_playlistModeWidget->setGeometry(centralWidget()->rect());
        m_playlistEffect->setOpacity(0.0);
        m_playlistModeWidget->show();
        m_playlistModeWidget->raise();

        QPropertyAnimation *anim = new QPropertyAnimation(m_playlistEffect, "opacity");
        anim->setDuration(300);
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setEasingCurve(QEasingCurve::InOutQuad);
        anim->start(QAbstractAnimation::DeleteWhenStopped);

        m_viewToggleButton->setIcon(QIcon::fromTheme("view-list-tree"));
    } else {
        QPropertyAnimation *anim = new QPropertyAnimation(m_playlistEffect, "opacity");
        anim->setDuration(300);
        anim->setStartValue(1.0);
        anim->setEndValue(0.0);
        anim->setEasingCurve(QEasingCurve::InOutQuad);
        connect(anim, &QPropertyAnimation::finished, this, [this]() {
            m_playlistModeWidget->hide();
        });
        anim->start(QAbstractAnimation::DeleteWhenStopped);

        m_viewToggleButton->setIcon(QIcon::fromTheme("view-fullscreen"));
    }
    */
   if (m_playlistMode) {

        // force correct geometry BEFORE showing
        m_playlistModeWidget->setGeometry(centralWidget()->rect());
        m_fadeOverlay->setGeometry(centralWidget()->rect());
        // m_nowPlayingAmbient->setGeometry(m_nowPlayingPanel->rect());

        refreshPlaylistView(m_engine->currentDirPath());
        m_nowPlayingPanel->setTitle(m_titleLabel->text());
        m_nowPlayingPanel->setArtist(m_artistLabel->text());
        m_nowPlayingPanel->setCover(m_currentCover);
        // m_nowPlayingAmbient->setGeometry(m_nowPlayingPanel->rect());
        m_nowPlayingAmbient->updateFromCover(m_currentCover);

        m_playlistModeWidget->move(0, 0);
        m_playlistModeWidget->resize(centralWidget()->size());
        m_playlistModeWidget->setGeometry(centralWidget()->rect());
        m_ambientBar->hide();
        m_treeView->hide();
        m_playlistModeWidget->show();
        m_playlistModeWidget->raise();
        m_ambientBar->lower();

    m_playlistAmbient->updateFromCover(m_currentCover);

    QTimer::singleShot(50, this, [this]() {
        m_nowPlayingPanel->setCover(m_currentCover);
        m_nowPlayingAmbient->setGeometry(m_nowPlayingPanel->rect());
    m_playlistAmbient->setGeometry(m_playlistView->rect());
    // m_playlistAmbient->raise();
    update();
    });

        // fade overlay on top, then hide it
        m_fadeOverlay->setGeometry(centralWidget()->rect());
        m_fadeOverlay->show();
        m_fadeOverlay->raise();

        QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(m_fadeOverlay);
        m_fadeOverlay->setGraphicsEffect(effect);
        effect->setOpacity(1.0);

        QPropertyAnimation *anim = new QPropertyAnimation(effect, "opacity");
        anim->setDuration(300);
        anim->setStartValue(1.0);
        anim->setEndValue(0.0);
        anim->setEasingCurve(QEasingCurve::InOutQuad);
        connect(anim, &QPropertyAnimation::finished, this, [this]() {
            m_fadeOverlay->hide();
            m_fadeOverlay->setGraphicsEffect(nullptr);
        });
        anim->start(QAbstractAnimation::DeleteWhenStopped);

        m_viewToggleButton->setIcon(QIcon::fromTheme("view-list-tree"));
    } else {
        m_fadeOverlay->setGeometry(centralWidget()->rect());
        m_fadeOverlay->show();
        m_fadeOverlay->raise();

        QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(m_fadeOverlay);
        m_fadeOverlay->setGraphicsEffect(effect);
        effect->setOpacity(0.0);

        QPropertyAnimation *anim = new QPropertyAnimation(effect, "opacity");
        anim->setDuration(300);
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setEasingCurve(QEasingCurve::InOutQuad);
        connect(anim, &QPropertyAnimation::finished, this, [this]() {
            m_playlistModeWidget->hide();
        m_ambientBar->raise();
        m_ambientBar->show();
        m_treeView->show();
        m_treeView->setFocus();
            m_fadeOverlay->hide();
            m_fadeOverlay->setGraphicsEffect(nullptr);
        });
        anim->start(QAbstractAnimation::DeleteWhenStopped);

        m_viewToggleButton->setIcon(QIcon::fromTheme("view-fullscreen"));
    }
}

void MainWindow::refreshPlaylistView(const QString &dirPath)
{
    if (dirPath.isEmpty()) return;
    m_playlistView->loadDirectory(dirPath, m_engine->currentTrack());
}


/*
void MainWindow::resizeEvent(QResizeEvent *e)
{
    QMainWindow::resizeEvent(e);
    if (m_playlistModeWidget && centralWidget()) {
        m_playlistModeWidget->setGeometry(centralWidget()->rect());
        m_playlistModeWidget->raise();
    }
    if (m_fadeOverlay && centralWidget())
        m_fadeOverlay->setGeometry(centralWidget()->rect());
    // if (m_nowPlayingAmbient && m_nowPlayingPanel)
    //     m_nowPlayingAmbient->setGeometry(m_nowPlayingPanel->rect());
    if (m_playlistMode && m_nowPlayingPanel)
        m_nowPlayingPanel->setCover(m_currentCover);
}
*/

// void MainWindow::resizeEvent(QResizeEvent *e)
// {
//     QMainWindow::resizeEvent(e);
//     if (m_playlistModeWidget && centralWidget())
//         m_playlistModeWidget->resize(centralWidget()->size());
// m_playlistModeWidget->update();
//     if (m_fadeOverlay && centralWidget())
//         m_fadeOverlay->setGeometry(centralWidget()->rect());
//     if (m_nowPlayingAmbient && m_nowPlayingPanel)
//         m_nowPlayingAmbient->setGeometry(m_nowPlayingPanel->rect());
//     if (m_playlistMode && m_nowPlayingPanel && !m_currentCover.isNull())
//         m_nowPlayingPanel->setCover(m_currentCover);
//     if (m_playlistAmbient && m_playlistView)
//         m_playlistAmbient->setGeometry(m_playlistView->rect());
//     if (m_playlistMode && m_nowPlayingPanel && !m_currentCover.isNull())
//         m_nowPlayingPanel->setCover(m_currentCover);
// if (m_playlistMode) {
// m_playlistModeWidget->setGeometry(centralWidget()->rect());
// m_playlistModeWidget->repaint();
// m_nowPlayingPanel->repaint();
// m_playlistView->repaint();
// }
//         // qDebug() << "coverLabel size:" << m_nowPlayingPanel->coverLabel()->size();
// }

void MainWindow::resizeEvent(QResizeEvent *e)
{
    QMainWindow::resizeEvent(e);
    if (m_playlistModeWidget && centralWidget()) {
        m_playlistModeWidget->setGeometry(centralWidget()->rect());
        if (m_playlistMode) {
            m_playlistModeWidget->repaint();
            m_nowPlayingPanel->repaint();
            m_playlistView->repaint();
        }
    }
    if (m_fadeOverlay && centralWidget())
        m_fadeOverlay->setGeometry(centralWidget()->rect());
    if (m_nowPlayingAmbient && m_nowPlayingPanel)
        m_nowPlayingAmbient->setGeometry(m_nowPlayingPanel->rect());
    if (m_playlistAmbient && m_playlistView)
        m_playlistAmbient->setGeometry(m_playlistView->rect());
    if (m_playlistMode && m_nowPlayingPanel && !m_currentCover.isNull())
        m_nowPlayingPanel->setCover(m_currentCover);
}

void MainWindow::setupNowPlayingControls()
{
    // --- playback buttons ---
    QToolButton *prev = new QToolButton();
    m_nowPlayingPlayButton = new QToolButton();
    QToolButton *next = new QToolButton();

    QToolButton *loopBtn = new QToolButton();
    QToolButton *shuffleBtn = new QToolButton();

    prev->setIcon(QIcon::fromTheme("media-skip-backward"));
    m_nowPlayingPlayButton->setIcon(QIcon::fromTheme("media-playback-pause"));
    next->setIcon(QIcon::fromTheme("media-skip-forward"));

    for (QToolButton *btn : {prev, m_nowPlayingPlayButton, next}) {
        btn->setFixedSize(52, 52);
        btn->setIconSize(QSize(28, 28));
        btn->setAutoRaise(true);
    }

    connect(prev, &QToolButton::clicked, this, &MainWindow::onPreviousClicked);
    connect(next, &QToolButton::clicked, this, &MainWindow::onNextClicked);
    connect(m_nowPlayingPlayButton, &QToolButton::clicked,
            this, &MainWindow::onPlayPauseClicked);

    m_nowPlayingPanel->controlsLayout()->addWidget(prev);
    m_nowPlayingPanel->controlsLayout()->addWidget(m_nowPlayingPlayButton);
    m_nowPlayingPanel->controlsLayout()->addWidget(next);

    // --- volume ---
    m_nowPlayingMuteButton = new QToolButton();
    m_nowPlayingMuteButton->setIcon(QIcon::fromTheme("audio-volume-high"));
    m_nowPlayingMuteButton->setFixedSize(36, 36);
    m_nowPlayingMuteButton->setIconSize(QSize(20, 20));
    m_nowPlayingMuteButton->setAutoRaise(true);
    connect(m_nowPlayingMuteButton, &QToolButton::clicked,
            this, &MainWindow::onMuteClicked);
            if (m_nowPlayingMuteButton)
            m_nowPlayingMuteButton->setIcon(QIcon::fromTheme(
                m_muted ? "audio-volume-muted" : "audio-volume-high"));

    m_nowPlayingVolume = new QSlider(Qt::Horizontal);
    m_nowPlayingVolume->setRange(0, 100);
    m_nowPlayingVolume->setValue(100);
    m_nowPlayingVolume->setFixedWidth(120);
    m_nowPlayingVolume->setStyleSheet(
        "QSlider::groove:horizontal {"
        "  height: 3px;"
        "  background: rgba(255,255,255,60);"
        "  border-radius: 2px;"
        "}"
        "QSlider::sub-page:horizontal {"
        "  background: white;"
        "  border-radius: 2px;"
        "}"
        "QSlider::handle:horizontal {"
        "  width: 10px; height: 10px;"
        "  background: white;"
        "  border-radius: 5px;"
        "  margin: -4px 0;"
        "}"
    );

    m_nowPlayingVolumeLabel = new QLabel("100%");
    m_nowPlayingVolumeLabel->setStyleSheet("color: rgba(255,255,255,180); font-size: 10px;");
    m_nowPlayingVolumeLabel->setFixedWidth(36);

    connect(m_nowPlayingVolume, &QSlider::valueChanged, this, [this](int val) {
        m_volumeSlider->setValue(val);
        onVolumeChanged(val);
        m_nowPlayingVolumeLabel->setText(QString("%1%").arg(val));
    });

    // sync with main volume slider
    connect(m_volumeSlider, &QSlider::valueChanged, this, [this](int val) {
        m_nowPlayingVolume->setValue(val);
        m_nowPlayingVolumeLabel->setText(QString("%1%").arg(val));
    });

    m_nowPlayingPanel->volumeLayout()->addWidget(m_nowPlayingMuteButton);
    m_nowPlayingPanel->volumeLayout()->addWidget(m_nowPlayingVolume);
    m_nowPlayingPanel->volumeLayout()->addWidget(m_nowPlayingVolumeLabel);

    // --- seekbar ---
    SeekSlider *seekBar = m_nowPlayingPanel->seekBar();
    seekBar->installEventFilter(this);

    connect(seekBar, &QSlider::sliderPressed, this, [this]() {
        m_seeking = true;
    });
    connect(seekBar, &QSlider::sliderReleased, this, [this]() {
        m_seeking = false;
        double duration = 0;
        mpv_get_property(m_engine->mpvHandle(), "duration",
            MPV_FORMAT_DOUBLE, &duration);
        double seekTo = (m_nowPlayingPanel->seekBar()->value() / 1000.0) * duration;
        const QString cmd = QString::number(seekTo, 'f', 2);
        QByteArray ba = cmd.toUtf8();
        const char *args[] = {"seek", ba.constData(), "absolute", nullptr};
        mpv_command(m_engine->mpvHandle(), args);
    });
    connect(seekBar, &QSlider::sliderMoved, this, [this](int) {
        double duration = 0;
        mpv_get_property(m_engine->mpvHandle(), "duration",
            MPV_FORMAT_DOUBLE, &duration);
        double seekTo = (m_nowPlayingPanel->seekBar()->value() / 1000.0) * duration;
        const QString cmd = QString::number(seekTo, 'f', 2);
        QByteArray ba = cmd.toUtf8();
        const char *args[] = {"seek", ba.constData(), "absolute", nullptr};
        mpv_command(m_engine->mpvHandle(), args);
    });
    m_nowPlayingVolume->installEventFilter(this);

    //////
    loopBtn->setFixedSize(44, 44);
    loopBtn->setIconSize(QSize(22, 22));
    loopBtn->setAutoRaise(true);
    shuffleBtn->setFixedSize(44, 44);
    shuffleBtn->setIconSize(QSize(22, 22));
    shuffleBtn->setAutoRaise(true);


    // sync icons with main buttons
    loopBtn->setIcon(m_loopButton->icon());
    shuffleBtn->setIcon(m_shuffleButton->icon());

    connect(loopBtn, &QToolButton::clicked, this, &MainWindow::onLoopClicked);
    connect(shuffleBtn, &QToolButton::clicked, this, &MainWindow::onShuffleClicked);

    // // keep icons in sync when main buttons change
    // connect(m_loopButton, &QToolButton::iconChanged, loopBtn, &QToolButton::setIcon);  // doesn't exist

    m_nowPlayingLoopButton = new QToolButton();
    m_nowPlayingShuffleButton = new QToolButton();

    m_nowPlayingLoopButton->setIcon(m_loopButton->icon());
    m_nowPlayingShuffleButton->setIcon(m_shuffleButton->icon());
    m_nowPlayingLoopButton->setToolTip(m_loopButton->toolTip());
    m_nowPlayingShuffleButton->setToolTip(m_shuffleButton->toolTip());

    for (auto *btn : {m_nowPlayingLoopButton, m_nowPlayingShuffleButton}) {
        btn->setFixedSize(44, 44);
        btn->setIconSize(QSize(22, 22));
        btn->setAutoRaise(true);
    }

    connect(m_nowPlayingLoopButton, &QToolButton::clicked,
            this, &MainWindow::onLoopClicked);
    connect(m_nowPlayingShuffleButton, &QToolButton::clicked,
            this, &MainWindow::onShuffleClicked);

    // add to a row between controls and volume
    QHBoxLayout *loopShuffleRow = new QHBoxLayout();
    loopShuffleRow->setAlignment(Qt::AlignCenter);
    loopShuffleRow->addWidget(m_nowPlayingLoopButton);
    loopShuffleRow->addWidget(m_nowPlayingShuffleButton);

    m_nowPlayingPanel->extraLayout()->addWidget(m_nowPlayingLoopButton);
    m_nowPlayingPanel->extraLayout()->addWidget(m_nowPlayingShuffleButton);
}

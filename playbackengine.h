#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <mpv/client.h>

enum class LoopMode {
    None,
    Track,
    Directory,
    DirectoryRecursive,
    Queue
};

enum class ShuffleMode {
    Off,
    Directory,
    All
};

class PlaybackEngine : public QObject
{
    Q_OBJECT

public:
    explicit PlaybackEngine(QObject *parent = nullptr);
    ~PlaybackEngine();

    void play(const QString &filePath);
    void enqueue(const QStringList &filePaths);
    void playQueue(const QStringList &filePaths);
    void playFrom(const QStringList &filePaths, int index);

    void pause();
    void resume();
    void stop();
    void next();
    void previous();

    bool wasManualChange() const { return m_manualTrackChange; }
    void clearManualChange() { m_manualTrackChange = false; }

    // void setManualChange(bool val) { m_manualTrackChange = true; }

    bool isPlaying() const;
    mpv_handle *mpvHandle() const { return m_mpv; }

    // void setLoopMode(LoopMode mode) { m_loopMode = mode; }
    // void setShuffleMode(ShuffleMode mode) { m_shuffleMode = mode; }
    // LoopMode loopMode() const { return m_loopMode; }
    // ShuffleMode shuffleMode() const { return m_shuffleMode; }

    // loop
    void setLoopMode(LoopMode mode) { m_loopMode = mode; }
    LoopMode loopMode() const { return m_loopMode; }

    // shuffle
    void setShuffleMode(ShuffleMode mode) { m_shuffleMode = mode; }
    ShuffleMode shuffleMode() const { return m_shuffleMode; }

    // paths
    void setRootPath(const QString &path) { m_rootPath = path; }
    void setCurrentDirPath(const QString &path) { m_currentDirPath = path; }
    QString currentDirPath() const { return m_currentDirPath; }

    // queue
    void restoreQueue();
    QString currentTrack() const {
        if (m_currentIndex >= 0 && m_currentIndex < m_queue.size())
            return m_queue[m_currentIndex];
        return QString();
    }

    QStringList queue() const { return m_queue; }
    int currentIndex() const { return m_currentIndex; }
    void jumpTo(int index) { loadTrack(index); }

    void setLoopTargetPath(const QString &path) { m_loopTargetPath = path; }
    QString loopTargetPath() const { return m_loopTargetPath; }
    void setShuffleTargetPath(const QString &path) { m_shuffleTargetPath = path; }
    QString shuffleTargetPath() const { return m_shuffleTargetPath; }

    void setManualChange(bool val) { m_manualTrackChange = val; }

signals:
    void trackChanged(const QString &filePath);
    void playbackFinished();
    void errorOccurred(const QString &message);
    void pauseStateChanged(bool paused);
    void durationChanged(double duration);
    void loopModeChanged(LoopMode mode);
    void shuffleModeChanged(ShuffleMode mode);


private:
    mpv_handle *m_mpv = nullptr;
    QStringList m_queue;
    QStringList m_originalQueue;
    int m_currentIndex = -1;
    int m_originalIndex = -1;
    LoopMode m_loopMode = LoopMode::None;
    ShuffleMode m_shuffleMode = ShuffleMode::Off;
    QString m_rootPath;
    QString m_currentDirPath;
    bool m_manualTrackChange = false;

    QString m_loopTargetPath;
    QString m_shuffleTargetPath;

    void loadTrack(int index);
    void handleTrackEnd();
    // void buildShuffledQueue(const QStringList &files);
    void buildShuffledQueue(const QStringList &files, const QString &startWith = QString());
    QStringList collectAudioFiles(const QString &path, bool recursive = true) const;
    static bool isAudioFile(const QString &path);
    static void mpvWakeupCallback(void *ctx);


private slots:
    void handleMpvEvents();
};

/*
#include "playbackengine.h"
#include <QCoreApplication>
#include <QTimer>
#include <QDir>
#include <QFileInfo>
#include <algorithm>
#include <random>

static const QStringList AUDIO_EXTENSIONS = {
    "mp3", "flac", "ogg", "opus", "wav", "aac",
    "m4a", "wma", "ape", "mpc", "aiff", "tta"
};

static bool isAudioFile(const QString &path)
{
    QFileInfo info(path);
    return AUDIO_EXTENSIONS.contains(info.suffix().toLower());
}

static QStringList collectAudioFiles(const QString &path)
{
    QStringList files;
    QDir dir(path);
    dir.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    dir.setSorting(QDir::Name | QDir::DirsFirst | QDir::IgnoreCase);

    for (const QFileInfo &info : dir.entryInfoList()) {
        if (info.isDir())
            files += collectAudioFiles(info.filePath());
        else if (isAudioFile(info.filePath()))
            files.append(info.filePath());
    }

    return files;
}

PlaybackEngine::PlaybackEngine(QObject *parent)
    : QObject(parent)
{
    m_mpv = mpv_create();
    if (!m_mpv) {
        emit errorOccurred("Failed to create mpv instance");
        return;
    }

    mpv_set_option_string(m_mpv, "audio-display", "no");
    mpv_set_option_string(m_mpv, "video", "no");
    mpv_set_option_string(m_mpv, "audio-client-name", "redemption");

    if (mpv_initialize(m_mpv) < 0) {
        emit errorOccurred("Failed to initialize mpv");
        return;
    }

    mpv_set_wakeup_callback(m_mpv, mpvWakeupCallback, this);
    mpv_observe_property(m_mpv, 0, "pause", MPV_FORMAT_FLAG);
    mpv_observe_property(m_mpv, 0, "duration", MPV_FORMAT_DOUBLE);
}

PlaybackEngine::~PlaybackEngine()
{
    if (m_mpv)
        mpv_terminate_destroy(m_mpv);
}

void PlaybackEngine::mpvWakeupCallback(void *ctx)
{
    PlaybackEngine *engine = static_cast<PlaybackEngine*>(ctx);
    QMetaObject::invokeMethod(engine, "handleMpvEvents", Qt::QueuedConnection);
}

void PlaybackEngine::handleMpvEvents()
{
    while (true) {
        mpv_event *event = mpv_wait_event(m_mpv, 0);
        if (event->event_id == MPV_EVENT_NONE)
            break;

        if (event->event_id == MPV_EVENT_END_FILE) {
            mpv_event_end_file *ef = static_cast<mpv_event_end_file*>(event->data);
            if (ef->reason == MPV_END_FILE_REASON_EOF) {
                handleTrackEnd();
            }
        }
        // if (event->event_id == MPV_EVENT_PROPERTY_CHANGE) {
        //     mpv_event_property *prop = static_cast<mpv_event_property*>(event->data);
        //     if (QString(prop->name) == "pause" && prop->format == MPV_FORMAT_FLAG) {
        //         bool paused = *static_cast<int*>(prop->data);
        //         emit pauseStateChanged(paused);
        //     }
        // }
        if (event->event_id == MPV_EVENT_PROPERTY_CHANGE) {
            mpv_event_property *prop = static_cast<mpv_event_property*>(event->data);
            if (QString(prop->name) == "pause" && prop->format == MPV_FORMAT_FLAG) {
                bool paused = *static_cast<int*>(prop->data);
                emit pauseStateChanged(paused);
            } else if (QString(prop->name) == "duration" && prop->format == MPV_FORMAT_DOUBLE) {
                double dur = *static_cast<double*>(prop->data);
                if (dur > 0)
                    emit durationChanged(dur);
            }
        }
    }
}

void PlaybackEngine::handleTrackEnd()
{
    if (m_loopMode == LoopMode::Track) {
        loadTrack(m_currentIndex);
        return;
    }

    if (m_loopMode == LoopMode::Directory) {
        // stay within current directory
        QString nextPath = m_currentIndex + 1 < m_queue.size()
            ? m_queue[m_currentIndex + 1] : QString();

        if (!nextPath.isEmpty() &&
            QFileInfo(nextPath).absolutePath() == m_currentDirPath) {
            loadTrack(m_currentIndex + 1);
        } else {
            // end of directory, loop back
            QStringList files = collectAudioFiles(m_currentDirPath);
            m_queue = files;
            m_currentIndex = 0;
            loadTrack(0);
        }
        return;
    }

    if (m_currentIndex + 1 < m_queue.size()) {
        loadTrack(m_currentIndex + 1);
        return;
    }

    // end of queue
    if (m_loopMode == LoopMode::Queue) {
        m_currentIndex = 0;
        loadTrack(0);
        return;
    }

    if (m_shuffleMode == ShuffleMode::Directory) {
        QStringList files = collectAudioFiles(m_currentDirPath);
        buildShuffledQueue(files);
        return;
    }

    if (m_shuffleMode == ShuffleMode::All) {
        QStringList files = collectAudioFiles(m_rootPath);
        buildShuffledQueue(files);
        return;
    }

    emit playbackFinished();
}


void PlaybackEngine::buildShuffledQueue(const QStringList &files)
{
    QStringList shuffled = files;
    std::shuffle(shuffled.begin(), shuffled.end(), std::mt19937{std::random_device{}()});
    m_queue = shuffled;
    m_currentIndex = 0;
    loadTrack(0);
}

void PlaybackEngine::play(const QString &filePath)
{
    playQueue({filePath});
}

void PlaybackEngine::playQueue(const QStringList &filePaths)
{
    if (m_shuffleMode == ShuffleMode::Directory) {
        QStringList files = collectAudioFiles(m_currentDirPath);
        buildShuffledQueue(files);
    } else if (m_shuffleMode == ShuffleMode::All) {
        QStringList files = collectAudioFiles(m_rootPath);
        buildShuffledQueue(files);
    } else {
        m_queue = filePaths;
        m_currentIndex = 0;
        loadTrack(0);
    }
}

void PlaybackEngine::playFrom(const QStringList &filePaths, int index)
{
    if (m_shuffleMode == ShuffleMode::Directory) {
        QStringList files = collectAudioFiles(m_currentDirPath);
        buildShuffledQueue(files);
    } else if (m_shuffleMode == ShuffleMode::All) {
        QStringList files = collectAudioFiles(m_rootPath);
        buildShuffledQueue(files);
    } else {
        m_queue = filePaths;
        m_currentIndex = index;
        loadTrack(index);
    }
}

void PlaybackEngine::loadTrack(int index)
{
    if (index < 0 || index >= m_queue.size())
        return;

    m_currentIndex = index;
    QString path = m_queue[index];

    QByteArray ba = path.toUtf8();
    const char *args[] = {"loadfile", ba.constData(), nullptr};
    mpv_command(m_mpv, args);

    // ensure playback starts — clears any lingering pause state
    int no = 0;
    mpv_set_property(m_mpv, "pause", MPV_FORMAT_FLAG, &no);

    emit trackChanged(path);
}

void PlaybackEngine::pause()
{
    int yes = 1;
    mpv_set_property(m_mpv, "pause", MPV_FORMAT_FLAG, &yes);
}

void PlaybackEngine::resume()
{
    int no = 0;
    mpv_set_property(m_mpv, "pause", MPV_FORMAT_FLAG, &no);
}

void PlaybackEngine::stop()
{
    const char *args[] = {"stop", nullptr};
    mpv_command(m_mpv, args);
    m_currentIndex = -1;
    m_queue.clear();
}

void PlaybackEngine::next()
{
    if (m_loopMode == LoopMode::Track) {
        loadTrack(m_currentIndex);
        return;
    }

    if (m_loopMode == LoopMode::Directory) {
        QString nextPath = m_currentIndex + 1 < m_queue.size()
            ? m_queue[m_currentIndex + 1] : QString();

        if (!nextPath.isEmpty() &&
            QFileInfo(nextPath).absolutePath() == m_currentDirPath) {
            loadTrack(m_currentIndex + 1);
        } else {
            QStringList files = collectAudioFiles(m_currentDirPath);
            m_queue = files;
            m_currentIndex = 0;
            loadTrack(0);
        }
        return;
    }

    if (m_currentIndex + 1 < m_queue.size())
        loadTrack(m_currentIndex + 1);
    else
        emit playbackFinished();
}

void PlaybackEngine::previous()
{
    if (m_loopMode == LoopMode::Track) {
        loadTrack(m_currentIndex);
        return;
    }
    if (m_currentIndex > 0)
        loadTrack(m_currentIndex - 1);
}

bool PlaybackEngine::isPlaying() const
{
    int pause = 0;
    mpv_get_property(m_mpv, "pause", MPV_FORMAT_FLAG, &pause);
    return !pause;
}

*/


#include "playbackengine.h"
#include <QCoreApplication>
#include <QTimer>
#include <QDir>
#include <QFileInfo>
#include <algorithm>
#include <random>

static const QStringList AUDIO_EXTENSIONS = {
    "mp3", "flac", "ogg", "opus", "wav", "aac",
    "m4a", "wma", "ape", "mpc", "aiff", "tta"
};

bool PlaybackEngine::isAudioFile(const QString &path)
{
    QFileInfo info(path);
    return AUDIO_EXTENSIONS.contains(info.suffix().toLower());
}

// QStringList PlaybackEngine::collectAudioFiles(const QString &path, bool recursive) const
// {
//     QStringList files;
//     QDir dir(path);
//     dir.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
//     dir.setSorting(QDir::Name | QDir::DirsFirst | QDir::IgnoreCase);

//     for (const QFileInfo &info : dir.entryInfoList()) {
//     if (info.isDir()) {
//         if (recursive)
//             files += collectAudioFiles(info.filePath(), true);
//     } else if (isAudioFile(info.filePath())) {
//         files.append(info.filePath());
//     }
// }

//     return files;
// }
QStringList PlaybackEngine::collectAudioFiles(const QString &path, bool recursive) const
{
    QStringList files;
    QDir dir(path);
    dir.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

    QFileInfoList entries = dir.entryInfoList();
    std::stable_sort(entries.begin(), entries.end(),
        [](const QFileInfo &a, const QFileInfo &b) {
            if (a.isDir() != b.isDir())
                return a.isDir() > b.isDir();
            // natural sort
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
        if (info.isDir()) {
            if (recursive)
                files += collectAudioFiles(info.filePath(), true);
        } else if (isAudioFile(info.filePath())) {
            files.append(info.filePath());
        }
    }

    return files;
}

PlaybackEngine::PlaybackEngine(QObject *parent)
    : QObject(parent)
{
    m_mpv = mpv_create();
    if (!m_mpv) {
        emit errorOccurred("Failed to create mpv instance");
        return;
    }

    mpv_set_option_string(m_mpv, "audio-display", "no");
    mpv_set_option_string(m_mpv, "video", "no");
    mpv_set_option_string(m_mpv, "gapless-audio", "yes");
    mpv_set_option_string(m_mpv, "audio-client-name", "redemption");

    if (mpv_initialize(m_mpv) < 0) {
        emit errorOccurred("Failed to initialize mpv");
        return;
    }

    mpv_set_wakeup_callback(m_mpv, mpvWakeupCallback, this);
    mpv_observe_property(m_mpv, 0, "pause", MPV_FORMAT_FLAG);
    mpv_observe_property(m_mpv, 0, "duration", MPV_FORMAT_DOUBLE);
}

PlaybackEngine::~PlaybackEngine()
{
    if (m_mpv)
        mpv_terminate_destroy(m_mpv);
}

void PlaybackEngine::mpvWakeupCallback(void *ctx)
{
    PlaybackEngine *engine = static_cast<PlaybackEngine*>(ctx);
    QMetaObject::invokeMethod(engine, "handleMpvEvents", Qt::QueuedConnection);
}

void PlaybackEngine::handleMpvEvents()
{
    while (true) {
        mpv_event *event = mpv_wait_event(m_mpv, 0);
        if (event->event_id == MPV_EVENT_NONE)
            break;

        if (event->event_id == MPV_EVENT_END_FILE) {
            mpv_event_end_file *ef = static_cast<mpv_event_end_file*>(event->data);
            if (ef->reason == MPV_END_FILE_REASON_EOF) {
                handleTrackEnd();
            }
        }
        // if (event->event_id == MPV_EVENT_PROPERTY_CHANGE) {
        //     mpv_event_property *prop = static_cast<mpv_event_property*>(event->data);
        //     if (QString(prop->name) == "pause" && prop->format == MPV_FORMAT_FLAG) {
        //         bool paused = *static_cast<int*>(prop->data);
        //         emit pauseStateChanged(paused);
        //     }
        // }
        if (event->event_id == MPV_EVENT_PROPERTY_CHANGE) {
            mpv_event_property *prop = static_cast<mpv_event_property*>(event->data);
            if (QString(prop->name) == "pause" && prop->format == MPV_FORMAT_FLAG) {
                bool paused = *static_cast<int*>(prop->data);
                emit pauseStateChanged(paused);
            } else if (QString(prop->name) == "duration" && prop->format == MPV_FORMAT_DOUBLE) {
                double dur = *static_cast<double*>(prop->data);
                if (dur > 0)
                    emit durationChanged(dur);
            }
        }
    }
}

void PlaybackEngine::handleTrackEnd()
{

    // loop track — just repeat
    if (m_loopMode == LoopMode::Track) {
        loadTrack(m_currentIndex);
        return;
    }

    QString currentPath = currentTrack();
    QString currentDir = QFileInfo(currentPath).absolutePath();

    // loop directory (shallow) — only immediate dir files
    if (m_loopMode == LoopMode::Directory) {
        if (m_currentIndex + 1 < m_queue.size()) {
            QString nextPath = m_queue[m_currentIndex + 1];
            QString nextDir = QFileInfo(nextPath).absolutePath();
            // if (nextDir == m_currentDirPath) {
            if (nextDir == m_loopTargetPath) {
                loadTrack(m_currentIndex + 1);
                return;
            }
        }
        // end of directory — restart from first file in dir
        QStringList files = collectAudioFiles(m_loopTargetPath, false);
        if (!files.isEmpty()) {
            m_queue = files;
            m_currentIndex = 0;
            loadTrack(0);
        }
        return;
    }

    // loop directory recursive
    if (m_loopMode == LoopMode::DirectoryRecursive) {
        if (m_currentIndex + 1 < m_queue.size()) {
            QString nextPath = m_queue[m_currentIndex + 1];
            if (nextPath.startsWith(m_loopTargetPath)) {
                loadTrack(m_currentIndex + 1);
                return;
            }
        }
        // end of recursive directory — restart
        QStringList files = collectAudioFiles(m_loopTargetPath, true);
        if (!files.isEmpty()) {
            m_queue = files;
            m_currentIndex = 0;
            loadTrack(0);
        }
        return;
    }

    // try to advance normally
    if (m_currentIndex + 1 < m_queue.size()) {
        loadTrack(m_currentIndex + 1);
        return;
    }

    // end of queue
    if (m_loopMode == LoopMode::Queue) {
        m_currentIndex = 0;
        loadTrack(0);
        return;
    }

    // shuffle modes — reshuffle and restart
    if (m_shuffleMode == ShuffleMode::Directory) {
        QStringList files = collectAudioFiles(m_shuffleTargetPath, true);
        buildShuffledQueue(files);
        return;
    }

    if (m_shuffleMode == ShuffleMode::All) {
        QStringList files = collectAudioFiles(m_rootPath, true);
        buildShuffledQueue(files);
        return;
    }

    emit playbackFinished();


}


// void PlaybackEngine::buildShuffledQueue(const QStringList &files)
// {
//     QStringList shuffled = files;
//     std::shuffle(shuffled.begin(), shuffled.end(), std::mt19937{std::random_device{}()});
//     m_queue = shuffled;
//     m_currentIndex = 0;
//     loadTrack(0);
// }
void PlaybackEngine::buildShuffledQueue(const QStringList &files,
                                         const QString &startWith)
{
    QStringList shuffled = files;
    std::shuffle(shuffled.begin(), shuffled.end(),
                 std::mt19937{std::random_device{}()});

    if (!startWith.isEmpty()) {
        int idx = shuffled.indexOf(startWith);
        if (idx > 0) {
            shuffled.move(idx, 0);
        }
    }

    m_queue = shuffled;
    m_currentIndex = 0;
    loadTrack(0);
}

void PlaybackEngine::play(const QString &filePath)
{
    playQueue({filePath});
}

void PlaybackEngine::playQueue(const QStringList &filePaths)
{
    m_originalQueue = filePaths;
    m_originalIndex = 0;

    if (m_shuffleMode == ShuffleMode::Directory) {
        buildShuffledQueue(collectAudioFiles(m_currentDirPath, true));
    } else if (m_shuffleMode == ShuffleMode::All) {
        buildShuffledQueue(collectAudioFiles(m_rootPath, true));
    } else {
        m_queue = filePaths;
        m_currentIndex = 0;
        loadTrack(0);
    }
}

void PlaybackEngine::playFrom(const QStringList &filePaths, int index)
{
    m_originalQueue = filePaths;
    m_originalIndex = index;

    QString clickedSong = (index >= 0 && index < filePaths.size())
        ? filePaths[index] : QString();

    if (m_shuffleMode == ShuffleMode::Directory) {
        buildShuffledQueue(collectAudioFiles(m_currentDirPath, true), clickedSong);
    } else if (m_shuffleMode == ShuffleMode::All) {
        buildShuffledQueue(collectAudioFiles(m_rootPath, true), clickedSong);
    } else {
        m_queue = filePaths;
        m_currentIndex = index;
        loadTrack(index);
    }
}

void PlaybackEngine::loadTrack(int index)
{
    if (index < 0 || index >= m_queue.size())
        return;

    m_currentIndex = index;
    QString path = m_queue[index];

    QByteArray ba = path.toUtf8();
    const char *args[] = {"loadfile", ba.constData(), nullptr};
    mpv_command(m_mpv, args);

    // ensure playback starts — clears any lingering pause state
    int no = 0;
    mpv_set_property(m_mpv, "pause", MPV_FORMAT_FLAG, &no);

    emit trackChanged(path);
}

void PlaybackEngine::pause()
{
    int yes = 1;
    mpv_set_property(m_mpv, "pause", MPV_FORMAT_FLAG, &yes);
}

void PlaybackEngine::resume()
{
    int no = 0;
    mpv_set_property(m_mpv, "pause", MPV_FORMAT_FLAG, &no);
}

void PlaybackEngine::stop()
{
    const char *args[] = {"stop", nullptr};
    mpv_command(m_mpv, args);
    m_currentIndex = -1;
    m_queue.clear();
}

void PlaybackEngine::next()
{
     m_manualTrackChange = true;

    if (m_loopMode == LoopMode::Track) {
        loadTrack(m_currentIndex);
        return;
    }

    if (m_loopMode == LoopMode::Directory) {
        if (m_currentIndex + 1 < m_queue.size()) {
            QString nextPath = m_queue[m_currentIndex + 1];
            if (QFileInfo(nextPath).absolutePath() == m_loopTargetPath) {
                loadTrack(m_currentIndex + 1);
                return;
            }
        }
        QStringList files = collectAudioFiles(m_loopTargetPath, false);
        m_queue = files;
        m_currentIndex = 0;
        loadTrack(0);
        return;
    }

    // check if next song is outside loop directory
    // if (m_loopMode == LoopMode::Directory ||
    //     m_loopMode == LoopMode::DirectoryRecursive) {
    //     if (m_currentIndex + 1 < m_queue.size()) {
    //         QString nextPath = m_queue[m_currentIndex + 1];
    //         bool inDir = m_loopMode == LoopMode::Directory
    //             ? QFileInfo(nextPath).absolutePath() == m_currentDirPath
    //             : nextPath.startsWith(m_currentDirPath);
    //         if (inDir) {
    //             loadTrack(m_currentIndex + 1);
    //         } else {
    //             // wrap to start of directory
    //             QStringList files = collectAudioFiles(m_currentDirPath,
    //                 m_loopMode == LoopMode::DirectoryRecursive);
    //             m_queue = files;
    //             m_currentIndex = 0;
    //             loadTrack(0);
    //         }
    //     }
    //     return;
    if (m_loopMode == LoopMode::DirectoryRecursive) {
        if (m_currentIndex + 1 < m_queue.size()) {
            QString nextPath = m_queue[m_currentIndex + 1];
            if (nextPath.startsWith(m_loopTargetPath)) {
                loadTrack(m_currentIndex + 1);
                return;
            }
        }
        QStringList files = collectAudioFiles(m_loopTargetPath, true);
        m_queue = files;
        m_currentIndex = 0;
        loadTrack(0);
        return;
    }

    if (m_currentIndex + 1 < m_queue.size())
        loadTrack(m_currentIndex + 1);
    else if (m_loopMode == LoopMode::Queue)
        loadTrack(0);
    else
        emit playbackFinished();

}

void PlaybackEngine::previous()
{
    m_manualTrackChange = true;
    if (m_loopMode == LoopMode::Track) {
        loadTrack(m_currentIndex);
        return;
    }

    if (m_currentIndex > 0)
        loadTrack(m_currentIndex - 1);
    else if (m_loopMode == LoopMode::Queue)
        loadTrack(m_queue.size() - 1);
}

void PlaybackEngine::restoreQueue()
{
    if (m_originalQueue.isEmpty())
        return;
    QString current = currentTrack();
    m_queue = m_originalQueue;
    int idx = m_queue.indexOf(current);
    m_currentIndex = idx >= 0 ? idx : m_originalIndex;
    // don't reload the track, just restore position
}


bool PlaybackEngine::isPlaying() const
{
    int pause = 0;
    mpv_get_property(m_mpv, "pause", MPV_FORMAT_FLAG, &pause);
    return !pause;
}



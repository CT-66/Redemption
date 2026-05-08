#pragma once

#include <QObject>
#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>
#include <QVariantMap>
#include <QApplication>
#include "playbackengine.h"
#include <QTimer>

class MprisPlayer : public QObject
{
    Q_OBJECT

public:
    explicit MprisPlayer(PlaybackEngine *engine, QObject *parent = nullptr);
    void updateMetadata(const QString &title, const QString &artist,
                        const QString &filePath, const QPixmap &cover);
    void updatePlaybackStatus();
    void updatePosition();
    QVariantMap metadata() const { return m_metadata; }
    void updateDuration(double duration);

signals:
    void propertiesChanged(const QString &interface,
                           const QVariantMap &changed,
                           const QStringList &invalidated);

private:
    PlaybackEngine *m_engine = nullptr;
    QVariantMap m_metadata;
    QString m_trackId;
    QTimer *m_positionTimer = nullptr;
};

class MprisRootAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2")
    Q_PROPERTY(QString Identity READ identity)
    Q_PROPERTY(bool CanQuit READ canQuit)
    Q_PROPERTY(bool CanRaise READ canRaise)
    Q_PROPERTY(bool HasTrackList READ hasTrackList)
    Q_PROPERTY(QStringList SupportedUriSchemes READ supportedUriSchemes)
    Q_PROPERTY(QStringList SupportedMimeTypes READ supportedMimeTypes)

public:
    explicit MprisRootAdaptor(MprisPlayer *parent);

    QString identity() const { return "Redemption"; }
    bool canQuit() const { return true; }
    bool canRaise() const { return false; }
    bool hasTrackList() const { return false; }
    QStringList supportedUriSchemes() const { return {"file"}; }
    QStringList supportedMimeTypes() const { return {"audio/mpeg", "audio/flac", "audio/ogg"}; }

public slots:
    void Quit() { qApp->quit(); }
    void Raise() {}
};

class MprisPlayerAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Player")
    Q_PROPERTY(QString PlaybackStatus READ playbackStatus)
    Q_PROPERTY(QString LoopStatus READ loopStatus WRITE setLoopStatus)
    Q_PROPERTY(double Rate READ rate WRITE setRate)
    Q_PROPERTY(bool Shuffle READ shuffle WRITE setShuffle)
    Q_PROPERTY(QVariantMap Metadata READ metadata)
    Q_PROPERTY(double Volume READ volume WRITE setVolume)
    Q_PROPERTY(qlonglong Position READ position)
    Q_PROPERTY(double MinimumRate READ minimumRate)
    Q_PROPERTY(double MaximumRate READ maximumRate)
    Q_PROPERTY(bool CanGoNext READ canGoNext)
    Q_PROPERTY(bool CanGoPrevious READ canGoPrevious)
    Q_PROPERTY(bool CanPlay READ canPlay)
    Q_PROPERTY(bool CanPause READ canPause)
    Q_PROPERTY(bool CanSeek READ canSeek)
    Q_PROPERTY(bool CanControl READ canControl)

public:
    explicit MprisPlayerAdaptor(MprisPlayer *mpris, PlaybackEngine *engine);

    QString playbackStatus() const;
    QString loopStatus() const { return "None"; }
    void setLoopStatus(const QString &) {}
    double rate() const { return 1.0; }
    void setRate(double) {}
    bool shuffle() const { return false; }
    void setShuffle(bool) {}
    QVariantMap metadata() const { return m_mpris->metadata(); }
    double volume() const;
    void setVolume(double vol);
    qlonglong position() const;
    double minimumRate() const { return 1.0; }
    double maximumRate() const { return 1.0; }
    bool canGoNext() const { return true; }
    bool canGoPrevious() const { return true; }
    bool canPlay() const { return true; }
    bool canPause() const { return true; }
    bool canSeek() const { return true; }
    bool canControl() const { return true; }

public slots:
    void Next();
    void Previous();
    void Pause();
    void PlayPause();
    void Stop();
    void Play();
    void Seek(qlonglong offset);
    void SetPosition(const QDBusObjectPath &trackId, qlonglong position);
    void OpenUri(const QString &) {}

signals:
    void Seeked(qlonglong position);

private:
    MprisPlayer *m_mpris = nullptr;
    PlaybackEngine *m_engine = nullptr;
    QString m_lastCoverPath;
};

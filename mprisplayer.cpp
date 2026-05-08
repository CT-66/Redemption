#include "mprisplayer.h"
#include <QDBusConnection>
#include <QDBusMessage>
#include <QApplication>
#include <QFileInfo>
#include <QTemporaryFile>
#include <QStandardPaths>
#include <QUrl>
#include <QFile>
#include <QDir>

MprisPlayer::MprisPlayer(PlaybackEngine *engine, QObject *parent)
    : QObject(parent), m_engine(engine)
{
    new MprisRootAdaptor(this);
    new MprisPlayerAdaptor(this, engine);

    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerService("org.mpris.MediaPlayer2.redemption");
    dbus.registerObject("/org/mpris/MediaPlayer2", this);

    m_positionTimer = new QTimer(this);
    m_positionTimer->setInterval(1000);
    connect(m_positionTimer, &QTimer::timeout, this, [this]() {
        if (m_engine->isPlaying()) {
            double pos = 0;
            mpv_get_property(m_engine->mpvHandle(), "time-pos", MPV_FORMAT_DOUBLE, &pos);
            qlonglong posUs = static_cast<qlonglong>(pos * 1e6);

            QDBusMessage signal = QDBusMessage::createSignal(
                "/org/mpris/MediaPlayer2",
                "org.freedesktop.DBus.Properties",
                "PropertiesChanged");
            QVariantMap changed;
            changed["Position"] = posUs;
            signal << "org.mpris.MediaPlayer2.Player"
                   << changed
                   << QStringList();
            QDBusConnection::sessionBus().send(signal);
        }
    });
    m_positionTimer->start();
}

// void MprisPlayer::updateMetadata(const QString &title, const QString &artist,
//                                   const QString &filePath, const QPixmap &cover)
// {
//     m_trackId = "/org/musicplayer/track/" +
//         QString::number(qHash(filePath));

//     m_metadata.clear();
//     m_metadata["mpris:trackid"] = QVariant::fromValue(
//         QDBusObjectPath(m_trackId));
//     m_metadata["xesam:title"] = title;
//     if (!artist.isEmpty())
//         m_metadata["xesam:artist"] = QStringList{artist};
//     m_metadata["xesam:url"] = QUrl::fromLocalFile(filePath).toString();

//     // save cover art to temp file for MPRIS
//     if (!cover.isNull()) {
//         QString coverPath = QStandardPaths::writableLocation(
//             QStandardPaths::TempLocation) + "/musicplayer_cover_" +
//             QString::number(qHash(filePath)) + ".png";
//         if (!QFile::exists(coverPath))
//             cover.save(coverPath);
//         m_metadata["mpris:artUrl"] = QUrl::fromLocalFile(coverPath).toString();
//     }

//     // notify properties changed
//     QVariantMap changed;
//     changed["Metadata"] = m_metadata;
//     changed["PlaybackStatus"] = m_engine->isPlaying() ? "Playing" : "Paused";

//     QDBusMessage signal = QDBusMessage::createSignal(
//         "/org/mpris/MediaPlayer2",
//         "org.freedesktop.DBus.Properties",
//         "PropertiesChanged");
//     signal << "org.mpris.MediaPlayer2.Player"
//            << changed
//            << QStringList();
//     QDBusConnection::sessionBus().send(signal);
// }

void MprisPlayer::updateMetadata(const QString &title, const QString &artist,
                                  const QString &filePath, const QPixmap &cover)
{
    m_trackId = "/org/redemption/track/" +
        QString::number(qHash(filePath));

    m_metadata.clear();
    m_metadata["mpris:trackid"] = QVariant::fromValue(
        QDBusObjectPath(m_trackId));
    m_metadata["xesam:title"] = title;
    if (!artist.isEmpty())
        m_metadata["xesam:artist"] = QStringList{artist};
    m_metadata["xesam:url"] = QUrl::fromLocalFile(filePath).toString();

    // add duration in microseconds
    double duration = 0;
    mpv_get_property(m_engine->mpvHandle(), "duration", MPV_FORMAT_DOUBLE, &duration);
    if (duration > 0)
        m_metadata["mpris:length"] = static_cast<qlonglong>(duration * 1e6);

    // cover art
    // if (!cover.isNull()) {
    //     QString coverPath = QStandardPaths::writableLocation(
    //         QStandardPaths::TempLocation) + "/musicplayer_cover_" +
    //         QString::number(qHash(filePath)) + ".png";
    //     if (!QFile::exists(coverPath))
    //         cover.save(coverPath);
    //     m_metadata["mpris:artUrl"] = QUrl::fromLocalFile(coverPath).toString();
    // }
    if (!cover.isNull()) {
        QString cacheDir = QStandardPaths::writableLocation(
            QStandardPaths::TempLocation) + "/Redemption";
        QDir().mkpath(cacheDir);

        QString coverPath = cacheDir + "/cover_" +
            QString::number(qHash(filePath)) + ".png";
        if (!QFile::exists(coverPath))
            cover.save(coverPath);
        m_metadata["mpris:artUrl"] = QUrl::fromLocalFile(coverPath).toString();
    }

    // notify properties changed
    QVariantMap changed;
    changed["Metadata"] = m_metadata;
    changed["PlaybackStatus"] = m_engine->isPlaying() ? "Playing" : "Paused";

    QDBusMessage signal = QDBusMessage::createSignal(
        "/org/mpris/MediaPlayer2",
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged");
    signal << "org.mpris.MediaPlayer2.Player"
           << changed
           << QStringList();
    QDBusConnection::sessionBus().send(signal);
}

void MprisPlayer::updatePlaybackStatus()
{
    QVariantMap changed;
    changed["PlaybackStatus"] = m_engine->isPlaying() ? "Playing" : "Paused";

    QDBusMessage signal = QDBusMessage::createSignal(
        "/org/mpris/MediaPlayer2",
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged");
    signal << "org.mpris.MediaPlayer2.Player"
           << changed
           << QStringList();
    QDBusConnection::sessionBus().send(signal);
}

void MprisPlayer::updatePosition()
{
    double pos = 0;
    mpv_get_property(m_engine->mpvHandle(), "time-pos", MPV_FORMAT_DOUBLE, &pos);
    qlonglong posUs = static_cast<qlonglong>(pos * 1e6);

    QDBusMessage signal = QDBusMessage::createSignal(
        "/org/mpris/MediaPlayer2",
        "org.mpris.MediaPlayer2.Player",
        "Seeked");
    signal << posUs;
    QDBusConnection::sessionBus().send(signal);
}

// --- MprisRootAdaptor ---

MprisRootAdaptor::MprisRootAdaptor(MprisPlayer *parent)
    : QDBusAbstractAdaptor(parent) {}

// --- MprisPlayerAdaptor ---

MprisPlayerAdaptor::MprisPlayerAdaptor(MprisPlayer *mpris, PlaybackEngine *engine)
    : QDBusAbstractAdaptor(mpris), m_mpris(mpris), m_engine(engine) {}

QString MprisPlayerAdaptor::playbackStatus() const
{
    return m_engine->isPlaying() ? "Playing" : "Paused";
}

double MprisPlayerAdaptor::volume() const
{
    double vol = 0;
    mpv_get_property(m_engine->mpvHandle(), "volume", MPV_FORMAT_DOUBLE, &vol);
    return vol / 100.0;
}

void MprisPlayerAdaptor::setVolume(double vol)
{
    double v = vol * 100.0;
    mpv_set_property(m_engine->mpvHandle(), "volume", MPV_FORMAT_DOUBLE, &v);
}

qlonglong MprisPlayerAdaptor::position() const
{
    double pos = 0;
    mpv_get_property(m_engine->mpvHandle(), "time-pos", MPV_FORMAT_DOUBLE, &pos);
    return static_cast<qlonglong>(pos * 1e6);
}

void MprisPlayerAdaptor::Next() { m_engine->next(); }
void MprisPlayerAdaptor::Previous() { m_engine->previous(); }
void MprisPlayerAdaptor::Pause() { m_engine->pause(); }
void MprisPlayerAdaptor::Play() { m_engine->resume(); }
void MprisPlayerAdaptor::Stop() { m_engine->stop(); }

void MprisPlayerAdaptor::PlayPause()
{
    if (m_engine->isPlaying())
        m_engine->pause();
    else
        m_engine->resume();
}

void MprisPlayerAdaptor::Seek(qlonglong offset)
{
    double pos = 0;
    mpv_get_property(m_engine->mpvHandle(), "time-pos", MPV_FORMAT_DOUBLE, &pos);
    pos += offset / 1e6;
    const QString cmd = QString::number(pos, 'f', 2);
    QByteArray ba = cmd.toUtf8();
    const char *args[] = {"seek", ba.constData(), "absolute", nullptr};
    mpv_command(m_engine->mpvHandle(), args);
}

void MprisPlayerAdaptor::SetPosition(const QDBusObjectPath &, qlonglong position)
{
    double pos = position / 1e6;
    const QString cmd = QString::number(pos, 'f', 2);
    QByteArray ba = cmd.toUtf8();
    const char *args[] = {"seek", ba.constData(), "absolute", nullptr};
    mpv_command(m_engine->mpvHandle(), args);
}


void MprisPlayer::updateDuration(double duration)
{
    m_metadata["mpris:length"] = static_cast<qlonglong>(duration * 1e6);

    QVariantMap changed;
    changed["Metadata"] = m_metadata;

    QDBusMessage signal = QDBusMessage::createSignal(
        "/org/mpris/MediaPlayer2",
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged");
    signal << "org.mpris.MediaPlayer2.Player"
           << changed
           << QStringList();
    QDBusConnection::sessionBus().send(signal);
}

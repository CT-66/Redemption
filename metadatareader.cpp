
#include "metadatareader.h"

#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/id3v2tag.h>
#include <taglib/mpegfile.h>
#include <taglib/flacfile.h>
#include <taglib/flacpicture.h>
#include <taglib/vorbisfile.h>
#include <taglib/xiphcomment.h>
#include <taglib/mp4file.h>
#include <taglib/mp4tag.h>
#include <taglib/asffile.h>
#include <taglib/aifffile.h>

#include <QBuffer>

TrackMetadata MetadataReader::read(const QString &filePath)
{
    TrackMetadata meta;
    QByteArray pathBytes = filePath.toLocal8Bit();

    TagLib::FileRef f(pathBytes.constData());
    if (f.isNull() || !f.tag())
        return meta;

    TagLib::Tag *tag = f.tag();

    QString title = QString::fromStdString(tag->title().to8Bit(true));
    QString artist = QString::fromStdString(tag->artist().to8Bit(true));

    if (!title.isEmpty()) {
        meta.title = title;
        meta.hasTitle = true;
    }

    if (!artist.isEmpty()) {
        meta.artist = artist;
        meta.hasArtist = true;
    }

    // Try cover art — FLAC first
    if (TagLib::FLAC::File *flacFile = dynamic_cast<TagLib::FLAC::File*>(f.file())) {
        const TagLib::List<TagLib::FLAC::Picture*> &pics = flacFile->pictureList();
        if (!pics.isEmpty()) {
            TagLib::FLAC::Picture *pic = pics.front();
            QByteArray data(pic->data().data(), pic->data().size());
            QPixmap pixmap;
            if (pixmap.loadFromData(data)) {
                meta.cover = pixmap;
                meta.hasCover = true;
            }
        }
    }
    // MP3 ID3v2
    else if (TagLib::MPEG::File *mpegFile = dynamic_cast<TagLib::MPEG::File*>(f.file())) {
        TagLib::ID3v2::Tag *id3tag = mpegFile->ID3v2Tag();
        if (id3tag) {
            TagLib::ID3v2::FrameList frames = id3tag->frameListMap()["APIC"];
            if (!frames.isEmpty()) {
                auto *frame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(frames.front());
                if (frame) {
                    QByteArray data(frame->picture().data(), frame->picture().size());
                    QPixmap pixmap;
                    if (pixmap.loadFromData(data)) {
                        meta.cover = pixmap;
                        meta.hasCover = true;
                    }
                }
            }
        }
    }
    // OGG Vorbis
    else if (TagLib::Ogg::Vorbis::File *oggFile = dynamic_cast<TagLib::Ogg::Vorbis::File*>(f.file())) {
        TagLib::Ogg::XiphComment *xiph = oggFile->tag();
        if (xiph) {
            const TagLib::List<TagLib::FLAC::Picture*> &pics = xiph->pictureList();
            if (!pics.isEmpty()) {
                TagLib::FLAC::Picture *pic = pics.front();
                QByteArray data(pic->data().data(), pic->data().size());
                QPixmap pixmap;
                if (pixmap.loadFromData(data)) {
                    meta.cover = pixmap;
                    meta.hasCover = true;
                }
            }
        }
    }

    // m4a files
    else if (TagLib::MP4::File *mp4File = dynamic_cast<TagLib::MP4::File*>(f.file())) {
        TagLib::MP4::Tag *tag = mp4File->tag();
        if (tag) {
            TagLib::MP4::ItemMap items = tag->itemMap();
            if (items.contains("covr")) {
                TagLib::MP4::CoverArtList covers = items["covr"].toCoverArtList();
                if (!covers.isEmpty()) {
                    TagLib::MP4::CoverArt cover = covers.front();
                    QByteArray data(cover.data().data(), cover.data().size());
                    QPixmap pixmap;
                    if (pixmap.loadFromData(data)) {
                        meta.cover = pixmap;
                        meta.hasCover = true;
                    }
                }
            }
        }
    }

    // other formats
    else if (TagLib::ASF::File *asfFile = dynamic_cast<TagLib::ASF::File*>(f.file())) {
        TagLib::ASF::Tag *tag = asfFile->tag();
        if (tag) {
            TagLib::ASF::AttributeListMap attrs = tag->attributeListMap();
            if (attrs.contains("WM/Picture")) {
                TagLib::ASF::AttributeList list = attrs["WM/Picture"];
                if (!list.isEmpty()) {
                    TagLib::ASF::Picture pic = list.front().toPicture();
                    QByteArray data(pic.picture().data(), pic.picture().size());
                    QPixmap pixmap;
                    if (pixmap.loadFromData(data)) {
                        meta.cover = pixmap;
                        meta.hasCover = true;
                    }
                }
            }
        }
    }

    else if (TagLib::RIFF::AIFF::File *aiffFile =
             dynamic_cast<TagLib::RIFF::AIFF::File*>(f.file())) {
        TagLib::ID3v2::Tag *id3tag = aiffFile->tag();
        if (id3tag) {
            TagLib::ID3v2::FrameList frames = id3tag->frameListMap()["APIC"];
            if (!frames.isEmpty()) {
                auto *frame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(
                    frames.front());
                if (frame) {
                    QByteArray data(frame->picture().data(), frame->picture().size());
                    QPixmap pixmap;
                    if (pixmap.loadFromData(data)) {
                        meta.cover = pixmap;
                        meta.hasCover = true;
                    }
                }
            }
        }
    }

    return meta;
}

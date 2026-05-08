
#pragma once

#include <QString>
#include <QPixmap>

struct TrackMetadata {
    QString title;
    QString artist;
    QPixmap cover;
    bool hasTitle = false;
    bool hasArtist = false;
    bool hasCover = false;
};

class MetadataReader
{
public:
    static TrackMetadata read(const QString &filePath);
};

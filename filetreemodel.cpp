#include "filetreemodel.h"
#include "metadatareader.h"

#include <QDir>
#include <QFileInfo>
#include <QIcon>

#include <taglib/fileref.h>
#include <taglib/audioproperties.h>

static const QStringList AUDIO_EXTENSIONS = {
    "mp3", "flac", "ogg", "opus", "wav", "aac",
    "m4a", "wma", "ape", "mpc", "aiff", "tta"
};

// --- MetadataLoaderThread ---

void MetadataLoaderThread::run()
{
    for (const QString &path : m_files) {
        if (isInterruptionRequested())
            break;

        TrackMetadata meta = MetadataReader::read(path);

        QString title;
        if (meta.hasTitle)
            title = meta.title;

        QString duration;
        // read duration via taglib
        TagLib::FileRef f(path.toLocal8Bit().constData());
        if (!f.isNull() && f.audioProperties()) {
            int secs = f.audioProperties()->lengthInSeconds();
            duration = QString("%1:%2")
                .arg(secs / 60)
                .arg(secs % 60, 2, 10, QChar('0'));
        }

        emit metadataReady(path, title, duration);
    }
}

// --- naturalLessThan ---

static bool naturalLessThan(const QString &a, const QString &b)
{
    int i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (a[i].isDigit() && b[j].isDigit()) {
            QString numA, numB;
            while (i < a.size() && a[i].isDigit()) numA += a[i++];
            while (j < b.size() && b[j].isDigit()) numB += b[j++];
            if (numA != numB)
                return numA.toInt() < numB.toInt();
        } else {
            if (a[i].toLower() != b[j].toLower())
                return a[i].toLower() < b[j].toLower();
            ++i; ++j;
        }
    }
    return a.size() < b.size();
}

// --- FileTreeModel ---

FileTreeModel::FileTreeModel(const QString &rootPath, QObject *parent)
    : QAbstractItemModel(parent)
{
    m_root = new FileNode();
    m_root->path = rootPath;
    m_root->displayName = rootPath;
    m_root->isDir = true;

    populateNode(m_root);
}

FileTreeModel::~FileTreeModel()
{
    if (m_loaderThread) {
        m_loaderThread->requestInterruption();
        m_loaderThread->wait();
    }
    delete m_root;
}

void FileTreeModel::populateNode(FileNode *node)
{
    QDir dir(node->path);
    dir.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    dir.setSorting(QDir::Name | QDir::DirsFirst | QDir::IgnoreCase);

    for (const QFileInfo &info : dir.entryInfoList()) {
        if (info.isDir()) {
            FileNode *child = new FileNode();
            child->path = info.filePath();
            child->displayName = info.fileName();
            child->isDir = true;
            child->parent = node;
            populateNode(child);

            if (!child->children.isEmpty())
                node->children.append(child);
            else
                delete child;

        } else if (isAudioFile(info.filePath())) {
            FileNode *child = new FileNode();
            child->path = info.filePath();
            child->displayName = QFileInfo(info.filePath()).fileName();
            child->isDir = false;
            child->parent = node;

            // register in path index for fast lookup
            m_pathIndex[info.filePath()] = child;

            node->children.append(child);
        }
    }

    sortChildren(node);
}

void FileTreeModel::startMetadataLoading()
{
    // collect all audio files
    QStringList files = collectAudioFiles(m_root->path);

    m_loaderThread = new MetadataLoaderThread(files, this);

    connect(m_loaderThread, &MetadataLoaderThread::metadataReady,
            this, &FileTreeModel::onMetadataReady,
            Qt::QueuedConnection);

    m_loaderThread->start(QThread::LowPriority);
}

void FileTreeModel::onMetadataReady(const QString &path, const QString &title,
                                     const QString &duration)
{
    FileNode *node = m_pathIndex.value(path, nullptr);
    if (!node)
        return;

    bool changed = false;

    if (!title.isEmpty() && node->displayName != title) {
        node->displayName = title;
        changed = true;
    }

    if (!duration.isEmpty() && node->duration != duration) {
        node->duration = duration;
        changed = true;
    }

    node->metadataLoaded = true;

    if (changed) {
        QModelIndex idx = indexForPath(path);
        if (idx.isValid())
            emit dataChanged(idx, idx.siblingAtColumn(1));
    }
}

void FileTreeModel::sortChildren(FileNode *node)
{
    std::stable_sort(node->children.begin(), node->children.end(),
        [](const FileNode *a, const FileNode *b) {
            if (a->isDir != b->isDir)
                return a->isDir > b->isDir;
            return naturalLessThan(QFileInfo(a->path).fileName().toLower(),
                                   QFileInfo(b->path).fileName().toLower());
        });
}

bool FileTreeModel::isAudioFile(const QString &path)
{
    QFileInfo info(path);
    return AUDIO_EXTENSIONS.contains(info.suffix().toLower());
}

QStringList FileTreeModel::collectAudioFiles(const QString &path) const
{
    QStringList files;
    QDir dir(path);
    dir.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    dir.setSorting(QDir::Name | QDir::DirsFirst | QDir::IgnoreCase);

    QFileInfoList entries = dir.entryInfoList();

    std::stable_sort(entries.begin(), entries.end(),
        [](const QFileInfo &a, const QFileInfo &b) {
            if (a.isDir() != b.isDir())
                return a.isDir() > b.isDir();
            return naturalLessThan(a.fileName().toLower(), b.fileName().toLower());
        });

    for (const QFileInfo &info : entries) {
        if (info.isDir())
            files += collectAudioFiles(info.filePath());
        else if (isAudioFile(info.filePath()))
            files.append(info.filePath());
    }

    return files;
}

QStringList FileTreeModel::buildQueueFrom(FileNode *node) const
{
    if (node->isDir)
        return collectAudioFiles(node->path);

    QStringList queue;
    queue.append(node->path);

    FileNode *current = node;
    FileNode *parent = node->parent;

    while (parent) {
        int idx = parent->children.indexOf(current);

        for (int i = idx + 1; i < parent->children.size(); ++i) {
            FileNode *sibling = parent->children[i];
            if (sibling->isDir)
                queue += collectAudioFiles(sibling->path);
            else
                queue.append(sibling->path);
        }

        current = parent;
        parent = parent->parent;
    }

    return queue;
}

QModelIndex FileTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    FileNode *parentNode = parent.isValid()
        ? static_cast<FileNode*>(parent.internalPointer())
        : m_root;

    FileNode *child = parentNode->children.value(row);
    if (child)
        return createIndex(row, column, child);

    return QModelIndex();
}

QModelIndex FileTreeModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();

    FileNode *node = static_cast<FileNode*>(child.internalPointer());
    FileNode *parentNode = node->parent;

    if (!parentNode || parentNode == m_root)
        return QModelIndex();

    FileNode *grandparent = parentNode->parent;
    if (!grandparent)
        return QModelIndex();

    int row = grandparent->children.indexOf(parentNode);
    return createIndex(row, 0, parentNode);
}

QModelIndex FileTreeModel::indexForPath(const QString &path) const
{
    return indexForNode(m_root, path, QModelIndex());
}

QModelIndex FileTreeModel::indexForNode(FileNode *node, const QString &path,
                                         const QModelIndex &parent) const
{
    for (int i = 0; i < node->children.size(); ++i) {
        FileNode *child = node->children[i];
        QModelIndex childIndex = index(i, 0, parent);
        if (child->path == path)
            return childIndex;
        if (child->isDir) {
            QModelIndex result = indexForNode(child, path, childIndex);
            if (result.isValid())
                return result;
        }
    }
    return QModelIndex();
}

int FileTreeModel::rowCount(const QModelIndex &parent) const
{
    FileNode *node = parent.isValid()
        ? static_cast<FileNode*>(parent.internalPointer())
        : m_root;

    return node->children.size();
}

int FileTreeModel::columnCount(const QModelIndex &) const
{
    return 2;
}

QVariant FileTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    FileNode *node = static_cast<FileNode*>(index.internalPointer());

    if (role == Qt::DisplayRole) {
        if (index.column() == 0)
            return node->displayName;
        if (index.column() == 1 && !node->isDir && !node->duration.isEmpty())
            return node->duration;
        return QVariant();
    }

    // if (role == Qt::DecorationRole && index.column() == 0)
    //     return node->isDir
    //         ? QIcon::fromTheme("folder")
    //         : QIcon::fromTheme("audio-x-generic");

    //     if (role == Qt::DecorationRole && index.column() == 0) {
    //     if (node->isDir) {
    //         bool expanded = m_treeView && m_treeView->isExpanded(index);
    //         // return QIcon::fromTheme(expanded ? "folder-open" : "folder");
    //         return QIcon::fromTheme(expanded ? "folder" : "folder-open");
    //     }
    //     return QIcon::fromTheme("audio-x-generic");
    // }
    if (role == Qt::DecorationRole && index.column() == 0) {
        if (node->isDir) {
            bool expanded = m_treeView && m_treeView->isExpanded(index);
            QIcon base = expanded
                ? QIcon::fromTheme("folder")
                : QIcon::fromTheme("folder-open");

            // loop directory overlay
            if ((m_loopMode == (int)LoopMode::Directory ||
                 m_loopMode == (int)LoopMode::DirectoryRecursive) &&
                node->path == m_loopPath) {
                return QIcon::fromTheme("media-playlist-repeat");
            }
            // shuffle directory overlay
            if (m_shuffleMode == (int)ShuffleMode::Directory && node->path == m_shufflePath) {
                return QIcon::fromTheme("media-playlist-shuffle");
            }
            return base;
        }

        // loop track overlay
        if (m_loopMode == (int)LoopMode::Track && node->path == m_loopPath) {
            return QIcon::fromTheme("media-playlist-repeat-song");
        }
        // return QIcon::fromTheme("audio-x-generic");
        return QIcon::fromTheme("library-music-symbolic");
    }

    return QVariant();

}

FileNode *FileTreeModel::nodeFromIndex(const QModelIndex &index) const
{
    if (!index.isValid())
        return m_root;
    return static_cast<FileNode*>(index.internalPointer());
}


void FileTreeModel::setLoopState(int loopMode, const QString &loopPath)
{
    m_loopMode = loopMode;
    m_loopPath = loopPath;
}

void FileTreeModel::setShuffleState(int shuffleMode, const QString &shufflePath)
{
    m_shuffleMode = shuffleMode;
    m_shufflePath = shufflePath;
}

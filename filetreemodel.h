#pragma once

#include <QAbstractItemModel>
#include <QFileInfo>
#include <QList>
#include <QString>
#include <QThread>
#include <QTreeView>

#include "playbackengine.h"

struct FileNode {
    QString path;
    QString displayName;
    QString duration;
    bool isDir;
    bool metadataLoaded = false;
    FileNode *parent = nullptr;
    QList<FileNode*> children;

    ~FileNode() {
        qDeleteAll(children);
    }
};

class MetadataLoaderThread : public QThread
{
    Q_OBJECT
public:
    explicit MetadataLoaderThread(const QStringList &files, QObject *parent = nullptr)
        : QThread(parent), m_files(files) {}

    void run() override;

signals:
    void metadataReady(const QString &path, const QString &title, const QString &duration);


private:
    QStringList m_files;
};

class FileTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit FileTreeModel(const QString &rootPath, QObject *parent = nullptr);
    ~FileTreeModel();

    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    FileNode *nodeFromIndex(const QModelIndex &index) const;
    QStringList collectAudioFiles(const QString &path) const;
    QStringList buildQueueFrom(FileNode *node) const;
    QModelIndex indexForPath(const QString &path) const;

    static bool isAudioFile(const QString &path);

    void setLoopState(int loopMode, const QString &loopPath);
    void setShuffleState(int shuffleMode, const QString &shufflePath);

    void startMetadataLoading();

    void setTreeView(QTreeView *view) { m_treeView = view; }

private slots:
    void onMetadataReady(const QString &path, const QString &title, const QString &duration);

private:
    FileNode *m_root = nullptr;
    MetadataLoaderThread *m_loaderThread = nullptr;
    QHash<QString, FileNode*> m_pathIndex;
    QTreeView *m_treeView = nullptr;

    int m_loopMode = 0;
    QString m_loopPath;
    int m_shuffleMode = 0;
    QString m_shufflePath;

    void populateNode(FileNode *node);
    void sortChildren(FileNode *node);
    QModelIndex indexForNode(FileNode *node, const QString &path, const QModelIndex &parent) const;

};

/*

#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QListWidgetItem>
#include <QFileInfo>

struct SearchResult {
    QString path;
    QString displayName;
    QString artist;
    QString relativePath;
};

class SearchDialog : public QDialog
{
public:
    explicit SearchDialog(const QList<SearchResult> &results, QWidget *parent = nullptr)
        : QDialog(parent), m_results(results)
    {
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);

        if (parent)
            setFixedSize(parent->size());

        // inner container
        QWidget *container = new QWidget(this);
        // container->setFixedSize(500, 400);
        // container->move((width() - 500) / 2, (height() - 400) / 2);
        // container->setStyleSheet(
        //     "QWidget {"
        //     "  background: palette(window);"
        //     "  border-radius: 8px;"
        //     "  border: 1px solid palette(mid);"
        //     "}"
        // );
        container->setFixedSize(650, 60);
        container->move((width() - 650) / 2, height() / 4);
        container->setObjectName("searchContainer");
        container->setStyleSheet(
            "#searchContainer {"
            "  background: palette(window);"
            "  border-radius: 8px;"
            "  border: 1px solid palette(mid);"
            "}"
        );
        // container->setObjectName("searchContainer");
        // container->setStyleSheet(
        //     "#searchContainer {"
        //     "  background: palette(window);"
        //     "  border-radius: 8px;"
        //     "  border: 1px solid palette(mid);"
        //     "}"
        // );

        m_searchBox = new QLineEdit(container);
        m_searchBox->setPlaceholderText("Search...");
        m_searchBox->setStyleSheet(
            "QLineEdit {"
            "  border: none;"
            "  border-bottom: 1px solid palette(mid);"
            "  border-radius: 0px;"
            "  padding: 8px;"
            "  font-size: 14px;"
            "  background: transparent;"
            "}"
        );
        // m_searchBox->setFixedWidth(500);

        // m_listWidget = new QListWidget(container);
        // m_listWidget->setStyleSheet(
        //     "QListWidget {"
        //     "  border: none;"
        //     "  background: transparent;"
        //     "}"
        //     "QListWidget::item {"
        //     "  padding: 6px 8px;"
        //     "}"
        //     "QListWidget::item:selected {"
        //     "  background: palette(highlight);"
        //     "  color: palette(highlighted-text);"
        //     "}"
        // );
        m_listWidget = new QListWidget(container);
        m_listWidget->setStyleSheet(
            "QListWidget {"
            "  border: none;"
            "  background: transparent;"
            "}"
            "QListWidget::item:selected:!active {"
            "  background: palette(highlight);"
            "  color: palette(highlightedText);"
            "}"
        );

        QVBoxLayout *layout = new QVBoxLayout(container);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(m_searchBox);
        layout->addWidget(m_listWidget);

        connect(m_searchBox, &QLineEdit::textChanged,
                this, &SearchDialog::onTextChanged);
        connect(m_listWidget, &QListWidget::itemActivated,
                this, &SearchDialog::onItemActivated);

        m_searchBox->setFocus();
        // resizeToContent();
    }

    QString selectedPath() const { return m_selectedPath; }

protected:
    void mousePressEvent(QMouseEvent *e) override {
        // close if clicking outside the container
        QWidget *container = findChild<QWidget*>();
        if (container && !container->geometry().contains(e->pos()))
            reject();
        QDialog::mousePressEvent(e);
    }

    void keyPressEvent(QKeyEvent *e) override {
        if (e->key() == Qt::Key_Escape) {
            reject();
        } else if (e->key() == Qt::Key_Down) {
            m_listWidget->setFocus();
            if (m_listWidget->currentRow() < m_listWidget->count() - 1)
                m_listWidget->setCurrentRow(m_listWidget->currentRow() + 1);
        } else if (e->key() == Qt::Key_Up) {
            if (m_listWidget->currentRow() > 0)
                m_listWidget->setCurrentRow(m_listWidget->currentRow() - 1);
            else
                m_searchBox->setFocus();
        } else if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
            if (m_listWidget->currentItem())
                onItemActivated(m_listWidget->currentItem());
        } else {
            QDialog::keyPressEvent(e);
        }
    }

private:
    QLineEdit *m_searchBox = nullptr;
    QListWidget *m_listWidget = nullptr;
    QList<SearchResult> m_results;
    QString m_selectedPath;

    void resizeToContent()
    {
        int itemHeight = 36;
        int maxItems = 10;
        // int searchBoxHeight = m_searchBox->sizeHint().height();
        int searchBoxHeight = 48;
        int padding = 12;

        int itemCount = qMin(m_listWidget->count(), maxItems);
        int listHeight = itemCount * itemHeight;
        int totalHeight = searchBoxHeight + listHeight + padding;

        // totalHeight = qMax(totalHeight, searchBoxHeight + padding);

        QWidget *container = findChild<QWidget*>("searchContainer");
        if (container) {
            container->setFixedSize(650, totalHeight);
            container->move((width() - 650) / 2, height() / 4);
        }
    }
    // void resizeToContent()
    // {
    //     int itemHeight = 36;
    //     int maxItems = 10;
    //     int searchBoxHeight = 48;
    //     int padding = 12;

    //     int itemCount = qMin(m_listWidget->count(), maxItems);
    //     int listHeight = itemCount * itemHeight;
    //     int totalHeight = searchBoxHeight + listHeight + padding;

    //     QWidget *container = findChild<QWidget*>("searchContainer");
    //     if (container) {
    //         container->setFixedSize(650, totalHeight);
    //         container->move((width() - 650) / 2, height() / 4);
    //     }
    // }

    // void onTextChanged(const QString &text)
    // {
    //     m_listWidget->clear();
    //     if (text.trimmed().isEmpty())
    //         return;

    //     QString lower = text.toLower();
    //     for (const SearchResult &r : m_results) {
    //         if (r.displayName.toLower().contains(lower)) {
    //             // build display path: relative dir + display name
    //             QString dir = QFileInfo(r.relativePath).path();
    //             QString display = (dir.isEmpty() || dir == ".")
    //                 ? r.displayName
    //                 : dir + "/" + r.displayName;

    //             QListWidgetItem *item = new QListWidgetItem(display);
    //             item->setData(Qt::UserRole, r.path);
    //             m_listWidget->addItem(item);
    //         }
    //     }
    // }
    void onTextChanged(const QString &text)
    {
        m_listWidget->clear();
        if (text.trimmed().isEmpty()) {
            resizeToContent();
            return;
        }

        QString lower = text.toLower();
        QList<QListWidgetItem*> titleMatches;
        QList<QListWidgetItem*> dirMatches;

        for (const SearchResult &r : m_results) {
            QString dir = QFileInfo(r.relativePath).path();
            QString display = (dir.isEmpty() || dir == ".")
                ? r.displayName
                : dir + "/" + r.displayName;

            QListWidgetItem *item = new QListWidgetItem(display);
            item->setData(Qt::UserRole, r.path);

            if (r.displayName.toLower().contains(lower)) {
                titleMatches.append(item);
            } else if (dir.toLower().contains(lower)) {
                dirMatches.append(item);
            } else {
                delete item;
            }
        }

        for (auto *item : titleMatches)
            m_listWidget->addItem(item);
        for (auto *item : dirMatches)
            m_listWidget->addItem(item);

        if (m_listWidget->count() > 0)
            m_listWidget->setCurrentRow(0);

        resizeToContent();
    }

    void onItemActivated(QListWidgetItem *item)
    {
        m_selectedPath = item->data(Qt::UserRole).toString();
        accept();
    }
};

*/


/*

#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QListWidgetItem>
#include <QFileInfo>

struct SearchResult {
    QString path;
    QString displayName;
    QString artist;
    QString relativePath;
};

class SearchDialog : public QDialog
{
public:
    explicit SearchDialog(const QList<SearchResult> &results, QWidget *parent = nullptr)
        : QDialog(parent), m_results(results)
    {
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);

        if (parent)
            setFixedSize(parent->size());

        m_container = new QWidget(this);
        m_container->setObjectName("searchContainer");
        m_container->setStyleSheet(
            "#searchContainer {"
            "  background: palette(window);"
            "  border-radius: 8px;"
            "  border: 1px solid palette(mid);"
            "}"
        );

        m_searchBox = new QLineEdit(m_container);
        m_searchBox->setPlaceholderText("Search...");
        m_searchBox->setFixedHeight(48);
        m_searchBox->setStyleSheet(
            "QLineEdit {"
            "  border: none;"
            "  border-bottom: 1px solid palette(mid);"
            "  border-radius: 0px;"
            "  padding: 8px 12px;"
            "  font-size: 15px;"
            "  background: transparent;"
            "}"
        );

        m_listWidget = new QListWidget(m_container);
        m_listWidget->setStyleSheet(
            "QListWidget {"
            "  border: none;"
            "  background: transparent;"
            "  font-size: 13px;"
            "}"
            "QListWidget::item {"
            "  padding: 6px 12px;"
            "  min-height: 28px;"
            "}"
            "QListWidget::item:selected:!active {"
            "  background: palette(highlight);"
            "  color: palette(highlightedText);"
            "}"
        );

        m_layout = new QVBoxLayout(m_container);
        m_layout->setContentsMargins(0, 0, 0, 0);
        m_layout->setSpacing(0);
        m_layout->addWidget(m_searchBox);
        m_layout->addWidget(m_listWidget);

        connect(m_searchBox, &QLineEdit::textChanged,
                this, &SearchDialog::onTextChanged);
        connect(m_listWidget, &QListWidget::itemActivated,
                this, &SearchDialog::onItemActivated);

        m_searchBox->setFocus();
        resizeToContent();
    }

    QString selectedPath() const { return m_selectedPath; }

protected:
    void mousePressEvent(QMouseEvent *e) override {
        if (m_container && !m_container->geometry().contains(e->pos()))
            reject();
        QDialog::mousePressEvent(e);
    }

    void keyPressEvent(QKeyEvent *e) override {
        if (e->key() == Qt::Key_Escape) {
            reject();
        } else if (e->key() == Qt::Key_Down) {
            if (m_searchBox->hasFocus()) {
                m_listWidget->setFocus();
            } else {
                int next = m_listWidget->currentRow() + 1;
                if (next < m_listWidget->count())
                    m_listWidget->setCurrentRow(next);
                else
                    m_listWidget->setCurrentRow(0); // wrap to top
         }}
        // } else if (e->key() == Qt::Key_Up) {
        //     if (m_listWidget->currentRow() > 0)
        //         m_listWidget->setCurrentRow(m_listWidget->currentRow() - 1);
        //     else
        //         m_searchBox->setFocus();
            else if (e->key() == Qt::Key_Up) {
            if (m_listWidget->hasFocus() && m_listWidget->currentRow() == 0) {
                m_searchBox->setFocus();
            } else if (m_listWidget->hasFocus()) {
                m_listWidget->setCurrentRow(m_listWidget->currentRow() - 1);
            }
            } else if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
                if (m_listWidget->currentItem())
                    onItemActivated(m_listWidget->currentItem());
            } else {
                QDialog::keyPressEvent(e);
            }
    }

private:
    QWidget *m_container = nullptr;
    QLineEdit *m_searchBox = nullptr;
    QListWidget *m_listWidget = nullptr;
    QVBoxLayout *m_layout = nullptr;
    QList<SearchResult> m_results;
    QString m_selectedPath;

    void resizeToContent()
    {
        int itemHeight = 40;
        int maxItems = 10;
        int searchBoxHeight = 48;

        int itemCount = qMin(m_listWidget->count(), maxItems);
        int totalHeight = searchBoxHeight + (itemCount * itemHeight);

        m_container->setFixedSize(700, totalHeight);
        m_container->move((width() - 700) / 2, height() / 4);
    }

    void onTextChanged(const QString &text)
    {
        m_listWidget->clear();

        if (!text.trimmed().isEmpty()) {
            QString lower = text.toLower();
            QList<QListWidgetItem*> titleMatches;
            QList<QListWidgetItem*> dirMatches;

            for (const SearchResult &r : m_results) {
                QString dir = QFileInfo(r.relativePath).path();
                QString display = (dir.isEmpty() || dir == ".")
                    ? r.displayName
                    : dir + "/" + r.displayName;

                QListWidgetItem *item = new QListWidgetItem(display);
                item->setData(Qt::UserRole, r.path);

                if (r.displayName.toLower().contains(lower))
                    titleMatches.append(item);
                else if (dir.toLower().contains(lower))
                    dirMatches.append(item);
                else
                    delete item;
            }

            for (auto *item : titleMatches)
                m_listWidget->addItem(item);
            for (auto *item : dirMatches)
                m_listWidget->addItem(item);

            if (m_listWidget->count() > 0)
                m_listWidget->setCurrentRow(0);
        }

        resizeToContent();
    }

    void onItemActivated(QListWidgetItem *item)
    {
        m_selectedPath = item->data(Qt::UserRole).toString();
        accept();
    }
};

*/

#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QListWidgetItem>
#include <QFileInfo>
#include <QApplication>

struct SearchResult {
    QString path;
    QString displayName;
    QString artist;
    QString relativePath;
};

class SearchDialog : public QDialog
{
public:
    explicit SearchDialog(const QList<SearchResult> &results, QWidget *parent = nullptr,
                        bool loading = false)
        : QDialog(parent), m_results(results)
    {
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);

        if (parent)
            setFixedSize(parent->size());

        m_container = new QWidget(this);
        m_container->setObjectName("searchContainer");
        m_container->setStyleSheet(
            "#searchContainer {"
            "  background: palette(window);"
            "  border-radius: 8px;"
            "  border: 1px solid palette(mid);"
            "}"
        );

        m_searchBox = new QLineEdit(m_container);
        m_searchBox->setPlaceholderText("Search...");
        m_searchBox->setFixedHeight(48);
        m_searchBox->setStyleSheet(
            "QLineEdit {"
            "  border: none;"
            "  border-bottom: 1px solid palette(mid);"
            "  border-radius: 0px;"
            "  padding: 8px 12px;"
            "  font-size: 15px;"
            "  background: transparent;"
            "}"
        );

        m_listWidget = new QListWidget(m_container);
        m_listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_listWidget->setTextElideMode(Qt::ElideRight);
        m_listWidget->setStyleSheet(
            "QListWidget {"
            "  border: none;"
            "  background: transparent;"
            "  font-size: 13px;"
            "}"
            "QListWidget::item {"
            "  padding: 6px 12px;"
            "  min-height: 28px;"
            "}"
            "QListWidget::item:selected:!active {"
            "  background: palette(highlight);"
            "  color: palette(highlightedText);"
            "}"
        );

        if (loading) {
            QListWidgetItem *item = new QListWidgetItem("Building search index...");
            item->setFlags(Qt::NoItemFlags);
            m_listWidget->addItem(item);
        }

        m_listWidget->installEventFilter(this);
        m_searchBox->installEventFilter(this);

        m_layout = new QVBoxLayout(m_container);
        m_layout->setContentsMargins(0, 0, 0, 0);
        m_layout->setSpacing(0);
        m_layout->addWidget(m_searchBox);
        m_layout->addWidget(m_listWidget);

        connect(m_searchBox, &QLineEdit::textChanged,
                this, &SearchDialog::onTextChanged);
        connect(m_listWidget, &QListWidget::itemActivated,
                this, &SearchDialog::onItemActivated);

        m_searchBox->setFocus();
        resizeToContent();
    }

    QString selectedPath() const { return m_selectedPath; }

protected:
    bool eventFilter(QObject *obj, QEvent *event) override
    {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *key = static_cast<QKeyEvent*>(event);

            // ctrl+j/k work from both searchbox and listwidget
            if (key->key() == Qt::Key_J && key->modifiers() & Qt::ControlModifier) {
                int next = m_listWidget->currentRow() + 1;
                m_listWidget->setCurrentRow(next < m_listWidget->count() ? next : 0);
                return true;
            }
            if (key->key() == Qt::Key_K && key->modifiers() & Qt::ControlModifier) {
                int prev = m_listWidget->currentRow() - 1;
                m_listWidget->setCurrentRow(prev >= 0 ? prev : m_listWidget->count() - 1);
                return true;
            }
        }

        if (obj == m_listWidget && event->type() == QEvent::KeyPress) {
            QKeyEvent *key = static_cast<QKeyEvent*>(event);

            if (key->key() == Qt::Key_Down) {
                int next = m_listWidget->currentRow() + 1;
                m_listWidget->setCurrentRow(next < m_listWidget->count() ? next : 0);
                return true;
            }
            if (key->key() == Qt::Key_Up) {
                int prev = m_listWidget->currentRow() - 1;
                m_listWidget->setCurrentRow(prev >= 0 ? prev : m_listWidget->count() - 1);
                return true;
            }
            if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) {
                if (m_listWidget->currentItem())
                    onItemActivated(m_listWidget->currentItem());
                return true;
            }
            if (key->key() == Qt::Key_Escape) {
                reject();
                return true;
            }
            // redirect any printable key back to search box
            if (!key->text().isEmpty() && key->modifiers() == Qt::NoModifier) {
                m_searchBox->setFocus();
                QApplication::sendEvent(m_searchBox, event);
                return true;
            }
        }
        return QDialog::eventFilter(obj, event);
    }

    void mousePressEvent(QMouseEvent *e) override {
        if (m_container && !m_container->geometry().contains(e->pos()))
            reject();
        QDialog::mousePressEvent(e);
    }

    void keyPressEvent(QKeyEvent *e) override {
        if (e->key() == Qt::Key_Escape) {
            reject();
        // } else if (e->key() == Qt::Key_Down) {
        //     m_listWidget->setFocus();
        // } else if (e->key() == Qt::Key_Down) {
        //     if (m_listWidget->count() > 0) {
        //         m_listWidget->setCurrentRow(0);
        //         m_listWidget->setFocus();
        } else if (e->key() == Qt::Key_Down) {
            m_listWidget->setFocus();
            if (m_listWidget->currentRow() < m_listWidget->count() - 1)
                m_listWidget->setCurrentRow(m_listWidget->currentRow() + 1);
        } else if (e->key() == Qt::Key_Up) {
            if (m_listWidget->count() > 0) {
                m_listWidget->setFocus();
                m_listWidget->setCurrentRow(m_listWidget->count() - 1);
            }
        } else if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
            if (m_listWidget->currentItem())
                onItemActivated(m_listWidget->currentItem());
        } else {
            QDialog::keyPressEvent(e);
        }
    }

private:
    QWidget *m_container = nullptr;
    QLineEdit *m_searchBox = nullptr;
    QListWidget *m_listWidget = nullptr;
    QVBoxLayout *m_layout = nullptr;
    QList<SearchResult> m_results;
    QString m_selectedPath;

    void resizeToContent()
    {
        int itemHeight = 40;
        int maxItems = 10;
        int searchBoxHeight = 48;

        int itemCount = qMin(m_listWidget->count(), maxItems);
        int totalHeight = searchBoxHeight + (itemCount * itemHeight);

        m_container->setFixedSize(700, totalHeight);
        m_container->move((width() - 700) / 2, height() / 4);
    }

    void onTextChanged(const QString &text)
    {
        m_listWidget->clear();

        if (!text.trimmed().isEmpty()) {
            QString lower = text.toLower();
            QList<QListWidgetItem*> titleMatches;
            QList<QListWidgetItem*> dirMatches;

            for (const SearchResult &r : m_results) {
                QString dir = QFileInfo(r.relativePath).path();
                QString display = (dir.isEmpty() || dir == ".")
                    ? r.displayName
                    : dir + "/" + r.displayName;

                QListWidgetItem *item = new QListWidgetItem(display);
                item->setData(Qt::UserRole, r.path);
                item->setToolTip(display);

                if (r.displayName.toLower().contains(lower))
                    titleMatches.append(item);
                else if (dir.toLower().contains(lower))
                    dirMatches.append(item);
                else
                    delete item;
            }

            for (auto *item : titleMatches)
                m_listWidget->addItem(item);
            for (auto *item : dirMatches)
                m_listWidget->addItem(item);

            if (m_listWidget->count() > 0)
                m_listWidget->setCurrentRow(0);
        }

        resizeToContent();
    }

    void onItemActivated(QListWidgetItem *item)
    {
        m_selectedPath = item->data(Qt::UserRole).toString();
        accept();
    }
};

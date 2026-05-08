
#pragma once

#include <QDialog>
#include <QLabel>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QPixmap>

class CoverArtDialog : public QDialog
{
public:
    explicit CoverArtDialog(const QPixmap &pixmap, QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);

        // make dialog fill the parent window
        if (parent)
            setFixedSize(parent->size());

        QLabel *label = new QLabel(this);
        QPixmap scaled = pixmap.scaled(400, 400, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        label->setPixmap(scaled);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet(
            "background: palette(window);"
            "border-radius: 8px;"
            "padding: 8px;"
        );
        label->adjustSize();

        // center the label within the dialog
        label->move(
            (width() - label->width()) / 2,
            (height() - label->height()) / 2
        );
    }

protected:
    void mousePressEvent(QMouseEvent *) override {
        accept();
    }
};

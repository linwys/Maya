#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "player/track.hpp"

namespace ui {

class MetadataEditorDialog : public QDialog {
    Q_OBJECT
public:
    explicit MetadataEditorDialog(const player::Track& track, QWidget* parent = nullptr);
    ~MetadataEditorDialog() override = default;

    QString title() const;
    QString artist() const;
    QString album() const;
    QString cover_path() const { return m_cover_path; }

private:
    QLineEdit* m_title_input{nullptr};
    QLineEdit* m_artist_input{nullptr};
    QLineEdit* m_album_input{nullptr};
    QLabel* m_cover_label{nullptr};
    QString m_cover_path;
};

}
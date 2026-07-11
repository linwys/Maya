#pragma once
#include <QAbstractTableModel>
#include <vector>
#include "player/track.hpp"

namespace ui {

class TrackModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit TrackModel(QObject* parent = nullptr);
    ~TrackModel() override = default;

    void set_tracks(const std::vector<player::Track>* tracks);
    const player::Track& get_track(int row) const;
    const std::vector<player::Track>* all_tracks() const { return m_raw_tracks; }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;
    void set_search_query(const QString& query);

private:
    void apply_filter_and_sort();

    const std::vector<player::Track>* m_raw_tracks{nullptr};
    std::vector<size_t> m_indices;
    QString m_search_query;
    int m_sort_column{-1};
    Qt::SortOrder m_sort_order{Qt::AscendingOrder};
}; //track_model

} //ui

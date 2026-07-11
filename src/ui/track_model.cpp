#include "track_model.hpp"
#include "icons.hpp"

namespace ui {
static const QIcon& get_heart_filled() {
    static QIcon icon = icons::from_svg(icons::heart_filled, QColor("#ff4d4d"));
    return icon;
}

static const QIcon& get_heart_empty() {
    static QIcon icon = icons::from_svg(icons::heart, QColor("#8c8c8c"));
    return icon;
}

TrackModel::TrackModel(QObject* parent) : QAbstractTableModel(parent) {}

void TrackModel::set_tracks(const std::vector<player::Track>* tracks) {
    beginResetModel();
    m_raw_tracks = tracks;
    apply_filter_and_sort();
    endResetModel();
}

const player::Track& TrackModel::get_track(int row) const {
    return (*m_raw_tracks)[m_indices[row]];
}

int TrackModel::rowCount(const QModelIndex&) const {
    return static_cast<int>(m_indices.size());
}

int TrackModel::columnCount(const QModelIndex&) const {
    return 5;
}

QVariant TrackModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || !m_raw_tracks || index.row() >= static_cast<int>(m_indices.size())) {
        return QVariant();
    }

    const auto& track = (*m_raw_tracks)[m_indices[index.row()]];
    if (role == Qt::UserRole && index.column() == 0) {
        return track.id.toString();
    }

    switch (index.column()) {
        case 0:
            if (role == Qt::DisplayRole) {
                return QString::asprintf("%02d", index.row() + 1);
            }
            break;
        case 1:
            if (role == Qt::DisplayRole) {
                return track.title;
            } else if (role == Qt::UserRole) {
                return track.artist;
            } else if (role == Qt::UserRole + 1) {
                return track.file_path;
            }
            break;
        case 2:
            if (role == Qt::DisplayRole) {
                return track.album;
            }
            break;
        case 3:
            if (role == Qt::DecorationRole) {
                return track.is_favorite ? get_heart_filled() : get_heart_empty();
            }
            break;
        case 4:
            if (role == Qt::DisplayRole) {
                int secs = static_cast<int>(track.duration.count());
                return QString::asprintf("%d:%02d", secs / 60, secs % 60);
            }
            break;
    }
    return QVariant();
}

QVariant TrackModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case 0: return "#";
            case 1: return "TITLE";
            case 2: return "ALBUM";
            case 3: return "";//fill
            case 4: return "DURATION";
        }
    }
    return QVariant();
}

void TrackModel::sort(int column, Qt::SortOrder order) {
    m_sort_column = column;
    m_sort_order = order;
    beginResetModel();
    apply_filter_and_sort();
    endResetModel();
}

void TrackModel::set_search_query(const QString& query) {
    m_search_query = query.trimmed();
    beginResetModel();
    apply_filter_and_sort();
    endResetModel();
}

void TrackModel::apply_filter_and_sort() {
    m_indices.clear();
    if (!m_raw_tracks) return;

    for (size_t i = 0; i < m_raw_tracks->size(); ++i) {
        if (!m_search_query.isEmpty()) {
            const auto& t = (*m_raw_tracks)[i];
            bool match = t.title.contains(m_search_query, Qt::CaseInsensitive) ||
                         t.artist.contains(m_search_query, Qt::CaseInsensitive) ||
                         t.album.contains(m_search_query, Qt::CaseInsensitive);
            if (!match) continue;
        }
        m_indices.push_back(i);
    }

    if (m_sort_column != -1) {
        std::sort(m_indices.begin(), m_indices.end(), [this](size_t a, size_t b) {
            const auto& t1 = (*m_raw_tracks)[a];
            const auto& t2 = (*m_raw_tracks)[b];
            bool is_asc = (m_sort_order == Qt::AscendingOrder);
            switch (m_sort_column) {
                case 0: 
                    return is_asc ? (a < b) : (a > b);
                case 1: {
                    int cmp = QString::compare(t1.title, t2.title, Qt::CaseInsensitive);
                    if (cmp != 0) {
                        return is_asc ? (cmp < 0) : (cmp > 0);
                    }
                    return is_asc ? (a < b) : (a > b);
                }
                case 2: {
                    int cmp = QString::compare(t1.album, t2.album, Qt::CaseInsensitive);
                    if (cmp != 0) {
                        return is_asc ? (cmp < 0) : (cmp > 0);
                    }
                    return is_asc ? (a < b) : (a > b);
                }
                case 4: {
                    if (t1.duration != t2.duration) {
                        return is_asc ? (t1.duration < t2.duration) : (t1.duration > t2.duration);
                    }
                    return is_asc ? (a < b) : (a > b);
                }
                default: 
                    return is_asc ? (a < b) : (a > b);
            }
        });
    }
}   //trackModel

}//ui

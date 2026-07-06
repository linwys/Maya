#pragma once
#include <QIcon>
#include <QPixmap>
#include <QSvgRenderer>
#include <QPainter>
#include <QCache>
#include <QFile>
#include <QImageReader>

namespace ui {
namespace icons {
    // u can replace this with other svg from url
//https://fonts.google.com/icons?selected=Material+Symbols+Outlined:library_books:FILL@0;wght@400;GRAD@0;opsz@24&icon.size=24&icon.color=%23e3e3e3
inline const char* library = R"(
    <svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
        <path d="M4 6H2v14c0 1.1.9 2 2 2h14v-2H4V6zm16-4H8c-1.1 0-2 .9-2 2v12c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V4c0-1.1-.9-2-2-2zm-1 9H9V9h10v2zm-4 4H9v-2h6v2zm4-8H9V5h10v2z" fill="#ffffff"/>
    </svg>
)";

inline const char* playlists = R"(
    <svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
        <path d="M4 10h12v2H4zm0-4h12v2H4zm0 8h8v2H4zm10 0v6l5-3z" fill="#ffffff"/>
    </svg>
)";

inline const char* discover = R"(
    <svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
        <path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm1 17h-2v-2h2v2zm2.07-7.75l-.9.92C13.45 12.9 13 13.5 13 15h-2v-.5c0-1.1.45-2.1 1.17-2.83l1.24-1.26c.37-.36.59-.86.59-1.41 0-1.1-.9-2-2-2s-2 .9-2 2H7c0-2.76 2.24-5 5-5s5 2.24 5 5c0 1.04-.42 1.99-1.07 2.75z" fill="#ffffff"/>
    </svg>
)";

inline const char* downloads = R"(
    <svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
        <path d="M19.35 10.04C18.67 6.59 15.64 4 12 4 9.11 4 6.6 5.64 5.35 8.04 2.34 8.36 0 10.91 0 14c0 3.31 2.69 6 6 6h13c2.76 0 5-2.24 5-5 0-2.64-2.05-4.78-4.65-4.96zM17 13l-5 5-5-5h3V9h4v4h3z" fill="#ffffff"/>
    </svg>
)";

inline const char* settings = R"(
    <svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
        <path d="M19.14 12.94c.04-.3.06-.61.06-.94 0-.32-.02-.64-.07-.94l2.03-1.58c.18-.14.23-.41.12-.61l-1.92-3.32c-.12-.22-.37-.29-.59-.22l-2.39.96c-.5-.38-1.03-.7-1.62-.94l-.36-2.54c-.04-.24-.24-.41-.48-.41h-3.84c-.24 0-.43.17-.47.41l-.36 2.54c-.59.24-1.13.57-1.62.94l-2.39-.96c-.22-.08-.47 0-.59.22L2.74 8.87c-.12.21-.08.47.12.61l2.03 1.58c-.05.3-.09.63-.09.94s.02.64.07.94l-2.03 1.58c-.18.14-.23.41-.12.61l1.92 3.32c.12.22.37.29.59.22l2.39-.96c.5.38 1.03.7 1.62.94l.36 2.54c.05.24.24.41.48.41h3.84c.24 0-.44-.17-.47-.41l.36-2.54c.59-.24 1.13-.56 1.62-.94l2.39.96c.22.08.47 0 .59-.22l1.92-3.32c.12-.22.07-.47-.12-.61l-2.01-1.58zM12 15.6c-1.98 0-3.6-1.62-3.6-3.6s1.62-3.6 3.6-3.6 3.6 1.62 3.6 3.6-1.62 3.6-3.6 3.6" fill="#ffffff"/>
    </svg>
)";

inline const char* heart = R"(
    <svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
        <path d="M16.5 3c-1.74 0-3.41.81-4.5 2.09C10.91 3.81 9.24 3 7.5 3 4.42 3 2 5.42 2 8.5c0 3.78 3.4 6.86 8.55 11.54L12 21.35l1.45-1.32C18.6 15.36 22 12.28 22 8.5 22 5.42 19.58 3 16.5 3zm-4.4 15.55l-.1.1-.1-.1C7.14 14.24 4 11.39 4 8.5 4 6.5 5.5 5 7.5 5c1.54 0 3.04.99 3.57 2.36h1.87C13.46 5.99 14.96 5 16.5 5c2 0 3.5 1.5 3.5 3.5 0 2.89-3.14 5.74-7.9 10.05z" fill="#ffffff"/>
    </svg>
)";

inline const char* heart_filled = R"(
    <svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
        <path d="M12 21.35l-1.45-1.32C5.4 15.36 2 12.28 2 8.5 2 5.42 4.42 3 7.5 3c1.74 0 3.41.81 4.5 2.09C13.09 3.81 14.76 3 16.5 3 19.58 3 22 5.42 22 8.5c0 3.78-3.4 6.86-8.55 11.54L12 21.35z" fill="#ffffff"/>
    </svg>
)";

inline const char* play = R"(
    <svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
        <path d="M8 5v14l11-7z" fill="#ffffff"/>
    </svg>
)";

inline const char* pause = R"(
    <svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
        <path d="M6 19h4V5H6v14zm8-14v14h4V5h-4z" fill="#ffffff"/>
    </svg>
)";

inline const char* skip_forward = R"(
    <svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
        <path d="M6 18l8.5-6L6 6v12zM16 6v12h2V6h-2z" fill="#ffffff"/>
    </svg>
)";

inline const char* skip_backward = R"(
    <svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
        <path d="M6 6h2v12H6zm3.5 6l8.5 6V6z" fill="#ffffff"/>
    </svg>
)";

inline const char* shuffle = R"(
    <svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
        <path d="M10.59 9.17L5.41 4 4 5.41l5.17 5.17 1.42-1.41zM14.5 4l2.04 2.04L4 18.59 5.41 20 17.96 7.46 20 9.5V4h-5.5zm.33 9.41l-1.41 1.41 3.13 3.13L14.5 20H20v-5.5l-2.04 2.04-3.13-3.13z" fill="#ffffff"/>
    </svg>
)";

inline const char* repeat = R"(
    <svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
        <path d="M7 7h10v3l4-4-4-4v3H5v6h2V7zm10 10H7v-3l-4 4 4 4v-3h12v-6h-2v4z" fill="#ffffff"/>
    </svg>
)";

inline const char* clock = R"(
    <svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
        <path d="M11.99 2C6.47 2 2 6.48 2 12s4.47 10 10 10 10-4.48 10-10S17.51 2 11.99 2zM12 20c-4.42 0-8-3.58-8-8s3.58-8 8-8 8 3.58 8 8-3.58 8-8 8zm.5-13H11v6l5.25 3.15.75-1.23-4.5-2.67z" fill="#ffffff"/>
    </svg>
)";

inline QIcon from_svg(const char* svg_data, const QColor& color) {
    QSvgRenderer renderer;
    renderer.load(QByteArray(svg_data));
    QPixmap pixmap(24, 24);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    renderer.render(&painter, QRectF(pixmap.rect()));
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(pixmap.rect(), color);
    painter.end();
    return QIcon(pixmap);
}

namespace utils {
    inline QPixmap crop_to_square(const QPixmap& src, int target_size) {
        if (src.isNull()) return src;
        int size = std::min(src.width(), src.height());
        int x = (src.width() - size) / 2;
        int y = (src.height() - size) / 2;
        QPixmap cropped = src.copy(x, y, size, size);
        return cropped.scaled(target_size, target_size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    inline QPixmap get_cached_cover(const QString& file_path, int target_size) {
        static QCache<QString, QPixmap> cache(150);
        QString key = QString("%1_%2").arg(file_path).arg(target_size);
        if (cache.contains(key)) {
            return *cache.object(key);
        }
        
        QString base = file_path;
        int dot = base.lastIndexOf('.');
        if (dot != -1) {
            base.truncate(dot);
        }
        QString art_png = base + ".png";
        QString art_jpg = base + ".jpg";

        QString target_file;
        if (QFile::exists(art_png)) {
            target_file = art_png;
        } else if (QFile::exists(art_jpg)) {
            target_file = art_jpg;
        }

        QPixmap* result = new QPixmap();
        if (!target_file.isEmpty()) {
            QImageReader reader(target_file);
            if (reader.canRead()) {
                QSize orig_size = reader.size();
                if (orig_size.isValid()) {
                    int crop_size = std::min(orig_size.width(), orig_size.height());
                    int x = (orig_size.width() - crop_size) / 2;
                    int y = (orig_size.height() - crop_size) / 2;
                    
                    //crop&decode
                    reader.setClipRect(QRect(x, y, crop_size, crop_size));
                    reader.setScaledSize(QSize(target_size, target_size));
                }
                QImage img = reader.read();
                if (!img.isNull()) {
                    *result = QPixmap::fromImage(img);
                }
            }
        }
        
        cache.insert(key, result);
        return *result;
    }
}

}
}
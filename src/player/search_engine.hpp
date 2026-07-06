#pragma once
#include <QString>
#include <vector>
#include <algorithm>
#include <ranges>
#include "track.hpp"

namespace player {

class SearchEngine {
public:
    static size_t levenshtein_distance(std::u16string_view s1, std::u16string_view s2) {
        const size_t m = s1.size();
        const size_t n = s2.size();
        if (m == 0) return n;
        if (n == 0) return m;

        std::vector<size_t> dp(n + 1);
        for (size_t j = 0; j <= n; ++j) dp[j] = j;

        for (size_t i = 1; i <= m; ++i) {
            size_t prev_diagonal = dp[0];
            dp[0] = i;
            for (size_t j = 1; j <= n; ++j) {
                size_t temp = dp[j];
                if (std::tolower(static_cast<wchar_t>(s1[i - 1])) == 
                    std::tolower(static_cast<wchar_t>(s2[j - 1]))) {
                    dp[j] = prev_diagonal;
                } else {
                    dp[j] = std::min({dp[j - 1] + 1, dp[j] + 1, prev_diagonal + 1});
                }
                prev_diagonal = temp;
            }
        }
        return dp[n];
    }

    static bool match(const QString& query, const QString& target, size_t max_distance = 2) {
        if (query.isEmpty()) return true;
        if (target.isEmpty()) return false;

        QString q_low = query.toLower();
        QString t_low = target.toLower();

        if (t_low.contains(q_low)) {
            return true;
        }

        std::u16string_view sv1(reinterpret_cast<const char16_t*>(q_low.utf16()), q_low.size());
        std::u16string_view sv2(reinterpret_cast<const char16_t*>(t_low.utf16()), t_low.size());

        return levenshtein_distance(sv1, sv2) <= max_distance;
    }

    static std::vector<Track> filter(const std::vector<Track>& dataset, const QString& query) {
        auto matches_query = [&query](const Track& track) {
            return match(query, track.title) || match(query, track.artist) || match(query, track.album);
        };

        auto filtered_view = dataset 
            | std::views::filter(matches_query);

        std::vector<Track> results;
        std::ranges::copy(filtered_view, std::back_inserter(results));
        return results;
    }
};

}
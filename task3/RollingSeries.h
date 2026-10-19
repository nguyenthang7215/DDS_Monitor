// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <map>
#include <vector>
#include <utility>
#include <cmath>
#include <cstdint>
namespace task3 {
// Re-read a bounded raw window on every update, then replace the visible snapshot.
// Samples arriving before the earliest window bound are explicitly outside retention.
class RollingSeries {
  public:
    using Sample = std::pair<int64_t, double>;
    explicit RollingSeries(int64_t retentionMs = 120000, size_t limit = 256)
        : retention_(retentionMs), limit_(limit) {
    }
    void refresh(const std::vector<Sample>& raw, int64_t now) {
        points_.clear();
        for (const auto& p : raw)
            if (std::isfinite(p.second) && p.first >= now - retention_ && p.first <= now)
                points_.emplace(
                    p.first,
                    p.second); // multimap retains distinct real samples at same timestamp
        while (points_.size() > limit_)
            points_.erase(points_.begin());
    }
    std::vector<Sample> points() const {
        return {points_.begin(), points_.end()};
    }

  private:
    int64_t retention_;
    size_t limit_;
    std::multimap<int64_t, double> points_;
};
}

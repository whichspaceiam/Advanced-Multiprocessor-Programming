#include <algorithm>
#include <chrono>
#include <vector>
#pragma once

class Timer
{
    using time_point_t = std::chrono::high_resolution_clock::time_point;
    std::vector<time_point_t> timings_;

  public:
    explicit Timer()
    {
        timings_.reserve(10);
        checkpoint();
    }

    void checkpoint()
    {
        timings_.push_back(std::chrono::high_resolution_clock::now());
    }

    template <typename DurationUnit = std::chrono::milliseconds>
    std::vector<double> get_laps() const
    {
        std::vector<double> laps;
        if (timings_.size() < 2)
            return laps;

        laps.reserve(timings_.size() - 1);

        for (size_t i = 1; i < timings_.size(); ++i)
        {
            auto dt = std::chrono::duration_cast<DurationUnit>(timings_[i] -
                                                               timings_[i - 1])
                          .count();

            laps.push_back(dt);
        }
        return laps;
    }
};
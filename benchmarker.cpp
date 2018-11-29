#include "benchmarker.hpp"

Benchmarker::time_point Benchmarker::StartTimer() const {
  return std::chrono::steady_clock::now();
}

void Benchmarker::AddSecondsSinceStart(const std::string& key,
                                       const time_point& start_time) {
  std::chrono::duration<double> duration =
      std::chrono::steady_clock::now() - start_time;
  measurements_[key] += duration.count();
}

void Benchmarker::AddAmount(const std::string& key, double amount) {
  measurements_[key] += amount;
}

double Benchmarker::GetTotal(const std::string& key) const {
  try {
    return measurements_.at(key);
  } catch (std::out_of_range& _) {
    return 0;
  }
}

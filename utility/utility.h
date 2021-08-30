#pragma once
#include <string_view>

namespace utility {

template <typename T1, typename T2>
struct pair_hash {
  size_t operator()(const std::pair<T1, T2>& pair) const {
    size_t n = 37;
    return hash1(pair.first) * 37 + hash2(pair.second);
  };

  std::hash<T1> hash1;
  std::hash<T2> hash2;
};

struct WeightWithSpan {
  WeightWithSpan(double t, size_t s = 0, std::string_view r = "") : time(t), span(s), route(r){};

  WeightWithSpan& operator+=(const WeightWithSpan& other) & noexcept {
    time += other.time;
    span += other.span;
    return *this;
  }
  double time{0};
  size_t span{0};
  std::string_view route;
};

inline bool operator<(const WeightWithSpan& lhs,
                      const WeightWithSpan& rhs) noexcept {
  return lhs.time < rhs.time;
}

inline bool operator==(const WeightWithSpan& lhs,
                       const WeightWithSpan& rhs) noexcept {
  return lhs.time == rhs.time;
}

inline bool operator>(const WeightWithSpan& lhs,
                      const WeightWithSpan& rhs) noexcept {
  return lhs.time > rhs.time;
}

inline bool operator>=(const WeightWithSpan& lhs,
                       const WeightWithSpan& rhs) noexcept {
  return lhs.time >= rhs.time;
}

inline WeightWithSpan operator+(const WeightWithSpan& lhs,
                                const WeightWithSpan& rhs) noexcept {
  WeightWithSpan answer = lhs;
  answer += rhs;
  return answer;
}


}  // namespace utility
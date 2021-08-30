#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <memory>
#include <optional>
#include <cmath>
#include <functional>
#include <type_traits>
#include <set>
#include "utility/utility.h"


namespace bus {

class StopRecord;
class BusRecord;

using StopRecordPtr = std::shared_ptr<StopRecord>;
using BusRecordPtr = std::shared_ptr<BusRecord>;
using StopRecordWeakPtr = std::weak_ptr<StopRecord>;
using BusRecordWeakPtr = std::weak_ptr<BusRecord>;

static size_t CURRENT_STOP_ID = 0;

struct Coordinates {
 public:
  double latitude{0};
  double longitude{0};
};

class BusRecord {
 public:
  enum class RouteType { Circular, Linear };

  explicit BusRecord(RouteType type, std::string name);

  void AddStop(StopRecordPtr ptr);

  [[nodiscard]] auto GetStops() const -> const std::vector<StopRecordWeakPtr>&;

  [[nodiscard]] const std::string_view GetName() const noexcept;

  [[nodiscard]] double GetGeomLength() const;

  [[nodiscard]] double GetRouteLength() const;

  [[nodiscard]] RouteType GetType() const;

  [[nodiscard]] size_t GetUniqueStops() const;

  [[nodiscard]] size_t GetStopsOnRoute() const;

 private:
  std::string name_;
  std::vector<StopRecordWeakPtr> stops_;
  RouteType type_;
};



class StopRecord {
 public:
  StopRecord(std::string name);
  StopRecord(std::string name, Coordinates place);

  void SetCoordinates(Coordinates place);

  const Coordinates& GetCoordinates() const noexcept;

  void AddBus(std::string_view);

  void SetDist(std::string_view name, size_t dist);

  [[nodiscard]] const std::string_view GetName() const noexcept;

  [[nodiscard]] const std::set<std::string_view>& GetBuses() const noexcept;

  [[nodiscard]] std::optional<size_t> GetDist(
      std::string_view finish) const noexcept;

  [[nodiscard]] bool IsInitialized() {
    return initialized_;
  }

 private:
  //size_t stop_id_ = CURRENT_STOP_ID++;
  Coordinates place_;
  std::string name_;
  bool initialized_{false};
  std::set<std::string_view> buses_;
  std::unordered_map<std::string_view, size_t> distances_;
};



class BusManager {
 public:
  void AddBus(const std::string& name, const std::vector<std::string>& stops,
              BusRecord::RouteType type);

  void AddStop(const std::string& name, const Coordinates& pos,
               const std::vector<std::pair<std::string, size_t>>& dist = {});

  auto GetBus(const std::string& name) const
      -> std::optional<const std::reference_wrapper<BusRecord>>;

  auto GetStop(const std::string& name) const
      -> std::optional<const std::reference_wrapper<StopRecord>>;

 protected:
  template <typename T>
  using Index = std::unordered_map<std::string_view, T>;

  using StopIndex = Index<StopRecordPtr>;
  using BusIndex = Index<BusRecordPtr>;

  template <typename RecordPtr, typename IndexType>
  void AddRecord(Index<IndexType>& index,
                 RecordPtr record) {
    auto& underlying = *record;
    index.insert({underlying.GetName(), record});
  }

  template <typename IndexType>
  [[nodiscard]] auto GetRecord(const Index<IndexType>& index, std::string_view name) const -> std::optional<IndexType> {
    auto result = index.find(name);
    if (result == index.end()) {
      return std::nullopt;
    }
    return (*result).second;
  }

  template <typename IndexType>
  [[nodiscard]] IndexType GetOrCreateRecord(Index<IndexType>& index,
                                            std::string_view name) {
    auto result = index.find(name);
    if (result == index.end()) {
      auto record = std::make_shared<std::decay_t<decltype(*(std::declval<IndexType>()))>>(std::string(name));
      AddRecord(index, record);
      return record;
    }
    return (*result).second;
  }


 protected:
  StopIndex stop_index_;
  BusIndex bus_index_;
};

double GradToRad(double value) noexcept;

//---------------------------------------------------------

double HaversineDistance(const Coordinates& start,
                         const Coordinates& finish);

template <typename U>
inline double HaversineDistance(const std::weak_ptr<U> start,
                                const std::weak_ptr<U> finish) {
  return HaversineDistance(start.lock()->GetCoordinates(),
                           finish.lock()->GetCoordinates());
}

inline double HaversineDistance(const StopRecord& start,
                                const StopRecord& finish) {
  return HaversineDistance(start.GetCoordinates(),
                           finish.GetCoordinates());
}

//--------------------------------------------------------------

template <typename U>
inline double RouteDistance(const std::weak_ptr<U> start,
                                const std::weak_ptr<U> finish) {
  auto from = start.lock();
  auto to = finish.lock();
  auto distance = from->GetDist(to->GetName());
  if (distance) {
    return *distance;
  } else {
    return HaversineDistance(from->GetCoordinates(), to->GetCoordinates());
  }
}


inline double RouteDistance(const StopRecord& start,
                                const StopRecord& finish) {
  auto distance = start.GetDist(finish.GetName());
  if (distance) {
    return *distance;
  } else {
    return HaversineDistance(start.GetCoordinates(), finish.GetCoordinates());
  }
}

//-------------------------------------------------------------------------


template <typename InputIt>
double CalculateLength(InputIt first, InputIt last) {
  double length = 0;
  auto prev = first;
  if (prev == last) {
    return length;
  }
  for (auto it = ++first; it != last; ++it) {
    length += HaversineDistance(*prev, *it);
    prev = it;
  }
  return length;
}

template <typename InputIt>
double CalculateRouteLength(InputIt first, InputIt last) {
  double length = 0;
  auto prev = first;
  if (prev == last) {
    return length;
  }
  for (auto it = ++first; it != last; ++it) {
    length += RouteDistance(*prev, *it);
    prev = it;
  }
  return length;
}

static constexpr double PI = 3.1415926535;

static const std::unordered_map<uint8_t, std::string_view> NUM_TO_ERROR = {
    {1, "not found"}};

constexpr uint8_t kErrorNotFound = 1;

}  // namespace bus
#include "BusManager.h"
#include <stdexcept>

#ifdef _DEBUG
#include <cassert>
#endif


namespace bus {

//---------------------------------------------------------------
// StopRecord

StopRecord::StopRecord(std::string name) : name_(std::move(name)) {
}

StopRecord::StopRecord(std::string name, Coordinates place)
    : name_(std::move(name)), place_(std::move(place)), initialized_(true) {
}

const std::string_view StopRecord::GetName() const noexcept {
  return name_;
}

void StopRecord::SetCoordinates(Coordinates place) {


  place_ = place;
  initialized_ = true;
}

const Coordinates& StopRecord::GetCoordinates() const noexcept {
  return place_;
}

void StopRecord::AddBus(std::string_view bus) {
  buses_.insert(bus);
}

const std::set<std::string_view>& StopRecord::GetBuses() const noexcept {
  return buses_;
}

//void StopRecord::AddDist(std::string_view name, size_t dist) {
//  auto it = distances_.find(name);
//  if (it == distances_.end()) {
//    distances_.insert({name, dist});
//  }
//}

 void StopRecord::SetDist(std::string_view name, size_t dist) {
  distances_[name] = dist;
 }

[[nodiscard]] std::optional<size_t> StopRecord::GetDist(
    std::string_view finish) const noexcept {
  auto it = distances_.find(finish);
  if (it != distances_.end()) {
    return it->second;
  }
  return std::nullopt;
}

inline void SetDistIfNotExist(StopRecord& record, std::string_view name,
                              size_t dist) {
  auto maybe_dist = record.GetDist(name);
  if (!maybe_dist) {
    record.SetDist(name, dist);
  }
}


//-------------------------------------------------------------
// BusRecord

BusRecord::BusRecord(RouteType type, std::string name)
    : type_(type), name_(std::move(name)) {
}

void BusRecord::AddStop(StopRecordPtr ptr) {
  stops_.push_back(StopRecordWeakPtr(ptr));
}

auto BusRecord::GetStops() const -> const std::vector<StopRecordWeakPtr>& {
  return stops_;
}

const std::string_view BusRecord::GetName() const noexcept {
  return name_;
}

double BusRecord::GetGeomLength() const {
  if (type_ == RouteType::Circular) {
    return CalculateLength(stops_.begin(), stops_.end());
  } else {
    return CalculateLength(stops_.begin(), stops_.end()) * 2;
  }
}

double BusRecord::GetRouteLength() const {
  if (type_ == RouteType::Circular) {
    return CalculateRouteLength(stops_.begin(), stops_.end());
  } else {
    return CalculateRouteLength(stops_.begin(), stops_.end()) +
           CalculateRouteLength(stops_.rbegin(), stops_.rend());
  }
}

BusRecord::RouteType BusRecord::GetType() const {
  return type_;
}

size_t BusRecord::GetUniqueStops() const {
  return std::set<StopRecordWeakPtr, std::owner_less<StopRecordWeakPtr>>(stops_.begin(), stops_.end()).size();
}

[[nodiscard]] size_t BusRecord::GetStopsOnRoute() const {
  if (type_ == RouteType::Circular) {
    return stops_.size();
  } else {
    return stops_.empty() ? 0 : stops_.size() * 2 - 1;
  }
}

//-------------------------------------------------------
// BusManager

void BusManager::AddBus(const std::string& name, const std::vector<std::string>& stops,
                        BusRecord::RouteType type) {
  BusRecordPtr record = std::make_shared<BusRecord>(type, name);
  for (const auto& stop : stops) {
    auto stop_record = GetRecord(stop_index_, stop);

    if (!stop_record) {
      StopRecordPtr new_stop_record = std::make_shared<StopRecord>(stop);
      new_stop_record->AddBus(record->GetName());
      AddRecord(stop_index_, new_stop_record);
      record->AddStop(new_stop_record);
    } else {
      (*stop_record)->AddBus(record->GetName());
      record->AddStop(*stop_record); 
    }
  }
  AddRecord(bus_index_, record);
}

void BusManager::AddStop(
    const std::string& name, const Coordinates& pos,
    const std::vector<std::pair<std::string, size_t>>& dist) {

  auto record = GetOrCreateRecord(stop_index_, name);
  record->SetCoordinates(pos);

  for (const auto& [other_name, length] : dist) {
    auto other_record = GetOrCreateRecord(stop_index_, other_name);
    record->SetDist(other_record->GetName(), length);
    SetDistIfNotExist(*other_record, record->GetName(), length);
  }
  AddRecord(stop_index_, record);
}

std::optional<const std::reference_wrapper<BusRecord>> BusManager::GetBus(
    const std::string& name) const {
  auto record = GetRecord(bus_index_, name);
  if (!record.has_value()) {
    //throw std::runtime_error("bad database access! Bus not found with name " +
    //                         name);
    return std::nullopt;
  }
  return *(record.value());
}

std::optional<const std::reference_wrapper<StopRecord>> BusManager::GetStop(
    const std::string& name) const {
  auto record = GetRecord(stop_index_, name);
  if (!record.has_value()) {
    //throw std::runtime_error("bad database access! Stop not found with name " +
    //                         name);
    return std::nullopt;
  }
  return *(record.value());
}

double GradToRad(double value) noexcept {
  static constexpr double coeff = PI / 180;
  return coeff * value;
}

double HaversineDistance(const Coordinates& start, const Coordinates& finish) {
  double lat1 = GradToRad(start.latitude);
  double lat2 = GradToRad(finish.latitude);
  double long1 = GradToRad(start.longitude);
  double long2 = GradToRad(finish.longitude);
  using namespace std;
  return acos(sin(lat1) * sin(lat2) + cos(lat1) * cos(lat2) * cos(abs(long1 - long2))) * 6371000;
}

}  // namespace bus
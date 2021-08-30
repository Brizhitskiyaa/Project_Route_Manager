#include "Command.h"

void AddBusRequest::ParseFrom(std::string_view input) {
  auto [lhs, rhs] = SplitTwo(input, ":");
  ReadToken(lhs);
  name_ = lhs;

  rhs = TrimLeft(rhs);
  size_t pos = rhs.find('>');
  std::string_view delim;
  if (pos != rhs.npos) {
    delim = " > ";
    type_ = bus::BusRecord::RouteType::Circular;
  } else {
    delim = " - ";
    type_ = bus::BusRecord::RouteType::Linear;
  }

  std::string_view token;
  while ((token = ReadToken(rhs, delim)) != "") {
    stops_.push_back(std::string(token));
  }
}

void AddBusRequest::Process(bus::BusManager& manager) const {
  manager.AddBus(name_, stops_, type_);
}


void AddBusRequest::ParseFromJson(const Json::Node& node) {

  const auto& dict = node.AsMap();
  name_ = dict.at("name").AsString();
  type_ = dict.at("is_roundtrip").AsBool() ? bus::BusRecord::RouteType::Circular
                                           : bus::BusRecord::RouteType::Linear;
  const auto& stops = dict.at("stops").AsArray();
  for (const auto& node : stops) {
    stops_.push_back(node.AsString());
  }
}

//-------------------------------------------------------------------

void AddStopRequest::ParseFrom(std::string_view input) {
  auto [lhs, rhs] = SplitTwo(input, ": ");
  name_ = SplitTwo(lhs).second;

  /*auto [latitude, longitude] = SplitTwo(rhs, ", ");*/
  auto latitude = ReadToken(rhs, ", ");
  auto longitude = ReadToken(rhs, ", ");
  pos_ = {ConvertToNum<double>(latitude), ConvertToNum<double>(longitude)};

  while (rhs.size() != 0) {
    auto dist = ReadToken(rhs, ", ");
    auto [length, name] = SplitTwo(dist, "m to ");
    distances_.push_back({std::string(name), ConvertToNum<size_t>(length)});
  }
}

void AddStopRequest::Process(bus::BusManager& manager) const {
  manager.AddStop(name_, pos_, distances_);
}


void AddStopRequest::ParseFromJson(const Json::Node& node) {
  const auto& dict = node.AsMap();
  name_ = dict.at("name").AsString();
  pos_.latitude = dict.at("latitude").AsDouble();
  pos_.longitude = dict.at("longitude").AsDouble();

  const auto& distances = dict.at("road_distances").AsMap();
  for (const auto& [stop, node] : distances) {
    distances_.push_back({stop, node.AsDouble()});
  }
}


//-----------------------------------------------------------------

void GetBusInfoRequest::ParseFrom(std::string_view input) {
  auto [lhs, name] = SplitTwo(input);
  name_ = name;
}


void GetBusInfoRequest::ParseFromJson(const Json::Node& node) {
  const auto& dict = node.AsMap();
  name_ = dict.at("name").AsString();
  request_id_ = static_cast<size_t>(dict.at("id").AsDouble());
}


BusInfo GetBusInfoRequest::Process(
    const bus::BusManager& manager) const {
  auto maybe_record = manager.GetBus(name_);
  BusInfo answer;
  answer.request_id_ = request_id_;
  if (!maybe_record) {
    answer.error_code_ = bus::kErrorNotFound;
    return answer;
  }

  const auto& info = maybe_record.value().get();
  answer.route_length_ = info.GetRouteLength();
  answer.stops_on_route_ = info.GetStopsOnRoute();
  answer.unique_stops = info.GetUniqueStops();
  answer.curvature_ = answer.route_length_ / info.GetGeomLength();
  return answer;
}

//-----------------------------------------------------------------------------

std::ostream& operator<<(std::ostream& out, const BusInfo& info) {
  if (info.error_code_ != 0) {
    out << bus::NUM_TO_ERROR.find(info.error_code_)->second;
    return out;
  }
  out << info.stops_on_route_
      << " stops on route, " << info.unique_stops << " unique stops, "
      << info.route_length_ << " route length, " << info.curvature_ << " curvature";
  return out;
}

std::ostream& operator<<(std::ostream& out, const StopInfo& info) {
  if (info.error_code_ != 0) {
    out << bus::NUM_TO_ERROR.find(info.error_code_)->second;
    return out;
  }
  const auto& buses = *info.buses_.value();
  if (buses.empty()) {
    out << "no buses";
  } else {
    out << "buses";
    for (const auto& bus : buses) {
      out << " " << bus;
    }
  }
  return out;
}

Request::RequestHolder Request::Create(Request::Type type) {
  switch (type) {
    case Request::Type::ADD_BUS:
      return std::make_unique<AddBusRequest>();
    case Request::Type::ADD_STOP:
      return std::make_unique<AddStopRequest>();
    case Request::Type::GET_BUS:
      return std::make_unique<GetBusInfoRequest>();
    case Request::Type::GET_STOP:
      return std::make_unique<GetStopInfoRequest>();
    case Request::Type::GET_ROUTE:
      return std::make_unique<GetRouteInfoRequest>();
    default:
      return nullptr;
  }
}

//-----------------------------------------
// GetStopInfoRequest

void GetStopInfoRequest::ParseFrom(std::string_view input) {
  auto [lhs, name] = SplitTwo(input);
  name_ = name;
}


void GetStopInfoRequest::ParseFromJson(const Json::Node& node) {
  const auto& dict = node.AsMap();
  name_ = dict.at("name").AsString();
  request_id_ = static_cast<size_t>(dict.at("id").AsDouble());
}


StopInfo GetStopInfoRequest::Process(
    const bus::BusManager& manager) const {
  auto maybe_record = manager.GetStop(name_);
  StopInfo answer;
  answer.request_id_ = request_id_;
  if (!maybe_record) {
    answer.error_code_ = bus::kErrorNotFound;
    return answer;
  }
  const auto& info = maybe_record.value().get();
  answer.buses_ = std::addressof(info.GetBuses());
  return answer;
}

//-------------------------------------------------
//GetRouteInfoRequest


void GetRouteInfoRequest::ParseFromJson(const Json::Node& node) {
  const auto& dict = node.AsMap();
  from_ = dict.at("from").AsString();
  to_ = dict.at("to").AsString();
  request_id_ = static_cast<size_t>(dict.at("id").AsDouble());
}

void GetRouteInfoRequest::ParseFrom(std::string_view input) {
  // Not Implemented
  throw std::runtime_error("Not implemented");
}


RouteInfo GetRouteInfoRequest::Process(const bus::BusManager& manager) const {
  const auto& router = static_cast<const bus::BusManagerWithRouter&>(manager);
  auto maybe_info = router.GetRoute(from_, to_);
  RouteInfo answer;
  answer.request_id_ = request_id_;

  if (!maybe_info) {
    answer.error_code_ = bus::kErrorNotFound;
    return answer;
  }

  RouteInterpreter interpreter{router};
  auto info = interpreter.InterpretRoute(*maybe_info);
  answer.route_items_ = std::move(info.first);
  answer.total_time_ = info.second;

  return answer;
}

std::pair<std::vector<RouteInfo::RouteItemVar>, double>
RouteInterpreter::InterpretRoute(const Router::RouteInfo& info) const {
  using Item = RouteInfo::RouteItemVar;
  std::vector<RouteInfo::RouteItemVar> route_info;
  double total_time = info.weight.time;

  for (size_t i = 0; i < info.edge_count; ++i) {
    auto maybe_item = GetItem(info.id, i);
    if (maybe_item) {
      route_info.push_back(std::move(*maybe_item));
    }
  }
  return {std::move(route_info), total_time};
}

std::optional<RouteInfo::RouteItemVar> RouteInterpreter::GetItem(
    Router::RouteId id, size_t current_edge_num) const {
  using Item = RouteInfo::RouteItemVar;
  auto edge_id = router_.GetRouteEdge(id, current_edge_num);
  const auto& edge = graph_.GetEdge(edge_id);

  auto [stop, route] = manager_.GetStopFromIndex(edge.from).value();
  if (route == bus::NodeType::WAIT) {
    double time = edge.weight.time;
    Item item = WaitRouteItem{std::string(stop), time};

    return item;

  } else {
    std::string current_route = std::string(edge.weight.route);
    uint32_t span_count = edge.weight.span;
    double time = edge.weight.time;

    Item item = BusRouteItem{std::move(current_route), time, span_count};
    return item;
  }
}

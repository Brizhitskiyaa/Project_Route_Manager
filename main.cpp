#include <iostream>
#include <fstream>
#include <iomanip>


#include "utility/test_runner.h"
#include "utility/profile.h"
#include "BusManager.h"
#include "Parcing/Parcing.h"
#include "Command.h"
#include "Json/json.h"
#include <algorithm>

//----------------------------------------------------------------------------------------
// Forward declarations

using Dictionary = std::unordered_map<std::string_view, Request::Type>;

std::vector<Request::RequestHolder> ParseRequests(
    const Dictionary& index, std::istream& input);

void ProcessModifyRequests(const std::vector<Request::RequestHolder>& requests,
                           bus::BusManager& manager);

//----------------------------------------------------------------------------------------

void TrimRightLeft() {
  std::string str = "   Trim me!!!!!!";
  std::string_view left_trim = TrimLeft(str);
  std::string_view full_trim = TrimRight(left_trim, "!");
  auto no_trim = TrimLeft(full_trim, "*");
  auto still_no_trim = TrimRight(full_trim, "*");


  ASSERT_EQUAL(left_trim, "Trim me!!!!!!");
  ASSERT_EQUAL(full_trim, "Trim me");
  ASSERT_EQUAL(no_trim, "Trim me");
  ASSERT_EQUAL(still_no_trim, "Trim me");

}

void TrimEmpty() {
  std::string empty = "";
  std::string really_empty = "   ";
  auto empty_left = TrimLeft(empty);
  auto empty_right = TrimRight(empty);
  auto trim_to_empty_r = TrimRight(really_empty);
  auto trim_to_empty_l = TrimLeft(really_empty);

  ASSERT_EQUAL(empty_left, "");
  ASSERT_EQUAL(empty_right, "");
  ASSERT_EQUAL(trim_to_empty_l, "");
  ASSERT_EQUAL(trim_to_empty_r, "");
}

void BusRecordSimple() {
  using namespace bus;
  using Type =  BusRecord::RouteType;

  BusRecord rec1(Type::Linear, "name1");
  BusRecord rec2(Type::Circular, "name2");

  ASSERT_EQUAL(rec1.GetName(), "name1");
  ASSERT_EQUAL(rec2.GetName(), "name2");


}

void StopRecordSimple() {
    using namespace bus;

    StopRecord stop1("stop1");
    StopRecord stop2("stop2", {.latitude = 2, .longitude = 1});

    ASSERT_EQUAL(stop1.GetName(), "stop1");

    ASSERT(!stop1.IsInitialized());
    ASSERT(stop2.IsInitialized());

    stop1.SetCoordinates({1, 1});

    ASSERT(stop1.IsInitialized());
}

void AddGetStop() {
  using namespace bus;

  BusManager manager;
  // Add stops
  manager.AddStop("stop1", {1.0, 1.0});
  manager.AddStop("stop2", {1.0, 2.0});
  manager.AddStop("stop3", {-1.0, 1.0});

  const StopRecord& stop1 = manager.GetStop("stop1").value();
  const StopRecord& stop2 = manager.GetStop("stop2").value();
  const StopRecord& stop3 = manager.GetStop("stop3").value();

  ASSERT_EQUAL(stop1.GetName(), "stop1");
  ASSERT_EQUAL(stop2.GetName(), "stop2");
  ASSERT_EQUAL(stop3.GetName(), "stop3");

}

void AddGetBus() {
  using namespace bus;

  BusManager manager;
  // Add stops
  manager.AddStop("stop1", {1.0, 1.0});
  manager.AddStop("stop2", {1.0, 2.0});
  manager.AddStop("stop3", {-1.0, 1.0});

  manager.AddBus("101", {"stop1", "stop3"}, BusRecord::RouteType::Linear);

  const BusRecord& bus1 = manager.GetBus("101").value();


  auto stop1_ptr = bus1.GetStops()[0].lock();
  auto stop3_ptr = bus1.GetStops()[1].lock();
  ASSERT_EQUAL(stop1_ptr->GetName(), "stop1");

  const auto& pos1 = stop1_ptr->GetCoordinates();
  const auto& pos3 = stop3_ptr->GetCoordinates();
  ASSERT_EQUAL(pos1.latitude, 1.0);
  ASSERT_EQUAL(pos1.longitude, 1.0);
  ASSERT_EQUAL(pos3.latitude, -1.0);
  ASSERT_EQUAL(pos3.longitude, 1.0);
}

void AddBusRequestParser() {
  std::string line =
      "Bus 256: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo "
      "Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye";
  std::string another_line =
      "Bus 750: Tolstopaltsevo - Marushkino - "
      "Rasskazovka";
  AddBusRequest request{};
  AddBusRequest another{};
  request.ParseFrom(line);
  another.ParseFrom(another_line);
  ASSERT_EQUAL(static_cast<int>(request.type_), static_cast<int>(bus::BusRecord::RouteType::Circular));
  ASSERT_EQUAL(request.name_, "256");
  std::vector<std::string> stops = {"Biryulyovo Zapadnoye",
                                    "Biryusinka",
                                    "Universam",
                                    "Biryulyovo Tovarnaya",
                                    "Biryulyovo Passazhirskaya",
                                    "Biryulyovo Zapadnoye"};
  ASSERT_EQUAL(request.stops_, stops);

  ASSERT_EQUAL(static_cast<int>(another.type_),
               static_cast<int>(bus::BusRecord::RouteType::Linear));
  ASSERT_EQUAL(another.name_, "750");
  std::vector<std::string> another_stops = {"Tolstopaltsevo", "Marushkino",
                                            "Rasskazovka"};
  ASSERT_EQUAL(another.stops_, another_stops);
}


void AddStopRequestParser() {
  std::string line = "Stop Marushkino: 55.595884, 37.209755";
  std::string new_line = "Stop Biryulyovo Zapadnoye: 55.574371, 37.6517, 7500m to Rossoshanskaya ulitsa, 1800m to Biryusinka, 2400m to Universam";
  AddStopRequest request{};
  AddStopRequest new_request{};
  request.ParseFrom(line);
  new_request.ParseFrom(new_line);

  ASSERT_EQUAL(request.name_, "Marushkino");
  ASSERT_EQUAL(request.pos_.latitude, 55.595884);
  ASSERT_EQUAL(request.pos_.longitude, 37.209755);

  ASSERT((new_request.distances_[0] == std::make_pair<std::string, size_t>("Rossoshanskaya ulitsa", 7500)));
  ASSERT((new_request.distances_[2] ==
          std::make_pair<std::string, size_t>("Universam", 2400)));
}

void GetBusRequestParser() {
  std::string line = "Bus 256";
  GetBusInfoRequest request{};
  request.ParseFrom(line);

  ASSERT_EQUAL(request.name_, "256");
}

void CalculateLengthTest() {
  using namespace bus;
  Coordinates stop1 = {55.611087, 37.20829};
  Coordinates stop2 = {55.595884, 37.209755};
  Coordinates stop3 = {55.632761, 37.333324};

  std::vector Stops = {stop1, stop2, stop3};
  double length = CalculateLength(Stops.begin(), Stops.end());
  double zero = CalculateLength(Stops.begin(), Stops.begin() + 1);
  double also_zero = CalculateLength(Stops.end() - 1, Stops.end());

  ASSERT_EQUAL(zero, 0);
  ASSERT_EQUAL(also_zero, 0);
  ASSERT(std::abs(length * 2 - 20939.5) < 0.1);
}

void ParseModifyRequestsTest() {
  std::string line =
      "3\nBus 256: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya >\
        Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye\nBus 750: Tolstopaltsevo -\
        Marushkino - Rasskazovka\nStop Rasskazovka: 55.632761, 37.333324";

  std::stringstream input;
  input << line;
  auto requests = ParseRequests(STR_TO_MOD_REQUEST_TYPE, input);
  ASSERT(requests.size() == 3);

  const auto& first = static_cast<const AddBusRequest&>(*requests[0]);
  const auto& second = static_cast<const AddBusRequest&>(*requests[1]);
  const auto& third = static_cast<const AddStopRequest&>(*requests[2]);

  ASSERT_EQUAL(first.name_, "256");
  ASSERT_EQUAL(second.name_, "750");
  ASSERT_EQUAL(third.name_, "Rasskazovka");
  ASSERT_EQUAL(third.pos_.latitude, 55.632761);
  ASSERT_EQUAL(third.pos_.longitude, 37.333324);
}

void AddBusThenStops() {
  using namespace bus;
  BusManager manager;
  std::string line =
      "3\nBus 256: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya >\
        Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye\nBus 750: Tolstopaltsevo -\
        Marushkino - Rasskazovka\nStop Rasskazovka: 55.632761, 37.333324";

  std::stringstream input;
  input << line;
  auto requests = ParseRequests(STR_TO_MOD_REQUEST_TYPE, input);
  ProcessModifyRequests(requests, manager);

  const auto stop1_opt = manager.GetStop("Biryusinka");
  const auto stop2_opt = manager.GetStop("Rasskazovka");

  ASSERT(stop1_opt.has_value());
  ASSERT(stop2_opt.has_value());
  ASSERT(!stop1_opt->get().IsInitialized());
  ASSERT(stop2_opt->get().IsInitialized());

}


void MapToJsonTest() {
  using namespace Json;
  std::map<std::string, double> map1 = {{"one", 1.0}, {"two", 2.0}};
  std::unordered_map<std::string, double> map2 = {{"one", 1.0}, {"two", 2.0}};
  std::string one = "one";
  std::string two = "two";
  std::map<std::string_view, int> map3;
  map3.insert({one, 1});
  map3.insert({two, 2});
  std::map<int, int> map4 = {{1, 1}, {2, 2}};
  ToJson(map1, std::cout);
  ToJson(map2, std::cout);
  ToJson(map3, std::cout);
}

void VecToJsonTest() {
  using namespace Json;
  std::map<std::string, std::vector<std::string>> map1 = {{"one", {"ass", "we", "can"}}, {"two", {"ass", "we", "can"}}};
  std::map<std::string, std::vector<std::string>> map2 = {{"one", {}}, {"two", {}}};

  ToJson(map1, std::cout);
  ToJson(map2, std::cout);
}

void ToJsonAndBack() {
  std::string test_line = 
    R"(  {
  "base_requests": [
    {
      "type": "Bus",
      "name": "256",
      "is_roundtrip": true
    }
  ],
  "stat_requests": [
    {
      "type": "Bus",
      "name": "256",
      "id": 1965312327
    }
  ]
})";
  std::stringstream out;
  out << test_line;

  using namespace Json;
  auto doc = Load(out);
  out.str(std::string());
  ToJson(doc.GetRoot(), std::cout);
  std::string first = out.str();

  auto doc_copy = Load(out);
  out.str(std::string());
  ToJson(doc_copy.GetRoot(), out, 0);
  std::string second = out.str();
  ASSERT_EQUAL(first, second);
}

// END TESTS
//--------------------------------------------------------------------------------------------



std::optional<Request::Type> GetRequestType(std::string_view line,
                                            const Dictionary& index) {
  auto keyword = ReadToken(line);
  auto it = index.find(keyword);
  if (it == index.end()) {
    return std::nullopt;
  } else {
    return it->second;
  }
}

Request::RequestHolder MakeRequestFromLine(const Dictionary& index,
                                           std::string_view line) {
  const auto request_type = GetRequestType(line, index);
  if (!request_type) {
    throw std::runtime_error("Couldn't identify request from the line");
    return nullptr;
  }
  auto request = Request::Create(*request_type);
  request->ParseFrom(line);
  return request;
 }


std::vector<Request::RequestHolder> ParseRequests(
    const Dictionary& index, std::istream& input) {
  size_t count = ReadNumberOnLine<size_t>(input);
  std::vector<Request::RequestHolder> requests;
  requests.reserve(count);

  for (int i = 0; i < count; ++i) {
    std::string line;
    std::getline(input, line);

    if (auto request = MakeRequestFromLine(index, line)) {
      requests.push_back(std::move(request));
    }
  }
  return requests;
}

void ProcessModifyRequests(const std::vector<Request::RequestHolder>& requests, bus::BusManager& manager) {
  for (const auto& request : requests) {
    const auto& mod_request = static_cast<const ModifyRequest&>(*request);
    mod_request.Process(manager);
  }
}

template <typename Info>
void PrintRequest(const Info& opt, std::ostream& out = std::cout) {
  out << std::setprecision(6) << opt << '\n';
}

void ProcessReadRequests(const std::vector<Request::RequestHolder>& requests,
                         const bus::BusManager& manager) {
  for (const auto& request : requests) {
    if (request->type_ == Request::Type::GET_BUS) {
      const auto& read_request =
          static_cast<const GetBusInfoRequest&>(*request);
      auto info = read_request.Process(manager);
      std::cout << "Bus " << read_request.name_ << ": ";
      PrintRequest(info);
    } else if (request->type_ == Request::Type::GET_STOP) {
      const auto& read_request =
          static_cast<const GetStopInfoRequest&>(*request);
      auto info = read_request.Process(manager);
      std::cout << "Stop " << read_request.name_ << ": ";
      PrintRequest(info);
    } else {
      throw std::runtime_error("Unsupported request");
    }
  }
}

void BusinessLogic() {
  using namespace bus;
  BusManager manager;

  auto mod_requests = ParseRequests(STR_TO_MOD_REQUEST_TYPE, std::cin);
  auto read_requests = ParseRequests(STR_TO_READ_REQUEST_TYPE, std::cin);
  ProcessModifyRequests(mod_requests, manager);
  ProcessReadRequests(read_requests, manager);

}

//---------------------------------------------------------------
// Task 4, adding json support


std::optional<Request::Type> GetRequestType(const Json::Node& node,
                                            const Dictionary& index) {
  const auto& dict = node.AsMap();
  auto it = dict.find("type");

  if (it == dict.end()) {
    return std::nullopt;
  } 
  const auto& word = it->second.AsString();
  auto idx_it = index.find(word);
  if (idx_it == index.end()) {
    return std::nullopt;
  } else {
    return idx_it->second;
  }
}

Request::RequestHolder MakeRequestFromJson(const Dictionary& index,
                                           const Json::Node& node) {
  const auto request_type = GetRequestType(node, index);
  if (!request_type) {
    throw std::runtime_error("Couldn't identify request from the line");
    return nullptr;
  }
  auto request = Request::Create(*request_type);
  request->ParseFromJson(node);
  return request;
}

Json::Node AnswerToJson(StopInfo& info) {
  using Json::Node;
  using namespace std::string_literals;
  std::map<std::string, Node> result = {
      {"request_id", Node(info.request_id_)}};

  if (auto code = info.error_code_) {
    result.emplace(
        std::make_pair(std::string("error_message"),
                       Node(std::string(bus::NUM_TO_ERROR.at(code)))));
    return Node(std::move(result));
  }

  const auto& buses = *info.buses_.value();
  std::vector<Node> copy_buses;
  copy_buses.reserve(buses.size());
  std::transform(
      buses.begin(), buses.end(), std::back_inserter(copy_buses), [](std::string_view sv) {
        return Node(std::string(sv));
      });

  result.insert(
      {"buses", Node(std::move(copy_buses))});
  return Node(std::move(result));
}

Json::Node AnswerToJson(BusInfo& info) {
  using Json::Node;
  using namespace std::string_literals;
  std::map<std::string, Node> result = {
      {"request_id", Node(info.request_id_)}};
  if (auto code = info.error_code_) {
    result.emplace(
        std::make_pair(std::string("error_message"),
                       Node(std::string(bus::NUM_TO_ERROR.at(code)))));
    return Node(std::move(result));
  }
  result.insert({"route_length"s, info.route_length_});
  result.insert({"curvature"s, info.curvature_});
  result.insert({"stop_count"s, (double)info.stops_on_route_});
  result.insert({"unique_stop_count"s, (double)info.unique_stops});
  return Node(std::move(result));
}

Json::Node AnswerToJson(RouteInfo& info) {
  using Json::Node;
  using namespace std::string_literals;

  struct RouteInfoVarVisitor {
    Json::Node operator()(WaitRouteItem& item) {
      std::map<std::string, Node> answer = {
          {"type"s, "Wait"s},
          {"time"s, item.time_},
          {"stop_name"s, std::move(item.stop_name_)}};
      return Node(std::move(answer));
    }

    Json::Node operator()(BusRouteItem& item) {
      std::map<std::string, Node> answer = {
          {"type"s, "Bus"s},
          {"time"s, item.time_},
          {"bus"s, std::move(item.bus_)},
          {"span_count"s, (double)item.span_count_}};
      return Node(std::move(answer));
    }
  };

  std::map<std::string, Node> result = {{"request_id"s, info.request_id_}};
  if (auto code = info.error_code_) {
    result.insert(
        {"error_message"s, std::string(bus::NUM_TO_ERROR.at(code))});
    return Node(std::move(result));
  }

  result.insert({"total_time"s, info.total_time_});
  std::vector<Node> items;
  items.reserve(info.route_items_.size());
  std::transform(info.route_items_.begin(), info.route_items_.end(),
                 std::back_inserter(items), [](auto& var) {
                   return std::visit(RouteInfoVarVisitor{}, var);
                 });
  result.insert({"items"s, std::move(items)});

  return Node(std::move(result));
}

std::vector<Json::Node> ProcessReadRequestsToJson(
    const std::vector<Request::RequestHolder>& requests,
    const bus::BusManager& manager) {
  std::vector<Json::Node> result = {};
  for (const auto& request : requests) {
    if (request->type_ == Request::Type::GET_BUS) {
      const auto& read_request =
          static_cast<const GetBusInfoRequest&>(*request);
      auto info = read_request.Process(manager);
      result.push_back(AnswerToJson(info));
    } else if (request->type_ == Request::Type::GET_STOP) {
      const auto& read_request =
          static_cast<const GetStopInfoRequest&>(*request);
      auto info = read_request.Process(manager);
      result.push_back(AnswerToJson(info));
    } else if (request->type_ == Request::Type::GET_ROUTE) {
      const auto& read_request =
          static_cast<const GetRouteInfoRequest&>(*request);
      auto info = read_request.Process(manager);
      result.push_back(AnswerToJson(info));
    } else {
      throw std::runtime_error("Unsupported request");
    }
  }
  return result;
} 

std::vector<Request::RequestHolder> ParseRequests(const Dictionary& index,
                                                  Json::Node node) {
  const auto& json_requests = node.AsArray();
  size_t count = json_requests.size();
  std::vector<Request::RequestHolder> requests;
  requests.reserve(count);

  for (auto& req : json_requests) {

    if (auto request = MakeRequestFromJson(index, req)) {
      requests.push_back(std::move(request));
    }
  }
  return requests;
}


void JsonBusinessLogic() {
  using namespace bus;
  BusManager manager;
  auto doc = Json::Load(std::cin);
  auto& dict = doc.GetRoot().AsMap();

  const auto& modify_requests_nodes = dict.at("base_requests");
  const auto& read_requests_nodes = dict.at("stat_requests");

  auto mod_requests = ParseRequests(STR_TO_MOD_REQUEST_TYPE, modify_requests_nodes);
  auto read_requests = ParseRequests(STR_TO_READ_REQUEST_TYPE, read_requests_nodes);

  ProcessModifyRequests(mod_requests, manager);
  auto nodes = ProcessReadRequestsToJson(read_requests, manager);
  Json::ToJson(nodes, std::cout);
}

void FinalLogic() {
  using namespace bus;
  std::fstream Input;
  Input.open("json_input.txt", std::ios::in);

  auto doc = Json::Load(Input);
  auto& dict = doc.GetRoot().AsMap();

  const auto& routing = dict.at("routing_settings").AsMap();
  BusManagerWithRouter manager {
    routing.at("bus_velocity").AsDouble(), routing.at("bus_wait_time").AsDouble()
  };

  const auto& modify_requests_nodes = dict.at("base_requests");
  const auto& read_requests_nodes = dict.at("stat_requests");

  auto mod_requests =
      ParseRequests(STR_TO_MOD_REQUEST_TYPE, modify_requests_nodes);
  auto read_requests =
      ParseRequests(STR_TO_READ_REQUEST_TYPE, read_requests_nodes);

  ProcessModifyRequests(mod_requests, manager);
  manager.InitializeRouter();

  auto nodes = ProcessReadRequestsToJson(read_requests, manager);
  Json::ToJson(nodes, std::cout);
}

int main() {
  //TestRunner tr;
  //RUN_TEST(tr, MapToJsonTest);
  //RUN_TEST(tr, VecToJsonTest);
  //RUN_TEST(tr, BusRecordSimple);
  //RUN_TEST(tr, StopRecordSimple);
  //RUN_TEST(tr, AddGetStop);
  //RUN_TEST(tr, AddGetBus);
  //RUN_TEST(tr, TrimRightLeft);
  //RUN_TEST(tr, TrimEmpty);
  //RUN_TEST(tr, AddBusRequestParser);
  //RUN_TEST(tr, AddStopRequestParser);
  //RUN_TEST(tr, GetBusRequestParser);
  //RUN_TEST(tr, CalculateLengthTest);
  //RUN_TEST(tr, ParseModifyRequestsTest);
  //RUN_TEST(tr, AddBusThenStops);
  //RUN_TEST(tr, ToJsonAndBack);
  //LOG_DURATION("total");
  FinalLogic();
  return 0;
}



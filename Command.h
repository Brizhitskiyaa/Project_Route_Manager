#pragma once
#include "BusManager.h"
#include "StopsGraphManager.h"
#include "Parcing/Parcing.h"
#include "Json/json.h"

struct Request {
  using RequestHolder = std::unique_ptr<Request>;

  enum class Type {
    ADD_STOP,
    ADD_BUS,
    GET_BUS,
    GET_STOP,
    GET_ROUTE
  };


  Request(Type type) : type_(type) {};
  static RequestHolder Create(Type type);
  virtual void ParseFrom(std::string_view input) = 0;
  virtual void ParseFromJson(const Json::Node& node) = 0;
  virtual ~Request() = default;

  const Type type_;
};

const std::unordered_map<std::string_view, Request::Type>
    STR_TO_MOD_REQUEST_TYPE = {{"Bus", Request::Type::ADD_BUS}, {"Stop", Request::Type::ADD_STOP}};

const std::unordered_map<std::string_view, Request::Type>
    STR_TO_READ_REQUEST_TYPE = {
        {"Bus", Request::Type::GET_BUS}, {"Stop", Request::Type::GET_STOP}, {"Route", Request::Type::GET_ROUTE}};

const std::unordered_map<Request::Type, std::string_view>
    MOD_REQUEST_TYPE_TO_STR = {{Request::Type::ADD_BUS, "Bus"}, {Request::Type::ADD_STOP, "Stop"}};

const std::unordered_map<Request::Type, std::string_view>
    READ_REQUEST_TYPE_TO_STR = {
        {Request::Type::GET_BUS, "Bus"}, {Request::Type::GET_STOP, "Stop"}, {Request::Type::GET_ROUTE, "Route"}};



template <typename ResultType>
struct ReadRequest : Request{
  using Request::Request;
  virtual ResultType Process(const bus::BusManager& manager) const = 0;
  size_t request_id_{0};
};


struct ModifyRequest : Request {
  using Request::Request;
  virtual void Process(bus::BusManager& manager) const = 0;
};

// -------------------------------------------------------------------
// Add stop request

struct AddStopRequest : ModifyRequest {
  AddStopRequest() : ModifyRequest(Request::Type::ADD_STOP){};
  void ParseFromJson(const Json::Node& node) override;
  void ParseFrom(std::string_view input) override;
  void Process(bus::BusManager& manager) const override;

  std::string name_;
  bus::Coordinates pos_;
  std::vector<std::pair<std::string, size_t>> distances_;
};

// -----------------------------------------------------------
// Add bus request

struct AddBusRequest : ModifyRequest {
  AddBusRequest() : ModifyRequest(Request::Type::ADD_BUS){};
  void ParseFromJson(const Json::Node& node) override;
  void ParseFrom(std::string_view input) override;
  void Process(bus::BusManager& manager) const override;
  
  std::string name_;
  std::vector<std::string> stops_;
  bus::BusRecord::RouteType type_{};
};

// --------------------------------------------------------
// Get bus info request

struct BusInfo {
  size_t stops_on_route_{0};
  size_t unique_stops{0};
  double route_length_{0};
  double curvature_{0};
  size_t request_id_{0};
  uint8_t error_code_{0};
};

std::ostream& operator<<(std::ostream& out, const BusInfo& info);

struct GetBusInfoRequest : ReadRequest<BusInfo>{
  GetBusInfoRequest() : ReadRequest(Request::Type::GET_BUS){};
  void ParseFromJson(const Json::Node& node) override;
  void ParseFrom(std::string_view input) override;
  BusInfo Process(const bus::BusManager& manager) const override;

  std::string name_;
};

// --------------------------------------------------------------
// Get stop info request

struct StopInfo {
  StopInfo(const std::set<std::string_view>& buses) : buses_(std::addressof(buses)){};
  StopInfo() = default;
  std::optional<const std::set<std::string_view>*> buses_;
  size_t request_id_{0};
  //const std::set<std::string_view>& buses_;
  uint8_t error_code_{0};
};

std::ostream& operator<<(std::ostream& out, const StopInfo& info);

struct GetStopInfoRequest : ReadRequest<StopInfo> {
  GetStopInfoRequest() : ReadRequest(Request::Type::GET_STOP){};
  void ParseFromJson(const Json::Node& node) override;
  void ParseFrom(std::string_view input) override;
  StopInfo Process(const bus::BusManager& manager) const override;

  std::string name_;
};

// ---------------------------------------------------------------
// Get Route request 

struct RouteItem {


  RouteItem(double time) : time_(time) {};
  double time_{0.0};
};

struct BusRouteItem : RouteItem {
  BusRouteItem(std::string bus, double time, uint32_t span)
      : bus_(std::move(bus)), RouteItem(time), span_count_(span){};
  std::string bus_;
  uint32_t span_count_{0};
};

struct WaitRouteItem : RouteItem {
  WaitRouteItem(std::string stop, double time) : stop_name_(std::move(stop)), RouteItem(time) {};
  std::string stop_name_;
};

struct RouteInfo {
  RouteInfo() = default;
  using RouteItemVar = std::variant<BusRouteItem, WaitRouteItem>;
  std::vector<RouteItemVar> route_items_;
  double total_time_{0};
  size_t request_id_{0};
  uint8_t error_code_{0};
};

struct GetRouteInfoRequest : ReadRequest<RouteInfo> {
  GetRouteInfoRequest() : ReadRequest(Request::Type::GET_ROUTE){};

  void ParseFromJson(const Json::Node& node) override;
  void ParseFrom(std::string_view input) override;
  RouteInfo Process(const bus::BusManager& manager) const override;

  std::string from_;
  std::string to_;
};

class RouteInterpreter {
 public:
  using Router = bus::BusManagerWithRouter::Router;
  using Graph = bus::BusManagerWithRouter::GraphType;

  RouteInterpreter(const bus::BusManagerWithRouter& manager)
      : manager_(manager),
        router_(manager.GetRouter()),
        graph_(manager.GetRouteGraph()){};

  std::pair<std::vector<RouteInfo::RouteItemVar>, double> InterpretRoute(
      const Router::RouteInfo& info) const;

 private:
  std::optional<RouteInfo::RouteItemVar> GetItem(Router::RouteId id, size_t current_edge) const;


  const bus::BusManagerWithRouter& manager_;
  const Router& router_;
  const Graph& graph_;
};
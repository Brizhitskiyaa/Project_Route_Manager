#pragma once
#include "BusManager.h"
#include "graph.h"
#include "router.h"
#include <stdexcept>




namespace bus {
const std::string_view kWaitRouteType =
    "Wait";  // constant which distinguishes nodes describing waiting at a stop

enum class NodeType {
  BUS, WAIT
};

class BusManagerWithRouter : public bus::BusManager {
 public:
  using WeightWithSpan = utility::WeightWithSpan;
  using GraphType = Graph::DirectedWeightedGraph<WeightWithSpan>;
  using Router = Graph::Router<WeightWithSpan>;
  using IndexHash = utility::pair_hash<std::string_view, NodeType>;
  using CacheHash = utility::pair_hash<std::string_view, std::string_view>;
  using StopIndex =
      std::unordered_map<std::pair<std::string_view, NodeType>,
                         Graph::VertexId,
      IndexHash>;
  using ReverseStopIndex = std::unordered_map<
      Graph::VertexId, std::pair<std::string_view, NodeType>>;
  // Using simple cache without eviction here as an example
  using RouteCache = std::unordered_map<std::pair<std::string_view, std::string_view>, std::optional<Router::RouteInfo>, CacheHash>;

  BusManagerWithRouter(double velocity, double wait_time)
      : velocity_(velocity * 1000 / 60), wait_time_(wait_time){}; // velocity in metre per minute

  void InitializeRouter();
  
  [[nodiscard]] const GraphType& GetRouteGraph() const;
  [[nodiscard]] const Router& GetRouter() const;
  [[nodiscard]] std::optional<size_t> GetIndexFromStop(
      std::string_view name, NodeType type) const;
  [[nodiscard]] std::optional<std::pair<std::string_view, NodeType>>
  GetStopFromIndex(size_t num) const;

  // Checks for route in cache and invokes BuildNewRoute if cannot find
  [[nodiscard]] std::optional<Router::RouteInfo> GetRoute(
      std::string_view from, std::string_view to) const;



 private:
  // static void AddAllEdges(BusManagerWithRouter& manager);
  // another variant
  using StopsIter = std::vector<bus::StopRecordWeakPtr>::const_iterator;
  void AddAllEdges();
  template <typename Iter, typename = std::enable_if_t<std::is_same_v<
                               std::remove_const_t<typename Iter::value_type>,
                               bus::StopRecordWeakPtr>>>
  void AddEdgesHelper(Iter begin, Iter end, std::string_view route);

  void InitializeGraph();

  // Uses Router to get route information and saves it in cache
  [[nodiscard]] std::optional<Router::RouteInfo> BuildNewRoute(std::string_view from, std::string_view to) const;

  double velocity_;
  double wait_time_;
  std::optional<GraphType> graph_{std::nullopt};
  std::optional<Router> router_{std::nullopt};

  // Every stop is represented by the amount of nodes equaling to nummer of routes going through it + 1
  // (every stop has a separate node representing standing at the stop waiting for a bus)
  StopIndex index_;
  ReverseStopIndex reverse_index_;
  mutable RouteCache route_cache_;
};

}  // namespace bus
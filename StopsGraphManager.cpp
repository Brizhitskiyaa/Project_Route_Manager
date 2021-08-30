#include "StopsGraphManager.h"

namespace bus {
 
void BusManagerWithRouter::AddAllEdges() {
  using namespace Graph;
  auto& graph = graph_.value();

  for (const auto& [name, record_ptr] : bus_index_) {
    const BusRecord& bus_record = *(record_ptr.get());
    const auto& stops = bus_record.GetStops();

    for (auto it = stops.begin(); it != stops.end(); ++it) {
      const StopRecord& current_stop = *(it->lock().get());
      VertexId stop_wait_num =
          GetIndexFromStop(current_stop.GetName(), NodeType::WAIT).value();
      VertexId route_stop_num =
          GetIndexFromStop(current_stop.GetName(), NodeType::BUS).value();

      // first adding edges representing getting on the bus from the
      // stop
      graph.AddEdge({stop_wait_num, route_stop_num, WeightWithSpan{wait_time_}});
    }


    AddEdgesHelper(stops.begin(), stops.end(), bus_record.GetName());
    if (bus_record.GetType() == BusRecord::RouteType::Linear) {
      AddEdgesHelper(stops.rbegin(), stops.rend(), bus_record.GetName());
    }

  }
}

template <typename Iter, typename>
void BusManagerWithRouter::AddEdgesHelper(Iter begin, Iter end, std::string_view route) {
  using namespace Graph;
  size_t count = std::distance(begin, end);
  if (count < 2) {
    return;
  };

  std::vector<double> partial_sums(count, 0.0);
  double rolling_sum = 0.0;

  for (size_t i = 1; i < count; ++i) {
    double step_dist = RouteDistance(*(begin + i - 1), *(begin + i));
    partial_sums[i] = (rolling_sum += step_dist);
  }

  for (size_t i = 0; i < count; ++i) {
    for (size_t j = i + 1; j < count; ++j) {
      const StopRecord& current_stop = *((begin + i)->lock().get()); //kek
      const StopRecord& end_stop = *((begin + j)->lock().get());

      VertexId current_stop_num =
          GetIndexFromStop(current_stop.GetName(), NodeType::BUS).value();
      VertexId end_stop_num =
          GetIndexFromStop(end_stop.GetName(), NodeType::WAIT).value();
      size_t span_count = j - i;

      graph_->AddEdge(
          {current_stop_num, end_stop_num,
           WeightWithSpan{(partial_sums[j] - partial_sums[i]) / velocity_,
                          span_count, route}});
    }
  }
}

void BusManagerWithRouter::InitializeGraph() {
  size_t current_node = 0;

  for (const auto& [name, record_ptr] : stop_index_) {
    const StopRecord& stop_record = *(record_ptr.get());
    // First add wait node
    index_.insert({{name, NodeType::WAIT}, current_node});
    reverse_index_.insert({current_node++, {name, NodeType::WAIT}});
    // Then add node for routes going through stop

    index_.insert({{name, NodeType::BUS}, current_node});
    reverse_index_.insert({{current_node++, {name, NodeType::BUS}}});

  }

  if (current_node == 0) {
    return; // No stops - we are done
  }

  graph_ = GraphType(current_node);

  // Now iterate along the routes and add edges
  AddAllEdges();
}

std::optional<Graph::VertexId> BusManagerWithRouter::GetIndexFromStop(
    std::string_view name, NodeType route) const {
  auto iter = index_.find({name, route});
  if (iter == index_.end()) {
    return {};
  }
  return iter->second;
}

std::optional<std::pair<std::string_view, NodeType>> BusManagerWithRouter::GetStopFromIndex(size_t num) const {
  auto iter = reverse_index_.find(num);
  if (iter == reverse_index_.end()) {
    return {};
  }
  return iter->second;
}

void BusManagerWithRouter::InitializeRouter() {
  InitializeGraph();
  router_.emplace(Router(GetRouteGraph()));
}

const BusManagerWithRouter::GraphType& BusManagerWithRouter::GetRouteGraph()
    const {
  if (!graph_.has_value()) {
    throw std::runtime_error("Route graph has not been created");
  }
  return graph_.value();
}

std::optional<BusManagerWithRouter::Router::RouteInfo>
BusManagerWithRouter::BuildNewRoute(std::string_view from,
                                    std::string_view to) const {
  using namespace Graph;
  auto start_it = index_.find({from, NodeType::WAIT});
  auto end_it = index_.find({to, NodeType::WAIT});

  VertexId node_from = start_it->second;
  VertexId node_to = end_it->second;

  // prepare views to strings saved in database
  // we can get them from index here without looking for records specifically
  from = start_it->first.first;
  to = end_it->first.first;

  auto info = router_->BuildRoute(node_from, node_to);
  route_cache_.insert({{from, to}, info});
  return info;
}

std::optional<BusManagerWithRouter::Router::RouteInfo>
BusManagerWithRouter::GetRoute(std::string_view from, std::string_view to) const {
  auto it = route_cache_.find({from, to});
  if (it != route_cache_.end()) {
    return it->second;
  } else {
    return BuildNewRoute(from, to);
  }
}

 const BusManagerWithRouter::Router& BusManagerWithRouter::GetRouter() const {
  if (!router_.has_value()) {
    throw std::runtime_error("Router has not been initialized");
  }
  return router_.value();
 }

}  // namespace bus
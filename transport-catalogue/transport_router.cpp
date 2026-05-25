// Вставьте сюда решение из предыдущего спринт
// КАКОГО ЕЩЁ ПРЕДЫДУЩЕГО СПРИНТА? Можно лучше
#include "transport_router.h"
#include <stdexcept>

namespace transport::router {

TransportRouter::TransportRouter(const catalogue::TransportCatalogue& catalogue, const RoutingSettings& settings)
    : catalogue_(catalogue)
    , settings_(settings)
    , graph_(0)
{
    BuildGraph();
}

void TransportRouter::BuildGraph() {
    const auto& stops = catalogue_.GetAllStops();
    const size_t vertex_count = stops.size() * 2;
    
    graph_ = graph::DirectedWeightedGraph<double>(vertex_count);
    
    graph::VertexId vertex_id = 0;
    for (const auto& [stop_name, stop_ptr] : stops) {
        stop_to_wait_vertex_[stop_name] = vertex_id;
        vertex_id += 2;
    }
    
    const double bus_velocity_m_per_min = settings_.bus_velocity * 1000.0 / 60.0;
    
    for (const auto& [stop_name, stop_ptr] : stops) {
        graph::VertexId wait_vertex = stop_to_wait_vertex_[stop_name];
        graph::VertexId board_vertex = wait_vertex + 1;
        
        graph_.AddEdge({wait_vertex, board_vertex, static_cast<double>(settings_.bus_wait_time)});
        edge_info_.push_back({EdgeInfo::Type::WAIT, stop_name, 0});
    }
    
    for (const auto& [bus_name, bus_ptr] : catalogue_.GetAllBuses()) {
        const auto& stops_on_route = bus_ptr->stops;
        const size_t stop_count = stops_on_route.size();
        
        if (stop_count <= 1) continue;
        
        std::vector<const catalogue::StopData*> full_route_stops;
        for (const auto* stop : stops_on_route) {
            full_route_stops.push_back(stop);
        }
        
        if (!bus_ptr->is_roundtrip) {
            for (int i = static_cast<int>(stop_count) - 2; i >= 0; --i) {
                full_route_stops.push_back(stops_on_route[i]);
            }
        }
        
        for (size_t from_idx = 0; from_idx < full_route_stops.size(); ++from_idx) {
            const auto* from_stop = full_route_stops[from_idx];
            if (!from_stop) continue;
            
            graph::VertexId from_board_vertex = stop_to_wait_vertex_[from_stop->name] + 1;
            
            double cumulative_time = 0.0;
            int span_count = 0;
            
            for (size_t to_idx = from_idx + 1; to_idx < full_route_stops.size(); ++to_idx) {
                const auto* prev_stop = full_route_stops[to_idx - 1];
                const auto* curr_stop = full_route_stops[to_idx];
                
                if (!prev_stop || !curr_stop) continue;
                
                int distance = catalogue_.GetDistance(prev_stop, curr_stop);
                cumulative_time += distance / bus_velocity_m_per_min;
                ++span_count;
                
                graph::VertexId to_wait_vertex = stop_to_wait_vertex_[curr_stop->name];
                
                graph_.AddEdge({from_board_vertex, to_wait_vertex, cumulative_time});
                edge_info_.push_back({EdgeInfo::Type::BUS, bus_name, span_count});
            }
        }
    }
    
    router_ = std::make_unique<graph::Router<double>>(graph_);
}

std::optional<RouteInfo> TransportRouter::BuildRoute(std::string_view from, std::string_view to) const {
    if (from == to) {
        return RouteInfo{0.0, {}};
    }
    
    auto from_it = stop_to_wait_vertex_.find(from);
    auto to_it = stop_to_wait_vertex_.find(to);
    
    if (from_it == stop_to_wait_vertex_.end() || to_it == stop_to_wait_vertex_.end()) {
        return std::nullopt;
    }
    
    graph::VertexId from_wait_vertex = from_it->second;
    graph::VertexId to_wait_vertex = to_it->second;
    
    auto route = router_->BuildRoute(from_wait_vertex, to_wait_vertex);
    
    if (!route) {
        return std::nullopt;
    }
    
    RouteInfo result;
    result.total_time = route->weight;
    
    for (graph::EdgeId edge_id : route->edges) {
        const auto& edge_info = edge_info_[edge_id];
        
        if (edge_info.type == EdgeInfo::Type::WAIT) {
            WaitItem wait_item;
            wait_item.stop_name = std::string(edge_info.bus_or_stop_name);
            wait_item.time = static_cast<double>(settings_.bus_wait_time);
            result.items.push_back(wait_item);
        } else if (edge_info.type == EdgeInfo::Type::BUS) {
            BusItem bus_item;
            bus_item.bus = std::string(edge_info.bus_or_stop_name);
            bus_item.span_count = edge_info.span_count;
            bus_item.time = graph_.GetEdge(edge_id).weight;
            result.items.push_back(bus_item);
        }
    }
    
    return result;
}

} // namespace transport::router
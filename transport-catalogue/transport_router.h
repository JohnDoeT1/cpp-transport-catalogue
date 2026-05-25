#pragma once

#include "graph.h"
#include "router.h"
#include "transport_catalogue.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace transport::router {

struct RoutingSettings {
    int bus_wait_time = 0;
    double bus_velocity = 0.0;
};

struct WaitItem {
    std::string type = "Wait";
    std::string stop_name;
    double time = 0.0;
};

struct BusItem {
    std::string type = "Bus";
    std::string bus;
    int span_count = 0;
    double time = 0.0;
};

using RouteItem = std::variant<WaitItem, BusItem>;

struct RouteInfo {
    double total_time = 0.0;
    std::vector<RouteItem> items;
};

class TransportRouter {
public:
    TransportRouter(const catalogue::TransportCatalogue& catalogue, const RoutingSettings& settings);

    std::optional<RouteInfo> BuildRoute(std::string_view from, std::string_view to) const;

private:
    struct EdgeInfo {
        enum class Type { WAIT, BUS };
        Type type;
        std::string_view bus_or_stop_name;
        int span_count = 0;
    };

    void BuildGraph();

    const catalogue::TransportCatalogue& catalogue_;
    const RoutingSettings settings_;

    std::unordered_map<std::string_view, graph::VertexId> stop_to_wait_vertex_;
    
    graph::DirectedWeightedGraph<double> graph_;
    std::unique_ptr<graph::Router<double>> router_;
    
    std::vector<EdgeInfo> edge_info_;
};

} // namespace transport::router
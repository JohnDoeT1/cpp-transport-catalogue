#pragma once

#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include "geo.h"

namespace transport {
namespace catalogue {

struct Stop {
    std::string name;
    geo::Coordinates coordinates;
};

struct BusRoute {
    std::string name;
    std::vector<const Stop*> stops;
    bool is_circular;
};

struct RouteStats {
    size_t total_stops = 0;
    size_t unique_stops = 0;
    double total_distance = -1.0;
};

class TransportCatalogue {
public:
    void AddStop(const std::string& name, const geo::Coordinates& coords);
    void AddBus(const std::string& bus_name, 
                const std::vector<std::string_view>& stop_names,
                bool is_circular);

    const Stop* FindStop(const std::string& stop_name) const;
    const BusRoute* FindBus(const std::string& bus_name) const;

    RouteStats GetBusInfo(const std::string& bus_name) const;
    const std::unordered_set<std::string>& GetBusesByStop(const std::string& stop_name) const;

private:
    RouteStats GetRouteStats(const BusRoute& route) const;

    std::deque<Stop> stops_storage_;
    std::deque<BusRoute> routes_storage_;
    std::unordered_map<std::string, const Stop*> stops_;
    std::unordered_map<std::string, const BusRoute*> routes_;
    std::unordered_map<std::string, std::unordered_set<std::string>> stop_to_buses_;
};

} // namespace catalogue
} // namespace transport
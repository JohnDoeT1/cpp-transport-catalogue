#include "transport_catalogue.h"
#include <cmath>
#include <algorithm>

namespace transport {
namespace catalogue {

void
TransportCatalogue::AddStop(const std::string& name, double lat, double lon)
{
    stops_[name] = Stop{ name, {lat, lon} };
}

const Stop*
TransportCatalogue::FindStop(const std::string& name) const
{
    auto it = stops_.find(name);
    return it != stops_.end() ? &it->second : nullptr;
}

void
TransportCatalogue::AddBus(const std::string& bus_name,
    const std::vector<std::string>& stop_names,
    bool is_circular)
{
    std::vector<const Stop*> stop_pointers;
    stop_pointers.reserve(stop_names.size());

    for (const auto& stop_name : stop_names) {
        if (const Stop* stop = FindStop(stop_name)) {
            stop_pointers.push_back(stop);
            stop_to_buses_[stop_name].insert(bus_name);
        }
    }

    if (stop_pointers.empty())
        return;

    routes_[bus_name] = BusRoute{
        bus_name,
        std::move(stop_pointers),
        is_circular
    };
}

const BusRoute*
TransportCatalogue::FindBus(const std::string& bus_name) const
{
    auto it = routes_.find(bus_name);
    return it != routes_.end() ? &it->second : nullptr;
}

RouteStats
TransportCatalogue::GetBusInfo(const std::string& bus_name) const
{
    if (const BusRoute* route = FindBus(bus_name)) {
        return GetRouteStats(*route);
    }
    return {};
}

RouteStats
TransportCatalogue::GetRouteStats(const BusRoute& route) const
{
    RouteStats stats;
    stats.total_stops = route.stops.size();

    std::unordered_set<const Stop*> unique_stops;
    for (const Stop* stop : route.stops) {
        unique_stops.insert(stop);
    }
    stats.unique_stops = unique_stops.size();

    stats.total_distance = 0.0;
    if (route.is_circular) {
        for (size_t i = 0; i < route.stops.size(); ++i) {
            const Stop* from = route.stops[i];
            const Stop* to = route.stops[(i + 1) % route.stops.size()];
            stats.total_distance += geo::ComputeDistance(from->coordinates,
                to->coordinates);
        }
    } else {
        for (size_t i = 0; i + 1 < route.stops.size(); ++i) {
            const Stop* from = route.stops[i];
            const Stop* to = route.stops[i + 1];
            stats.total_distance += geo::ComputeDistance(from->coordinates,
                to->coordinates);
        }
    }

    return stats;
}

std::vector<std::string>
TransportCatalogue::GetBusesByStop(const std::string& stop_name) const
{
    std::vector<std::string> result;

    auto it = stop_to_buses_.find(stop_name);
    if (it != stop_to_buses_.end()) {
        result.reserve(it->second.size());
        for (const auto& bus : it->second) {
            result.push_back(bus);
        }
        std::sort(result.begin(), result.end());
    }

    return result;
}

} // namespace catalogue
} // namespace transport
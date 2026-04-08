#include "transport_catalogue.h"
#include <algorithm>
#include <cmath>

namespace transport {
namespace catalogue {

void TransportCatalogue::AddStop(const std::string& name, const geo::Coordinates& coords) {
    stops_storage_.push_back({name, coords});
    const Stop* stop_ptr = &stops_storage_.back();
    stops_[stop_ptr->name] = stop_ptr;
}

void TransportCatalogue::AddBus(const std::vector<std::string_view>& stop_names, const std::string& bus_name, bool is_circular) {
    std::vector<const Stop*> stops_ptrs;
    stops_ptrs.reserve(stop_names.size());

    for (auto stop_name : stop_names) {
        const Stop* stop = FindStop(stop_name);
        // Добавляем даже если stop == nullptr, это будет проверено при выводе
        stops_ptrs.push_back(stop);
    }
    
    // Не проверяем на пустоту, добавляем даже с nullptr
    routes_storage_.push_back({bus_name, std::move(stops_ptrs), is_circular});
    const BusRoute* route_ptr = &routes_storage_.back();
    routes_[route_ptr->name] = route_ptr;

    for (const Stop* stop : route_ptr->stops) {
        if (stop) {  // Проверяем, что указатель не нулевой
            stop_to_buses_[stop->name].insert(route_ptr->name);
        }
    }
}

void TransportCatalogue::SetDistance(const Stop* from, const Stop* to, int distance) {
    distances_[{from, to}] = distance;
}

int TransportCatalogue::GetDistance(const Stop* from, const Stop* to) const {
    if (!from || !to) return 0;
    
    auto it = distances_.find({from, to});
    if (it != distances_.end()) {
        return it->second;
    }
    
    // Если прямого расстояния нет, проверяем обратное
    auto rev_it = distances_.find({to, from});
    if (rev_it != distances_.end()) {
        return rev_it->second;
    }
    
    // Если никакое не задано, вычисляем географическое расстояние
    return static_cast<int>(geo::ComputeDistance(from->coordinates, to->coordinates));
}

const Stop* TransportCatalogue::FindStop(std::string_view stop_name) const {
    auto it = stops_.find(stop_name);
    return it != stops_.end() ? it->second : nullptr;
}

const BusRoute* TransportCatalogue::FindBus(std::string_view bus_name) const {
    auto it = routes_.find(bus_name);
    return it != routes_.end() ? it->second : nullptr;
}

RouteStats TransportCatalogue::GetBusInfo(std::string_view bus_name) const {
    if (const BusRoute* route = FindBus(bus_name)) return GetRouteStats(*route);
    return {};
}

RouteStats TransportCatalogue::GetRouteStats(const BusRoute& route) const {
    RouteStats stats;
    stats.total_stops = route.stops.size();
    
    // Подсчёт уникальных остановок (игнорируем nullptr)
    std::unordered_set<std::string_view> unique_stop_names;
    for (const Stop* stop : route.stops) {
        if (stop) {
            unique_stop_names.insert(stop->name);
        }
    }
    stats.unique_stops = unique_stop_names.size();

    if (route.stops.size() < 2) {
        stats.curvature = 0.0;
        stats.total_distance = 0.0;
        stats.geo_distance = 0.0;
        return stats;
    }

    stats.total_distance = 0.0;
    stats.geo_distance = 0.0;

    for (size_t i = 0; i + 1 < route.stops.size(); ++i) {
        const Stop* from = route.stops[i];
        const Stop* to = route.stops[i + 1];
        
        if (from && to) {
            stats.total_distance += GetDistance(from, to);
            stats.geo_distance += geo::ComputeDistance(from->coordinates, to->coordinates);
        }
    }

    stats.curvature = (stats.geo_distance > 0.0) ? stats.total_distance / stats.geo_distance : 0.0;
    return stats;
}

std::unordered_set<std::string_view> TransportCatalogue::GetBusesByStop(std::string_view stop_name) const {
    auto it = stop_to_buses_.find(stop_name);
    return it != stop_to_buses_.end() ? it->second : std::unordered_set<std::string_view>{};
}

} // namespace catalogue
} // namespace transport
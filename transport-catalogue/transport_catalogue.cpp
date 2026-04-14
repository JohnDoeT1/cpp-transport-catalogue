#include "transport_catalogue.h"
#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace transport {
namespace catalogue {

void TransportCatalogue::AddStop(const std::string& name, const geo::Coordinates& coords) {
    stops_storage_.push_back({name, coords});
    const StopData* stop_ptr = &stops_storage_.back();
    stops_[stop_ptr->name] = stop_ptr;
}

void TransportCatalogue::AddBus(const std::string& bus_name, bool is_roundtrip, 
                                const std::vector<std::string_view>& stop_names) {
    std::vector<const StopData*> stops_ptrs;
    stops_ptrs.reserve(stop_names.size());
    
    for (auto stop_name : stop_names) {
        const StopData* stop = FindStop(stop_name);
        stops_ptrs.push_back(stop);
    }
    
    buses_storage_.push_back({bus_name, std::move(stops_ptrs), is_roundtrip});
    const BusData* bus_ptr = &buses_storage_.back();
    buses_[bus_ptr->name] = bus_ptr;
    
    // Добавляем маршрут к каждой остановке (только уникальные остановки)
    std::unordered_set<std::string_view> unique_stops;
    for (const StopData* stop : bus_ptr->stops) {
        if (stop) {
            unique_stops.insert(stop->name);
        }
    }
    
    for (const auto& stop_name : unique_stops) {
        stop_to_buses_[stop_name].insert(bus_ptr->name);
    }
}

void TransportCatalogue::SetDistance(const std::string& from, const std::string& to, int distance) {
    const StopData* from_ptr = FindStop(from);
    const StopData* to_ptr = FindStop(to);
    if (from_ptr && to_ptr) {
        distances_[{from_ptr, to_ptr}] = distance;
    }
}

const StopData* TransportCatalogue::FindStop(std::string_view stop_name) const {
    auto it = stops_.find(stop_name);
    return it != stops_.end() ? it->second : nullptr;
}

const BusData* TransportCatalogue::FindBus(std::string_view bus_name) const {
    auto it = buses_.find(bus_name);
    return it != buses_.end() ? it->second : nullptr;
}

int TransportCatalogue::GetDistance(const StopData* from, const StopData* to) const {
    if (!from || !to) return 0;
    
    auto it = distances_.find({from, to});
    if (it != distances_.end()) {
        return it->second;
    }
    
    auto rev_it = distances_.find({to, from});
    if (rev_it != distances_.end()) {
        return rev_it->second;
    }
    
    return static_cast<int>(geo::ComputeDistance(from->coordinates, to->coordinates));
}

RouteStats TransportCatalogue::GetBusInfo(std::string_view bus_name) const {
    RouteStats stats;
    const BusData* bus = FindBus(bus_name);
    
    if (!bus || bus->stops.empty()) {
        return stats;
    }
    
    // Подсчет количества остановок
    if (bus->is_roundtrip) {
        stats.total_stops = bus->stops.size();
    } else {
        stats.total_stops = bus->stops.size() * 2 - 1;
    }
    
    // Подсчет уникальных остановок
    std::unordered_set<std::string_view> unique_names;
    for (const StopData* stop : bus->stops) {
        if (stop) {
            unique_names.insert(stop->name);
        }
    }
    stats.unique_stops = unique_names.size();
    
    // Расчет расстояний
    stats.route_length = 0;
    double geo_distance = 0.0;
    
    // Строим полный маршрут с учетом обратного пути для некольцевых
    std::vector<const StopData*> full_route;
    for (const auto* stop : bus->stops) {
        full_route.push_back(stop);
    }
    
    if (!bus->is_roundtrip) {
        for (int i = static_cast<int>(bus->stops.size()) - 2; i >= 0; --i) {
            full_route.push_back(bus->stops[i]);
        }
    }
    
    for (size_t i = 0; i + 1 < full_route.size(); ++i) {
        const StopData* from = full_route[i];
        const StopData* to = full_route[i + 1];
        
        if (from && to) {
            stats.route_length += GetDistance(from, to);
            geo_distance += geo::ComputeDistance(from->coordinates, to->coordinates);
        }
    }
    
    stats.curvature = (geo_distance > 0.0) ? stats.route_length / geo_distance : 0.0;
    return stats;
}

std::set<std::string> TransportCatalogue::GetBusesByStop(std::string_view stop_name) const {
    auto it = stop_to_buses_.find(stop_name);
    if (it != stop_to_buses_.end()) {
        return it->second;
    }
    return {};
}

} // namespace catalogue
} // namespace transport
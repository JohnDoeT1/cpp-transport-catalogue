#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <functional>
#include "geo.h"

namespace transport {
namespace catalogue {

struct PairHash {
    template <class T1, class T2>
    std::size_t operator() (const std::pair<T1, T2>& p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);
        return h1 ^ (h2 << 1);
    }
};

struct StopData {
    std::string name;
    geo::Coordinates coordinates;
};

struct BusData {
    std::string name;
    std::vector<const StopData*> stops;
    bool is_roundtrip;
};

struct RouteStats {
    size_t total_stops = 0;
    size_t unique_stops = 0;
    int route_length = 0;
    double curvature = 0.0;
};

class TransportCatalogue {
public:
    void AddStop(const std::string& name, const geo::Coordinates& coords);
    void AddBus(const std::string& bus_name, bool is_roundtrip, const std::vector<std::string_view>& stop_names);
    void SetDistance(const std::string& from, const std::string& to, int distance);
    
    const StopData* FindStop(std::string_view stop_name) const;
    const BusData* FindBus(std::string_view bus_name) const;
    
    RouteStats GetBusInfo(std::string_view bus_name) const;
    std::set<std::string> GetBusesByStop(std::string_view stop_name) const;
    
    const std::unordered_map<std::string_view, const BusData*>& GetAllBuses() const {
        return buses_;
    }
    
private:
    int GetDistance(const StopData* from, const StopData* to) const;
    
    std::deque<StopData> stops_storage_;
    std::deque<BusData> buses_storage_;
    std::unordered_map<std::string_view, const StopData*> stops_;
    std::unordered_map<std::string_view, const BusData*> buses_;
    std::unordered_map<std::string_view, std::set<std::string>> stop_to_buses_;
    std::unordered_map<std::pair<const StopData*, const StopData*>, int, PairHash> distances_;
};

} // namespace catalogue
} // namespace transport
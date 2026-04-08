#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include "geo.h"

namespace transport {
namespace catalogue {

// Хеш-функция для std::pair
struct PairHash {
    template <class T1, class T2>
    std::size_t operator() (const std::pair<T1, T2>& p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);
        return h1 ^ (h2 << 1);
    }
};

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
    double total_distance = -1.0;      // фактическое дорожное расстояние L
    double geo_distance = -1.0;        // географическое расстояние по прямой
    double curvature = -1.0;           // извилистость C = L / geo_distance
};

class TransportCatalogue {
public:
    void AddStop(const std::string& name, const geo::Coordinates& coords);
    void AddBus(const std::vector<std::string_view>& stop_names, const std::string& bus_name, bool is_circular);
    
    // Новые методы для работы с расстояниями между остановками
    void SetDistance(const Stop* from, const Stop* to, int distance);
    int GetDistance(const Stop* from, const Stop* to) const;

    const Stop* FindStop(std::string_view stop_name) const;
    const BusRoute* FindBus(std::string_view bus_name) const;

    RouteStats GetBusInfo(std::string_view bus_name) const;
    std::unordered_set<std::string_view> GetBusesByStop(std::string_view stop_name) const;

private:
    RouteStats GetRouteStats(const BusRoute& route) const;

    std::deque<Stop> stops_storage_;
    std::deque<BusRoute> routes_storage_;
    std::unordered_map<std::string_view, const Stop*> stops_;
    std::unordered_map<std::string_view, const BusRoute*> routes_;
    std::unordered_map<std::string_view, std::unordered_set<std::string_view>> stop_to_buses_;
    
    // Хранение расстояний между остановками
    // Ключ: pair<const Stop*, const Stop*>, значение: расстояние в метрах
    std::unordered_map<std::pair<const Stop*, const Stop*>, int, PairHash> distances_;
};

} // namespace catalogue
} // namespace transport
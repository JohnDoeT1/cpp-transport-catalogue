#include "stat_reader.h"
#include <iomanip>
#include <algorithm>
#include <vector>

namespace transport {
namespace output {

namespace {

std::string_view Trim(std::string_view string) {
    const auto start = string.find_first_not_of(' ');
    if (start == string.npos) return {};
    return string.substr(start, string.find_last_not_of(' ') + 1 - start);
}

void PrintBusInfo(const catalogue::TransportCatalogue& transport_catalogue,
                  std::string_view bus_name,
                  std::ostream& output) {
    auto stats = transport_catalogue.GetBusInfo(bus_name);
    if (stats.total_distance >= 0) {
        size_t stops_on_route = stats.total_stops;
        
        output << "Bus " << bus_name << ": "
               << stops_on_route << " stops on route, "
               << stats.unique_stops << " unique stops, "
               << std::fixed << std::setprecision(0) << stats.total_distance << " route length, "
               << std::setprecision(6) << stats.curvature << " curvature\n";
    } else {
        output << "Bus " << bus_name << ": not found\n";
    }
}

void PrintStopInfo(const catalogue::TransportCatalogue& transport_catalogue,
                   std::string_view stop_name,
                   std::ostream& output) {
    const catalogue::Stop* stop = transport_catalogue.FindStop(stop_name);
    if (!stop) {
        output << "Stop " << stop_name << ": not found\n";
        return;
    }

    auto buses_set = transport_catalogue.GetBusesByStop(stop_name);
    if (buses_set.empty()) {
        output << "Stop " << stop_name << ": no buses\n";
    } else {
        std::vector<std::string> buses(buses_set.begin(), buses_set.end());
        std::sort(buses.begin(), buses.end());
        output << "Stop " << stop_name << ": buses";
        for (const auto& bus : buses) output << " " << bus;
        output << "\n";
    }
}

} // namespace

void ParseAndPrintStat(const catalogue::TransportCatalogue& transport_catalogue,
                       std::string_view request,
                       std::ostream& output) {
    static const std::string_view kBusPrefix = "Bus ";
    static const std::string_view kStopPrefix = "Stop ";

    if (request.empty()) return;

    if (request.size() >= kBusPrefix.size() && request.substr(0, kBusPrefix.size()) == kBusPrefix) {
        std::string_view bus_name = Trim(request.substr(kBusPrefix.size()));
        PrintBusInfo(transport_catalogue, bus_name, output);
    } else if (request.size() >= kStopPrefix.size() && request.substr(0, kStopPrefix.size()) == kStopPrefix) {
        std::string_view stop_name = Trim(request.substr(kStopPrefix.size()));
        PrintStopInfo(transport_catalogue, stop_name, output);
    }
}

} // namespace output
} // namespace transport
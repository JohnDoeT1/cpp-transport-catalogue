#include "stat_reader.h"
#include <iomanip>
#include <algorithm>

namespace transport {
namespace output {

void
ParseAndPrintStat(const catalogue::TransportCatalogue& transport_catalogue,
    std::string_view request,
    std::ostream& output)
{
    static const std::string_view kBusPrefix = "Bus ";
    static const std::string_view kStopPrefix = "Stop ";

    if (request.empty()) {
        return;
    }

    if (request.starts_with(kBusPrefix)) {
        std::string bus_name(request.substr(kBusPrefix.size()));

        auto stats = transport_catalogue.GetBusInfo(bus_name);

        if (stats.total_distance >= 0) {
            output << "Bus " << bus_name << ": "
                << stats.total_stops << " stops on route, "
                << stats.unique_stops << " unique stops, "
                << std::fixed << std::setprecision(6)
                << stats.total_distance << " route length\n";
        } else {
            output << "Bus " << bus_name << ": not found\n";
        }
    } else if (request.starts_with(kStopPrefix)) {
        std::string stop_name(request.substr(kStopPrefix.size()));

        const catalogue::Stop* stop =
            transport_catalogue.FindStop(stop_name);

        if (stop == nullptr) {
            output << "Stop " << stop_name << ": not found\n";
        } else {
            auto buses = transport_catalogue.GetBusesByStop(stop_name);

            if (buses.empty()) {
                output << "Stop " << stop_name << ": no buses\n";
            } else {
                output << "Stop " << stop_name << ": buses";
                for (const auto& bus : buses) {
                    output << " " << bus;
                }
                output << "\n";
            }
        }
    }
}

} // namespace output
} // namespace transport
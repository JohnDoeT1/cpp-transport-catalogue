#include "input_reader.h"
#include <string>
#include <vector>
#include <cmath>

namespace transport {
namespace input {

namespace {

std::string_view Trim(std::string_view string) {
    const auto start = string.find_first_not_of(' ');
    if (start == string.npos) return {};
    return string.substr(start, string.find_last_not_of(' ') + 1 - start);
}

std::vector<std::string_view> ParseRoute(std::string_view route) {
    bool is_circular = route.find('>') != route.npos;
    std::vector<std::string_view> stops;

    char delim = is_circular ? '>' : '-';
    size_t pos = 0;

    while (pos < route.length()) {
        auto delim_pos = route.find(delim, pos);
        if (delim_pos == route.npos) delim_pos = route.length();
        auto stop = Trim(route.substr(pos, delim_pos - pos));
        if (!stop.empty()) stops.push_back(stop);
        pos = delim_pos + 1;
    }

    // Для некольцевого маршрута добавляем обратный путь
    if (!is_circular && stops.size() >= 2) {
        size_t original_size = stops.size();
        // Добавляем все остановки в обратном порядке, кроме последней
        // (которая станет первой на обратном пути)
        for (size_t i = original_size - 1; i > 0; --i) {
            stops.push_back(stops[i - 1]);
        }
    }

    return stops;
}

std::vector<StopDistance> ParseDistances(std::string_view description) {
    std::vector<StopDistance> distances;
    size_t pos = description.find(',', description.find(',') + 1);
    if (pos == std::string_view::npos) return distances;

    std::string_view dist_part = description.substr(pos + 1);
    size_t current = 0;

    while (current < dist_part.length()) {
        current = dist_part.find_first_not_of(" ,", current);
        if (current == std::string_view::npos) break;

        size_t num_end = dist_part.find('m', current);
        if (num_end == std::string_view::npos) break;

        int distance = std::stoi(std::string(dist_part.substr(current, num_end - current)));
        size_t to_pos = dist_part.find("to", num_end);
        if (to_pos == std::string_view::npos) break;

        size_t stop_start = dist_part.find_first_not_of(" ", to_pos + 2);
        if (stop_start == std::string_view::npos) break;

        size_t stop_end = dist_part.find(',', stop_start);
        if (stop_end == std::string_view::npos) stop_end = dist_part.length();

        distances.push_back({std::string(Trim(dist_part.substr(stop_start, stop_end - stop_start))), distance});
        current = stop_end;
    }

    return distances;
}

CommandDescription ParseCommandDescription(std::string_view line) {
    auto colon_pos = line.find(':');
    if (colon_pos == line.npos) return {};

    std::string_view before_colon = Trim(line.substr(0, colon_pos));
    auto space_pos = before_colon.find(' ');
    if (space_pos == before_colon.npos) return {};

    std::string command(before_colon.substr(0, space_pos));
    std::string id(before_colon.substr(space_pos + 1));

    std::string_view description = Trim(line.substr(colon_pos + 1));

    CommandDescription cmd;
    cmd.command = command;
    cmd.id = std::string(Trim(id));
    cmd.description = std::string(description);

    if (command == "Stop") {
        cmd.distances = ParseDistances(description);
    }

    return cmd;
}

} // namespace

void InputReader::ParseLine(std::string_view line) {
    auto cmd = ParseCommandDescription(line);
    if (cmd) commands_.push_back(std::move(cmd));
}

void InputReader::ApplyCommands(catalogue::TransportCatalogue& catalogue) const {
    // Сначала добавляем все остановки
    for (const auto& cmd : commands_) {
        if (cmd.command == "Stop") {
            std::string_view desc = cmd.description;
            
            // Ищем первую запятую после координаты lat
            size_t comma1 = desc.find(',');
            if (comma1 == std::string_view::npos) continue;
            
            // Ищем вторую запятую после координаты lng
            size_t comma2 = desc.find(',', comma1 + 1);
            
            std::string lat_str;
            std::string lng_str;
            
            if (comma2 == std::string_view::npos) {
                // Нет расстояний, только координаты
                lat_str = std::string(Trim(desc.substr(0, comma1)));
                lng_str = std::string(Trim(desc.substr(comma1 + 1)));
            } else {
                // Есть расстояния
                lat_str = std::string(Trim(desc.substr(0, comma1)));
                lng_str = std::string(Trim(desc.substr(comma1 + 1, comma2 - comma1 - 1)));
            }
            
            double lat = std::stod(lat_str);
            double lng = std::stod(lng_str);
            
            catalogue.AddStop(cmd.id, {lat, lng});
        }
    }

    // Затем устанавливаем расстояния
    for (const auto& cmd : commands_) {
        if (cmd.command == "Stop") {
            const auto* from_stop = catalogue.FindStop(cmd.id);
            if (!from_stop) continue;
            for (const auto& dist : cmd.distances) {
                const auto* to_stop = catalogue.FindStop(dist.stop_name);
                if (to_stop) {
                    catalogue.SetDistance(from_stop, to_stop, dist.distance);
                }
            }
        }
    }

    // Наконец, добавляем маршруты
    for (const auto& cmd : commands_) {
        if (cmd.command == "Bus") {
            bool is_circular = cmd.description.find('>') != std::string::npos;
            auto stops_views = ParseRoute(cmd.description);
            catalogue.AddBus(stops_views, cmd.id, is_circular);
        }
    }
}

} // namespace input
} // namespace transport
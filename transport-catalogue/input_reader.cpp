#include "input_reader.h"
#include <string>
#include <vector>
#include <cmath>

namespace transport {
namespace input {

namespace {

geo::Coordinates
ParseCoordinates(std::string_view str)
{
    static const double nan = std::nan("");

    auto not_space = str.find_first_not_of(' ');
    auto comma = str.find(',');

    if (comma == str.npos) {
        return { nan, nan };
    }

    auto not_space2 = str.find_first_not_of(' ', comma + 1);

    double lat = std::stod(std::string(str.substr(not_space,
        comma - not_space)));
    double lng = std::stod(std::string(str.substr(not_space2)));

    return { lat, lng };
}

std::string_view
Trim(std::string_view string)
{
    const auto start = string.find_first_not_of(' ');
    if (start == string.npos) {
        return {};
    }
    return string.substr(start,
        string.find_last_not_of(' ') + 1 - start);
}

std::vector<std::string_view>
Split(std::string_view string, char delim)
{
    std::vector<std::string_view> result;
    size_t pos = 0;

    while ((pos = string.find_first_not_of(' ', pos)) < string.length()) {
        auto delim_pos = string.find(delim, pos);
        if (delim_pos == string.npos) {
            delim_pos = string.size();
        }
        if (auto substr = Trim(string.substr(pos, delim_pos - pos));
            !substr.empty()) {
            result.push_back(substr);
        }
        pos = delim_pos + 1;
    }

    return result;
}

std::vector<std::string_view>
ParseRoute(std::string_view route)
{
    bool is_circular = route.find('>') != route.npos;
    auto stops = Split(route, is_circular ? '>' : '-');

    if (is_circular) {
        return stops;
    }

    /* Linear route "there and back" */
    std::vector<std::string_view> result = stops;
    if (stops.size() > 1) {
        for (int i = static_cast<int>(stops.size()) - 2; i >= 0; --i) {
            result.push_back(stops[i]);
        }
    }

    return result;
}

CommandDescription
ParseCommandDescription(std::string_view line)
{
    auto colon_pos = line.find(':');
    if (colon_pos == line.npos)
        return {};

    auto first_space = line.find(' ');
    if (first_space == line.npos || first_space >= colon_pos)
        return {};

    std::string command(line.substr(0, first_space));
    std::string id(line.substr(first_space + 1,
        colon_pos - first_space - 1));
    std::string description(line.substr(colon_pos + 1));

    return {command, id, description};
}

} // anonymous namespace

void
InputReader::ParseLine(std::string_view line)
{
    auto command_description = ParseCommandDescription(line);
    if (command_description) {
        commands_.push_back(std::move(command_description));
    }
}

void
InputReader::ApplyCommands(catalogue::TransportCatalogue& catalogue) const
{
    /* Add all stops first */
    for (const auto& cmd : commands_) {
        if (cmd.command == "Stop") {
            auto coords = ParseCoordinates(cmd.description);
            catalogue.AddStop(cmd.id, coords.lat, coords.lng);
        }
    }

    /* Then add all buses */
    for (const auto& cmd : commands_) {
        if (cmd.command == "Bus") {
            bool is_circular = cmd.description.find('>') != std::string::npos;
            auto stop_names_views = ParseRoute(cmd.description);

            std::vector<std::string> stop_names;
            stop_names.reserve(stop_names_views.size());
            for (auto sv : stop_names_views) {
                stop_names.emplace_back(sv);
            }

            catalogue.AddBus(cmd.id, stop_names, is_circular);
        }
    }
}

} // namespace input
} // namespace transport
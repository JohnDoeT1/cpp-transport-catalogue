#pragma once

#include <string>
#include <vector>
#include <string_view>
#include "transport_catalogue.h"

namespace transport {
namespace input {

struct StopDistance {
    std::string stop_name;
    int distance;
};

struct CommandDescription {
    explicit operator bool() const
    {
        return !command.empty();
    }

    bool operator!() const
    {
        return !operator bool();
    }

    std::string command;
    std::string id;
    std::string description;
    std::vector<StopDistance> distances;  // Новое поле для хранения расстояний до других остановок
};

class InputReader {
public:
    void ParseLine(std::string_view line);
    void ApplyCommands(catalogue::TransportCatalogue& catalogue) const;

private:
    std::vector<CommandDescription> commands_;
};

} // namespace input
} // namespace transport
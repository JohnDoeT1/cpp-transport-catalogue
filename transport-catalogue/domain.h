#pragma once

#include <string>
#include <vector>
#include <string_view>

namespace domain {

struct Stop {
    std::string name;
    double lat;
    double lng;
};

struct Bus {
    std::string name;
    std::vector<std::string_view> stops;
    bool is_roundtrip;
};

} // namespace domain
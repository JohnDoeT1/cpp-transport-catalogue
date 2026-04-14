#pragma once

#include "transport_catalogue.h"
#include "json.h"
#include "map_renderer.h"
#include <string>
#include <vector>
#include <optional>

namespace json_reader {

class JsonReader {
public:
    JsonReader(transport::catalogue::TransportCatalogue& catalogue);
    
    void ParseBaseRequests(const json::Node& base_requests);
    renderer::RenderSettings ParseRenderSettings(const json::Node& render_settings);
    
    json::Document ProcessStatRequests(const json::Node& stat_requests,
                                       const std::optional<renderer::RenderSettings>& render_settings = std::nullopt);
    
private:
    void ParseStop(const json::Dict& stop_dict);
    void ParseStopDistances(const json::Dict& stop_dict);
    void ParseBus(const json::Dict& bus_dict);
    
    json::Node ProcessStopRequest(const json::Dict& request);
    json::Node ProcessBusRequest(const json::Dict& request);
    json::Node ProcessMapRequest(const json::Dict& request, const renderer::MapRenderer& renderer);
    
    svg::Color ParseColor(const json::Node& color_node);
    
    transport::catalogue::TransportCatalogue& catalogue_;
};

} // namespace json_reader
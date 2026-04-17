#include "json_reader.h"
#include "json_builder.h"
#include <sstream>
#include <algorithm>
#include <set>

namespace json_reader {

JsonReader::JsonReader(transport::catalogue::TransportCatalogue& catalogue)
    : catalogue_(catalogue) {
}

void JsonReader::ParseStop(const json::Dict& stop_dict) {
    std::string name = stop_dict.at("name").AsString();
    double lat = stop_dict.at("latitude").AsDouble();
    double lng = stop_dict.at("longitude").AsDouble();
    
    catalogue_.AddStop(name, {lat, lng});
}

void JsonReader::ParseStopDistances(const json::Dict& stop_dict) {
    std::string name = stop_dict.at("name").AsString();
    
    if (stop_dict.count("road_distances")) {
        const auto& distances = stop_dict.at("road_distances").AsDict();
        for (const auto& [to_name, distance_node] : distances) {
            catalogue_.SetDistance(name, to_name, distance_node.AsInt());
        }
    }
}

void JsonReader::ParseBus(const json::Dict& bus_dict) {
    std::string name = bus_dict.at("name").AsString();
    bool is_roundtrip = bus_dict.at("is_roundtrip").AsBool();
    
    std::vector<std::string_view> stops;
    for (const auto& stop_node : bus_dict.at("stops").AsArray()) {
        stops.push_back(stop_node.AsString());
    }
    
    catalogue_.AddBus(name, is_roundtrip, stops);
}

void JsonReader::ParseBaseRequests(const json::Node& base_requests) {
    const auto& requests = base_requests.AsArray();
    
    // Первый проход: добавляем все остановки (только координаты)
    for (const auto& request : requests) {
        const auto& dict = request.AsDict();
        if (dict.at("type").AsString() == "Stop") {
            ParseStop(dict);
        }
    }
    
    // Второй проход: добавляем расстояния между остановками
    for (const auto& request : requests) {
        const auto& dict = request.AsDict();
        if (dict.at("type").AsString() == "Stop") {
            ParseStopDistances(dict);
        }
    }
    
    // Третий проход: добавляем маршруты
    for (const auto& request : requests) {
        const auto& dict = request.AsDict();
        if (dict.at("type").AsString() == "Bus") {
            ParseBus(dict);
        }
    }
}

svg::Color JsonReader::ParseColor(const json::Node& color_node) {
    if (color_node.IsString()) {
        return color_node.AsString();
    } else if (color_node.IsArray()) {
        const auto& arr = color_node.AsArray();
        if (arr.size() == 3) {
            return svg::Rgb(
                static_cast<uint8_t>(arr[0].AsInt()), 
                static_cast<uint8_t>(arr[1].AsInt()), 
                static_cast<uint8_t>(arr[2].AsInt())
            );
        } else if (arr.size() == 4) {
            return svg::Rgba(
                static_cast<uint8_t>(arr[0].AsInt()), 
                static_cast<uint8_t>(arr[1].AsInt()), 
                static_cast<uint8_t>(arr[2].AsInt()), 
                arr[3].AsDouble()
            );
        }
    }
    return svg::NoneColor;
}

renderer::RenderSettings JsonReader::ParseRenderSettings(const json::Node& render_settings) {
    renderer::RenderSettings settings;
    const auto& dict = render_settings.AsDict();
    
    settings.width = dict.at("width").AsDouble();
    settings.height = dict.at("height").AsDouble();
    settings.padding = dict.at("padding").AsDouble();
    settings.line_width = dict.at("line_width").AsDouble();
    settings.stop_radius = dict.at("stop_radius").AsDouble();
    settings.bus_label_font_size = dict.at("bus_label_font_size").AsInt();
    
    const auto& bus_offset = dict.at("bus_label_offset").AsArray();
    settings.bus_label_offset = {bus_offset[0].AsDouble(), bus_offset[1].AsDouble()};
    
    settings.stop_label_font_size = dict.at("stop_label_font_size").AsInt();
    
    const auto& stop_offset = dict.at("stop_label_offset").AsArray();
    settings.stop_label_offset = {stop_offset[0].AsDouble(), stop_offset[1].AsDouble()};
    
    settings.underlayer_color = ParseColor(dict.at("underlayer_color"));
    settings.underlayer_width = dict.at("underlayer_width").AsDouble();
    
    const auto& palette = dict.at("color_palette").AsArray();
    for (const auto& color : palette) {
        settings.color_palette.push_back(ParseColor(color));
    }
    
    return settings;
}

json::Node JsonReader::ProcessStopRequest(const json::Dict& request) {
    std::string name = request.at("name").AsString();
    int id = request.at("id").AsInt();
    
    const auto* stop = catalogue_.FindStop(name);
    
    if (!stop) {
        return json::Builder{}
            .StartDict()
                .Key("request_id").Value(id)
                .Key("error_message").Value(std::string("not found"))
            .EndDict()
            .Build();
    }
    
    auto buses = catalogue_.GetBusesByStop(name);
    json::Array buses_array;
    for (const auto& bus : buses) {
        buses_array.push_back(std::string(bus));
    }
    
    return json::Builder{}
        .StartDict()
            .Key("request_id").Value(id)
            .Key("buses").Value(buses_array)
        .EndDict()
        .Build();
}

json::Node JsonReader::ProcessBusRequest(const json::Dict& request) {
    std::string name = request.at("name").AsString();
    int id = request.at("id").AsInt();
    
    const auto* bus = catalogue_.FindBus(name);
    
    if (!bus) {
        return json::Builder{}
            .StartDict()
                .Key("request_id").Value(id)
                .Key("error_message").Value(std::string("not found"))
            .EndDict()
            .Build();
    }
    
    auto stats = catalogue_.GetBusInfo(name);
    
    return json::Builder{}
        .StartDict()
            .Key("request_id").Value(id)
            .Key("curvature").Value(stats.curvature)
            .Key("route_length").Value(static_cast<int>(stats.route_length))
            .Key("stop_count").Value(static_cast<int>(stats.total_stops))
            .Key("unique_stop_count").Value(static_cast<int>(stats.unique_stops))
        .EndDict()
        .Build();
}

json::Node JsonReader::ProcessMapRequest(const json::Dict& request,
                                          const renderer::MapRenderer& renderer) {
    int id = request.at("id").AsInt();
    
    std::ostringstream svg_stream;
    
    auto doc = renderer.Render(
        catalogue_.GetAllBuses(),
        catalogue_.GetAllStops()
    );
    
    doc.Render(svg_stream);
    
    return json::Builder{}
        .StartDict()
            .Key("request_id").Value(id)
            .Key("map").Value(svg_stream.str())
        .EndDict()
        .Build();
}

json::Document JsonReader::ProcessStatRequests(const json::Node& stat_requests,
                                                const std::optional<renderer::RenderSettings>& render_settings) {
    json::Builder builder;
    builder.StartArray();
    
    std::optional<renderer::MapRenderer> renderer;
    if (render_settings) {
        renderer.emplace(*render_settings);
    }
    
    for (const auto& request : stat_requests.AsArray()) {
        const auto& dict = request.AsDict();
        std::string type = dict.at("type").AsString();
        
        if (type == "Stop") {
            auto node = ProcessStopRequest(dict);
            std::visit([&builder](auto&& arg) {
                builder.Value(std::forward<decltype(arg)>(arg));
            }, node.GetValue());
        } else if (type == "Bus") {
            auto node = ProcessBusRequest(dict);
            std::visit([&builder](auto&& arg) {
                builder.Value(std::forward<decltype(arg)>(arg));
            }, node.GetValue());
        } else if (type == "Map" && renderer) {
            auto node = ProcessMapRequest(dict, *renderer);
            std::visit([&builder](auto&& arg) {
                builder.Value(std::forward<decltype(arg)>(arg));
            }, node.GetValue());
        }
    }
    
    return json::Document(builder.EndArray().Build());
}

} // namespace json_reader
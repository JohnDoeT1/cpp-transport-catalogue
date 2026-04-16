#include "map_renderer.h"
#include "transport_catalogue.h"
#include <algorithm>
#include <set>
#include <unordered_set>
#include <vector>

namespace renderer {

MapRenderer::MapRenderer(const RenderSettings& settings) : settings_(settings) {}

svg::Document MapRenderer::Render(
    const std::unordered_map<std::string_view, const transport::catalogue::BusData*>& buses,
    const std::unordered_map<std::string_view, const transport::catalogue::StopData*>& /* stops */) const {
    
    svg::Document doc;
    
    std::vector<geo::Coordinates> all_coords;
    
    for (const auto& [name, bus] : buses) {
        if (!bus->stops.empty()) {
            for (const auto* stop : bus->stops) {
                if (stop) {
                    all_coords.push_back(stop->coordinates);
                }
            }
        }
    }
    
    if (all_coords.empty()) {
        return doc;
    }
    
    SphereProjector projector(all_coords.begin(), all_coords.end(),
                             settings_.width, settings_.height, settings_.padding);
    
    RenderBusLines(doc, buses, projector);
    RenderBusLabels(doc, buses, projector);
    RenderStopCircles(doc, buses, projector);
    RenderStopLabels(doc, buses, projector);
    
    return doc;
}

void MapRenderer::RenderBusLines(
    svg::Document& doc,
    const std::unordered_map<std::string_view, const transport::catalogue::BusData*>& buses,
    const SphereProjector& projector) const {
    
    std::vector<std::string> bus_names;
    for (const auto& [name, bus] : buses) {
        if (!bus->stops.empty()) {
            bus_names.push_back(std::string(name));
        }
    }
    std::sort(bus_names.begin(), bus_names.end());
    
    size_t color_index = 0;
    for (const auto& bus_name : bus_names) {
        auto it = buses.find(bus_name);
        if (it == buses.end()) continue;
        
        const auto* bus = it->second;
        if (!bus || bus->stops.empty()) continue;
        
        svg::Polyline polyline;
        
        for (const auto* stop : bus->stops) {
            if (stop) {
                polyline.AddPoint(projector(stop->coordinates));
            }
        }
        
        if (!bus->is_roundtrip) {
            for (int i = static_cast<int>(bus->stops.size()) - 2; i >= 0; --i) {
                const auto* stop = bus->stops[i];
                if (stop) {
                    polyline.AddPoint(projector(stop->coordinates));
                }
            }
        }
        
        polyline.SetFillColor(svg::NoneColor)
                .SetStrokeColor(settings_.color_palette[color_index])
                .SetStrokeWidth(settings_.line_width)
                .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        
        doc.Add(std::move(polyline));
        
        color_index = (color_index + 1) % settings_.color_palette.size();
    }
}

void MapRenderer::RenderBusLabels(
    svg::Document& doc,
    const std::unordered_map<std::string_view, const transport::catalogue::BusData*>& buses,
    const SphereProjector& projector) const {
    
    std::vector<std::string> bus_names;
    for (const auto& [name, bus] : buses) {
        if (!bus->stops.empty()) {
            bus_names.push_back(std::string(name));
        }
    }
    std::sort(bus_names.begin(), bus_names.end());
    
    size_t color_index = 0;
    for (const auto& bus_name : bus_names) {
        auto it = buses.find(bus_name);
        if (it == buses.end()) continue;
        
        const auto* bus = it->second;
        if (!bus || bus->stops.empty()) continue;
        
        const auto* first_stop = bus->stops.front();
        if (first_stop) {
            svg::Text underlayer;
            underlayer.SetPosition(projector(first_stop->coordinates))
                      .SetOffset(settings_.bus_label_offset)
                      .SetFontSize(settings_.bus_label_font_size)
                      .SetFontFamily("Verdana")
                      .SetFontWeight("bold")
                      .SetData(bus_name)
                      .SetFillColor(settings_.underlayer_color)
                      .SetStrokeColor(settings_.underlayer_color)
                      .SetStrokeWidth(settings_.underlayer_width)
                      .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                      .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
            doc.Add(underlayer);
            
            svg::Text text;
            text.SetPosition(projector(first_stop->coordinates))
                .SetOffset(settings_.bus_label_offset)
                .SetFontSize(settings_.bus_label_font_size)
                .SetFontFamily("Verdana")
                .SetFontWeight("bold")
                .SetData(bus_name)
                .SetFillColor(settings_.color_palette[color_index]);
            doc.Add(text);
        }
        
        if (!bus->is_roundtrip) {
            const auto* last_stop = bus->stops.back();
            if (last_stop && last_stop != first_stop) {
                svg::Text underlayer;
                underlayer.SetPosition(projector(last_stop->coordinates))
                          .SetOffset(settings_.bus_label_offset)
                          .SetFontSize(settings_.bus_label_font_size)
                          .SetFontFamily("Verdana")
                          .SetFontWeight("bold")
                          .SetData(bus_name)
                          .SetFillColor(settings_.underlayer_color)
                          .SetStrokeColor(settings_.underlayer_color)
                          .SetStrokeWidth(settings_.underlayer_width)
                          .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                          .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
                doc.Add(underlayer);
                
                svg::Text text;
                text.SetPosition(projector(last_stop->coordinates))
                    .SetOffset(settings_.bus_label_offset)
                    .SetFontSize(settings_.bus_label_font_size)
                    .SetFontFamily("Verdana")
                    .SetFontWeight("bold")
                    .SetData(bus_name)
                    .SetFillColor(settings_.color_palette[color_index]);
                doc.Add(text);
            }
        }
        
        color_index = (color_index + 1) % settings_.color_palette.size();
    }
}

void MapRenderer::RenderStopCircles(
    svg::Document& doc,
    const std::unordered_map<std::string_view, const transport::catalogue::BusData*>& buses,
    const SphereProjector& projector) const {
    
    // Собираем уникальные указатели на остановки
    std::set<const transport::catalogue::StopData*> used_stops;
    for (const auto& [name, bus] : buses) {
        for (const auto* stop : bus->stops) {
            if (stop) {
                used_stops.insert(stop);
            }
        }
    }
    
    // Преобразуем в вектор и сортируем по алфавиту
    std::vector<const transport::catalogue::StopData*> sorted_stops(used_stops.begin(), used_stops.end());
    std::sort(sorted_stops.begin(), sorted_stops.end(),
              [](const transport::catalogue::StopData* a, const transport::catalogue::StopData* b) {
                  return a->name < b->name;
              });
    
    for (const auto* stop : sorted_stops) {
        svg::Circle circle;
        circle.SetCenter(projector(stop->coordinates))
              .SetRadius(settings_.stop_radius)
              .SetFillColor("white");
        doc.Add(circle);
    }
}

void MapRenderer::RenderStopLabels(
    svg::Document& doc,
    const std::unordered_map<std::string_view, const transport::catalogue::BusData*>& buses,
    const SphereProjector& projector) const {
    
    // Собираем уникальные указатели на остановки
    std::set<const transport::catalogue::StopData*> used_stops;
    for (const auto& [name, bus] : buses) {
        for (const auto* stop : bus->stops) {
            if (stop) {
                used_stops.insert(stop);
            }
        }
    }
    
    // Преобразуем в вектор и сортируем по алфавиту
    std::vector<const transport::catalogue::StopData*> sorted_stops(used_stops.begin(), used_stops.end());
    std::sort(sorted_stops.begin(), sorted_stops.end(),
              [](const transport::catalogue::StopData* a, const transport::catalogue::StopData* b) {
                  return a->name < b->name;
              });
    
    for (const auto* stop : sorted_stops) {
        svg::Text underlayer;
        underlayer.SetPosition(projector(stop->coordinates))
                  .SetOffset(settings_.stop_label_offset)
                  .SetFontSize(settings_.stop_label_font_size)
                  .SetFontFamily("Verdana")
                  .SetData(stop->name)
                  .SetFillColor(settings_.underlayer_color)
                  .SetStrokeColor(settings_.underlayer_color)
                  .SetStrokeWidth(settings_.underlayer_width)
                  .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                  .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        doc.Add(underlayer);
        
        svg::Text text;
        text.SetPosition(projector(stop->coordinates))
            .SetOffset(settings_.stop_label_offset)
            .SetFontSize(settings_.stop_label_font_size)
            .SetFontFamily("Verdana")
            .SetData(stop->name)
            .SetFillColor("black");
        doc.Add(text);
    }
}

} // namespace renderer
#include "map_renderer.h"
#include "transport_catalogue.h"
#include <algorithm>
#include <set>
#include <unordered_set>

namespace renderer {

MapRenderer::MapRenderer(const RenderSettings& settings) : settings_(settings) {}

svg::Document MapRenderer::Render(
    const std::unordered_map<std::string_view, const transport::catalogue::BusData*>& buses,
    std::function<const transport::catalogue::StopData*(std::string_view)> find_stop) const {
    
    svg::Document doc;
    
    // Собираем все координаты остановок, через которые проходят маршруты
    std::vector<geo::Coordinates> all_coords;
    std::set<std::string> bus_names;
    
    // Получаем все маршруты
    for (const auto& [name, bus] : buses) {
        if (!bus->stops.empty()) {
            bus_names.insert(std::string(name));
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
    
    // Рисуем все слои в правильном порядке
    RenderBusLines(doc, buses, projector);
    RenderBusLabels(doc, buses, projector);
    RenderStopCircles(doc, buses, find_stop, projector);
    RenderStopLabels(doc, buses, find_stop, projector);
    
    return doc;
}

void MapRenderer::RenderBusLines(
    svg::Document& doc,
    const std::unordered_map<std::string_view, const transport::catalogue::BusData*>& buses,
    const SphereProjector& projector) const {
    
    // Получаем отсортированные названия маршрутов
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
        if (!bus || bus->stops.empty()) {
            continue;
        }
        
        svg::Polyline polyline;
        
        // Добавляем все точки маршрута
        for (const auto* stop : bus->stops) {
            if (stop) {
                polyline.AddPoint(projector(stop->coordinates));
            }
        }
        
        // Для некольцевого маршрута добавляем обратный путь
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
    
    // Получаем отсортированные названия маршрутов
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
        if (!bus || bus->stops.empty()) {
            continue;
        }
        
        // Добавляем метку на первой остановке
        const auto* first_stop = bus->stops.front();
        if (first_stop) {
            // Подложка
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
            
            // Основной текст
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
        
        // Для некольцевого маршрута добавляем метку на последней остановке
        if (!bus->is_roundtrip) {
            const auto* last_stop = bus->stops.back();
            if (last_stop && last_stop != first_stop) {
                // Подложка
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
                
                // Основной текст
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
    std::function<const transport::catalogue::StopData*(std::string_view)> find_stop,
    const SphereProjector& projector) const {
    
    // Собираем все остановки, через которые проходит хотя бы один маршрут
    std::set<std::string> used_stops;
    for (const auto& [name, bus] : buses) {
        for (const auto* stop : bus->stops) {
            if (stop) {
                used_stops.insert(stop->name);
            }
        }
    }
    
    for (const auto& stop_name : used_stops) {
        const auto* stop = find_stop(stop_name);
        if (stop) {
            svg::Circle circle;
            circle.SetCenter(projector(stop->coordinates))
                  .SetRadius(settings_.stop_radius)
                  .SetFillColor("white");
            doc.Add(circle);
        }
    }
}

void MapRenderer::RenderStopLabels(
    svg::Document& doc,
    const std::unordered_map<std::string_view, const transport::catalogue::BusData*>& buses,
    std::function<const transport::catalogue::StopData*(std::string_view)> find_stop,
    const SphereProjector& projector) const {
    
    // Собираем все остановки, через которые проходит хотя бы один маршрут
    std::set<std::string> used_stops;
    for (const auto& [name, bus] : buses) {
        for (const auto* stop : bus->stops) {
            if (stop) {
                used_stops.insert(stop->name);
            }
        }
    }
    
    for (const auto& stop_name : used_stops) {
        const auto* stop = find_stop(stop_name);
        if (stop) {
            // Подложка
            svg::Text underlayer;
            underlayer.SetPosition(projector(stop->coordinates))
                      .SetOffset(settings_.stop_label_offset)
                      .SetFontSize(settings_.stop_label_font_size)
                      .SetFontFamily("Verdana")
                      .SetData(stop_name)
                      .SetFillColor(settings_.underlayer_color)
                      .SetStrokeColor(settings_.underlayer_color)
                      .SetStrokeWidth(settings_.underlayer_width)
                      .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                      .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
            doc.Add(underlayer);
            
            // Основной текст
            svg::Text text;
            text.SetPosition(projector(stop->coordinates))
                .SetOffset(settings_.stop_label_offset)
                .SetFontSize(settings_.stop_label_font_size)
                .SetFontFamily("Verdana")
                .SetData(stop_name)
                .SetFillColor("black");
            doc.Add(text);
        }
    }
}

} // namespace renderer
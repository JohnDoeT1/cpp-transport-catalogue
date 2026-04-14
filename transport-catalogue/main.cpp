#include "json.h"
#include "json_reader.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include <iostream>
#include <sstream>
#include <optional>

int main() {
    using namespace std;
    
    // Читаем JSON из stdin
    json::Document input_doc = json::Load(cin);
    const auto& root = input_doc.GetRoot().AsMap();
    
    // Создаем транспортный каталог
    transport::catalogue::TransportCatalogue catalogue;
    
    // Создаем reader
    json_reader::JsonReader reader(catalogue);
    
    // Обрабатываем базовые запросы
    if (root.count("base_requests")) {
        reader.ParseBaseRequests(root.at("base_requests"));
    }
    
    // Обрабатываем настройки рендеринга
    std::optional<renderer::RenderSettings> render_settings;
    if (root.count("render_settings")) {
        render_settings = reader.ParseRenderSettings(root.at("render_settings"));
    }
    
    // Обрабатываем запросы статистики и выводим JSON-ответ
    if (root.count("stat_requests")) {
        auto output_doc = reader.ProcessStatRequests(root.at("stat_requests"), render_settings);
        json::Print(output_doc, cout);
    }
    
    return 0;
}
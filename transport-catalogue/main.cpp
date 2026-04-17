#include "json_reader.h"
#include "json.h"
#include "json_builder.h"
#include "transport_catalogue.h"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

int main() {
    transport::catalogue::TransportCatalogue catalogue;
    json_reader::JsonReader reader(catalogue);
    
    // Читаем весь ввод
    string input_data;
    string line;
    while (getline(cin, line)) {
        input_data += line;
    }
    
    istringstream input_stream(input_data);
    auto doc = json::Load(input_stream);
    const auto& root = doc.GetRoot().AsDict();
    
    // Парсим базовые запросы
    if (root.count("base_requests")) {
        reader.ParseBaseRequests(root.at("base_requests"));
    }
    
    // Парсим настройки рендеринга
    std::optional<renderer::RenderSettings> render_settings;
    if (root.count("render_settings")) {
        render_settings = reader.ParseRenderSettings(root.at("render_settings"));
    }
    
    // Обрабатываем stat запросы
    if (root.count("stat_requests")) {
        auto result = reader.ProcessStatRequests(root.at("stat_requests"), render_settings);
        json::Print(result, cout);
    }
    
    return 0;
}
#include "json_reader.h"
#include "json.h"
#include "json_builder.h"
#include "transport_catalogue.h"
#include "transport_router.h"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

int main() {
    transport::catalogue::TransportCatalogue catalogue;
    json_reader::JsonReader reader(catalogue);
    
    string input_data;
    string line;
    while (getline(cin, line)) {
        input_data += line;
    }
    
    istringstream input_stream(input_data);
    auto doc = json::Load(input_stream);
    const auto& root = doc.GetRoot().AsDict();
    
    if (root.count("base_requests")) {
        reader.ParseBaseRequests(root.at("base_requests"));
    }
    
    std::optional<renderer::RenderSettings> render_settings;
    if (root.count("render_settings")) {
        render_settings = reader.ParseRenderSettings(root.at("render_settings"));
    }
    
    std::optional<transport::router::RoutingSettings> routing_settings;
    std::optional<transport::router::TransportRouter> router;
    if (root.count("routing_settings")) {
        routing_settings = reader.ParseRoutingSettings(root.at("routing_settings"));
        router.emplace(catalogue, *routing_settings);
    }
    
    if (root.count("stat_requests")) {
        auto result = reader.ProcessStatRequests(root.at("stat_requests"), render_settings, router);
        json::Print(result, cout);
    }
    
    return 0;
}
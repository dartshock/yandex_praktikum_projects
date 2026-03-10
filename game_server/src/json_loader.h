#pragma once

#include <boost/json.hpp>
#include <chrono>
#include <filesystem>
#include <unordered_map>

#include "model.h"

namespace json_loader {

namespace json = boost::json;

json::value GetJsonConfigFromFile(const std::filesystem::path& json_path);

struct LootGenConfig {
    double period;
    double probability;
};

LootGenConfig GetLootGenConfig(const boost::json::object& game_config);

model::Speed GetDefaultDogSpeed(const boost::json::object& game_config);

model::Capacity GetDefaultLootBagCapacity(
    const boost::json::object& game_config);

std::chrono::milliseconds GetDogRetirementTime(
    const boost::json::object& game_config);

std::unordered_map<size_t, size_t> GetLootTypesMap(
    const boost::json::array& loot_types_json);

void LoadRoad(model::Map& map, const boost::json::object& road);

void LoadBuilding(model::Map& map, const boost::json::object& building);

void LoadOffice(model::Map& map, const boost::json::object& office);

model::Game LoadGame(const std::filesystem::path& static_path);

}  // namespace json_loader

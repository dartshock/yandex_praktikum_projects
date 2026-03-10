#include "json_loader.h"

#include <fstream>
#include <string>

#include "data_keeper.h"
#include "loot_generator.h"
#include "utils.h"

namespace json_loader {

using namespace utils;
using namespace std::literals;

json::value GetJsonConfigFromFile(const std::filesystem::path& json_path) {
    std::ifstream in_file;
    in_file.open(json_path);
    if (!in_file.is_open()) {
        throw std::runtime_error("Can't open JSON config"s);
    }

    const auto size = std::filesystem::file_size(json_path);

    std::string content(size, '\0');
    in_file.read(content.data(), size);

    return ParseJson(content, "config"s);
}

LootGenConfig GetLootGenConfig(const boost::json::object& game_config) {
    auto loot_gen_config =
        GetJsonValueByKey(game_config, "lootGeneratorConfig"s, "config"s);

    const auto period =
        GetJsonValueByKey(loot_gen_config, "period"s, "lootGeneratorConfig"s);

    if (!period.is_double()) {
        throw(
            std::invalid_argument("Invalid lootGeneratorConfig period value"s));
    }

    const auto probability = GetJsonValueByKey(loot_gen_config, "probability"s,
                                               "lootGeneratorConfig"s);

    if (!probability.is_double()) {
        throw(std::invalid_argument(
            "Invalid lootGeneratorConfig probability value"s));
    }

    return LootGenConfig{period.as_double(), probability.as_double()};
}

model::Speed GetDefaultDogSpeed(const boost::json::object& game_config) {
    auto iter = game_config.find("defaultDogSpeed"s);
    return model::Speed{iter != game_config.end() ? iter->value().as_double()
                                                  : 1.0};
}

model::Capacity GetDefaultLootBagCapacity(
    const boost::json::object& game_config) {
    auto iter = game_config.find("defaultBagCapacity"s);
    return model::Capacity{iter != game_config.end()
                               ? static_cast<size_t>(iter->value().as_uint64())
                               : 3};
}

std::chrono::milliseconds GetDogRetirementTime(
    const boost::json::object& game_config) {
    auto iter = game_config.find("dogRetirementTime"s);
    return std::chrono::milliseconds{
        iter != game_config.end()
            ? std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::duration<double>(iter->value().as_double()))
            : std::chrono::minutes(1)};
}

std::unordered_map<size_t, size_t> GetLootTypesMap(
    const boost::json::array& loot_types_json) {
    std::unordered_map<size_t, size_t> result;
    size_t type = 0;
    for (const auto& loot_type : loot_types_json) {
        auto value = GetJsonValueByKey(loot_type, "value"s, "lootTypes"s);
        if (!value.is_int64()) {
            throw(std::invalid_argument("Invalid lootTypes value format"s));
        }
        result[type++] = value.as_int64();
    }
    return result;
}

void LoadRoad(model::Map& map, const boost::json::object& road) {
    auto x0_iter = road.find("x0"s);
    auto y0_iter = road.find("y0"s);
    auto x1_iter = road.find("x1"s);
    auto y1_iter = road.find("y1"s);

    if (x0_iter == road.end() || y0_iter == road.end() ||
        (x1_iter == road.end() && y1_iter == road.end())) {
        throw(
            std::runtime_error("wrong road format: "s + json::serialize(road)));
    }

    auto x0 = static_cast<int>(x0_iter->value().as_int64());
    auto y0 = static_cast<int>(y0_iter->value().as_int64());

    if (x1_iter != road.end()) {
        auto x1 = static_cast<int>(x1_iter->value().as_int64());
        map.AddRoad(
            model::Road{model::Road::HORIZONTAL, model::Point{x0, y0}, x1});
    } else {
        auto y1 = static_cast<int>(y1_iter->value().as_int64());
        map.AddRoad(
            model::Road{model::Road::VERTICAL, model::Point{x0, y0}, y1});
    }
}

void LoadBuilding(model::Map& map, const boost::json::object& building) {
    auto x_iter = building.find("x"s);
    auto y_iter = building.find("y"s);
    auto w_iter = building.find("w"s);
    auto h_iter = building.find("h"s);

    if (x_iter == building.end() || y_iter == building.end() ||
        w_iter == building.end() || h_iter == building.end()) {
        throw(std::runtime_error("wrong building format: "s +
                                 json::serialize(building)));
    }
    model::Point position{static_cast<int>(x_iter->value().as_int64()),
                          static_cast<int>(y_iter->value().as_int64())};
    model::Size size{static_cast<int>(w_iter->value().as_int64()),
                     static_cast<int>(h_iter->value().as_int64())};

    map.AddBuilding(model::Building{
        model::Rectangle{std::move(position), std::move(size)}});
}

void LoadOffice(model::Map& map, const boost::json::object& office) {
    auto id_iter = office.find("id"s);
    auto x_iter = office.find("x"s);
    auto y_iter = office.find("y"s);
    auto offsetX_iter = office.find("offsetX"s);
    auto offsetY_iter = office.find("offsetY"s);

    if (id_iter == office.end() || x_iter == office.end() ||
        y_iter == office.end() || offsetX_iter == office.end() ||
        offsetY_iter == office.end()) {
        throw(std::runtime_error("wrong office format: "s +
                                 json::serialize(office)));
    }
    std::string id = id_iter->value().as_string().c_str();
    model::Point position{static_cast<int>(x_iter->value().as_int64()),
                          static_cast<int>(y_iter->value().as_int64())};
    model::Offset offset{static_cast<int>(offsetX_iter->value().as_int64()),
                         static_cast<int>(offsetY_iter->value().as_int64())};

    map.AddOffice(model::Office{model::Office::Id{std::move(id)},
                                std::move(position), std::move(offset)});
}

model::Game LoadGame(const std::filesystem::path& static_path) {
    auto game_config = GetJsonConfigFromFile(static_path);
    const auto game_config_obj = GetJsonObject(game_config, "config");

    model::Speed default_dog_speed = GetDefaultDogSpeed(game_config_obj);
    model::Capacity default_lootbag_capacity =
        GetDefaultLootBagCapacity(game_config_obj);

    auto dog_retirement_time = GetDogRetirementTime(game_config_obj);

    auto loot_gen_config = GetLootGenConfig(game_config_obj);
    auto period_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::duration<double, std::milli>(loot_gen_config.period));
    loot_gen::LootGenerator loot_gen{period_ms, loot_gen_config.probability};

    model::Game game(std::move(loot_gen));

    const auto maps =
        GetJsonValueByKey(game_config_obj, "maps"s, "config"s).as_array();
    for (const auto& map_value : maps) {
        std::string id =
            GetJsonValueByKey(map_value, "id"s, "map"s).as_string().c_str();
        std::string name =
            GetJsonValueByKey(map_value, "name"s, "map"s).as_string().c_str();
        auto loot_types_json =
            GetJsonValueByKey(map_value, "lootTypes"s, "map"s);

        if (!loot_types_json.is_array() ||
            loot_types_json.as_array().size() < 1) {
            throw(std::invalid_argument("Invalid lootTypes format"s));
        }

        auto loot_types_map = GetLootTypesMap(loot_types_json.as_array());

        model::Map map(model::Map::Id{std::move(id)}, std::move(name),
                       std::move(loot_types_map),
                       data_keeper::ExtraData{std::move(loot_types_json)});

        auto roads = GetJsonValueByKey(map_value, "roads"s, "map"s).as_array();
        for (const auto& road : roads) {
            LoadRoad(map, road.as_object());
        }
        map.BuildRoadsGrid();

        auto buildings =
            GetJsonValueByKey(map_value, "buildings"s, "map"s).as_array();
        for (const auto& building : buildings) {
            LoadBuilding(map, building.as_object());
        }

        auto offices =
            GetJsonValueByKey(map_value, "offices"s, "map"s).as_array();
        for (const auto& office : offices) {
            LoadOffice(map, office.as_object());
        }

        if (auto dog_speed_iter = map_value.as_object().find("dogSpeed"s);
            dog_speed_iter != map_value.as_object().end()) {
            map.SetDogSpeed(dog_speed_iter->value().as_double());
        } else {
            map.SetDogSpeed(default_dog_speed);
        }

        if (auto bag_capacity_iter = map_value.as_object().find("bagCapacity"s);
            bag_capacity_iter != map_value.as_object().end()) {
            map.SetLootBagCapacity(bag_capacity_iter->value().as_int64());
        } else {
            map.SetLootBagCapacity(default_lootbag_capacity);
        }
        map.SetDogRetirementTime(dog_retirement_time);

        game.AddMap(std::move(map));
    }

    return game;
}

}  // namespace json_loader

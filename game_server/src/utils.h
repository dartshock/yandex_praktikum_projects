#pragma once

#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/json.hpp>
#include <filesystem>
#include <string>

namespace utils {

namespace fs = std::filesystem;
namespace json = boost::json;

using namespace std::literals;

std::string DecodePath(const std::string& encoded);

bool IsSubPath(fs::path path, fs::path base);

std::string StringToLowercase(const std::string& str);

std::string GetMIMEType(const std::string& ext);

double CalculateDistanceInUnits(double speed_units_in_s, size_t time_ms);

json::value ParseJson(std::string_view input,
                                      std::string_view value_name);

json::object GetJsonObject(const json::value& json_value,
                                           std::string_view object_name);

json::value GetJsonValueByKey(const json::value& json_value,
                                              std::string_view key,
                                              std::string_view object_name);

}  // namespace utils
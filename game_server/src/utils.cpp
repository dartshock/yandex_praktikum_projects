#include "utils.h"

#include <algorithm>
#include <cctype>

namespace utils {

std::string DecodePath(const std::string& encoded) {
    std::ostringstream decoded;

    for (size_t i = 0; i < encoded.length(); ++i) {
        if (encoded[i] == '%') {
            if (i + 2 < encoded.length()) {
                std::string hex = encoded.substr(i + 1, 2);
                char ch = static_cast<char>(std::stoi(hex, nullptr, 16));
                decoded << ch;
                i += 2;
            }
        } else if (encoded[i] == '+') {
            decoded << ' ';
        } else {
            decoded << encoded[i];
        }
    }

    return decoded.str();
}

bool IsSubPath(fs::path path, fs::path base) {
    path = fs::weakly_canonical(path);
    base = fs::weakly_canonical(base);

    for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
        if (p == path.end() || *p != *b) {
            return false;
        }
    }
    return true;
}

std::string StringToLowercase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string GetMIMEType(const std::string& ext) {
    static const std::string unknown_type{"application/octet-stream"s};
    static const std::unordered_map<std::string, std::string> mime_types = {
        {".htm"s, "text/html"s},       {".html"s, "text/html"s},
        {".css"s, "text/css"s},        {".txt"s, "text/plain"s},
        {".js"s, "text/javascript"s},  {".json"s, "application/json"s},
        {".xml"s, "application/xml"s}, {".png"s, "image/png"s},
        {".jpg"s, "image/jpeg"s},      {".jpe"s, "image/jpeg"s},
        {".jpeg"s, "image/jpeg"s},     {".gif"s, "image/gif"s},
        {".bmp"s, "image/bmp"s},       {".ico"s, "image/vnd.microsoft.icon"s},
        {".tiff"s, "image/tiff"s},     {".tif"s, "image/tiff"s},
        {".svg"s, "image/svg+xml"s},   {".svgz"s, "image/svg+xml"s},
        {".mp3"s, "audio/mpeg"s}};

    auto iter = mime_types.find(StringToLowercase(ext));
    if (iter != mime_types.end()) {
        return iter->second;
    } else {
        return unknown_type;
    }
}

double CalculateDistanceInUnits(double speed_units_in_s, size_t time_ms) {
    return speed_units_in_s * time_ms / 1000.0;
}

json::value ParseJson(std::string_view input,
                                      std::string_view value_name) {
    boost::system::error_code ec;
    auto json_value = json::parse(std::string(input), ec);
    if (ec) {
        throw std::invalid_argument(
            "Invalid format: "s + std::string(value_name) + " is not in JSON"s);
    }
    return json_value;
}

json::object GetJsonObject(const json::value& json_value,
                                           std::string_view object_name) {
    if (!json_value.is_object()) {
        throw(std::invalid_argument("Invalid format: "s +
                                    std::string(object_name) +
                                    " is not a JSON object"s));
    }
    return json_value.as_object();
}

json::value GetJsonValueByKey(const json::value& json_value,
                                              std::string_view key,
                                              std::string_view object_name) {
    const auto json_object =
        GetJsonObject(json_value, object_name);
    const auto iter = json_object.find(std::string(key));
    if (iter == json_object.end()) {
        throw(std::invalid_argument("Invalid"s + std::string(object_name) +
                                    " format: missing "s + std::string(key) +
                                    "key"s));
    }
    return iter->value();
}

}  // namespace utils
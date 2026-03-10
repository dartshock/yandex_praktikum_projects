#pragma once

#include <boost/json.hpp>

namespace data_keeper {

namespace json = boost::json;

class ExtraData {
public:
    ExtraData(json::value&& data)
        : data_(data) {
    }

    const json::value& GetData() const {
        return data_;
    }

private:
    json::value data_;
};

}  // namespace data_keeper
#pragma once

#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/date_time.hpp>
#include <boost/json.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <iostream>

namespace boost_logger {

using namespace std::literals;

namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace json = boost::json;

BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", json::value)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)

inline void JsonFormatter(logging::record_view const& rec,
                   logging::formatting_ostream& strm) {
    json::object obj;
    obj["timestamp"s] = to_iso_extended_string(*rec[timestamp]);
    obj["data"s] = *rec[additional_data];
    obj["message"s] = *rec[logging::expressions::smessage];
    strm << json::serialize(obj);
}

inline void InitBoostLogFilter() {
    logging::add_common_attributes();

    logging::add_console_log(std::cout, keywords::format = &JsonFormatter,
                             keywords::auto_flush = true);
}

}  // namespace boost_logger
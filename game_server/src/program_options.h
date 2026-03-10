#pragma once

#include <boost/program_options.hpp>
#include <iostream>
#include <optional>
#include <string>

namespace program_options {

using namespace std::literals;

struct Args {
    std::string config_path;
    std::string static_path;
    std::optional<std::string> state_path = std::nullopt;
    std::optional<size_t> tick_period = std::nullopt;
    std::optional<size_t> save_period = std::nullopt;
    bool random_spawn = false;
};

[[nodiscard]] inline std::optional<Args> ParseCommandLine(
    int argc, const char* const argv[]) {
    namespace po = boost::program_options;

    po::options_description desc{"Allowed options"s};

    Args args;
    std::string state_path_str;
    std::string tick_period_str;
    std::string save_period_str;
    desc.add_options()
        ("help,h", "produce help message")
        ("config-file,c", po::value(&args.config_path)->value_name("file"s), "set config file path")
        ("www-root,w", po::value(&args.static_path)->value_name("dir"s), "set static files root")
        ("state-file", po::value(&state_path_str)->value_name("file"s), "set state file path")
        ("tick-period,t", po::value(&tick_period_str)->value_name("milliseconds"s), "set tick period")
        ("save-state-period", po::value(&save_period_str)->value_name("milliseconds"s), "set save period")
        ("randomize-spawn-points", "spawn dogs at random positions");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help"s)) {
        std::cout << desc;
        return std::nullopt;
    }
    if (!vm.contains("config-file"s)) {
        throw std::runtime_error("Config file have not been specified"s);
    }
    if (!vm.contains("www-root"s)) {
        throw std::runtime_error("Static dir path is not specified"s);
    }
    if (vm.contains("state-file"s)) {
        args.state_path = state_path_str;
    }
    if (vm.contains("tick-period"s)) {
        try {
            if (!tick_period_str.empty() && tick_period_str[0] == '-') {
                throw std::invalid_argument("Negative tick period"s);
            }
            args.tick_period = std::stoull(tick_period_str);
        } catch (const std::exception& ex) {
            throw std::runtime_error("Tick period in wrong format: "s +
                                     ex.what());
        }
    }
    if (vm.contains("save-state-period"s)) {
        try {
            if (!save_period_str.empty() && save_period_str[0] == '-') {
                throw std::invalid_argument("Negative save period"s);
            }
            args.save_period = std::stoull(save_period_str);
        } catch (const std::exception& ex) {
            throw std::runtime_error("Save period in wrong format: "s +
                                     ex.what());
        }
    }

    args.random_spawn = vm.contains("randomize-spawn-points"s);

    return args;
}

}  // namespace program_options
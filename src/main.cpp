#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

#include "cache.h"
#include "persistence.h"
#include "server.h"
#include "stats.h"

namespace {

struct Options {
    int port = 6379;
    std::size_t max_keys = 1000;
    std::string aof_path = "quickcache.aof";
};

Options parseOptions(int argc, char** argv) {
    Options options;

    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];

        if (argument == "--port" && index + 1 < argc) {
            options.port = std::atoi(argv[++index]);
        } else if (argument == "--max-keys" && index + 1 < argc) {
            options.max_keys = static_cast<std::size_t>(std::stoull(argv[++index]));
        } else if (argument == "--aof" && index + 1 < argc) {
            options.aof_path = argv[++index];
        } else if (argument == "--help") {
            std::cout << "Usage: quickcache [--port 6379] [--max-keys 1000] [--aof quickcache.aof]" << std::endl;
            std::exit(0);
        } else {
            throw std::runtime_error("unknown or incomplete option: " + argument);
        }
    }

    return options;
}

} // namespace

int main(int argc, char** argv) {
    try {
        const auto options = parseOptions(argc, argv);
        quickcache::RuntimeStats stats;
        quickcache::Cache cache(options.max_keys, &stats);
        quickcache::AppendOnlyLog append_only_log(options.aof_path);

        if (!append_only_log.replay(cache)) {
            throw std::runtime_error("failed to replay append-only log");
        }
        if (!append_only_log.openForAppend()) {
            throw std::runtime_error("failed to open append-only log: " + append_only_log.path());
        }

        quickcache::Server server(cache, stats, &append_only_log, options.port);

        server.run();
    } catch (const std::exception& error) {
        std::cerr << "ERROR " << error.what() << std::endl;
        return 1;
    }

    return 0;
}

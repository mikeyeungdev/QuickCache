#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

#include "cache.h"
#include "persistence.h"
#include "server.h"

namespace {

struct Options {
    std::string host = "127.0.0.1";
    int port = 6379;
    std::size_t max_keys = 100000;
    std::string log_path = "quickcache.aof";
};

Options parse_options(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--host" && i + 1 < argc) {
            options.host = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            options.port = std::atoi(argv[++i]);
        } else if (arg == "--max-keys" && i + 1 < argc) {
            options.max_keys = static_cast<std::size_t>(std::stoull(argv[++i]));
        } else if (arg == "--log" && i + 1 < argc) {
            options.log_path = argv[++i];
        } else if (arg == "--help") {
            std::cout << "Usage: quickcache [--host 127.0.0.1] [--port 6379] [--max-keys 100000] [--log quickcache.aof]\n";
            std::exit(0);
        }
    }
    return options;
}

} // namespace

int main(int argc, char** argv) {
    const auto options = parse_options(argc, argv);

    try {
        quickcache::Cache cache(options.max_keys);
        quickcache::PersistenceLog log(options.log_path);
        log.replay(cache);
        if (!log.open()) {
            std::cerr << "Failed to open persistence log: " << log.path() << '\n';
            return 1;
        }

        quickcache::Server server(options.host, options.port, cache, log);
        server.run();
    } catch (const std::exception& ex) {
        std::cerr << "QuickCache error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}

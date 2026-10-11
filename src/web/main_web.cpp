// OOP: Application Entry Point (HTTP server)
#include "web/HttpServer.h"

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

namespace {

std::string argValue(int argc, char* argv[], const std::string& name,
                     const std::string& fallback) {
    for (int i = 1; i < argc; ++i) {
        if (name == argv[i] && i + 1 < argc) {
            return std::string(argv[i + 1]);
        }
    }
    return fallback;
}

bool hasFlag(int argc, char* argv[], const std::string& name) {
    for (int i = 1; i < argc; ++i) {
        if (name == argv[i]) return true;
    }
    return false;
}

// Set by the signal handler so the blocking listen() call can be interrupted.
volatile std::sig_atomic_t stopRequested = 0;

void onSignal(int) {
    stopRequested = 1;
}

std::string envOr(const char* name, const std::string& fallback) {
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0') return fallback;
    return std::string(value);
}

void usage() {
    std::cout <<
        "AI Interviewer web server\n\n"
        "Usage: ai_interviewer_web [options]\n"
        "  --host <host>       Bind address (default 127.0.0.1)\n"
        "  --port <port>       Bind port (default 8080)\n"
        "  --data-dir <dir>    Location of the JSON data files (default ./data)\n"
        "  --static-dir <dir>  Location of the browser front-end (default ./web)\n"
        "  --storage <b>       Storage backend: json or mongo (default json)\n"
        "  --uri <uri>         MongoDB connection string\n"
        "  --db <name>         MongoDB database name (default ai_interviewer)\n"
        "  --help              Show this help\n";
}

} // namespace

int main(int argc, char* argv[]) {
    if (hasFlag(argc, argv, "--help")) {
        usage();
        return 0;
    }

    web::HttpServer::Config config;
    config.host = argValue(argc, argv, "--host", envOr("WEB_HOST", "127.0.0.1"));
    config.port = std::atoi(argValue(argc, argv, "--port",
                                     envOr("WEB_PORT", "8080")).c_str());
    config.dataDir = argValue(argc, argv, "--data-dir", envOr("APP_DATA_DIR", "./data"));
    config.staticDir = argValue(argc, argv, "--static-dir", "./web");
    config.backend = argValue(argc, argv, "--storage", "json") == "mongo"
                         ? db::StorageBackend::Mongo
                         : db::StorageBackend::Json;
    config.dbName = argValue(argc, argv, "--db", envOr("MONGODB_DB", "ai_interviewer"));
    config.uri = argValue(argc, argv, "--uri", envOr("MONGODB_URI", "mongodb://localhost:27017"));

    if (config.port <= 0 || config.port > 65535) {
        std::cerr << "Invalid port: " << config.port << std::endl;
        return 1;
    }

    std::signal(SIGINT, onSignal);
#ifdef SIGBREAK
    std::signal(SIGBREAK, onSignal);
#endif

    try {
        web::HttpServer server(config);

        // listen() blocks, so the signal handler only sets a flag; a watcher
        // thread closes the server once the flag is raised.
        std::thread watcher([&server]() {
            while (!stopRequested) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            server.stop();
        });

        std::cout << "[web] serving http://" << config.host << ":" << config.port
                  << " (press Ctrl+C to stop)" << std::endl;
        const bool ok = server.start();
        if (watcher.joinable()) {
            watcher.join();
        }

        if (!ok) {
            std::cerr << "[web] failed to bind " << config.host << ":" << config.port << std::endl;
            return 1;
        }
        std::cout << "[web] stopped" << std::endl;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[web] fatal: " << ex.what() << std::endl;
        return 1;
    }
}

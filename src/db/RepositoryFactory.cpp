// OOP: Factory Pattern (Repository Creation)
//
// Uses if/else rather than switch on StorageBackend so the Mongo branch can be
// compiled out with USE_MONGO without triggering -Wswitch for the unhandled
// enumerator.
#include "db/RepositoryFactory.h"
#include "db/JsonUserRepository.h"
#include "db/JsonQuestionRepository.h"
#include "db/JsonAttemptRepository.h"
#include "db/JsonScoreboardRepository.h"
#include "db/JsonDailyChallengeRepository.h"
#ifdef USE_MONGO
#include "db/MongoUserRepository.h"
#include "db/MongoQuestionRepository.h"
#include "db/MongoAttemptRepository.h"
#include "db/MongoInterviewRepository.h"
#include "db/MongoFeedbackRepository.h"
#include "db/MongoScoreboardRepository.h"
#include "db/MongoDailyChallengeRepository.h"
#include "db/Database.h"
#include "db/DatabaseException.h"
#endif
#include <memory>
#include <iostream>

namespace db {

std::unique_ptr<core::IUserRepository> RepositoryFactory::createUserRepository(
    StorageBackend backend, const std::string& path, const std::string& dbName) {
    (void)dbName;    if (backend == StorageBackend::Mongo) {
#ifdef USE_MONGO
        return std::make_unique<MongoUserRepository>(dbName);
#else
        std::cerr << "[RepositoryFactory] MongoDB support not compiled in; using JSON\n";
#endif
    }
    return std::make_unique<JsonUserRepository>(path);
}

std::unique_ptr<core::IQuestionRepository> RepositoryFactory::createQuestionRepository(
    StorageBackend backend, const std::string& path, const std::string& dbName) {
    (void)dbName;    if (backend == StorageBackend::Mongo) {
#ifdef USE_MONGO
        return std::make_unique<MongoQuestionRepository>(dbName);
#else
        std::cerr << "[RepositoryFactory] MongoDB support not compiled in; using JSON\n";
#endif
    }
    return std::make_unique<JsonQuestionRepository>(path);
}

std::unique_ptr<core::IAttemptRepository> RepositoryFactory::createAttemptRepository(
    StorageBackend backend, const std::string& path, const std::string& dbName) {
    (void)dbName;    if (backend == StorageBackend::Mongo) {
#ifdef USE_MONGO
        return std::make_unique<MongoAttemptRepository>(dbName);
#else
        std::cerr << "[RepositoryFactory] MongoDB support not compiled in; using JSON\n";
#endif
    }
    return std::make_unique<JsonAttemptRepository>(path);
}

std::unique_ptr<core::IInterviewRepository> RepositoryFactory::createInterviewRepository(
    StorageBackend backend, const std::string& path, const std::string& dbName) {
    // Interviews are not persisted for the JSON backend.
    (void)path;
    (void)dbName;    if (backend == StorageBackend::Mongo) {
#ifdef USE_MONGO
        return std::make_unique<MongoInterviewRepository>(dbName);
#else
        return nullptr;
#endif
    }
    return nullptr;
}

std::unique_ptr<core::IFeedbackRepository> RepositoryFactory::createFeedbackRepository(
    StorageBackend backend, const std::string& path, const std::string& dbName) {
    // Feedback is not persisted for the JSON backend.
    (void)path;
    (void)dbName;    if (backend == StorageBackend::Mongo) {
#ifdef USE_MONGO
        return std::make_unique<MongoFeedbackRepository>(dbName);
#else
        return nullptr;
#endif
    }
    // Feedback is not persisted for the JSON backend.
    return nullptr;
}

std::unique_ptr<core::IScoreboardRepository> RepositoryFactory::createScoreboardRepository(
    StorageBackend backend, const std::string& path, const std::string& dbName) {
    (void)dbName;    if (backend == StorageBackend::Mongo) {
#ifdef USE_MONGO
        return std::make_unique<MongoScoreboardRepository>(dbName);
#else
        std::cerr << "[RepositoryFactory] MongoDB support not compiled in; using JSON\n";
#endif
    }
    return std::make_unique<JsonScoreboardRepository>(path);
}

std::unique_ptr<core::IDailyChallengeRepository> RepositoryFactory::createDailyChallengeRepository(
    StorageBackend backend, const std::string& path, const std::string& dbName) {
    (void)dbName;    if (backend == StorageBackend::Mongo) {
#ifdef USE_MONGO
        return std::make_unique<MongoDailyChallengeRepository>(dbName);
#else
        std::cerr << "[RepositoryFactory] MongoDB support not compiled in; using JSON\n";
#endif
    }
    return std::make_unique<JsonDailyChallengeRepository>(path);
}

// Factory with automatic fallback: tries Mongo, falls back to JSON on failure
std::unique_ptr<core::IUserRepository> RepositoryFactory::createUserRepositoryWithFallback(const std::string& jsonPath) {
#ifdef USE_MONGO
    try {
        auto repo = std::make_unique<MongoUserRepository>(Database::instance().databaseName());
        auto client = Database::instance().acquire();
        if (client) {
            std::cout << "[RepositoryFactory] Using MongoDB backend for UserRepository" << std::endl;
            return repo;
        }
    } catch (const std::exception& e) {
        std::cerr << "[RepositoryFactory] MongoDB unavailable, falling back to JSON: " << e.what() << std::endl;
    }
#endif
    std::cout << "[RepositoryFactory] Using JSON backend for UserRepository" << std::endl;
    return std::make_unique<JsonUserRepository>(jsonPath);
}

std::unique_ptr<core::IQuestionRepository> RepositoryFactory::createQuestionRepositoryWithFallback(const std::string& jsonPath) {
#ifdef USE_MONGO
    try {
        auto repo = std::make_unique<MongoQuestionRepository>(Database::instance().databaseName());
        auto client = Database::instance().acquire();
        if (client) {
            std::cout << "[RepositoryFactory] Using MongoDB backend for QuestionRepository" << std::endl;
            return repo;
        }
    } catch (const std::exception& e) {
        std::cerr << "[RepositoryFactory] MongoDB unavailable, falling back to JSON: " << e.what() << std::endl;
    }
#endif
    std::cout << "[RepositoryFactory] Using JSON backend for QuestionRepository" << std::endl;
    return std::make_unique<JsonQuestionRepository>(jsonPath);
}

std::unique_ptr<core::IAttemptRepository> RepositoryFactory::createAttemptRepositoryWithFallback(const std::string& jsonPath) {
#ifdef USE_MONGO
    try {
        auto repo = std::make_unique<MongoAttemptRepository>(Database::instance().databaseName());
        auto client = Database::instance().acquire();
        if (client) {
            std::cout << "[RepositoryFactory] Using MongoDB backend for AttemptRepository" << std::endl;
            return repo;
        }
    } catch (const std::exception& e) {
        std::cerr << "[RepositoryFactory] MongoDB unavailable, falling back to JSON: " << e.what() << std::endl;
    }
#endif
    std::cout << "[RepositoryFactory] Using JSON backend for AttemptRepository" << std::endl;
    return std::make_unique<JsonAttemptRepository>(jsonPath);
}

std::unique_ptr<core::IInterviewRepository> RepositoryFactory::createInterviewRepositoryWithFallback(
    const std::string& jsonPath) {
    // The JSON backend does not persist interview sessions, so this parameter
    // is only meaningful once a Mongo-backed JSON fallback is introduced.
    (void)jsonPath;
#ifdef USE_MONGO
    try {
        auto repo = std::make_unique<MongoInterviewRepository>(Database::instance().databaseName());
        auto client = Database::instance().acquire();
        if (client) {
            std::cout << "[RepositoryFactory] Using MongoDB backend for InterviewRepository" << std::endl;
            return repo;
        }
    } catch (const std::exception& e) {
        std::cerr << "[RepositoryFactory] MongoDB unavailable, falling back to null: " << e.what() << std::endl;
    }
#else
    (void)jsonPath;
#endif
    return nullptr;
}

std::unique_ptr<core::IFeedbackRepository> RepositoryFactory::createFeedbackRepositoryWithFallback(
    const std::string& jsonPath) {
    // Feedback is not persisted for the JSON backend.
    (void)jsonPath;
#ifdef USE_MONGO
    try {
        auto repo = std::make_unique<MongoFeedbackRepository>(Database::instance().databaseName());
        auto client = Database::instance().acquire();
        if (client) {
            std::cout << "[RepositoryFactory] Using MongoDB backend for FeedbackRepository" << std::endl;
            return repo;
        }
    } catch (const std::exception& e) {
        std::cerr << "[RepositoryFactory] MongoDB unavailable, falling back to null: " << e.what() << std::endl;
    }
#else
    (void)jsonPath;
#endif
    return nullptr;
}

std::unique_ptr<core::IScoreboardRepository> RepositoryFactory::createScoreboardRepositoryWithFallback(const std::string& jsonPath) {
    std::cout << "[RepositoryFactory] Using JSON backend for ScoreboardRepository" << std::endl;
    return std::make_unique<JsonScoreboardRepository>(jsonPath);
}

std::unique_ptr<core::IDailyChallengeRepository> RepositoryFactory::createDailyChallengeRepositoryWithFallback(const std::string& jsonPath) {
    std::cout << "[RepositoryFactory] Using JSON backend for DailyChallengeRepository" << std::endl;
    return std::make_unique<JsonDailyChallengeRepository>(jsonPath);
}

StorageBackend RepositoryFactory::parseBackend(const std::string& str) {
    if (str == "mongo") return StorageBackend::Mongo;
    return StorageBackend::Json;
}

} // namespace db

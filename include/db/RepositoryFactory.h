// OOP: Factory Pattern (Repository Creation)
#pragma once
#include "core/IUserRepository.h"
#include "core/IQuestionRepository.h"
#include "core/IAttemptRepository.h"
#include "core/IInterviewRepository.h"
#include "core/IFeedbackRepository.h"
#include "core/IScoreboardRepository.h"
#include "core/IDailyChallengeRepository.h"
#include <memory>
#include <string>

namespace db {

enum class StorageBackend {
    Json,
    Mongo
};

class RepositoryFactory {
public:
    // `path` is the JSON file used by the JSON backend; `dbName` is the MongoDB
    // database used by the Mongo backend. They are separate parameters because
    // the two backends take fundamentally different arguments, and passing a
    // file path as a database name silently writes to the wrong place.
    static std::unique_ptr<core::IUserRepository> createUserRepository(
        StorageBackend backend, const std::string& path, const std::string& dbName = "ai_interviewer");
    static std::unique_ptr<core::IQuestionRepository> createQuestionRepository(
        StorageBackend backend, const std::string& path, const std::string& dbName = "ai_interviewer");
    static std::unique_ptr<core::IAttemptRepository> createAttemptRepository(
        StorageBackend backend, const std::string& path, const std::string& dbName = "ai_interviewer");
    static std::unique_ptr<core::IInterviewRepository> createInterviewRepository(
        StorageBackend backend, const std::string& path, const std::string& dbName = "ai_interviewer");
    static std::unique_ptr<core::IFeedbackRepository> createFeedbackRepository(
        StorageBackend backend, const std::string& path, const std::string& dbName = "ai_interviewer");
    static std::unique_ptr<core::IScoreboardRepository> createScoreboardRepository(
        StorageBackend backend, const std::string& path, const std::string& dbName = "ai_interviewer");
    static std::unique_ptr<core::IDailyChallengeRepository> createDailyChallengeRepository(
        StorageBackend backend, const std::string& path, const std::string& dbName = "ai_interviewer");
    
    // Factory with automatic fallback: tries Mongo, falls back to JSON on failure
    static std::unique_ptr<core::IUserRepository> createUserRepositoryWithFallback(const std::string& jsonPath);
    static std::unique_ptr<core::IQuestionRepository> createQuestionRepositoryWithFallback(const std::string& jsonPath);
    static std::unique_ptr<core::IAttemptRepository> createAttemptRepositoryWithFallback(const std::string& jsonPath);
    static std::unique_ptr<core::IInterviewRepository> createInterviewRepositoryWithFallback(const std::string& jsonPath);
    static std::unique_ptr<core::IFeedbackRepository> createFeedbackRepositoryWithFallback(const std::string& jsonPath);
    static std::unique_ptr<core::IScoreboardRepository> createScoreboardRepositoryWithFallback(const std::string& jsonPath);
    static std::unique_ptr<core::IDailyChallengeRepository> createDailyChallengeRepositoryWithFallback(const std::string& jsonPath);
    
    static StorageBackend parseBackend(const std::string& str);
};

} // namespace db
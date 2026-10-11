// OOP: Service Layer (secure code execution sandbox)
#pragma once
#include "core/StrongId.h"
#include "core/Optional.h"
#include <string>
#include <vector>
#include <chrono>
#include <cstdint>

// The console build uses MinGW GCC 6.3, which has no <optional>, so the
// project's core::Optional shim is used for the three optional results.
namespace execution {
using core::Optional;
} // namespace execution

// NOTE: this project compiles with MinGW GCC 6.3, so <filesystem> is not
// available and POSIX paths are used as plain strings.
#ifndef _WIN32
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <signal.h>
#else
#include <windows.h>
#include <io.h>
#endif

namespace execution {

// Result of a single test case execution
struct TestCaseResult {
    bool passed = false;
    std::string output;
    std::string expected;
    std::string error;
    int exitCode = 0;
    long long timeMs = 0;
    long long memoryKb = 0;
    std::string errorType; // compile_error | runtime_error | timeout | memory_limit | wrong_answer | ok
};

// Result of running all test cases for a submission
struct ExecutionResult {
    bool allPassed = false;
    int passedCount = 0;
    int totalCount = 0;
    long long totalTimeMs = 0;
    long long peakMemoryKb = 0;
    std::vector<TestCaseResult> results;
    std::string compileError;
    std::string verdict; // Accepted | Wrong Answer | Compile Error | Runtime Error | Time Limit Exceeded | Memory Limit Exceeded
};

// Configuration for execution limits
struct ExecutionConfig {
    int timeLimitMs = 2000;       // per test case
    int memoryLimitMb = 256;      // MB
    // A per-run scratch directory. It is a relative path by default so the
    // server works on any host without needing a writable /tmp.
    std::string workDir = "build/exec";
};

class CodeExecutor {
public:
    explicit CodeExecutor(const ExecutionConfig& config = ExecutionConfig());
    ~CodeExecutor();

    // Compiles and runs the program against every test case.
    ExecutionResult runCode(const std::string& code,
                            const std::string& language,
                            const std::vector<std::pair<std::string, std::string>>& testCases,
                            int timeLimitMs = 2000,
                            int memoryLimitMb = 256);

    // Compile check without running (used by "Run").
    execution::Optional<std::string> compileOnly(const std::string& code, const std::string& language);

private:
    // Per-test-case outcome while running.
    struct TestRunResult {
        bool passed = false;
        std::string output;
        std::string error;
        int exitCode = 0;
        long long timeMs = 0;
        long long memoryKb = 0;
        bool timedOut = false;
        bool oomKilled = false;
        std::string errorType;
    };

    // Outcome of a spawned process.
    struct ProcessResult {
        int exitCode = -1;
        std::string stdoutData;
        std::string stderrData;
        long long timeMs = 0;
        long long memoryKb = 0;
        bool timedOut = false;
        bool oomKilled = false;
    };

    std::string createWorkDir() const;
    std::string writeSource(const std::string& code, const std::string& language,
                            const std::string& workDir) const;
    execution::Optional<std::string> compile(const std::string& sourcePath,
                                       const std::string& language,
                                       std::string& errorOut) const;
    TestRunResult runTestCase(const std::string& executable,
                              const std::string& input,
                              const std::string& expectedOutput,
                              int timeLimitMs,
                              int memoryLimitKb) const;
    ProcessResult runWithLimits(const std::string& command,
                                const std::string& input,
                                int timeLimitMs,
                                int memoryLimitKb) const;

    static bool compareOutput(const std::string& actual, const std::string& expected);

    ExecutionConfig config_;
    std::string workDir_;
};

} // namespace execution

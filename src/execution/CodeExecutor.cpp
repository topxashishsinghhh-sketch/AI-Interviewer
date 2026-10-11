// OOP: Service Layer (secure code execution sandbox)
#include "execution/CodeExecutor.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>

#ifndef _WIN32
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#else
#include <direct.h>
#endif

namespace execution {
namespace {

// Small helper so Windows and POSIX share one code path.
void makeDirectory(const std::string& path) {
#ifndef _WIN32
    mkdir(path.c_str(), 0755);
#else
    _mkdir(path.c_str());
#endif
}

void removeFile(const std::string& path) {
    std::remove(path.c_str());
}

} // namespace

CodeExecutor::CodeExecutor(const ExecutionConfig& config)
    : config_(config) {
    workDir_ = config_.workDir;
    makeDirectory(workDir_);
}

CodeExecutor::~CodeExecutor() = default;

std::string CodeExecutor::createWorkDir() const {
    // A per-call directory keeps concurrent submissions from colliding.
    const std::uint64_t now = static_cast<std::uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    std::ostringstream oss;
    oss << workDir_ << "/exec_" << now;
    const std::string dir = oss.str();
    makeDirectory(dir);
    return dir;
}

std::string CodeExecutor::writeSource(const std::string& code, const std::string& language,
                                      const std::string& workDir) const {
    std::string ext = ".cpp";
    if (language == "c") ext = ".c";
    else if (language != "cpp" && language != "c++") ext = ".txt";

    const std::string sourcePath = workDir + "/main" + ext;
    std::ofstream file(sourcePath.c_str());
    if (!file) {
        throw std::runtime_error("Failed to create source file");
    }
    file << code;
    file.close();
    return sourcePath;
}

Optional<std::string> CodeExecutor::compile(const std::string& sourcePath,
                                                 const std::string& language,
                                                 std::string& errorOut) const {
    std::ostringstream cmd;
    if (language == "c") {
        cmd << "gcc -std=c11 -O2 -pipe -static -s";
    } else {
        cmd << "g++ -std=c++17 -O2 -pipe -static -s";
    }
    // The output binary sits next to the source so each run is isolated.
    // The compiler's diagnostics go to a log file rather than a pipe, because
    // popen is not declared when MinGW compiles in strict-ANSI mode.
    cmd << " -o " << sourcePath << ".bin " << sourcePath
        << " > " << sourcePath << ".compilelog 2>&1";

    const std::string logPath = sourcePath + ".compilelog";
    const int status = std::system(cmd.str().c_str());

    std::string output;
    {
        std::ifstream log(logPath.c_str());
        std::ostringstream buffer;
        buffer << log.rdbuf();
        output = buffer.str();
    }
    removeFile(logPath);

    if (status != 0) {
        errorOut = output;
        return Optional<std::string>();
    }
    return sourcePath + ".bin";
}

Optional<std::string> CodeExecutor::compileOnly(const std::string& code,
                                                     const std::string& language) {
    const std::string dir = createWorkDir();
    const std::string source = writeSource(code, language, dir);
    std::string error;
    Optional<std::string> binary = compile(source, language, error);
    if (!binary) {
        removeFile(source);
        return error;
    }
    removeFile(source);
    removeFile(*binary);
    return Optional<std::string>();
}

ExecutionResult CodeExecutor::runCode(const std::string& code,
                                     const std::string& language,
                                     const std::vector<std::pair<std::string, std::string>>& testCases,
                                     int timeLimitMs,
                                     int memoryLimitMb) {
    ExecutionResult result;
    result.verdict = "Compile Error";
    result.allPassed = false;

    const std::string dir = createWorkDir();
    const std::string source = writeSource(code, language, dir);

    std::string compileError;
    Optional<std::string> binary = compile(source, language, compileError);
    if (!binary) {
        result.compileError = compileError;
        result.verdict = "Compile Error";
        removeFile(source);
        return result;
    }
    removeFile(source);

    const std::string exePath = *binary;
    const int total = static_cast<int>(testCases.size());
    int passed = 0;
    long long totalTime = 0;
    long long peakMem = 0;

    result.results.reserve(testCases.size());
    for (std::size_t i = 0; i < testCases.size(); ++i) {
        TestRunResult tr = runTestCase(exePath, testCases[i].first, testCases[i].second,
                                       timeLimitMs, memoryLimitMb * 1024);

        TestCaseResult tcResult;
        tcResult.passed = tr.passed;
        tcResult.output = tr.output;
        tcResult.expected = testCases[i].second;
        tcResult.error = tr.error;
        tcResult.exitCode = tr.exitCode;
        tcResult.timeMs = tr.timeMs;
        tcResult.memoryKb = tr.memoryKb;
        tcResult.errorType = tr.errorType;
        result.results.push_back(tcResult);

        if (tr.passed) ++passed;
        totalTime += tr.timeMs;
        if (tr.memoryKb > peakMem) peakMem = tr.memoryKb;
    }

    removeFile(exePath);

    result.allPassed = (passed == total);
    result.passedCount = passed;
    result.totalCount = total;
    result.totalTimeMs = totalTime;
    result.peakMemoryKb = peakMem;

    if (result.allPassed) {
        result.verdict = "Accepted";
    } else if (total == 0) {
        result.verdict = "No Test Cases";
    } else {
        // The first failure category encountered decides the headline verdict.
        bool sawTimeout = false;
        bool sawMemory = false;
        bool sawRuntime = false;
        for (std::size_t i = 0; i < result.results.size(); ++i) {
            const std::string& type = result.results[i].errorType;
            if (type == "timeout") sawTimeout = true;
            else if (type == "memory_limit") sawMemory = true;
            else if (type == "runtime_error") sawRuntime = true;
        }
        if (sawTimeout) result.verdict = "Time Limit Exceeded";
        else if (sawMemory) result.verdict = "Memory Limit Exceeded";
        else if (sawRuntime) result.verdict = "Runtime Error";
        else result.verdict = "Wrong Answer";
    }
    return result;
}

CodeExecutor::TestRunResult CodeExecutor::runTestCase(const std::string& executable,
                                                      const std::string& input,
                                                      const std::string& expectedOutput,
                                                      int timeLimitMs,
                                                      int memoryLimitKb) const {
    TestRunResult result;
    result.passed = false;
    result.errorType = "wrong_answer";

    ProcessResult proc = runWithLimits(executable, input, timeLimitMs, memoryLimitKb);

    result.exitCode = proc.exitCode;
    result.output = proc.stdoutData;
    result.error = proc.stderrData;
    result.timeMs = proc.timeMs;
    result.memoryKb = proc.memoryKb;
    result.timedOut = proc.timedOut;
    result.oomKilled = proc.oomKilled;

    if (proc.timedOut) {
        result.errorType = "timeout";
        result.error = "Time limit exceeded";
        return result;
    }
    if (proc.oomKilled) {
        result.errorType = "memory_limit";
        result.error = "Memory limit exceeded";
        return result;
    }
    if (proc.exitCode != 0) {
        result.errorType = "runtime_error";
        std::ostringstream oss;
        oss << "Runtime error (exit code " << proc.exitCode << ")";
        result.error = oss.str();
        return result;
    }
    if (compareOutput(proc.stdoutData, expectedOutput)) {
        result.passed = true;
        result.errorType = "ok";
        return result;
    }
    result.errorType = "wrong_answer";
    result.error = "Output mismatch";
    return result;
}

bool CodeExecutor::compareOutput(const std::string& actual, const std::string& expected) {
    // Windows binaries emit CRLF while the question banks store LF, so carriage
    // returns are *removed* (not replaced by a space, which would leave a
    // mismatch where the line ends). Trailing blank space is then trimmed so a
    // final newline does not count as a difference.
    auto normalize = [](std::string text) {
        text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
        while (!text.empty() &&
               (text.back() == '\n' || text.back() == ' ' || text.back() == '\t')) {
            text.pop_back();
        }
        return text;
    };
    return normalize(actual) == normalize(expected);
}

CodeExecutor::ProcessResult CodeExecutor::runWithLimits(const std::string& command,
                                                        const std::string& input,
                                                        int timeLimitMs,
                                                        int memoryLimitKb) const {
#ifdef _WIN32
    (void)memoryLimitKb;
    // Windows sandbox: CreateProcess inside a Job Object, with stdin/stdout
    // routed through temp files.
    //
    // Files rather than pipes, because a pipe whose buffer fills while the
    // parent is not reading deadlocks the child; a file cannot. The Job Object
    // supplies the CPU-time limit (POSIX gets it from rlimit), and a
    // wall-clock WaitForSingleObject timeout covers blocked I/O, with
    // TerminateJobObject as the kill switch.
    ProcessResult result;
    result.exitCode = -1;

    const std::string tag = std::to_string(
        static_cast<unsigned long long>(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    const std::string inFile = workDir_ + "/stdin_" + tag + ".txt";
    const std::string outFile = workDir_ + "/stdout_" + tag + ".txt";

    {
        std::ofstream in(inFile.c_str(), std::ios::binary);
        in << input;
    }

    SECURITY_ATTRIBUTES inheritable;
    ZeroMemory(&inheritable, sizeof(inheritable));
    inheritable.nLength = sizeof(inheritable);
    inheritable.bInheritHandle = TRUE;

    const HANDLE inHandle = CreateFileA(
        inFile.c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        &inheritable, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    const HANDLE outHandle = CreateFileA(
        outFile.c_str(), GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        &inheritable, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (inHandle == INVALID_HANDLE_VALUE || outHandle == INVALID_HANDLE_VALUE) {
        if (inHandle != INVALID_HANDLE_VALUE) CloseHandle(inHandle);
        if (outHandle != INVALID_HANDLE_VALUE) CloseHandle(outHandle);
        removeFile(inFile);
        removeFile(outFile);
        result.stderrData = "Could not open the redirection files";
        return result;
    }

    STARTUPINFOW startup;
    ZeroMemory(&startup, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = inHandle;
    startup.hStdOutput = outHandle;
    startup.hStdError = outHandle;

    PROCESS_INFORMATION process;
    ZeroMemory(&process, sizeof(process));

    // CreateProcessW takes a mutable command-line buffer.
    std::wstring wideCommand(command.begin(), command.end());
    const BOOL started = CreateProcessW(
        nullptr, &wideCommand[0], nullptr, nullptr, TRUE,
        CREATE_SUSPENDED, nullptr, nullptr, &startup, &process);

    // The child inherited its own handles, so the parent's copies can close.
    CloseHandle(inHandle);
    CloseHandle(outHandle);

    if (!started) {
        removeFile(inFile);
        removeFile(outFile);
        result.stderrData = "Could not start the child process";
        return result;
    }

    HANDLE job = CreateJobObjectW(nullptr, nullptr);
    if (job != nullptr) {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits;
        ZeroMemory(&limits, sizeof(limits));
        limits.BasicLimitInformation.LimitFlags =
            JOB_OBJECT_LIMIT_JOB_TIME | JOB_OBJECT_LIMIT_PROCESS_MEMORY;
        if (timeLimitMs > 0) {
            // The job timer uses 100ns units, so ms * 10000.
            limits.BasicLimitInformation.PerJobUserTimeLimit.QuadPart =
                static_cast<LONGLONG>(timeLimitMs) * 10000LL;
        }
        limits.ProcessMemoryLimit =
            static_cast<SIZE_T>(256ULL) * 1024ULL * 1024ULL;
        SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits,
                                sizeof(limits));
        AssignProcessToJobObject(job, process.hProcess);
    }

    ResumeThread(process.hThread);
    CloseHandle(process.hThread);

    const DWORD waitMs = timeLimitMs > 0
        ? static_cast<DWORD>(timeLimitMs + 500)
        : INFINITE;
    if (WaitForSingleObject(process.hProcess, waitMs) == WAIT_TIMEOUT) {
        if (job != nullptr) TerminateJobObject(job, 1);
        WaitForSingleObject(process.hProcess, 1000);
        result.timedOut = true;
    }

    DWORD exitCode = 0;
    if (GetExitCodeProcess(process.hProcess, &exitCode)) {
        result.exitCode = static_cast<int>(exitCode);
    }

    {
        std::ifstream out(outFile.c_str(), std::ios::binary);
        std::ostringstream buffer;
        buffer << out.rdbuf();
        result.stdoutData = buffer.str();
    }

    if (job != nullptr) CloseHandle(job);
    CloseHandle(process.hProcess);
    removeFile(inFile);
    removeFile(outFile);
    return result;
#else
    ProcessResult result;
    result.exitCode = -1;

    // Pipes: child stdin, child stdout, child stderr.
    int inPipe[2] = {0, 0};
    int outPipe[2] = {0, 0};
    int errPipe[2] = {0, 0};
    if (pipe(inPipe) != 0 || pipe(outPipe) != 0 || pipe(errPipe) != 0) {
        return result;
    }

    const pid_t pid = fork();
    if (pid < 0) {
        close(inPipe[0]); close(inPipe[1]);
        close(outPipe[0]); close(outPipe[1]);
        close(errPipe[0]); close(errPipe[1]);
        return result;
    }

    if (pid == 0) {
        // ---- child ----
        close(inPipe[1]);
        close(outPipe[0]);
        close(errPipe[0]);

        dup2(inPipe[0], STDIN_FILENO);
        dup2(outPipe[1], STDOUT_FILENO);
        dup2(errPipe[1], STDERR_FILENO);
        close(inPipe[0]);
        close(outPipe[1]);
        close(errPipe[1]);

        // Address-space and data caps bound the damage a runaway allocation can
        // do before the CPU-time kill switch fires.
        if (memoryLimitKb > 0) {
            struct rlimit rl;
            rl.rlim_cur = static_cast<rlim_t>(memoryLimitKb) * 1024;
            rl.rlim_max = rl.rlim_cur;
            setrlimit(RLIMIT_AS, &rl);
            setrlimit(RLIMIT_DATA, &rl);
        }
        if (timeLimitMs > 0) {
            struct rlimit rl;
            rl.rlim_cur = static_cast<rlim_t>((timeLimitMs + 999) / 1000);
            rl.rlim_max = rl.rlim_cur;
            setrlimit(RLIMIT_CPU, &rl);
        }

        execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }

    // ---- parent ----
    close(inPipe[0]);
    close(outPipe[1]);
    close(errPipe[1]);

    // Hand the child its stdin, then close so it sees EOF.
    if (!input.empty()) {
        const ssize_t ignored = write(inPipe[1], input.data(), input.size());
        (void)ignored;
    }
    close(inPipe[1]);

    // Non-blocking reads so a child that fills its stdout pipe cannot deadlock
    // the parent before the timeout is checked.
    const int flags = fcntl(outPipe[0], F_GETFL, 0);
    fcntl(outPipe[0], F_SETFL, flags | O_NONBLOCK);
    const int errFlags = fcntl(errPipe[0], F_GETFL, 0);
    fcntl(errPipe[0], F_SETFL, errFlags | O_NONBLOCK);

    const std::chrono::steady_clock::time_point start =
        std::chrono::steady_clock::now();
    const std::chrono::steady_clock::time_point deadline =
        start + std::chrono::milliseconds(timeLimitMs + 200);

    char buffer[4096];
    bool childDone = false;
    while (!childDone) {
        ssize_t n = read(outPipe[0], buffer, sizeof(buffer));
        while (n > 0) {
            result.stdoutData.append(buffer, static_cast<std::size_t>(n));
            n = read(outPipe[0], buffer, sizeof(buffer));
        }
        n = read(errPipe[0], buffer, sizeof(buffer));
        while (n > 0) {
            result.stderrData.append(buffer, static_cast<std::size_t>(n));
            n = read(errPipe[0], buffer, sizeof(buffer));
        }

        int status = 0;
        const pid_t waited = waitpid(pid, &status, WNOHANG);
        if (waited == pid) {
            childDone = true;
            if (WIFEXITED(status)) result.exitCode = WEXITSTATUS(status);
            else if (WIFSIGNALED(status)) result.exitCode = 128 + WTERMSIG(status);
        } else if (std::chrono::steady_clock::now() >= deadline) {
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            result.timedOut = true;
            break;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    }

    close(outPipe[0]);
    close(errPipe[0]);

    const std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    result.timeMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    return result;
#endif
}

} // namespace execution

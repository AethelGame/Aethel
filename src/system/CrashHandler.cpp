#include <atomic>
#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>

#if defined(_WIN32)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbghelp.h>
#else
#include <csignal>
#include <execinfo.h>
#include <unistd.h>
#endif

#include <system/CrashHandler.h>
#include <system/Logger.h>

namespace {

std::atomic<bool> gInstalled{false};
std::string gDumpDir = "logs";

std::string makeTimestampedPath(const std::string& dir, const std::string& ext) {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

    std::filesystem::create_directories(dir);

    std::ostringstream ss;
    ss << dir;
    if (!dir.empty() && dir.back() != '/' && dir.back() != '\\') {
        ss << '/';
    }
    ss << "crash_" << ms << ext;
    return ss.str();
}

#if defined(_WIN32)
LONG WINAPI UnhandledExceptionFilterFn(EXCEPTION_POINTERS* info) {
    std::string dumpPath = makeTimestampedPath(gDumpDir, ".dmp");

    HANDLE hFile = CreateFileA(dumpPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION mei{};
        mei.ThreadId = GetCurrentThreadId();
        mei.ExceptionPointers = info;
        mei.ClientPointers = TRUE;

        MiniDumpWriteDump(
            GetCurrentProcess(),
            GetCurrentProcessId(),
            hFile,
            MiniDumpNormal,
            &mei,
            nullptr,
            nullptr
        );
        CloseHandle(hFile);
        GAME_LOG_INFO("Minidump written to " + dumpPath);
    } else {
        GAME_LOG_ERROR("Failed to create minidump file at " + dumpPath);
    }
    return EXCEPTION_EXECUTE_HANDLER;
}
#else
void posixSignalHandler(int sig, siginfo_t* /*info*/, void* /*ucontext*/) {
    const char* sigName = strsignal(sig);
    
    void* buffer[64];
    int frames = backtrace(buffer, 64);
    char** symbols = backtrace_symbols(buffer, frames);

    std::ostringstream ss;
    ss << "Caught signal " << sig << " (" << (sigName ? sigName : "unknown") << ")\n";
    ss << "Backtrace (" << frames << " frames):\n";
    if (symbols) {
        for (int i = 0; i < frames; ++i) {
            ss << "  [" << i << "] " << symbols[i] << "\n";
        }
    }

    GAME_LOG_INFO(ss.str());

    auto msg = ss.str();
    write(STDERR_FILENO, msg.c_str(), msg.size());

    std::signal(sig, SIG_DFL);
    std::raise(sig);
}
#endif

void installPosixHandlers() {
#if !defined(_WIN32)
    struct sigaction sa{};
    sa.sa_sigaction = posixSignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO | SA_RESETHAND;

    int signals[] = {SIGSEGV, SIGABRT, SIGILL, SIGFPE, SIGBUS};
    for (int s : signals) {
        sigaction(s, &sa, nullptr);
    }
#endif
}

void installTerminateHandler() {
    std::set_terminate([] {
        GAME_LOG_INFO("std::terminate called");
        std::abort();
    });
}

}

void InstallCrashHandler(const std::string& dumpDirectory) {
    bool expected = false;
    if (!gInstalled.compare_exchange_strong(expected, true)) {
        return;
    }

    if (!dumpDirectory.empty()) {
        gDumpDir = dumpDirectory;
    }

#if defined(_WIN32)
    SetUnhandledExceptionFilter(UnhandledExceptionFilterFn);
#else
    installPosixHandlers();
#endif
    installTerminateHandler();
}


#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <windows.h>
#include <DbgHelp.h>

#pragma comment(lib, "Dbghelp.lib")

// --- 1. Memory Tracker ---
struct MemoryMetrics {
    size_t TotalAllocated = 0, TotalFreed = 0;
    size_t CurrentUsage() const { return TotalAllocated - TotalFreed; }
};
static MemoryMetrics s_Metrics;

void* operator new(size_t size) { s_Metrics.TotalAllocated += size; return malloc(size); }
void operator delete(void* memory, size_t size) noexcept { s_Metrics.TotalFreed += size; free(memory); }
void operator delete(void* memory) noexcept { free(memory); }
void PrintMemoryUsage(const char* context) {
    std::cout << "[" << context << "] Memory in use: " << s_Metrics.CurrentUsage() << " bytes\n";
}

// --- 2. Crash Handler ---
LONG WINAPI AutomatedCrashHandler(EXCEPTION_POINTERS* exceptionInfo) {
    std::cout << "\n[FATAL ERROR] Engine crashed! Generating dump for CI/CD...\n";
    HANDLE dumpFile = CreateFileA("engine_crash.dmp", GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (dumpFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
        dumpInfo.ThreadId = GetCurrentThreadId();
        dumpInfo.ExceptionPointers = exceptionInfo;
        dumpInfo.ClientPointers = TRUE;
        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), dumpFile, MiniDumpWithProcessThreadData, &dumpInfo, nullptr, nullptr);
        CloseHandle(dumpFile);
        std::cout << "[SUCCESS] Minidump saved as 'engine_crash.dmp'.\n";
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

// --- 3. Entities ---
class Entity {
public:
    virtual ~Entity() = default;
    virtual void Update(float deltaTime) = 0;
};

class Player : public Entity {
    float x = 0;
public:
    Player() { std::cout << "  -> Player Spawned\n"; }
    ~Player() { std::cout << "  -> Player Destroyed\n"; }
    void Update(float deltaTime) override { x += 10.0f * deltaTime; }
};

// --- 4. IPC Pipe Reader (Uses Persistent Pipe Handle) ---
std::string WaitForExternalCommand(HANDLE hPipe) {
    std::cout << "\n[IPC] Waiting for command from C# Automation Tool...\n";

    // Wait for C# client to connect
    BOOL connected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

    if (!connected) {
        std::cerr << "[ERROR] Client connection failed. Error code: " << GetLastError() << "\n";
        return "ERROR";
    }

    char buffer[128] = { 0 };
    DWORD bytesRead = 0;
    std::string command = "";

    // Read the command sent by C#
    if (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        command = buffer;
    }

    // Disconnect client to be ready for the next incoming connection
    DisconnectNamedPipe(hPipe);
    return command;
}

// --- 5. Main Game Loop ---
int main() {
    SetUnhandledExceptionFilter(AutomatedCrashHandler);
    std::cout << "=== Game Engine Automation Server ===\n";

    // Create the persistent Named Pipe ONCE
// Create the persistent Named Pipe ONCE (Allow multiple instances so ghost processes don't block us)
    HANDLE hPipe = CreateNamedPipeA(
        "\\\\.\\pipe\\GameAutomationPipe",
        PIPE_ACCESS_INBOUND,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES, 1024, 1024, 0, NULL);

    if (hPipe == INVALID_HANDLE_VALUE) {
        std::cerr << "[FATAL] Failed to create pipe. Error code: " << GetLastError() << "\n";
        std::cerr << "Make sure no other instance of CrashHandler.exe is running in Task Manager.\n";
        system("pause");
        return 1;
    }

    std::vector<std::unique_ptr<Entity>> gameWorld;

    while (true) {
        std::string cmd = WaitForExternalCommand(hPipe);

        if (cmd == "ERROR" || cmd.empty()) {
            continue;
        }

        std::cout << "[IPC] Received Command: '" << cmd << "'\n";

        if (cmd == "SPAWN_PLAYER") {
            gameWorld.push_back(std::make_unique<Player>());
            PrintMemoryUsage("Post-Spawn");
        }
        else if (cmd == "FORCE_CRASH") {
            std::cout << "  [TEST] Forcing Access Violation via nullptr dereference...\n";
            int* nullPtr = nullptr;
            *nullPtr = 99; // Intercepted by AutomatedCrashHandler
        }
        else if (cmd == "SHUTDOWN") {
            std::cout << "  [TEST] Shutting down engine cleanly...\n";
            break;
        }
    }

    CloseHandle(hPipe);
    gameWorld.clear();
    PrintMemoryUsage("Shutdown");
    return 0;
}
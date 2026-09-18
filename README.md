# Distributed Game Engine Test & Automation Framework (DTest Architecture)

A resilient, cross-language automated testing framework designed to simulate AAA game engine testing pipelines (inspired by Ubisoft's DTest architecture).

## Key Features
- **Native C++ Engine Test Harness:** Overrides global `operator new`/`delete` for granular runtime heap tracking; intercepts hardware access violations and generates unhandled `MiniDump` crash reports (`.dmp`) for post-mortem debugging.
- **Inter-Process Communication (IPC):** High-throughput Windows Named Pipes bridge allowing an external test driver to command the engine runtime without blocking simulation threads.
- **Autonomous C# Test Runner:** Headless test driver executing smoke tests, stress allocations, and clean teardowns via IPC.
- **CI/CD Integration:** Automated GitHub Actions pipeline compiling MSVC native C++ and .NET C# codebases, executing end-to-end headless integration suites, and validating exit statuses.

## Tech Stack
- **Languages:** C++ (C++17/20), C# (.NET 8.0)
- **APIs & Frameworks:** Win32 API (`DbgHelp`, Windows Named Pipes), RAII (`std::unique_ptr`), System.IO.Pipes
- **Tooling & CI/CD:** Visual Studio, MSBuild, WinDbg, GitHub Actions / Jenkins-ready

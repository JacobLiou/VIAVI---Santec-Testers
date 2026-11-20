# UDL.SantecPolarity - VS2015/v140 rewrite

This package is a clean implementation for one integrated polarity tester controlled only through TCP and line-oriented SCPI commands. It intentionally does not reuse the previous IL/RL/VISA/SDK architecture.

## Three independent projects

1. `Driver/UDL.SantecPolarity.Driver.sln` - native C ABI DLL.
2. `Simulator/UDL.SantecPolarity.Simulator.sln` - standalone TCP/SCPI instrument simulator.
3. `TestApp/UDL.SantecPolarity.TestApp.sln` - standalone native Win32 GUI test tool.

Every project contains its own source and headers. There are no shared include directories, project references, NuGet packages, MFC requirements, or vendor SDK dependencies. The root solution is only a convenience view and defines no project dependency.

## Compatibility target

- Visual Studio 2015 Update 3
- Platform toolset v140
- Windows 8.1 SDK (`WindowsTargetPlatformVersion=8.1`)
- Win32 and x64; Debug and Release
- conservative VS2015-compatible C++ (no C++17, filesystem, optional, string_view, or newer language switches)

## Quick start

1. Open and build the simulator solution. Run it with no arguments to listen on TCP 5025.
2. Open and build the test application solution. Connect to `127.0.0.1:5025`.
3. Select 24 cores, Type A, and PASS; click Run Test.
4. Repeat with CROSS, OPEN, SHORT, and REVERSE to verify failure reporting.
5. Build the driver solution for integration into production software.

See `SCPI_PROTOCOL.md` for the assumed integrated-instrument extension and the exact wire format.

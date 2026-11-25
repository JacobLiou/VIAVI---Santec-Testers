# Validation record

## Protocol / product status (2026-11 cross-validation)

- Invented `POLARITY:*` tree: **FROZEN stub only**. Not vendor firmware. Not production.
- Production baseline: `RLM-100 + OSX-100 + RD-SP-100-SM` (see Final Plan + `CROSS_VALIDATION.md`).
- Supplier `doc/` XNS/Cable Assembly package: GUI workflow; **no SCPI** for polarity Mapping.
- Do not treat a green stub build as polarity driver acceptance.

## Historical stub package checks

- All 3 `.vcxproj` files are well-formed XML.
- Each project has Debug/Release and Win32/x64 configurations.
- Every configuration uses `PlatformToolset=v140`, Windows 8.1 SDK, `_WIN32_WINNT=0x0603`, and `WINVER=0x0603`.
- All source/header/resource references exist and remain within the owning project directory.
- There are no `ProjectReference` entries or parent/shared include paths.
- All 9 C++ translation units pass GCC C++11 syntax checks with `-Wall -Wextra -Wpedantic` using compile-only Windows API declarations.
- The simulator's actual SCPI parser and mapping implementation passes executable tests for 12/24 cores, Type A/B/C, PASS, CROSS, OPEN, and undefined-query handling.
- The 14 exported C ABI names are unique and present in the public header.
- Source tree originally claimed "source only"; `doc/` may now contain vendor installers — do not install them for protocol validation.

The container does not contain Visual Studio 2015, v140, the Windows 8.1 SDK, or the Windows resource compiler. A final native Windows build remains required on a machine with Visual Studio 2015 Update 3 and the Windows 8.1 SDK installed.

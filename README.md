# Supercell Wx

[![CI](https://github.com/Ethan-Ka/superkawleywx/actions/workflows/ci.yml/badge.svg?branch=develop)](https://github.com/dpaulat/supercell-wx/actions/workflows/ci.yml)
[![Documentation Status](https://readthedocs.org/projects/supercell-wx/badge/?version=latest)](https://supercell-wx.readthedocs.io/en/latest/?badge=latest)

Supercell Wx is a free, open source application to visualize live and archive
NEXRAD Level 2 and Level 3 data, and severe weather alerts. It displays
continuously updating weather data on top of a responsive map, providing the
capability to monitor weather events using reflectivity, velocity, and other
products.

Please be sure to check out the documentation before getting started: [Supercell Wx Documentation](https://supercell-wx.rtfd.io/)

![image](https://supercell-wx.readthedocs.io/en/latest/_images/initial-setup-03-initial-configured-small.png)

## SuperKawleyWx 

This is my fork of the SupercellWx application. I will compile a list of features and changes I have made somewhere here.

## Supported Platforms

Supercell Wx supports the following 64-bit operating systems:

- Windows 10 (1809 or later)
- Windows 11
- Linux
  - Arch Linux (EndeavourOS, SteamOS [Steam Deck], and other Arch derivatives)
  - Fedora Linux 34+
  - openSUSE Tumbleweed
  - Ubuntu 22.04+
  - NixOS 25.05+
  - Most distributions supporting the GCC Standard C++ Library 11+
- macOS
  - 15.0+ for Intel-based Macs
  - 14.0+ for Apple silicon-based Macs

## Linux Dependencies

Supercell Wx requires the following Linux dependencies:

- Linux/X11 (Wayland works too) with support for GCC 11, OpenGL 3.3 and OpenGL ES 3.0
- X11/XCB libraries including xcb-cursor

## FAQ

Frequently asked questions:

- Q: Why is the map black when loading for the first time?

  - A. You must obtain a free API key from either (or both) [MapTiler](https://cloud.maptiler.com/auth/widget?next=https://cloud.maptiler.com/maps/) which currently does not require a credit/debit card, or [Mapbox](https://account.mapbox.com/) which ***does*** require a credit/debit card, but as of writing, you will receive 200K free requests per month, which should be sufficient for an individual user.

- Q: Why is it that when I change my color table, API key, grid width/height settings, nothing happens after hitting apply?

  - A. As of right now, you must restart Supercell Wx in order to apply these changes. In future iterations, this will no longer be an issue.

- Q: How can I contribute?
  - A. Head to [Developer Setup](https://supercell-wx.readthedocs.io/en/stable/development/developer-setup.html) and [Contributing](CONTRIBUTING.md) to configure the Supercell Wx development environment for your IDE. Currently Visual Studio and Visual Studio Code are recommended, with other IDEs remaining untested at this time.

---

## Building and Debugging SuperKawleyWx in Visual Studio 2026 (Windows)

These steps assume you have already cloned the repository. Run them once to set up the build environment, then use the "Build & Debug" steps any time you want to run your fork.

### Prerequisites

- **Visual Studio 2026** (Community or higher) with the **Desktop development with C++** workload installed
- **Python 3.x** (available on PATH)
- **CMake 3.24+** (included with VS2026, or install separately)
- **Qt 6.10.1 for MSVC 2022 x64** — install via the Qt Online Installer to `C:\Qt\6.10.1\msvc2022_64`
  - If Qt is installed elsewhere, update `CMAKE_PREFIX_PATH` in `CMakeUserPresets.json` and the `PATH` in `.vs\launch.vs.json`

### One-time environment setup

Open a **Developer Command Prompt for VS 2026** (Start menu → "Developer Command Prompt"), navigate to the repo root, and run:

```bat
tools\configure-environment.bat
```

This installs Python dependencies, sets up Conan package manager profiles (including the Debug variant), and exits. You only need to run this once (or again after deleting `.venv` or `~/.conan2/profiles`).

### First-time CMake configure (also re-run after adding new source files or dependencies)

Still in the Developer Command Prompt, run:

```bat
tools\setup-windows-vs2026-debug.bat
```

This installs Conan packages and runs `cmake` to generate the Visual Studio solution at `build-debug-vs2026\`. Wait for it to finish — it downloads and builds several large libraries (AWS SDK, MapLibre) and can take 20–40 minutes the first time.

> **Alternatively, use VS2026's built-in CMake support** (Open Folder, see below) — VS2026 can configure and build without running the bat file, as long as the Conan packages are already installed.

### Build and debug in Visual Studio 2026

1. **Open the project folder**: In VS2026, choose **File → Open → Folder…** and select the repo root (`superkawleywx\`). VS2026 detects `CMakeLists.txt` and loads the CMake workspace automatically.

2. **Select the build configuration**: In the toolbar dropdown (next to the green play button), choose **`Windows VS 2026 x64 Debug`**. If you used the local Qt path variant, choose **`Windows VS 2026 x64 Debug (Local Qt)`** instead.

3. **Build the project**: Press **Ctrl+Shift+B** (or **Build → Build All**). The first build compiles all external libraries and the application — expect 10–30 minutes. Subsequent incremental builds are fast.

4. **Start debugging**: In the toolbar "Select Startup Item" dropdown, choose **`supercell-wx.exe`**, then press **F5**. The application will launch under the VS2026 debugger with full breakpoint and watch support.

   - The `.vs\launch.vs.json` file already adds `C:\Qt\6.10.1\msvc2022_64\bin` to PATH so Qt DLLs are found at runtime.

5. **Output location**: The built executable is at:
   ```
   build\windows-vs2026-x64-debug\Debug\bin\supercell-wx.exe
   ```

### Troubleshooting

| Symptom | Fix |
|---|---|
| *"Could not find Qt6Cored.dll"* or similar at launch | Confirm `C:\Qt\6.10.1\msvc2022_64\bin` is the `PATH` entry in `.vs\launch.vs.json`. Update the path if Qt is installed elsewhere. |
| *"The system cannot find the file specified"* when pressing F5 | The exe hasn't been built yet. Run **Build → Build All** first. |
| CMake configure fails with conan profile errors | Re-run `tools\configure-environment.bat` to recreate conan profiles. |
| Map is black on first launch | Enter a MapTiler or Mapbox API key in Settings. See the FAQ above. |

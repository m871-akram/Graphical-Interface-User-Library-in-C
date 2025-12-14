#  ProjetC_IG — Graphical Interface Library in C

A lightweight, cross-platform GUI library written in C for building graphical user interfaces.
This project provides a complete toolkit for creating windows, widgets, drawing, and handling user interactions :



- Cross-platform backend (macOS, Windows, X11/Linux)
- Widget system (frames, buttons, custom widget classes)
- Event handling for user interactions
- Geometry management via a placer
- Low-level drawing primitives
- Extensible architecture (widget classes)

---

##  Project Structure

```
.
├── api/            # Public API headers
├── implem/         # Library implementation (.c/.h)
├── tests/          # Example apps (serve as manual tests)
├── _macos/         # macOS-specific binaries/sources (eibase)
├── _win/           # Windows-specific binaries/sources (eibase)
├── _x11/           # X11/Linux-specific binaries/sources (eibase)
├── cmake/          # CMake build helpers (do out-of-source builds)
├── docs/           # Doxygen config & generated output
├── misc/           # Miscellaneous files
├── CMakeLists.txt  # Top-level build configuration
└── README.md
```

---

##  Requirements

- CMake ≥ 3.20
- C compiler: Clang/GCC/MSVC
- Runtime/libs:
   - SDL2, SDL2_ttf, FreeImage
   - Platform shim `eibase` (provided in repo under `_macos`, `_x11`, `_win`)

Platform notes (as used by CMakeLists):
- macOS: headers expected under `/opt/local/include` and `/opt/local/include/SDL2`; libraries usually in `/opt/local/lib` (MacPorts). Xcode CLT required.
- Linux: headers under `/usr/include/SDL2`; link with `-lm`. Install packages e.g. on Debian/Ubuntu: `sudo apt-get install libsdl2-dev libsdl2-ttf-dev libfreeimage-dev`.
- Windows: CMake references `C:/projetc/SDL2_windows` for SDL2 and FreeImage.
   - TODO: Document the exact Windows setup steps and provide prebuilt/links.

---


##  Build and Run

Out-of-source builds are strongly recommended. You can use CLion (preferred) or plain CMake.

- CLion: open the project, select a target (e.g., `minimal`) and run/debug.
- Plain CMake:

```bash
# Clone the repository
git clone <repo_url>
cd Graphical-Interface-User-Interface-Programming-Library-in-C

# Create build directory (outside source tree)
mkdir build && cd build

# Configure
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Build the library and example targets
cmake --build . --target ig_all

# Or build a single example
cmake --build . --target minimal

# Run the executable (path from your build dir)
./minimal
```
-  Évitez d’exécuter cmake depuis la racine du dépôt car cela peut générer beaucoup de fichiers. Utilisez un répertoire de build séparé (p. ex. `build/`) ou laissez CLion s’en charger. Le répertoire `cmake/` contient des fichiers auxiliaires pour CMake.


---

## ▶ Entry Points (Executables) 

All executables are defined under `tests/` and linked against the static library `ei`.
Build any of them via `cmake --build <build_dir> --target <name>` then run `./<name>` from the build dir.

Executables:
- minimal
- hello_world
- button
- frame
- lines
- dessin_relief
- la_souris_verte
- puzzle
- two048
- minesweeper
- test_d_sor3a
- ext_testclass (links with `testclass` + `ei`)

Library:
- ei (static library built from `implem/*.c`)

---

##  API Overview (Headers in `api/`)

- ei_application.h — Application lifecycle management
- ei_widget.h — Widget creation and hierarchy
- ei_widget_configure.h — Widget configuration
- ei_event.h — Event handling system
- ei_draw.h — Drawing primitives
- ei_placer.h — Geometry management
- ei_types.h — Common type definitions
- hw_interface.h — Hardware abstraction layer

---


##  Documentation

Generate the API documentation using Doxygen (config at `docs/doxygen.cfg`). From project root:

```bash
cmake --build <build_dir> --target doc
# or directly
( cd docs && doxygen doxygen.cfg )
```

Output will be available at:
- docs/html/index.html (HTML)
- docs/latex (LaTeX)



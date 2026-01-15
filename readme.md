# fsearch-runner

Lightweight KRunner plugin that queries an existing FSearch SQLite database to provide instant filename search inside KDE Plasma's KRunner.

## Features

- Fast filename search using FSearch's indexed database
- Native C++/Qt implementation for low latency
- Shows file icons and opens files with the default application

## Prerequisites

- FSearch (with an up-to-date database)
- Qt6, KDE Frameworks (KRunner development headers)
- CMake (>= 3.16) and a C++17 compiler

If you need build notes, see `build_instructions.md` in this repo.

## Quick build & install

Run from the project root:

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel
sudo cmake --install .
```

After installing, restart KRunner:

```bash
# Stop then start KRunner (Plasma 6 or 5)
kquitapp5 krunner 2>/dev/null || kquitapp6 krunner || true
kstart5 krunner 2>/dev/null || kstart krunner &
```

## Run / Test

- Press Alt+Space (or your KRunner shortcut) and type a filename to see results.

## Configuration

The plugin reads the FSearch database (default: `~/.local/share/fsearch/fsearch.db`). If your database lives elsewhere, update the path in the source before building.

Source files of interest:

- `fsearchrunner.cpp` — plugin implementation and SQL queries
- `fsearchrunner.h` — public plugin API
- `fsearch_database_loader.*` — database helpers

## Contributing

Contributions welcome. Open issues or submit merge requests for bugs, features, or improvements.

## License

See repository LICENSE or the original FSearch project license.
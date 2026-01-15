# FSearch KRunner Plugin - Build Instructions

This is a native C++ KRunner plugin that integrates FSearch's database with KDE Plasma.

## Prerequisites

### Manjaro/Arch Linux

Install the required development packages:

```bash
# For Plasma 6
sudo pacman -S base-devel cmake extra-cmake-modules \
    qt6-base kf6-krunner kf6-ki18n kf6-kio sqlite

# For Plasma 5
sudo pacman -S base-devel cmake extra-cmake-modules \
    qt5-base kf5-runner kf5-ki18n kf5-kio sqlite
```

### Other Distributions

**Debian/Ubuntu:**
```bash
# Plasma 6
sudo apt install build-essential cmake extra-cmake-modules \
    qt6-base-dev libkf6runner-dev libkf6i18n-dev libkf6kio-dev \
    libsqlite3-dev

# Plasma 5
sudo apt install build-essential cmake extra-cmake-modules \
    qtbase5-dev libkf5runner-dev libkf5i18n-dev libkf5kio-dev \
    libsqlite3-dev
```

**Fedora:**
```bash
# Plasma 6
sudo dnf install cmake extra-cmake-modules \
    qt6-qtbase-devel kf6-krunner-devel kf6-ki18n-devel kf6-kio-devel \
    sqlite-devel

# Plasma 5
sudo dnf install cmake extra-cmake-modules \
    qt5-qtbase-devel kf5-krunner-devel kf5-ki18n-devel kf5-kio-devel \
    sqlite-devel
```

## Project Structure

Create the following directory structure:

```
plasma-runner-fsearch/
├── CMakeLists.txt
├── fsearchrunner.h
├── fsearchrunner.cpp
└── plasma-runner-fsearch.json
```

## Determining Your Plasma Version

Check which version of Plasma you're running:

```bash
plasmashell --version
```

- If it shows version 5.x, use the Plasma 5 CMakeLists.txt
- If it shows version 6.x, use the Plasma 6 CMakeLists.txt

## Build Steps

### 1. Create Build Directory

```bash
cd plasma-runner-fsearch
mkdir build
cd build
```

### 2. Configure with CMake

**For Plasma 6:**
```bash
cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release
```

**For Plasma 5:**
```bash
cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release
```

If you get errors about missing packages, check the CMake output carefully and install any missing dependencies.

### 3. Build

```bash
make -j$(nproc)
```

The `-j$(nproc)` flag uses all available CPU cores for faster compilation.

### 4. Install

```bash
sudo make install
```

## Post-Installation

### 1. Restart KRunner

```bash
kquitapp5 krunner 2>/dev/null || kquitapp6 krunner 2>/dev/null
kstart5 krunner 2>/dev/null || kstart krunner 2>/dev/null &
```

Or simply log out and log back in.

### 2. Verify Installation

Check if the plugin is loaded:

```bash
# For Plasma 6
krunner --list

# For Plasma 5
qdbus org.kde.krunner /App org.kde.krunner.App.runners
```

### 3. Enable the Plugin

Open KRunner settings:

```bash
kcmshell5 kcm_plasmasearch
# or for Plasma 6
kcmshell6 kcm_plasmasearch
```

Make sure "FSearch" is enabled in the list of runners.

## Verifying FSearch Database

Before using the plugin, ensure FSearch has created its database:

```bash
ls -la ~/.local/share/fsearch/
```

You should see a file named `fsearch.db`. If not:

1. Open FSearch
2. Go to File → Update Database
3. Wait for indexing to complete

## Testing the Plugin

1. Press `Alt+Space` or `Alt+F2` to open KRunner
2. Type a filename that exists on your system
3. You should see results from FSearch with file icons

## Troubleshooting

### Plugin Not Loading

**Check plugin files exist:**
```bash
# Plasma 6
ls /usr/lib/qt6/plugins/kf6/krunner/krunner_fsearch.so

# Plasma 5
ls /usr/lib/qt/plugins/kf5/krunner/krunner_fsearch.so
```

**Check for errors:**
```bash
journalctl --user -f | grep -i krunner
```

### Database Not Found

The plugin looks for the database at: `~/.local/share/fsearch/fsearch.db`

If FSearch stores it elsewhere on your system, you can modify the path in `fsearchrunner.cpp`:

```cpp
m_databasePath = dataLocation + "/fsearch/fsearch.db";
```

### No Results Appearing

The database schema might differ in your version of FSearch. To inspect it:

```bash
sqlite3 ~/.local/share/fsearch/fsearch.db
```

Then run:
```sql
.schema files
SELECT * FROM files LIMIT 5;
.quit
```

If the column names are different, update the SQL query in `fsearchrunner.cpp`:

```cpp
const char *sql = "SELECT path, name, type, size, mtime FROM files "
```

Common schema variations:
- Columns might be named differently (e.g., `filepath` instead of `path`)
- Type column might use numbers instead of strings
- Additional columns like `is_folder` or `extension`

### Compilation Errors

**Missing headers:**
- Ensure all development packages are installed
- Check that you're using the correct CMakeLists.txt for your Plasma version

**SQLite errors:**
Make sure sqlite3 development files are installed:
```bash
pkg-config --modversion sqlite3
```

### Performance Issues

If searching is slow:

1. Check database size:
   ```bash
   du -h ~/.local/share/fsearch/fsearch.db
   ```

2. Reduce the result limit in `fsearchrunner.cpp`:
   ```cpp
   QList<FileEntry> results = searchDatabase(query, 25); // Reduced from 50
   ```

3. Increase minimum query length in `fsearchrunner.cpp`:
   ```cpp
   if (query.length() < 3) { // Changed from 2
   ```

## Uninstallation

```bash
cd build
sudo make uninstall
```

Or manually remove:
```bash
# Plasma 6
sudo rm /usr/lib/qt6/plugins/kf6/krunner/krunner_fsearch.so

# Plasma 5
sudo rm /usr/lib/qt/plugins/kf5/krunner/krunner_fsearch.so

# Metadata
sudo rm /usr/share/krunner/dbusplugins/plasma-runner-fsearch.json
```

Then restart KRunner.

## Development Tips

### Quick Rebuild After Changes

```bash
cd build
make -j$(nproc) && sudo make install
kquitapp5 krunner; kstart5 krunner
```

### Debugging

Run KRunner from terminal to see debug output:

```bash
kquitapp5 krunner
QT_LOGGING_RULES="*=true" krunner
```

Add debug statements in your code:
```cpp
qDebug() << "FSearch: Searching for" << query;
```

### Testing SQL Queries

Test queries directly on the database:

```bash
sqlite3 ~/.local/share/fsearch/fsearch.db
```

```sql
SELECT path, name, type, size, mtime 
FROM files 
WHERE name LIKE '%test%' 
LIMIT 10;
```

## Advanced Customization

### Change Icon

Edit `plasma-runner-fsearch.json`:
```json
"Icon": "edit-find"  // or any other icon name
```

### Add Search Syntax

In `fsearchrunner.cpp`, add to the constructor:

```cpp
addSyntax(KRunner::RunnerSyntax(":q:", i18n("Search files by name")));
```

### Add Actions

Add context menu actions in `fsearchrunner.cpp`:

```cpp
QList<QAction*> FSearchRunner::actionsForMatch(const KRunner::QueryMatch &match)
{
    QList<QAction*> actions;
    
    QAction *openFolder = new QAction(QIcon::fromTheme("folder-open"), 
                                      i18n("Open Containing Folder"), this);
    actions << openFolder;
    
    return actions;
}
```

## Further Reading

- [KRunner Documentation](https://develop.kde.org/docs/plasma/krunner/)
- [FSearch GitHub](https://github.com/cboxdoerfer/fsearch)
- [KDE API Documentation](https://api.kde.org/)

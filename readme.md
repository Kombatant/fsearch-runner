# FSearch KRunner Plugin

A native C++ KRunner plugin for KDE Plasma that integrates [FSearch](https://github.com/cboxdoerfer/fsearch)'s database, providing instant file search directly from KRunner.

![License](https://img.shields.io/badge/license-GPL--2.0-blue.svg)
![Platform](https://img.shields.io/badge/platform-Linux-lightgrey.svg)
![KDE](https://img.shields.io/badge/KDE-Plasma%205%2F6-blue.svg)

## Features

- **Instant Search**: Search your entire filesystem from KRunner (Alt+Space)
- **Native Performance**: Written in C++ for maximum speed
- **Smart Relevance**: Intelligent ranking of search results
- **File Type Icons**: Automatic icon detection based on file types
- **Seamless Integration**: Works with Application Launcher and Plasma search
- **Direct File Opening**: Open files with default applications
- **Lightweight**: Minimal memory footprint

## Screenshots

*Press Alt+Space, type a filename, and get instant results:*

```
🔍 Search...
├─ 📄 document.pdf          ~/Documents/Work/document.pdf
├─ 📁 Documents             ~/Documents
├─ 📄 document_v2.pdf       ~/Archive/2023/document_v2.pdf
└─ 📄 my-document.txt       ~/Desktop/my-document.txt
```

## Prerequisites

### Required Software

- **FSearch**: Must be installed and database created
- **KDE Plasma**: Version 5.x or 6.x
- **SQLite3**: For database access
- **CMake**: Version 3.16 or higher
- **C++17 Compiler**: GCC or Clang

### Manjaro/Arch Linux

```bash
# Install FSearch
sudo pacman -S fsearch

# Install build dependencies
sudo pacman -S base-devel cmake extra-cmake-modules qt6-base kf6-krunner kf6-ki18n kf6-kio sqlite
```

### Before Building

1. **Install FSearch**:
   ```bash
   sudo pacman -S fsearch
   ```

2. **Create FSearch Database**:
   - Open FSearch
   - Go to File → Update Database
   - Wait for indexing to complete
   - Verify database exists: `ls ~/.local/share/fsearch/fsearch.db`

## Quick Start

### 1. Clone/Download the Plugin

Create a directory with these files:
- `CMakeLists.txt`
- `fsearchrunner.h`
- `fsearchrunner.cpp`
- `plasma-runner-fsearch.json`

### 2. Build and Install

```bash
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

### 3. Restart KRunner

```bash
kquitapp5 krunner 2>/dev/null || kquitapp6 krunner
kstart5 krunner 2>/dev/null || kstart krunner &
```

### 4. Test It

1. Press `Alt+Space`
2. Type a filename
3. See results appear instantly!

## Detailed Documentation

- [BUILD.md](BUILD.md) - Complete build instructions for all platforms
- [Database Schema Inspector](inspect_fsearch_schema.sh) - Tool to examine your FSearch database

## How It Works

```
┌─────────────┐
│   KRunner   │
└──────┬──────┘
       │ User types query
       ▼
┌─────────────────────┐
│ FSearch KRunner     │
│ Plugin (C++)        │
└──────┬──────────────┘
       │ SQL query
       ▼
┌─────────────────────┐
│ FSearch SQLite DB   │
│ ~/.local/share/     │
│ fsearch/fsearch.db  │
└─────────────────────┘
```

The plugin:
1. Monitors KRunner queries
2. Queries FSearch's SQLite database
3. Ranks results by relevance
4. Returns matches with appropriate icons
5. Opens files when selected

## Configuration

### Minimum Query Length

Default: 2 characters. To change, edit `fsearchrunner.cpp`:

```cpp
if (query.length() < 3) { // Changed to 3
    return;
}
```

### Result Limit

Default: 50 results. To change, edit `fsearchrunner.cpp`:

```cpp
QList<FileEntry> results = searchDatabase(query, 100); // Changed to 100
```

### Database Path

Default: `~/.local/share/fsearch/fsearch.db`

If FSearch stores the database elsewhere, edit `fsearchrunner.cpp`:

```cpp
m_databasePath = "/custom/path/to/fsearch.db";
```

## Troubleshooting

### Plugin Not Loading

**Check installation:**
```bash
# Plasma 6
ls /usr/lib/qt6/plugins/kf6/krunner/krunner_fsearch.so

# Plasma 5  
ls /usr/lib/qt/plugins/kf5/krunner/krunner_fsearch.so
```

**Check KRunner settings:**
```bash
kcmshell5 kcm_plasmasearch
```

Ensure "FSearch" is enabled.

### No Results

**Verify database:**
```bash
ls -lh ~/.local/share/fsearch/fsearch.db
```

**Test database:**
```bash
sqlite3 ~/.local/share/fsearch/fsearch.db "SELECT COUNT(*) FROM files;"
```

**Inspect schema:**
```bash
./inspect_fsearch_schema.sh
```

### Schema Mismatch

Different FSearch versions may use different database schemas. Use the inspector script:

```bash
chmod +x inspect_fsearch_schema.sh
./inspect_fsearch_schema.sh
```

It will show you the exact column names to use in your SQL queries.

### Performance Issues

**Large databases** (millions of files):
- Reduce result limit
- Increase minimum query length
- Consider adding indexes (though FSearch should handle this)

**Memory usage:**
- The plugin keeps the database connection open for performance
- This uses minimal RAM (~1-2 MB)

## Advanced Usage

### Adding Custom Actions

Edit `fsearchrunner.cpp` to add right-click actions:

```cpp
QList<QAction*> FSearchRunner::actionsForMatch(const QueryMatch &match)
{
    QList<QAction*> actions;
    
    auto *openFolder = new QAction(QIcon::fromTheme("folder-open"),
                                   i18n("Open Containing Folder"), this);
    openFolder->setData(match.data());
    connect(openFolder, &QAction::triggered, this, [this, match]() {
        // Open folder logic
    });
    actions << openFolder;
    
    return actions;
}
```

### Custom Relevance Scoring

Modify `calculateRelevance()` in `fsearchrunner.cpp`:

```cpp
qreal FSearchRunner::calculateRelevance(const QString &name, const QString &query)
{
    // Your custom scoring logic
    // Return value between 0.0 and 1.0
}
```

### Filtering by File Type

Add file type filters to the SQL query:

```cpp
const char *sql = "SELECT path, name, type, size, mtime FROM files "
                 "WHERE name LIKE ?1 AND type != 'directory' " // Only files
                 "ORDER BY name COLLATE NOCASE LIMIT ?2";
```

## Comparison with Alternatives

| Feature | FSearch Plugin | Baloo | locate | find |
|---------|---------------|-------|--------|------|
| Real-time results | ✓ | ✓ | ✓ | ✗ |
| KRunner integration | ✓ | ✓ | ✗ | ✗ |
| Low resource usage | ✓ | ✗ | ✓ | ✓ |
| Instant indexing | ✓ | ✗ | ✗ | N/A |
| File content search | ✗ | ✓ | ✗ | ✗ |

## Contributing

Contributions welcome! Areas for improvement:

- [ ] Add configuration UI
- [ ] Support for file content search
- [ ] Custom search filters
- [ ] Multiple database support
- [ ] Internationalization
- [ ] File preview support

## FAQ

**Q: Does this replace Baloo?**  
A: No, they can coexist. This plugin is lighter and faster for filename searches, while Baloo provides content search.

**Q: How often does the database update?**  
A: FSearch handles database updates. This plugin just reads it. Configure FSearch to auto-update.

**Q: Can I search file contents?**  
A: Not currently. FSearch only indexes filenames and metadata.

**Q: Does it work on Wayland?**  
A: Yes, it works on both X11 and Wayland.

**Q: What about performance on large databases?**  
A: FSearch is designed for speed. Databases with millions of files work well.

## License

GPL-2.0+ - See the FSearch project for details.

## Credits

- Based on [FSearch](https://github.com/cboxdoerfer/fsearch) by Christian Boxdörfer
- KRunner framework by KDE
- Inspired by Everything Search Engine for Windows

## Support

- **Issues**: Open an issue in this repository
- **FSearch Issues**: Use the [FSearch issue tracker](https://github.com/cboxdoerfer/fsearch/issues)
- **KDE Help**: Visit [KDE Community](https://community.kde.org/)

## Changelog

### Version 1.0 (Initial Release)
- Basic file search functionality
- Smart relevance ranking
- File type icon detection
- Integration with KRunner and Application Launcher

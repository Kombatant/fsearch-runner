#ifndef FSEARCH_DATABASE_LOADER_H
#define FSEARCH_DATABASE_LOADER_H

#include <QString>
#include <QVector>
#include <QFile>
#include <cstdint>

struct FSearchEntry {
    QString name;
    QString path;  // parent path
    uint64_t size;
    uint64_t mtime;
    bool isFolder;
    uint32_t parentIdx;
};

class FSearchDatabaseLoader {
public:
    FSearchDatabaseLoader();
    ~FSearchDatabaseLoader();
    
    bool loadDatabase(const QString &dbPath);
    QVector<FSearchEntry> search(const QString &query, int limit = 50) const;
    
    int getNumFiles() const { return m_files.size(); }
    int getNumFolders() const { return m_folders.size(); }
    
private:
    bool readHeader(QFile &file);
    bool readFolders(QFile &file, uint32_t numFolders, uint64_t folderBlockSize, uint64_t indexFlags);
    bool readFiles(QFile &file, uint32_t numFiles, uint64_t fileBlockSize, uint64_t indexFlags);
    QString buildFullPath(const FSearchEntry &entry) const;
    
    QVector<FSearchEntry> m_folders;
    QVector<FSearchEntry> m_files;
    uint8_t m_majorVersion;
    uint8_t m_minorVersion;
};

#endif // FSEARCH_DATABASE_LOADER_H

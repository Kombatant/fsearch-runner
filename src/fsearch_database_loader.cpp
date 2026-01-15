#include "fsearch_database_loader.h"
#include <QDebug>
#include <QDataStream>
#include <cstring>

// Database format constants
#define DATABASE_MAGIC_NUMBER "FSDB"
#define DATABASE_MAJOR_VERSION 0
#define DATABASE_MINOR_VERSION 9

// Index flags
#define DATABASE_INDEX_FLAG_NAME (1 << 0)
#define DATABASE_INDEX_FLAG_PATH (1 << 1)
#define DATABASE_INDEX_FLAG_SIZE (1 << 2)
#define DATABASE_INDEX_FLAG_MODIFICATION_TIME (1 << 3)

FSearchDatabaseLoader::FSearchDatabaseLoader()
    : m_majorVersion(0)
    , m_minorVersion(0)
{
}

FSearchDatabaseLoader::~FSearchDatabaseLoader()
{
}

bool FSearchDatabaseLoader::loadDatabase(const QString &dbPath)
{
    QFile file(dbPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open database file:" << dbPath;
        return false;
    }

    // Read header
    if (!readHeader(file)) {
        qWarning() << "Failed to read database header";
        return false;
    }

    // Read index flags
    uint64_t indexFlags = 0;
    if (file.read(reinterpret_cast<char*>(&indexFlags), 8) != 8) {
        qWarning() << "Failed to read index flags";
        return false;
    }

    // Read number of folders and files
    uint32_t numFolders = 0;
    uint32_t numFiles = 0;
    if (file.read(reinterpret_cast<char*>(&numFolders), 4) != 4) {
        qWarning() << "Failed to read number of folders";
        return false;
    }
    if (file.read(reinterpret_cast<char*>(&numFiles), 4) != 4) {
        qWarning() << "Failed to read number of files";
        return false;
    }

    qDebug() << "Loading" << numFolders << "folders and" << numFiles << "files";

    // Read block sizes
    uint64_t folderBlockSize = 0;
    uint64_t fileBlockSize = 0;
    if (file.read(reinterpret_cast<char*>(&folderBlockSize), 8) != 8) {
        qWarning() << "Failed to read folder block size";
        return false;
    }
    if (file.read(reinterpret_cast<char*>(&fileBlockSize), 8) != 8) {
        qWarning() << "Failed to read file block size";
        return false;
    }

    // Skip indexes and excludes metadata (not implemented yet in FSearch either)
    uint32_t numIndexes = 0;
    uint32_t numExcludes = 0;
    if (file.read(reinterpret_cast<char*>(&numIndexes), 4) != 4) {
        return false;
    }
    if (file.read(reinterpret_cast<char*>(&numExcludes), 4) != 4) {
        return false;
    }

    // Pre-allocate vectors
    m_folders.reserve(numFolders);
    m_files.reserve(numFiles);

    // Read folders
    if (!readFolders(file, numFolders, folderBlockSize, indexFlags)) {
        qWarning() << "Failed to read folders";
        return false;
    }

    // Read files
    if (!readFiles(file, numFiles, fileBlockSize, indexFlags)) {
        qWarning() << "Failed to read files";
        return false;
    }

    qDebug() << "Successfully loaded" << m_folders.size() << "folders and" << m_files.size() << "files";
    return true;
}

bool FSearchDatabaseLoader::readHeader(QFile &file)
{
    // Read magic number
    char magic[5] = {0};
    if (file.read(magic, 4) != 4) {
        return false;
    }

    if (std::strcmp(magic, DATABASE_MAGIC_NUMBER) != 0) {
        qWarning() << "Invalid magic number:" << magic;
        return false;
    }

    // Read version
    if (file.read(reinterpret_cast<char*>(&m_majorVersion), 1) != 1) {
        return false;
    }
    if (file.read(reinterpret_cast<char*>(&m_minorVersion), 1) != 1) {
        return false;
    }

    if (m_majorVersion != DATABASE_MAJOR_VERSION) {
        qWarning() << "Unsupported major version:" << m_majorVersion;
        return false;
    }

    if (m_minorVersion > DATABASE_MINOR_VERSION) {
        qWarning() << "Unsupported minor version:" << m_minorVersion;
        return false;
    }

    qDebug() << "Database version:" << m_majorVersion << "." << m_minorVersion;
    return true;
}

bool FSearchDatabaseLoader::readFolders(QFile &file, uint32_t numFolders, uint64_t folderBlockSize, uint64_t indexFlags)
{
    if (folderBlockSize == 0 || numFolders == 0) {
        return true;
    }

    // Read entire folder block into memory for faster processing
    QByteArray folderBlock = file.read(folderBlockSize);
    if (folderBlock.size() != static_cast<qint64>(folderBlockSize)) {
        qWarning() << "Failed to read folder block";
        return false;
    }

    const uint8_t *data = reinterpret_cast<const uint8_t*>(folderBlock.constData());
    const uint8_t *dataEnd = data + folderBlockSize;
    QString previousName;

    for (uint32_t i = 0; i < numFolders && data < dataEnd; i++) {
        FSearchEntry folder;
        folder.isFolder = true;

        // Skip db_index (2 bytes, currently unused)
        if (data + 2 > dataEnd) break;
        data += 2;

        // Read name using delta encoding
        if (data + 2 > dataEnd) break;
        uint8_t nameOffset = *data++;
        uint8_t nameLen = *data++;

        // Build name from previous name
        QString name = previousName.left(nameOffset);
        if (nameLen > 0) {
            if (data + nameLen > dataEnd) break;
            name.append(QString::fromUtf8(reinterpret_cast<const char*>(data), nameLen));
            data += nameLen;
        }
        folder.name = name;
        previousName = name;

        // Read size if available
        if ((indexFlags & DATABASE_INDEX_FLAG_SIZE) != 0) {
            if (data + 8 > dataEnd) break;
            std::memcpy(&folder.size, data, 8);
            data += 8;
        }

        // Read modification time if available
        if ((indexFlags & DATABASE_INDEX_FLAG_MODIFICATION_TIME) != 0) {
            if (data + 8 > dataEnd) break;
            std::memcpy(&folder.mtime, data, 8);
            data += 8;
        }

        // Read parent index
        if (data + 4 > dataEnd) break;
        std::memcpy(&folder.parentIdx, data, 4);
        data += 4;

        m_folders.append(folder);
    }

    return m_folders.size() == static_cast<int>(numFolders);
}

bool FSearchDatabaseLoader::readFiles(QFile &file, uint32_t numFiles, uint64_t fileBlockSize, uint64_t indexFlags)
{
    if (fileBlockSize == 0 || numFiles == 0) {
        return true;
    }

    // Read entire file block into memory for faster processing
    QByteArray fileBlock = file.read(fileBlockSize);
    if (fileBlock.size() != static_cast<qint64>(fileBlockSize)) {
        qWarning() << "Failed to read file block";
        return false;
    }

    const uint8_t *data = reinterpret_cast<const uint8_t*>(fileBlock.constData());
    const uint8_t *dataEnd = data + fileBlockSize;
    QString previousName;

    for (uint32_t i = 0; i < numFiles && data < dataEnd; i++) {
        FSearchEntry fileEntry;
        fileEntry.isFolder = false;

        // Read name using delta encoding
        if (data + 2 > dataEnd) break;
        uint8_t nameOffset = *data++;
        uint8_t nameLen = *data++;

        // Build name from previous name
        QString name = previousName.left(nameOffset);
        if (nameLen > 0) {
            if (data + nameLen > dataEnd) break;
            name.append(QString::fromUtf8(reinterpret_cast<const char*>(data), nameLen));
            data += nameLen;
        }
        fileEntry.name = name;
        previousName = name;

        // Read size if available
        if ((indexFlags & DATABASE_INDEX_FLAG_SIZE) != 0) {
            if (data + 8 > dataEnd) break;
            std::memcpy(&fileEntry.size, data, 8);
            data += 8;
        }

        // Read modification time if available
        if ((indexFlags & DATABASE_INDEX_FLAG_MODIFICATION_TIME) != 0) {
            if (data + 8 > dataEnd) break;
            std::memcpy(&fileEntry.mtime, data, 8);
            data += 8;
        }

        // Read parent index
        if (data + 4 > dataEnd) break;
        std::memcpy(&fileEntry.parentIdx, data, 4);
        data += 4;

        m_files.append(fileEntry);
    }

    return m_files.size() == static_cast<int>(numFiles);
}

QString FSearchDatabaseLoader::buildFullPath(const FSearchEntry &entry) const
{
    QString fullPath;
    
    // Build path by traversing up the parent chain
    QVector<QString> pathComponents;
    
    if (entry.parentIdx < static_cast<uint32_t>(m_folders.size())) {
        uint32_t currentIdx = entry.parentIdx;
        int maxDepth = 100; // Prevent infinite loops
        
        while (currentIdx < static_cast<uint32_t>(m_folders.size()) && maxDepth-- > 0) {
            const FSearchEntry &folder = m_folders[currentIdx];
            if (!folder.name.isEmpty()) {
                pathComponents.prepend(folder.name);
            }
            
            // Check if we've reached the root (parent points to itself or invalid)
            if (folder.parentIdx == currentIdx || folder.parentIdx >= static_cast<uint32_t>(m_folders.size())) {
                break;
            }
            currentIdx = folder.parentIdx;
        }
    }

    // Build the full path
    if (!pathComponents.isEmpty()) {
        fullPath = QStringLiteral("/") + pathComponents.join(QStringLiteral("/"));
    } else {
        fullPath = QStringLiteral("/");
    }

    return fullPath;
}

QVector<FSearchEntry> FSearchDatabaseLoader::search(const QString &query, int limit) const
{
    QVector<FSearchEntry> results;
    QString lowerQuery = query.toLower();

    // Search through files first
    for (const FSearchEntry &file : m_files) {
        if (results.size() >= limit) {
            break;
        }

        QString lowerName = file.name.toLower();
        if (lowerName.contains(lowerQuery)) {
            FSearchEntry result = file;
            result.path = buildFullPath(file);
            results.append(result);
        }
    }

    // Then search through folders if we still have room
    for (const FSearchEntry &folder : m_folders) {
        if (results.size() >= limit) {
            break;
        }

        QString lowerName = folder.name.toLower();
        if (lowerName.contains(lowerQuery) && !folder.name.isEmpty()) {
            FSearchEntry result = folder;
            result.path = buildFullPath(folder);
            results.append(result);
        }
    }

    return results;
}

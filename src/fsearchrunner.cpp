#include "fsearchrunner.h"

#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QUrl>

#include <KIO/OpenFileManagerWindowJob>
#include <KIO/OpenUrlJob>
#include <KLocalizedString>

K_PLUGIN_CLASS_WITH_JSON(FSearchRunner, "plasma-runner-fsearch.json")

FSearchRunner::FSearchRunner(QObject *parent, const KPluginMetaData &metaData)
    : AbstractRunner(parent, metaData)
    , m_dbLoader(nullptr)
    , m_databaseLoaded(false)
{
    // Determine database path
    QString dataLocation = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    m_databasePath = dataLocation + QStringLiteral("/fsearch/fsearch.db");
    
    // Connect lifecycle signals
    connect(this, &AbstractRunner::prepare, this, &FSearchRunner::prepareForMatchSession);
    connect(this, &AbstractRunner::teardown, this, &FSearchRunner::tearDownMatchSession);
}

FSearchRunner::~FSearchRunner()
{
    delete m_dbLoader;
}

void FSearchRunner::init()
{
    // Defer actual database loading to prepare phase
    qDebug() << "FSearch runner initialized. Database path:" << m_databasePath;
}

void FSearchRunner::prepareForMatchSession()
{
    if (!m_databaseLoaded) {
        QMutexLocker locker(&m_dbMutex);
        
        if (m_dbLoader) {
            return; // Already loaded
        }
        
        QFileInfo dbFile(m_databasePath);
        if (!dbFile.exists()) {
            qWarning() << "FSearch database not found at:" << m_databasePath;
            qWarning() << "Please run FSearch and update the database first.";
            return;
        }
        
        m_dbLoader = new FSearchDatabaseLoader();
        if (m_dbLoader->loadDatabase(m_databasePath)) {
            m_databaseLoaded = true;
            qDebug() << "FSearch database loaded successfully";
            qDebug() << "Loaded" << m_dbLoader->getNumFiles() << "files and" << m_dbLoader->getNumFolders() << "folders";
        } else {
            qWarning() << "Failed to load FSearch database";
            delete m_dbLoader;
            m_dbLoader = nullptr;
        }
    }
}

void FSearchRunner::tearDownMatchSession()
{
    // Keep database loaded for performance
}

QString FSearchRunner::getIconForFile(const FSearchEntry &entry)
{
    // Check if it's a directory
    if (entry.isFolder) {
        return QStringLiteral("folder");
    }
    
    // Use QMimeDatabase to determine icon based on file extension
    QString fullPath = entry.path.isEmpty() ? entry.name : entry.path + QStringLiteral("/") + entry.name;
    QMimeDatabase mimeDb;
    QMimeType mimeType = mimeDb.mimeTypeForFile(fullPath, QMimeDatabase::MatchExtension);
    
    QString iconName = mimeType.iconName();
    if (!iconName.isEmpty()) {
        return iconName;
    }
    
    // Fallback icons
    return QStringLiteral("text-plain");
}

qreal FSearchRunner::calculateRelevance(const QString &name, const QString &query)
{
    QString lowerName = name.toLower();
    QString lowerQuery = query.toLower();
    
    // Exact match
    if (lowerName == lowerQuery) {
        return 1.0;
    }
    
    // Starts with query
    if (lowerName.startsWith(lowerQuery)) {
        return 0.9;
    }
    
    // Contains query
    if (lowerName.contains(lowerQuery)) {
        // Higher relevance if query is closer to the beginning
        int index = lowerName.indexOf(lowerQuery);
        qreal position_factor = 1.0 - (static_cast<qreal>(index) / lowerName.length());
        return 0.7 * position_factor;
    }
    
    // Word boundary match
    QStringList words = lowerName.split(QRegularExpression(QStringLiteral("[\\s_-]")));
    for (const QString &word : words) {
        if (word.startsWith(lowerQuery)) {
            return 0.8;
        }
    }
    
    return 0.5;
}

void FSearchRunner::match(KRunner::RunnerContext &context)
{
    if (!context.isValid()) {
        return;
    }
    
    const QString query = context.query();
    
    // Require at least 2 characters
    if (query.length() < 2) {
        return;
    }
    
    // Don't match if query looks like a command
    if (query.contains(QLatin1Char('/')) || query.contains(QLatin1Char('\\'))) {
        return;
    }
    
    QMutexLocker locker(&m_dbMutex);
    
    if (!m_dbLoader || !m_databaseLoaded) {
        return;
    }
    
    // Search database
    QVector<FSearchEntry> results = m_dbLoader->search(query, 50);
    
    QList<KRunner::QueryMatch> matches;
    
    for (const FSearchEntry &entry : results) {
        KRunner::QueryMatch match(this);
        
        // Construct full path
        QString fullPath = entry.path.isEmpty() ? entry.name : entry.path + QStringLiteral("/") + entry.name;
        
        // Set match properties
        match.setId(QStringLiteral("fsearch-") + fullPath);
        match.setText(entry.name);
        match.setSubtext(fullPath);
        match.setIconName(getIconForFile(entry));
        
        // Calculate relevance
        qreal relevance = calculateRelevance(entry.name, query);
        match.setRelevance(relevance);
        
        // Store full path as data
        match.setData(fullPath);
        
        // Set URLs for better integration
        match.setUrls({QUrl::fromLocalFile(fullPath)});
        
        matches.append(match);
    }
    
    context.addMatches(matches);
}

void FSearchRunner::run(const KRunner::RunnerContext &context, const KRunner::QueryMatch &match)
{
    Q_UNUSED(context);
    
    QString filePath = match.data().toString();
    
    if (filePath.isEmpty()) {
        return;
    }
    
    QUrl url = QUrl::fromLocalFile(filePath);
    
    // Open file with default application
    auto *job = new KIO::OpenUrlJob(url);
    job->start();
}

#include "fsearchrunner.moc"

#ifndef FSEARCHRUNNER_H
#define FSEARCHRUNNER_H

#include <KRunner/AbstractRunner>
#include "fsearch_database_loader.h"
#include <QMutex>

class FSearchRunner : public KRunner::AbstractRunner
{
    Q_OBJECT

public:
    FSearchRunner(QObject *parent, const KPluginMetaData &metaData);
    ~FSearchRunner() override;

    void match(KRunner::RunnerContext &context) override;
    void run(const KRunner::RunnerContext &context, const KRunner::QueryMatch &match) override;

private Q_SLOTS:
    void init() override;
    void prepareForMatchSession();
    void tearDownMatchSession();

private:
    QString getIconForFile(const FSearchEntry &entry);
    qreal calculateRelevance(const QString &name, const QString &query);

    FSearchDatabaseLoader *m_dbLoader;
    QString m_databasePath;
    QMutex m_dbMutex;
    bool m_databaseLoaded;
};

#endif // FSEARCHRUNNER_H

#pragma once

#include <QObject>
#include <QThread>
#include <QString>
#include <memory>

#include "LicenseClient.h"
#include "MachineInfo.h"

using namespace jwt_client;

class LicenseWorker : public QObject {
    Q_OBJECT

public:
    explicit LicenseWorker(QObject* parent = nullptr);

public slots:
    void doActivate(const QString& server, const QString& keyCode, const QString& machineHash);
    void doCheckLicense(const QString& server, const QString& keyCode,
                        const QString& token, const QString& publicKey);
    void doConnectionTest(const QString& server);
    void doDeactivate(const QString& server, const QString& keyCode);
    void doRefreshMachineHash(const QString& server);

signals:
    void activated(bool success, const QString& message, const QString& token,
                   const QString& publicKey, const QString& expiresAt,
                   const QString& sessionId, const QString& errorCode,
                   const QString& errorMessage);
    void checkLicenseDone(bool valid, const QString& message,
                          const QString& token, const QString& publicKey,
                          const QString& expiresAt, const QString& sessionId);
    void connectionTestDone(bool reachable, const QString& message);
    void deactivateDone(bool success, const QString& message);
    void machineHashReady(const QString& hash);
    void errorOccurred(const QString& error);

private:
    std::unique_ptr<LicenseClient> m_client;
    std::string m_machineHash;
};

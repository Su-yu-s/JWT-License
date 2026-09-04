#include "licenseworker.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <memory>

using namespace jwt_client;

LicenseWorker::LicenseWorker(QObject* parent)
    : QObject(parent)
{
}

void LicenseWorker::doActivate(const QString& server, const QString& keyCode,
                                const QString& machineHash) {
    try {
        m_client = std::make_unique<LicenseClient>(server.toStdString(),
            [this](const std::string& msg) {
                emit errorOccurred(QString::fromStdString(msg));
            });

        ActivateResponse resp = m_client->activate(keyCode.toStdString(),
                                                    machineHash.toStdString());
        if (resp.success) {
            if (!resp.publicKeyPem.empty()) {
                JwtToken token = m_client->verifyJwt(resp.token, resp.publicKeyPem);
                emit activated(true,
                    QString("激活成功，Token 已验证。") +
                        (!token.productId.empty() ? QString(" 产品: %1").arg(QString::fromStdString(token.productId)) : ""),
                    QString::fromStdString(resp.token),
                    QString::fromStdString(resp.publicKeyPem),
                    QString::fromStdString(resp.expiresAt),
                    QString::number(resp.sessionId),
                    {}, {});
            } else {
                emit activated(true, "激活成功（已收到 Token）",
                    QString::fromStdString(resp.token),
                    QString::fromStdString(resp.publicKeyPem),
                    QString::fromStdString(resp.expiresAt),
                    QString::number(resp.sessionId),
                    {}, {});
            }
        } else {
            emit activated(false,
                QString::fromStdString(resp.errorMessage),
                {}, {}, {}, {},
                QString::fromStdString(resp.errorCode),
                QString::fromStdString(resp.errorMessage));
        }
    } catch (const std::exception& e) {
        QString errMsg = QString::fromStdString(e.what());
        emit errorOccurred(errMsg);

        // 翻译常见 JWT 错误为中文
        QString userMsg;
        if (errMsg.contains("signature verification failed"))
            userMsg = "JWT 签名验证失败，公钥与 Token 不匹配";
        else if (errMsg.contains("issuer mismatch"))
            userMsg = "JWT 签发者不匹配，Token 来源不可信";
        else if (errMsg.contains("audience mismatch"))
            userMsg = "JWT 受众不匹配，产品 ID 不一致";
        else if (errMsg.contains("token expired"))
            userMsg = "JWT Token 已过期";
        else if (errMsg.contains("Invalid JWT format"))
            userMsg = "JWT Token 格式无效";
        else
            userMsg = QString("异常: %1").arg(errMsg);

        emit activated(false, userMsg,
            {}, {}, {}, {}, {}, {});
    }
}

void LicenseWorker::doCheckLicense(const QString& server, const QString& keyCode,
                                    const QString& token, const QString& publicKey) {
    Q_UNUSED(keyCode);
    try {
        m_client = std::make_unique<LicenseClient>(server.toStdString());

        JwtToken jwt = m_client->verifyJwt(token.toStdString(), publicKey.toStdString());
        bool expired = m_client->isTokenExpired(jwt);
        if (expired) {
            emit checkLicenseDone(false, "Token 已过期",
                token, publicKey,
                QString::fromStdString(std::to_string(jwt.expiresAt)),
                QString::fromStdString(jwt.sessionId));
        } else {
            emit checkLicenseDone(true,
                QString("Token 有效。产品: %1, 会话: %2")
                    .arg(QString::fromStdString(jwt.productId))
                    .arg(jwt.sessionId.empty() ? "—" : QString::fromStdString(jwt.sessionId)),
                token, publicKey,
                QString::fromStdString(std::to_string(jwt.expiresAt)),
                QString::fromStdString(jwt.sessionId));
        }
    } catch (const std::exception& e) {
        QString errMsg = QString::fromStdString(e.what());
        QString userMsg;
        if (errMsg.contains("signature verification failed"))
            userMsg = "JWT 签名验证失败，公钥与 Token 不匹配";
        else if (errMsg.contains("issuer mismatch"))
            userMsg = "JWT 签发者不匹配，Token 来源不可信";
        else if (errMsg.contains("audience mismatch"))
            userMsg = "JWT 受众不匹配，产品 ID 不一致";
        else if (errMsg.contains("token expired"))
            userMsg = "JWT Token 已过期";
        else if (errMsg.contains("Invalid JWT format"))
            userMsg = "JWT Token 格式无效";
        else
            userMsg = errMsg;
        emit checkLicenseDone(false, userMsg,
            {}, {}, {}, {});
    }
}

void LicenseWorker::doConnectionTest(const QString& server) {
    try {
        m_client = std::make_unique<LicenseClient>(server.toStdString());
        auto jwks = m_client->fetchJwks();
        if (!jwks.keys.empty() || m_client->getLastStatusCode() > 0) {
            emit connectionTestDone(true, "连接成功");
        } else {
            emit connectionTestDone(true, "服务器可达");
        }
    } catch (const std::exception& e) {
        emit connectionTestDone(false, QString::fromStdString(e.what()));
    }
}

void LicenseWorker::doDeactivate(const QString& server, const QString& keyCode) {
    try {
        m_client = std::make_unique<LicenseClient>(server.toStdString());
        DeactivateResponse resp = m_client->deactivate(keyCode.toStdString());
        if (resp.success) {
            emit deactivateDone(true,
                QString("已注销。会话: %1").arg(resp.sessionId));
        } else {
            QString msg = resp.errorMessage.empty()
                ? "注销失败"
                : QString::fromStdString(resp.errorMessage);
            emit deactivateDone(false, msg);
        }
    } catch (const std::exception& e) {
        emit deactivateDone(false, QString::fromStdString(e.what()));
    }
}

void LicenseWorker::doRefreshMachineHash(const QString& server) {
    try {
        m_machineHash = generateMachineHash();
        emit machineHashReady(QString::fromStdString(m_machineHash));

        // 后台静默检测服务器连通性（仅记录，不发射额外信号）
        m_client = std::make_unique<LicenseClient>(server.toStdString());
        m_client->onlineCheck("");
        // 结果仅用于静默验证，不覆盖设备指纹
    } catch (const std::exception& e) {
        m_machineHash = generateMachineHash();
        emit machineHashReady(QString::fromStdString(m_machineHash));
    }
}

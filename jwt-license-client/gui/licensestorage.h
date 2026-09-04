#pragma once

#include <QString>
#include <QDateTime>

// =====================================================================
// LicenseStorage — 许可证本地持久化
// 存储路径: %APPDATA%/JwtLicenseClient/license.json
// =====================================================================

struct LicenseRecord {
    bool    valid = false;
    QString serverUrl;
    QString keyCode;
    QString token;
    QString publicKeyPem;
    QString expiresAt;
    QString issuedAt;
    QString sessionId;
    QString machineHash;
    int     capacity = 0;

    bool isExpired() const {
        if (expiresAt.isEmpty()) return false;
        QDateTime exp = QDateTime::fromString(expiresAt, "yyyy-MM-dd HH:mm:ss");
        return exp.isValid() && exp <= QDateTime::currentDateTime();
    }

    int daysLeft() const {
        if (expiresAt.isEmpty()) return -1;
        QDateTime exp = QDateTime::fromString(expiresAt, "yyyy-MM-dd HH:mm:ss");
        if (!exp.isValid()) return -1;
        return QDateTime::currentDateTime().daysTo(exp);
    }
};

class LicenseStorage {
public:
    // 保存许可证到本地文件
    static bool save(const LicenseRecord& record);

    // 从本地文件加载许可证
    static LicenseRecord load();

    // 是否存有许可证数据
    static bool exists();

    // 删除本地许可证文件（注销时调用）
    static bool clear();

    // 检查本地记录的 key + 机器指纹是否匹配（用于判断"占用"是否为本机旧许可）
    static bool matchesMachine(const QString& keyCode, const QString& machineHash);

private:
    static QString storagePath();
};

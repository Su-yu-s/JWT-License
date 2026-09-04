#include "licensestorage.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QStandardPaths>

// =====================================================================
// 存储路径: %APPDATA%/JwtLicenseClient/license.dat
// =====================================================================

QString LicenseStorage::storagePath() {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/license.dat";
}

// =====================================================================
// 保存
// =====================================================================

bool LicenseStorage::save(const LicenseRecord& record) {
    QJsonObject obj;
    obj["serverUrl"]   = record.serverUrl;
    obj["keyCode"]     = record.keyCode;
    obj["token"]       = record.token;
    obj["publicKeyPem"]= record.publicKeyPem;
    obj["expiresAt"]   = record.expiresAt;
    obj["issuedAt"]    = record.issuedAt;
    obj["sessionId"]   = record.sessionId;
    obj["machineHash"] = record.machineHash;
    obj["capacity"]    = record.capacity;

    QJsonDocument doc(obj);
    QFile file(storagePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

// =====================================================================
// 加载
// =====================================================================

LicenseRecord LicenseStorage::load() {
    LicenseRecord rec;

    QFile file(storagePath());
    if (!file.open(QIODevice::ReadOnly)) return rec;

    QByteArray raw = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(raw);
    if (!doc.isObject()) return rec;

    QJsonObject obj = doc.object();
    rec.valid        = true;
    rec.serverUrl    = obj.value("serverUrl").toString();
    rec.keyCode      = obj.value("keyCode").toString();
    rec.token        = obj.value("token").toString();
    rec.publicKeyPem = obj.value("publicKeyPem").toString();
    rec.expiresAt    = obj.value("expiresAt").toString();
    rec.issuedAt     = obj.value("issuedAt").toString();
    rec.sessionId    = obj.value("sessionId").toString();
    rec.machineHash  = obj.value("machineHash").toString();
    rec.capacity     = obj.value("capacity").toInt();

    return rec;
}

// =====================================================================
// 是否存在
// =====================================================================

bool LicenseStorage::exists() {
    return QFile::exists(storagePath());
}

// =====================================================================
// 删除
// =====================================================================

bool LicenseStorage::clear() {
    QFile file(storagePath());
    return !file.exists() || file.remove();
}

// =====================================================================
// 检查本地记录的 key+机器指纹是否匹配
// =====================================================================

bool LicenseStorage::matchesMachine(const QString& keyCode, const QString& machineHash) {
    LicenseRecord rec = load();
    if (!rec.valid) return false;
    return rec.keyCode == keyCode && rec.machineHash == machineHash;
}

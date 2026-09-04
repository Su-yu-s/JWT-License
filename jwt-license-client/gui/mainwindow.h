#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QTimer>
#include <QString>

class LogDialog;
class LicenseWorker;

enum class LicenseState {
    Inactive,
    Activating,
    Active,
    ExpiringSoon,
    Expired,
    Revoked
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    void fadeIn();

public slots:
    void onActivate();
    void onTestConnection();
    void onCheckLicense();
    void onDeactivate();
    void onCopyDiagnostic();
    void onViewLog();
    void onToggleKeyVisibility();
    void updateStatusCard();
    void onMachineHashReady(const QString& hash);

signals:
    void statusChanged(const QString& text, const QString& indicatorColor = {});

private:
    void buildUi();
    void applyDarkTheme();
    void applyLightTheme();
    void toggleTheme();
    void disableAllButtons(bool disabled);
    static QString timestamp();

    // State management
    void setLicenseState(LicenseState state);
    void updateActivationButton();
    void updateLicenseDetails();
    void computeExpirySubtitle();
    int computeProgressPercent();
    QString assembleDiagnosticInfo();

    // 本地持久化
    void loadLocalLicense();
    void saveLocalLicense();
    void clearLocalLicense();

    // Helpers
    static QString maskKeyCode(const QString& key);
    static QString stateToString(LicenseState state);
    static QString mapErrorCode(const QString& code, const QString& msg);
    void refreshDetailColors();
    void refreshIconButtons();

    // ---- Header ----
    QLabel* m_titleLabel;
    QPushButton* m_themeButton;

    // ---- Status card ----
    QWidget* m_statusCardWidget;
    QLabel* m_statusIcon;
    QLabel* m_statusTitle;
    QLabel* m_statusSubtitle;
    QProgressBar* m_expiryProgress;

    // ---- Activation config ----
    QLineEdit* m_serverEdit;
    QPushButton* m_testConnectionBtn;
    QLineEdit* m_keyEdit;
    QPushButton* m_keyEyeBtn;
    QPushButton* m_activateButton;

    // ---- License details ----
    QLabel* m_licTypeValue;
    QLabel* m_expiryValue;
    QLabel* m_capacityValue;
    QLabel* m_lastOnlineValue;
    QLabel* m_sessionIdValue;
    QLabel* m_fingerprintValue;
    QPushButton* m_copyDiagBtn;

    // ---- Action buttons ----
    QPushButton* m_checkLicenseBtn;
    QPushButton* m_deactivateBtn;

    // ---- Bottom links ----
    QLabel* m_logLink;
    QLabel* m_copyDiagLink;

    // ---- State ----
    bool m_darkTheme = true;
    LicenseState m_licenseState = LicenseState::Inactive;
    LicenseWorker* m_worker = nullptr;
    QString m_currentMachineHash;
    QString m_logContent;

    // Cached values from last activation
    QString m_cachedToken;
    QString m_cachedPublicKey;
    QString m_cachedExpiresAt;
    QString m_cachedIssuedAt;
    QString m_cachedSessionId;
    int m_cachedCapacity = 0;
    QString m_lastOnlineTime;
};

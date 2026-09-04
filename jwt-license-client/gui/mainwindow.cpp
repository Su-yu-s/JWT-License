#include "mainwindow.h"
#include "licenseworker.h"
#include "licensestorage.h"
#include "toastnotification.h"
#include "logdialog.h"
#include "MachineInfo.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QTimer>
#include <QPropertyAnimation>
#include <QScreen>
#include <QClipboard>
#include <QFontDatabase>
#include <QDesktopServices>
#include <QUrl>
#include <QDateTime>
#include <QMouseEvent>
#include <QShortcut>
#include <QSizePolicy>
#include <QPainter>
#include <QPainterPath>
#include <QIcon>
#include <QPixmap>

using namespace jwt_client;

// =====================================================================
// FingerprintLabel — truncated with tooltip on hover
// =====================================================================

class FingerprintLabel : public QLabel {
public:
    explicit FingerprintLabel(const QString& text, QWidget* parent = nullptr)
        : QLabel(text, parent)
    {
        setObjectName("fingerprintValue");
        setWordWrap(false);
        setMouseTracking(true);
        updateDisplay();
    }

    void setText(const QString& text) {
        m_fullText = text;
        updateDisplay();
    }

protected:
    void enterEvent(QEnterEvent* event) override {
        if (m_fullText.length() > 24) {
            setToolTip(m_fullText);
        }
        QLabel::enterEvent(event);
    }
    void leaveEvent(QEvent* event) override {
        setToolTip({});
        QLabel::leaveEvent(event);
    }

private:
    void updateDisplay() {
        if (m_fullText.length() > 24) {
            QLabel::setText(m_fullText.left(24) + "…");
        } else {
            QLabel::setText(m_fullText);
        }
    }

    QString m_fullText;
};

// =====================================================================
// Helper: create styled labels
// =====================================================================

static QLabel* makeDetailLabel(const QString& text) {
    auto* lbl = new QLabel(text);
    lbl->setObjectName("detailLabel");
    lbl->setFont(QFont("Segoe UI", 11, QFont::Light));
    return lbl;
}

static QLabel* makeDetailValue(const QString& text) {
    auto* lbl = new QLabel(text);
    lbl->setObjectName("detailValue");
    lbl->setFont(QFont("Segoe UI", 13, QFont::Normal));
    return lbl;
}

static QIcon makeSunIcon(const QColor& color) {
    QPixmap pixmap(18, 18);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(color, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(QPointF(9, 9), 3.6, 3.6);

    const QPointF rays[][2] = {
        {QPointF(9, 1.8), QPointF(9, 3.6)},
        {QPointF(9, 14.4), QPointF(9, 16.2)},
        {QPointF(1.8, 9), QPointF(3.6, 9)},
        {QPointF(14.4, 9), QPointF(16.2, 9)},
        {QPointF(3.9, 3.9), QPointF(5.2, 5.2)},
        {QPointF(12.8, 12.8), QPointF(14.1, 14.1)},
        {QPointF(14.1, 3.9), QPointF(12.8, 5.2)},
        {QPointF(5.2, 12.8), QPointF(3.9, 14.1)}
    };

    for (const auto& ray : rays) {
        painter.drawLine(ray[0], ray[1]);
    }

    return QIcon(pixmap);
}

static QIcon makeMoonIcon(const QColor& color) {
    QPixmap pixmap(18, 18);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawEllipse(QRectF(4.2, 3.0, 9.6, 12.0));
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    painter.drawEllipse(QRectF(8.0, 1.8, 9.8, 12.4));

    return QIcon(pixmap);
}

static QIcon makeEyeIcon(const QColor& color, bool crossed) {
    QPixmap pixmap(18, 18);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(color, 1.55, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    QPainterPath eyePath;
    eyePath.moveTo(2.3, 9.0);
    eyePath.cubicTo(4.6, 5.6, 13.4, 5.6, 15.7, 9.0);
    eyePath.cubicTo(13.4, 12.4, 4.6, 12.4, 2.3, 9.0);
    painter.drawPath(eyePath);
    painter.drawEllipse(QPointF(9.0, 9.0), 2.0, 2.0);

    if (crossed) {
        painter.drawLine(QPointF(3.9, 14.0), QPointF(14.1, 4.0));
    }

    return QIcon(pixmap);
}

// =====================================================================
// 主题样式，参考 Codex 桌面应用的克制产品界面
// =====================================================================

static const char* LIGHT_THEME = R"(
MainWindow, QWidget {
    background: #f8fafc;
    color: #202124;
    font-family: "Segoe UI", "Microsoft YaHei", sans-serif;
    font-size: 13px;
}

QLabel {
    background: transparent;
}

QLabel#titleLabel {
    color: #202124;
    font-size: 20px;
    font-weight: 600;
}

QLabel#titleSubtitle {
    color: #6b7280;
    font-size: 12px;
}

QPushButton#themeToggle {
    background: #f3f6f8;
    border: 1px solid #dfe5ea;
    color: #5f6872;
    border-radius: 8px;
    font-size: 13px;
}
QPushButton#themeToggle:hover {
    background: #e9f2f8;
    border-color: #d2dde6;
    color: #202124;
}

#statusCardWidget {
    background: #ffffff;
    border: 1px solid #e4e8ee;
    border-radius: 10px;
}

#statusIconInactive { color: #8a949e; }
#statusIconActive { color: #0f766e; }
#statusIconWarning { color: #b7791f; }
#statusIconError { color: #b42318; }
#statusIconLoading { color: #52677a; }

QGroupBox {
    background: #ffffff;
    border: 1px solid #e4e8ee;
    border-radius: 10px;
    margin-top: 10px;
    padding: 18px 16px 14px 16px;
    font-weight: 600;
}
QGroupBox::title {
    subcontrol-origin: margin;
    left: 12px;
    padding: 0 8px;
    color: #4b5563;
    background: #f8fafc;
    font-size: 12px;
    font-weight: 600;
}

QLineEdit {
    background: #ffffff;
    color: #202124;
    border: 1px solid #d7dde4;
    border-radius: 8px;
    padding: 8px 10px;
    font-size: 13px;
    selection-background-color: #dbeafe;
}
QLineEdit:focus {
    border: 1px solid #9aa8b5;
    background: #ffffff;
}
QLineEdit:disabled {
    background: #f1f5f9;
    color: #9aa3ad;
    border-color: #e2e8f0;
}

QPushButton#activateBtn {
    background: #202124;
    color: #ffffff;
    border: 1px solid #202124;
    border-radius: 8px;
    font-weight: 600;
    padding: 6px 16px;
    font-size: 13px;
}
QPushButton#activateBtn:hover {
    background: #34373c;
    border-color: #34373c;
}
QPushButton#activateBtn:pressed {
    background: #111827;
    border-color: #111827;
}
QPushButton#activateBtn:disabled {
    background: #e5e7eb;
    color: #9ca3af;
    border-color: #e5e7eb;
}

QPushButton {
    background: #ffffff;
    color: #202124;
    border: 1px solid #d7dde4;
    border-radius: 8px;
    padding: 5px 10px;
    font-size: 13px;
    font-weight: 500;
}
QPushButton:hover {
    background: #f3f7fa;
    border-color: #cbd5e1;
}
QPushButton:pressed {
    background: #e7eef5;
    border-color: #b8c4d0;
}
QPushButton:disabled {
    background: #f1f5f9;
    color: #a8b0ba;
    border-color: #e2e8f0;
}

QPushButton#deactivateBtn {
    background: transparent;
    color: #b42318;
    border: 1px solid transparent;
    font-weight: 500;
}
QPushButton#deactivateBtn:hover {
    background: #fff1f0;
    border-color: #ffd7d3;
    color: #9f1d13;
}
QPushButton#deactivateBtn:pressed {
    background: #ffe4e0;
}
QPushButton#deactivateBtn:disabled {
    color: #d7aaa6;
}

QPushButton#testConnectionBtn {
    background: transparent;
    border: 1px solid transparent;
    color: #475569;
    font-size: 12px;
    font-weight: 600;
    padding: 4px 8px;
}
QPushButton#testConnectionBtn:hover {
    color: #202124;
    background: #eaf3f8;
    border-color: #d5e2ea;
}
QPushButton#testConnectionBtn:disabled {
    color: #a8b0ba;
}

QPushButton#keyEyeBtn {
    background: #f8fafc;
    border: 1px solid #d7dde4;
    border-radius: 8px;
    color: #64748b;
    font-size: 13px;
}
QPushButton#keyEyeBtn:hover {
    color: #202124;
    border-color: #cbd5e1;
    background: #eef6fb;
}

QPushButton#copyDiagBtn {
    background: transparent;
    border: 1px solid #d7dde4;
    color: #475569;
    font-size: 12px;
    font-weight: 600;
    border-radius: 8px;
    padding: 5px 12px;
}
QPushButton#copyDiagBtn:hover {
    background: #eaf3f8;
    border-color: #d5e2ea;
}

QLabel#detailLabel {
    color: #6b7280;
    font-size: 11px;
    font-weight: 600;
    background: transparent;
}
QLabel#detailValue {
    color: #202124;
    font-size: 13px;
    background: transparent;
}
QLabel#fingerprintValue {
    color: #6b7280;
    font-size: 11px;
    font-family: "Cascadia Code", "Consolas", "Courier New", monospace;
    background: transparent;
}

QLabel#bottomLink {
    color: #475569;
    font-size: 12px;
    font-weight: 600;
}
QLabel#bottomLink:hover {
    color: #202124;
}

QProgressBar {
    background: #e5ebf0;
    border: none;
    border-radius: 4px;
    height: 4px;
    text-align: center;
}
QProgressBar::chunk {
    border-radius: 4px;
}

#statusTitle {
    font-size: 22px;
    font-weight: 600;
    color: #202124;
}
#statusSubtitle {
    font-size: 13px;
    color: #6b7280;
}
)";

static const char* DARK_THEME = R"(
MainWindow, QWidget {
    background: #111418;
    color: #e5e7eb;
    font-family: "Segoe UI", "Microsoft YaHei", sans-serif;
    font-size: 13px;
}

QLabel {
    background: transparent;
}

QLabel#titleLabel {
    color: #e5e7eb;
    font-size: 20px;
    font-weight: 600;
}

QLabel#titleSubtitle {
    color: #9ca3af;
    font-size: 12px;
}

QPushButton#themeToggle {
    background: #1b2026;
    border: 1px solid #2b333d;
    color: #cbd5e1;
    border-radius: 8px;
    font-size: 13px;
}
QPushButton#themeToggle:hover {
    background: #222a33;
    border-color: #3a4654;
    color: #f8fafc;
}

#statusCardWidget {
    background: #171b20;
    border: 1px solid #2b333d;
    border-radius: 10px;
}

#statusIconInactive { color: #7b8794; }
#statusIconActive { color: #5eead4; }
#statusIconWarning { color: #fbbf24; }
#statusIconError { color: #f87171; }
#statusIconLoading { color: #93a4b7; }

QGroupBox {
    background: #171b20;
    border: 1px solid #2b333d;
    border-radius: 10px;
    margin-top: 10px;
    padding: 18px 16px 14px 16px;
    font-weight: 600;
}
QGroupBox::title {
    subcontrol-origin: margin;
    left: 12px;
    padding: 0 8px;
    color: #aab4bf;
    background: #111418;
    font-size: 12px;
    font-weight: 600;
}

QLineEdit {
    background: #101418;
    color: #e5e7eb;
    border: 1px solid #2b333d;
    border-radius: 8px;
    padding: 8px 10px;
    font-size: 13px;
    selection-background-color: #1e3a5f;
}
QLineEdit:focus {
    border: 1px solid #607080;
    background: #101418;
}
QLineEdit:disabled {
    background: #151a20;
    color: #6b7280;
    border-color: #252c35;
}

QPushButton#activateBtn {
    background: #e5e7eb;
    color: #111418;
    border: 1px solid #e5e7eb;
    border-radius: 8px;
    font-weight: 600;
    padding: 6px 16px;
    font-size: 13px;
}
QPushButton#activateBtn:hover {
    background: #f8fafc;
    border-color: #f8fafc;
}
QPushButton#activateBtn:pressed {
    background: #cbd5e1;
    border-color: #cbd5e1;
}
QPushButton#activateBtn:disabled {
    background: #2b333d;
    color: #6b7280;
    border-color: #2b333d;
}

QPushButton {
    background: #171b20;
    color: #e5e7eb;
    border: 1px solid #2b333d;
    border-radius: 8px;
    padding: 5px 10px;
    font-size: 13px;
    font-weight: 500;
}
QPushButton:hover {
    background: #1f2630;
    border-color: #3a4654;
}
QPushButton:pressed {
    background: #101418;
}
QPushButton:disabled {
    background: #151a20;
    color: #6b7280;
    border-color: #252c35;
}

QPushButton#deactivateBtn {
    background: transparent;
    color: #f87171;
    border: 1px solid transparent;
    font-weight: 500;
}
QPushButton#deactivateBtn:hover {
    background: #2a1717;
    border-color: #5b2929;
    color: #fca5a5;
}
QPushButton#deactivateBtn:disabled {
    color: #7f4b4b;
}

QPushButton#testConnectionBtn {
    background: transparent;
    border: 1px solid transparent;
    color: #cbd5e1;
    font-size: 12px;
    font-weight: 600;
    padding: 4px 8px;
}
QPushButton#testConnectionBtn:hover {
    color: #f8fafc;
    background: #1e2933;
    border-color: #334155;
}
QPushButton#testConnectionBtn:disabled {
    color: #5b6570;
}

QPushButton#keyEyeBtn {
    background: #151a20;
    border: 1px solid #2b333d;
    border-radius: 8px;
    color: #cbd5e1;
    font-size: 13px;
}
QPushButton#keyEyeBtn:hover {
    color: #f8fafc;
    border-color: #3a4654;
    background: #1f2630;
}

QPushButton#copyDiagBtn {
    background: transparent;
    border: 1px solid #2b333d;
    color: #cbd5e1;
    font-size: 12px;
    font-weight: 600;
    border-radius: 8px;
    padding: 5px 12px;
}
QPushButton#copyDiagBtn:hover {
    background: #1e2933;
    border-color: #334155;
}

QLabel#detailLabel {
    color: #9ca3af;
    font-size: 11px;
    font-weight: 600;
    background: transparent;
}
QLabel#detailValue {
    color: #e5e7eb;
    font-size: 13px;
    background: transparent;
}
QLabel#fingerprintValue {
    color: #9ca3af;
    font-size: 11px;
    font-family: "Cascadia Code", "Consolas", "Courier New", monospace;
    background: transparent;
}

QLabel#bottomLink {
    color: #cbd5e1;
    font-size: 12px;
    font-weight: 600;
}
QLabel#bottomLink:hover {
    color: #f8fafc;
}

QProgressBar {
    background: #2b333d;
    border: none;
    border-radius: 4px;
    height: 4px;
    text-align: center;
}
QProgressBar::chunk {
    border-radius: 4px;
}

#statusTitle {
    font-size: 22px;
    font-weight: 600;
    color: #e5e7eb;
}
#statusSubtitle {
    font-size: 13px;
    color: #9ca3af;
}
)";

// =====================================================================
// MainWindow
// =====================================================================

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("iK客户端");
    setMinimumSize(900, 560);
    resize(960, 610);

    buildUi();

    // 初始主题为亮色
    applyLightTheme();
    ToastNotification::setDarkMode(false);

    // Worker thread
    QThread* workerThread = new QThread(this);
    m_worker = new LicenseWorker();
    m_worker->moveToThread(workerThread);
    workerThread->start();

    // ---- Signal connections ----

    // Activate
    connect(m_worker, &LicenseWorker::activated,
            this, [this](bool success, const QString& message,
                        const QString& token, const QString& publicKey,
                        const QString& expiresAt, const QString& sessionId,
                        const QString& errorCode, const QString& errorMessage) {
        if (success) {
            m_cachedToken = token;
            m_cachedPublicKey = publicKey;
            m_cachedExpiresAt = expiresAt;
            m_cachedSessionId = sessionId;
            m_cachedIssuedAt = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
            m_lastOnlineTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

            QDateTime expDate = QDateTime::fromString(expiresAt, "yyyy-MM-dd HH:mm:ss");
            if (expDate.isValid()) {
                int daysLeft = QDateTime::currentDateTime().daysTo(expDate);
                if (daysLeft <= 0) setLicenseState(LicenseState::Expired);
                else if (daysLeft <= 7) setLicenseState(LicenseState::ExpiringSoon);
                else setLicenseState(LicenseState::Active);
            } else {
                setLicenseState(LicenseState::Active);
            }

            saveLocalLicense();

            m_logContent += timestamp() + " 许可证激活成功。\n";
            m_logContent += timestamp() + message + "\n";
            ToastNotification::showSuccess(this, message);
        } else {
            QString userMsg = mapErrorCode(errorCode, errorMessage);
            ToastNotification::showError(this, userMsg);
            m_logContent += timestamp() + " 激活失败: " + userMsg + "\n";
            setLicenseState(LicenseState::Inactive);
        }
        updateStatusCard();
        disableAllButtons(false);
    });

    // Check license
    connect(m_worker, &LicenseWorker::checkLicenseDone,
            this, [this](bool valid, const QString& message,
                         const QString&, const QString&,
                         const QString& expiresAt, const QString& sessionId) {
        m_checkLicenseBtn->setEnabled(true);
        m_checkLicenseBtn->setText("检查许可证");
        if (valid) {
            ToastNotification::showSuccess(this, "许可证有效: " + message);
            m_logContent += timestamp() + " 许可证校验通过: " + message + "\n";
        } else {
            ToastNotification::showError(this, "许可证无效: " + message);
            m_logContent += timestamp() + " 许可证校验失败: " + message + "\n";
        }
    });

    // Connection test
    connect(m_worker, &LicenseWorker::connectionTestDone,
            this, [this](bool reachable, const QString& message) {
        m_testConnectionBtn->setEnabled(true);
        m_testConnectionBtn->setText("测试连接");
        if (reachable) {
            ToastNotification::showSuccess(this, message);
            m_logContent += timestamp() + " 连接测试: " + message + "\n";
        } else {
            ToastNotification::showError(this, message);
            m_logContent += timestamp() + " 连接测试失败: " + message + "\n";
        }
    });

    // Deactivate
    connect(m_worker, &LicenseWorker::deactivateDone,
            this, [this](bool success, const QString& message) {
        if (success) {
            m_cachedToken.clear();
            m_cachedPublicKey.clear();
            m_cachedExpiresAt.clear();
            m_cachedIssuedAt.clear();
            m_cachedSessionId.clear();
            m_cachedCapacity = 0;
            m_lastOnlineTime.clear();
            clearLocalLicense();
            setLicenseState(LicenseState::Inactive);
            m_logContent += timestamp() + " 已注销: " + message + "\n";
            ToastNotification::showSuccess(this, message);
        } else {
            ToastNotification::showError(this, "注销失败: " + message);
            m_logContent += timestamp() + " 注销失败: " + message + "\n";
        }
        updateStatusCard();
        m_deactivateBtn->setEnabled(true);
        m_deactivateBtn->setText("解除授权");
    });

    // Machine hash
    connect(m_worker, &LicenseWorker::machineHashReady,
            this, [this](const QString& hash) {
        m_currentMachineHash = hash;
        m_logContent += timestamp() + " 设备指纹: " + hash.left(24) + "\n";
        if (m_fingerprintValue) {
            static_cast<FingerprintLabel*>(m_fingerprintValue)->setText(hash);
        }
        QTimer::singleShot(300, this, [this]() { loadLocalLicense(); });
    });

    // General log
    connect(m_worker, &LicenseWorker::errorOccurred,
            this, [this](const QString& msg) {
        m_logContent += timestamp() + " [系统] " + msg + "\n";
    });

    // Auto machine hash
    QTimer::singleShot(500, this, [this]() {
        QMetaObject::invokeMethod(m_worker, "doRefreshMachineHash",
            Qt::QueuedConnection,
            Q_ARG(QString, m_serverEdit->text().trimmed()));
    });

    updateStatusCard();

    // ---- 键盘快捷键 ----
    auto* activateShortcut = new QShortcut(QKeySequence(Qt::Key_Return), this);
    connect(activateShortcut, &QShortcut::activated, this, &MainWindow::onActivate);
    auto* activateShortcut2 = new QShortcut(QKeySequence(Qt::Key_Enter), this);
    connect(activateShortcut2, &QShortcut::activated, this, &MainWindow::onActivate);

    auto* logShortcut = new QShortcut(QKeySequence("Ctrl+L"), this);
    connect(logShortcut, &QShortcut::activated, this, &MainWindow::onViewLog);

    auto* diagShortcut = new QShortcut(QKeySequence("Ctrl+D"), this);
    connect(diagShortcut, &QShortcut::activated, this, &MainWindow::onCopyDiagnostic);
}

MainWindow::~MainWindow() {
}

void MainWindow::fadeIn() {
    setWindowOpacity(0.0);
    QPropertyAnimation* anim = new QPropertyAnimation(this, "windowOpacity");
    anim->setDuration(300);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setEasingCurve(QEasingCurve::InOutQuad);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

// =====================================================================
// UI Construction
// =====================================================================

void MainWindow::buildUi() {
    auto* central = new QWidget(this);
    setCentralWidget(central);

    auto* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(24, 16, 24, 14);
    mainLayout->setSpacing(12);

    // ---- 顶部标题区 ----
    auto* titleLayout = new QHBoxLayout();
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(12);

    auto* titleTextLayout = new QVBoxLayout();
    titleTextLayout->setContentsMargins(0, 0, 0, 0);
    titleTextLayout->setSpacing(2);

    m_titleLabel = new QLabel("iK客户端", this);
    m_titleLabel->setObjectName("titleLabel");
    m_titleLabel->setFont(QFont("Segoe UI", 20, QFont::DemiBold));
    titleTextLayout->addWidget(m_titleLabel);

    auto* subtitleLabel = new QLabel("许可证激活与本机授权", this);
    subtitleLabel->setObjectName("titleSubtitle");
    subtitleLabel->setFont(QFont("Segoe UI", 12, QFont::Normal));
    titleTextLayout->addWidget(subtitleLabel);

    titleLayout->addLayout(titleTextLayout);
    titleLayout->addStretch();

    m_themeButton = new QPushButton("", this);
    m_themeButton->setObjectName("themeToggle");
    m_themeButton->setFixedSize(32, 30);
    m_themeButton->setIconSize(QSize(18, 18));
    m_themeButton->setCursor(Qt::PointingHandCursor);
    m_themeButton->setToolTip("切换亮色/暗色主题");
    connect(m_themeButton, &QPushButton::clicked, this, &MainWindow::toggleTheme);
    titleLayout->addWidget(m_themeButton);

    mainLayout->addLayout(titleLayout);

    // ---- 主工作区 ----
    auto* workspaceLayout = new QHBoxLayout();
    workspaceLayout->setSpacing(12);

    auto* leftColumn = new QVBoxLayout();
    leftColumn->setContentsMargins(0, 0, 0, 0);
    leftColumn->setSpacing(8);

    auto* rightColumn = new QVBoxLayout();
    rightColumn->setContentsMargins(0, 0, 0, 0);
    rightColumn->setSpacing(8);

    // === 左侧：激活配置 ===
    auto* leftPanel = new QGroupBox(this);
    leftPanel->setTitle("激活");
    leftPanel->setObjectName("configPanel");
    leftPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    auto* configLayout = new QVBoxLayout(leftPanel);
    configLayout->setSpacing(8);
    configLayout->setContentsMargins(16, 12, 16, 13);

    auto* serverLabelRow = new QHBoxLayout();
    serverLabelRow->setSpacing(8);
    auto* m_serverLbl = new QLabel("服务器地址", leftPanel);
    m_serverLbl->setObjectName("detailLabel");
    serverLabelRow->addWidget(m_serverLbl);
    serverLabelRow->addStretch();

    m_testConnectionBtn = new QPushButton("测试连接", leftPanel);
    m_testConnectionBtn->setObjectName("testConnectionBtn");
    m_testConnectionBtn->setCursor(Qt::PointingHandCursor);
    m_testConnectionBtn->setFixedSize(76, 24);
    connect(m_testConnectionBtn, &QPushButton::clicked, this, &MainWindow::onTestConnection);
    serverLabelRow->addWidget(m_testConnectionBtn);

    m_serverEdit = new QLineEdit(leftPanel);
    m_serverEdit->setText("http://121.40.34.68:8080/");
    m_serverEdit->setPlaceholderText("https://license.example.com");
    m_serverEdit->setMinimumHeight(32);

    configLayout->addLayout(serverLabelRow);
    configLayout->addWidget(m_serverEdit);
    configLayout->addSpacing(4);

    auto* keyLabelRow = new QHBoxLayout();
    keyLabelRow->setSpacing(8);
    auto* m_keyLbl = new QLabel("许可证密钥", leftPanel);
    m_keyLbl->setObjectName("detailLabel");
    keyLabelRow->addWidget(m_keyLbl);
    keyLabelRow->addStretch();
    configLayout->addLayout(keyLabelRow);

    auto* keyInputRow = new QHBoxLayout();
    keyInputRow->setSpacing(8);
    m_keyEdit = new QLineEdit(leftPanel);
    m_keyEdit->setPlaceholderText("XXXX-XXXX-XXXX-XXXX");
    m_keyEdit->setEchoMode(QLineEdit::Password);
    m_keyEdit->setMinimumHeight(32);
    keyInputRow->addWidget(m_keyEdit, 1);

    m_keyEyeBtn = new QPushButton("", leftPanel);
    m_keyEyeBtn->setObjectName("keyEyeBtn");
    m_keyEyeBtn->setFixedSize(32, 32);
    m_keyEyeBtn->setIconSize(QSize(18, 18));
    m_keyEyeBtn->setCursor(Qt::PointingHandCursor);
    m_keyEyeBtn->setToolTip("显示/隐藏密钥");
    connect(m_keyEyeBtn, &QPushButton::clicked, this, &MainWindow::onToggleKeyVisibility);
    keyInputRow->addWidget(m_keyEyeBtn);

    configLayout->addLayout(keyInputRow);

    configLayout->addSpacing(6);

    m_activateButton = new QPushButton("激活", leftPanel);
    m_activateButton->setObjectName("activateBtn");
    m_activateButton->setFixedSize(112, 32);
    m_activateButton->setCursor(Qt::PointingHandCursor);
    connect(m_activateButton, &QPushButton::clicked, this, &MainWindow::onActivate);
    configLayout->addWidget(m_activateButton, 0, Qt::AlignRight);

    leftPanel->setMinimumWidth(410);
    leftColumn->addWidget(leftPanel);

    // === 左侧：授权操作 ===
    auto* actionPanel = new QGroupBox(this);
    actionPanel->setTitle("操作");
    actionPanel->setObjectName("actionPanel");
    actionPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    auto* actionLayout = new QVBoxLayout(actionPanel);
    actionLayout->setSpacing(0);
    actionLayout->setContentsMargins(16, 12, 16, 12);

    m_checkLicenseBtn = new QPushButton("检查许可证", actionPanel);
    m_deactivateBtn = new QPushButton("解除授权", actionPanel);
    m_deactivateBtn->setObjectName("deactivateBtn");

    m_checkLicenseBtn->setFixedSize(112, 30);
    m_deactivateBtn->setFixedSize(88, 30);

    connect(m_checkLicenseBtn, &QPushButton::clicked, this, &MainWindow::onCheckLicense);
    connect(m_deactivateBtn, &QPushButton::clicked, this, &MainWindow::onDeactivate);

    auto* actionButtonRow = new QHBoxLayout();
    actionButtonRow->setContentsMargins(0, 0, 0, 0);
    actionButtonRow->setSpacing(8);
    actionButtonRow->addWidget(m_checkLicenseBtn);
    actionButtonRow->addWidget(m_deactivateBtn);
    actionButtonRow->addStretch();
    actionLayout->addLayout(actionButtonRow);
    leftColumn->addWidget(actionPanel);
    leftColumn->addStretch();

    // === 右侧：状态摘要 ===
    m_statusCardWidget = new QWidget(this);
    m_statusCardWidget->setObjectName("statusCardWidget");
    m_statusCardWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    auto* statusLayout = new QVBoxLayout(m_statusCardWidget);
    statusLayout->setContentsMargins(16, 12, 16, 12);
    statusLayout->setSpacing(8);

    auto* statusHeader = new QHBoxLayout();
    statusHeader->setSpacing(12);

    m_statusIcon = new QLabel(m_statusCardWidget);
    m_statusIcon->setObjectName("statusIconInactive");
    m_statusIcon->setFixedSize(34, 34);
    m_statusIcon->setAlignment(Qt::AlignCenter);
    m_statusIcon->setFont(QFont("Segoe UI Symbol", 24, QFont::Normal));
    statusHeader->addWidget(m_statusIcon);

    auto* statusTextLayout = new QVBoxLayout();
    statusTextLayout->setContentsMargins(0, 0, 0, 0);
    statusTextLayout->setSpacing(2);

    m_statusTitle = new QLabel(m_statusCardWidget);
    m_statusTitle->setObjectName("statusTitle");
    m_statusTitle->setFont(QFont("Segoe UI", 18, QFont::DemiBold));
    statusTextLayout->addWidget(m_statusTitle);

    m_statusSubtitle = new QLabel(m_statusCardWidget);
    m_statusSubtitle->setObjectName("statusSubtitle");
    m_statusSubtitle->setFont(QFont("Segoe UI", 13, QFont::Normal));
    m_statusSubtitle->setWordWrap(true);
    statusTextLayout->addWidget(m_statusSubtitle);

    statusHeader->addLayout(statusTextLayout, 1);
    statusLayout->addLayout(statusHeader);

    m_expiryProgress = new QProgressBar(m_statusCardWidget);
    m_expiryProgress->setRange(0, 100);
    m_expiryProgress->setValue(0);
    m_expiryProgress->setTextVisible(false);
    m_expiryProgress->setMaximumHeight(4);
    statusLayout->addWidget(m_expiryProgress);

    rightColumn->addWidget(m_statusCardWidget);

    // === 右侧：许可证详情 ===
    auto* rightPanel = new QGroupBox(this);
    rightPanel->setTitle("许可证详情");
    rightPanel->setObjectName("detailPanel");
    rightPanel->setMinimumWidth(360);
    rightPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    auto* detailLayout = new QVBoxLayout(rightPanel);
    detailLayout->setSpacing(7);
    detailLayout->setContentsMargins(16, 12, 16, 14);

    auto addDetailRow = [&](const QString& labelText, QLabel* valueLabel) {
        auto* row = new QHBoxLayout();
        row->setSpacing(10);
        auto* lbl = new QLabel(labelText, rightPanel);
        lbl->setObjectName("detailLabel");
        lbl->setFixedWidth(72);
        lbl->setMinimumHeight(22);
        valueLabel->setMinimumHeight(22);
        valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row->addWidget(lbl);
        row->addWidget(valueLabel, 1);
        detailLayout->addLayout(row);
    };

    m_licTypeValue = makeDetailValue("--");
    addDetailRow("授权类型", m_licTypeValue);

    m_expiryValue = makeDetailValue("--");
    m_expiryValue->setFont(QFont("Segoe UI", 15, QFont::DemiBold));
    addDetailRow("到期时间", m_expiryValue);

    m_capacityValue = makeDetailValue("--");
    addDetailRow("设备名额", m_capacityValue);

    m_lastOnlineValue = makeDetailValue("--");
    addDetailRow("最后在线", m_lastOnlineValue);

    m_sessionIdValue = makeDetailValue("--");
    m_sessionIdValue->setFont(QFont("Consolas", 11, QFont::Normal));
    addDetailRow("会话 ID", m_sessionIdValue);

    m_fingerprintValue = new FingerprintLabel("--", rightPanel);
    m_fingerprintValue->setFont(QFont("Consolas", 11, QFont::Normal));
    addDetailRow("设备指纹", static_cast<QLabel*>(m_fingerprintValue));

    detailLayout->addSpacing(8);

    m_copyDiagBtn = new QPushButton("复制诊断信息", rightPanel);
    m_copyDiagBtn->setObjectName("copyDiagBtn");
    m_copyDiagBtn->setCursor(Qt::PointingHandCursor);
    m_copyDiagBtn->setFixedSize(126, 30);
    connect(m_copyDiagBtn, &QPushButton::clicked, this, &MainWindow::onCopyDiagnostic);
    detailLayout->addWidget(m_copyDiagBtn, 0, Qt::AlignRight);

    rightColumn->addWidget(rightPanel);
    rightColumn->addStretch();

    workspaceLayout->addLayout(leftColumn, 5);
    workspaceLayout->addLayout(rightColumn, 4);
    mainLayout->addLayout(workspaceLayout, 1);

    // ---- 底部工具入口 ----
    auto* bottomBar = new QWidget(this);
    bottomBar->setStyleSheet("background-color: transparent; border-top: 1px solid transparent;");
    auto* bottomLayout = new QHBoxLayout(bottomBar);
    bottomLayout->setContentsMargins(0, 8, 0, 0);
    bottomLayout->setSpacing(18);

    m_logLink = new QLabel("<a href='log'>查看详细日志</a>", this);
    m_logLink->setObjectName("bottomLink");
    m_logLink->setCursor(Qt::PointingHandCursor);
    m_logLink->setFont(QFont("Segoe UI", 12, QFont::Normal));
    m_logLink->setTextFormat(Qt::RichText);
    connect(m_logLink, &QLabel::linkActivated, this, &MainWindow::onViewLog);

    m_copyDiagLink = new QLabel("<a href='copy'>复制诊断信息</a>", this);
    m_copyDiagLink->setObjectName("bottomLink");
    m_copyDiagLink->setCursor(Qt::PointingHandCursor);
    m_copyDiagLink->setFont(QFont("Segoe UI", 12, QFont::Normal));
    m_copyDiagLink->setTextFormat(Qt::RichText);
    connect(m_copyDiagLink, &QLabel::linkActivated, this, &MainWindow::onCopyDiagnostic);

    bottomLayout->addStretch();
    bottomLayout->addWidget(m_logLink);
    bottomLayout->addWidget(m_copyDiagLink);

    mainLayout->addWidget(bottomBar);

    // 初始禁用依赖本地许可证的操作
    m_checkLicenseBtn->setEnabled(false);
    m_deactivateBtn->setEnabled(false);
}

// =====================================================================
// Theme management
// =====================================================================

void MainWindow::applyDarkTheme() {
    setStyleSheet(DARK_THEME);
    m_darkTheme = true;
    ToastNotification::setDarkMode(true);
    m_themeButton->setToolTip("切换为亮色主题");
    m_titleLabel->setStyleSheet("color: #e5e7eb;");
    m_logLink->setText("<a href='log' style='color: #cbd5e1; text-decoration: none;'>查看详细日志</a>");
    m_copyDiagLink->setText("<a href='copy' style='color: #cbd5e1; text-decoration: none;'>复制诊断信息</a>");
    refreshIconButtons();
    refreshDetailColors();
    updateStatusCard();
}

void MainWindow::applyLightTheme() {
    setStyleSheet(LIGHT_THEME);
    m_darkTheme = false;
    ToastNotification::setDarkMode(false);
    m_themeButton->setToolTip("切换为暗色主题");
    m_titleLabel->setStyleSheet("color: #202124;");
    m_logLink->setText("<a href='log' style='color: #475569; text-decoration: none;'>查看详细日志</a>");
    m_copyDiagLink->setText("<a href='copy' style='color: #475569; text-decoration: none;'>复制诊断信息</a>");
    refreshIconButtons();
    refreshDetailColors();
    updateStatusCard();
}

void MainWindow::toggleTheme() {
    if (m_darkTheme) applyLightTheme();
    else applyDarkTheme();
}

void MainWindow::refreshDetailColors() {
    QString fgColor = m_darkTheme ? "#e5e7eb" : "#202124";
    QString subColor = m_darkTheme ? "#9ca3af" : "#6b7280";

    m_licTypeValue->setStyleSheet("color: " + fgColor + "; font-size: 13px;");
    m_expiryValue->setStyleSheet("color: " + fgColor + "; font-size: 15px; font-weight: 600;");
    m_capacityValue->setStyleSheet("color: " + fgColor + "; font-size: 13px;");
    m_lastOnlineValue->setStyleSheet("color: " + fgColor + "; font-size: 13px;");
    m_sessionIdValue->setStyleSheet("color: " + fgColor + "; font-size: 11px; font-family: Consolas, 'Courier New', monospace;");
    if (m_fingerprintValue) {
        m_fingerprintValue->setStyleSheet("color: " + subColor + "; font-size: 11px; font-family: Cascadia Code, 'Consolas', 'Courier New', monospace;");
    }
}

void MainWindow::refreshIconButtons() {
    const QColor primaryIcon = m_darkTheme ? QColor("#cbd5e1") : QColor("#475569");
    const QColor secondaryIcon = m_darkTheme ? QColor("#cbd5e1") : QColor("#64748b");

    if (m_themeButton) {
        m_themeButton->setText({});
        m_themeButton->setIcon(m_darkTheme ? makeSunIcon(primaryIcon) : makeMoonIcon(primaryIcon));
        m_themeButton->setIconSize(QSize(18, 18));
    }

    if (m_keyEyeBtn && m_keyEdit) {
        const bool keyVisible = m_keyEdit->echoMode() == QLineEdit::Normal;
        m_keyEyeBtn->setText({});
        m_keyEyeBtn->setIcon(makeEyeIcon(secondaryIcon, keyVisible));
        m_keyEyeBtn->setIconSize(QSize(18, 18));
        m_keyEyeBtn->setToolTip(keyVisible ? "隐藏密钥" : "显示密钥");
    }
}

// =====================================================================
// Slot implementations
// =====================================================================

void MainWindow::disableAllButtons(bool disabled) {
    m_activateButton->setEnabled(!disabled);
    if (!disabled) {
        updateActivationButton();
    }
}

void MainWindow::onActivate() {
    QString server = m_serverEdit->text().trimmed();
    QString keyCode = m_keyEdit->text().trimmed();

    if (keyCode.isEmpty()) {
        ToastNotification::showWarning(this, "请输入许可证密钥");
        return;
    }
    if (server.isEmpty()) {
        ToastNotification::showWarning(this, "请输入服务器地址");
        return;
    }
    if (!server.startsWith("http://") && !server.startsWith("https://")) {
        ToastNotification::showWarning(this, "服务器地址格式不正确");
        return;
    }
    if (m_currentMachineHash.isEmpty()) {
        ToastNotification::showInfo(this, "正在计算设备指纹，请稍候…");
        return;
    }

    setLicenseState(LicenseState::Activating);
    m_activateButton->setText("激活中…");
    m_activateButton->setEnabled(false);
    m_checkLicenseBtn->setEnabled(false);
    m_deactivateBtn->setEnabled(false);

    m_logContent += timestamp() + " 正在激活…\n";
    m_logContent += timestamp() + " 服务器: " + server + "\n";
    m_logContent += timestamp() + " 密钥: " + maskKeyCode(keyCode) + "\n";

    QMetaObject::invokeMethod(m_worker, "doActivate",
        Qt::QueuedConnection,
        Q_ARG(QString, server),
        Q_ARG(QString, keyCode),
        Q_ARG(QString, m_currentMachineHash));
}

void MainWindow::onTestConnection() {
    QString server = m_serverEdit->text().trimmed();
    if (server.isEmpty()) {
        ToastNotification::showWarning(this, "请输入服务器地址");
        return;
    }
    if (!server.startsWith("http://") && !server.startsWith("https://")) {
        ToastNotification::showWarning(this, "服务器地址格式不正确");
        return;
    }

    m_testConnectionBtn->setText("测试中…");
    m_testConnectionBtn->setEnabled(false);
    m_logContent += timestamp() + " 测试连接: " + server + "\n";

    QMetaObject::invokeMethod(m_worker, "doConnectionTest",
        Qt::QueuedConnection,
        Q_ARG(QString, server));
}

void MainWindow::onCheckLicense() {
    if (m_cachedToken.isEmpty() || m_cachedPublicKey.isEmpty()) {
        ToastNotification::showWarning(this, "请先激活许可证");
        return;
    }

    m_checkLicenseBtn->setText("检查中…");
    m_checkLicenseBtn->setEnabled(false);
    m_logContent += timestamp() + " 检查许可证…\n";

    QMetaObject::invokeMethod(m_worker, "doCheckLicense",
        Qt::QueuedConnection,
        Q_ARG(QString, m_serverEdit->text().trimmed()),
        Q_ARG(QString, m_keyEdit->text().trimmed()),
        Q_ARG(QString, m_cachedToken),
        Q_ARG(QString, m_cachedPublicKey));
}

void MainWindow::onDeactivate() {
    QString keyCode = m_keyEdit->text().trimmed();
    if (keyCode.isEmpty()) {
        ToastNotification::showWarning(this, "请输入许可证密钥");
        return;
    }

    m_deactivateBtn->setText("解除中…");
    m_deactivateBtn->setEnabled(false);
    m_logContent += timestamp() + " 解除授权…\n";

    QMetaObject::invokeMethod(m_worker, "doDeactivate",
        Qt::QueuedConnection,
        Q_ARG(QString, m_serverEdit->text().trimmed()),
        Q_ARG(QString, keyCode));
}

void MainWindow::onCopyDiagnostic() {
    QString diag = assembleDiagnosticInfo();
    QApplication::clipboard()->setText(diag);
    ToastNotification::showSuccess(this, "诊断信息已复制到剪贴板");
}

void MainWindow::onViewLog() {
    LogDialog dlg(this);
    dlg.setLogContent(m_logContent);
    dlg.exec();
}

void MainWindow::onToggleKeyVisibility() {
    if (m_keyEdit->echoMode() == QLineEdit::Password) {
        m_keyEdit->setEchoMode(QLineEdit::Normal);
    } else {
        m_keyEdit->setEchoMode(QLineEdit::Password);
    }
    refreshIconButtons();
}

void MainWindow::onMachineHashReady(const QString& hash) {
    m_currentMachineHash = hash;
    if (m_fingerprintValue) {
        static_cast<FingerprintLabel*>(m_fingerprintValue)->setText(hash);
    }
}

// =====================================================================
// Status card & state management
// =====================================================================

void MainWindow::setLicenseState(LicenseState state) {
    m_licenseState = state;
    updateStatusCard();
}

void MainWindow::updateStatusCard() {
    const QString titleBase = m_darkTheme ? "#e5e7eb" : "#202124";
    const QString subtitleColor = m_darkTheme ? "#9ca3af" : "#6b7280";
    const QString inactiveColor = m_darkTheme ? "#7b8794" : "#8a949e";
    const QString loadingColor = m_darkTheme ? "#93a4b7" : "#52677a";
    const QString activeColor = m_darkTheme ? "#5eead4" : "#0f766e";
    const QString warningColor = m_darkTheme ? "#fbbf24" : "#b7791f";
    const QString errorColor = m_darkTheme ? "#f87171" : "#b42318";

    QString iconColor = inactiveColor;
    QString titleColor = titleBase;
    QString chunkColor = inactiveColor;

    switch (m_licenseState) {
    case LicenseState::Inactive:
        m_statusIcon->setText("○");
        m_statusIcon->setObjectName("statusIconInactive");
        m_statusIcon->setFont(QFont("Segoe UI Symbol", 24, QFont::Normal));
        m_statusTitle->setText("未激活");
        m_statusSubtitle->setText("请输入许可证密钥以激活");
        m_expiryProgress->setValue(0);
        iconColor = inactiveColor;
        titleColor = titleBase;
        chunkColor = inactiveColor;
        break;

    case LicenseState::Activating:
        m_statusIcon->setText("●");
        m_statusIcon->setObjectName("statusIconLoading");
        m_statusIcon->setFont(QFont("Segoe UI Symbol", 24, QFont::Normal));
        m_statusTitle->setText("激活中…");
        m_statusSubtitle->setText("正在验证许可证密钥");
        m_expiryProgress->setValue(0);
        iconColor = loadingColor;
        titleColor = loadingColor;
        chunkColor = loadingColor;
        break;

    case LicenseState::Active:
        m_statusIcon->setText("●");
        m_statusIcon->setObjectName("statusIconActive");
        m_statusIcon->setFont(QFont("Segoe UI Symbol", 24, QFont::Normal));
        m_statusTitle->setText("许可证已激活");
        computeExpirySubtitle();
        m_expiryProgress->setValue(computeProgressPercent());
        iconColor = activeColor;
        titleColor = activeColor;
        chunkColor = activeColor;
        break;

    case LicenseState::ExpiringSoon:
        m_statusIcon->setText("◆");
        m_statusIcon->setObjectName("statusIconWarning");
        m_statusIcon->setFont(QFont("Segoe UI Symbol", 23, QFont::Normal));
        m_statusTitle->setText("许可证即将过期");
        computeExpirySubtitle();
        m_expiryProgress->setValue(computeProgressPercent());
        iconColor = warningColor;
        titleColor = warningColor;
        chunkColor = warningColor;
        break;

    case LicenseState::Expired:
        m_statusIcon->setText("×");
        m_statusIcon->setObjectName("statusIconError");
        m_statusIcon->setFont(QFont("Segoe UI Symbol", 24, QFont::Normal));
        m_statusTitle->setText("许可证已过期");
        computeExpirySubtitle();
        m_expiryProgress->setValue(100);
        iconColor = errorColor;
        titleColor = errorColor;
        chunkColor = errorColor;
        break;

    case LicenseState::Revoked:
        m_statusIcon->setText("×");
        m_statusIcon->setObjectName("statusIconError");
        m_statusIcon->setFont(QFont("Segoe UI Symbol", 24, QFont::Normal));
        m_statusTitle->setText("授权异常");
        m_statusSubtitle->setText("许可证可能已被吊销，请联系管理员");
        m_expiryProgress->setValue(0);
        iconColor = errorColor;
        titleColor = errorColor;
        chunkColor = errorColor;
        break;
    }

    m_statusIcon->setStyleSheet("color: " + iconColor + ";");
    m_statusTitle->setStyleSheet("font-size: 18px; font-weight: 600; color: " + titleColor + ";");
    m_statusSubtitle->setStyleSheet("color: " + subtitleColor + "; font-size: 13px;");
    m_expiryProgress->setStyleSheet(QString("QProgressBar::chunk { background-color: %1; border-radius: 4px; }").arg(chunkColor));

    updateLicenseDetails();
    updateActivationButton();
}

void MainWindow::updateActivationButton() {
    switch (m_licenseState) {
    case LicenseState::Inactive:
        m_activateButton->setVisible(true);
        m_activateButton->setText("激活");
        m_activateButton->setEnabled(true);
        m_checkLicenseBtn->setEnabled(false);
        m_deactivateBtn->setEnabled(false);
        break;

    case LicenseState::Activating:
        m_activateButton->setVisible(true);
        m_activateButton->setText("激活中…");
        m_activateButton->setEnabled(false);
        m_checkLicenseBtn->setEnabled(false);
        m_deactivateBtn->setEnabled(false);
        break;

    case LicenseState::Active:
        m_activateButton->setVisible(true);
        m_activateButton->setText("更新许可证");
        m_activateButton->setEnabled(true);
        m_checkLicenseBtn->setEnabled(true);
        m_deactivateBtn->setEnabled(true);
        break;

    case LicenseState::ExpiringSoon:
        m_activateButton->setVisible(true);
        m_activateButton->setText("重新激活");
        m_activateButton->setEnabled(true);
        m_checkLicenseBtn->setEnabled(true);
        m_deactivateBtn->setEnabled(true);
        break;

    case LicenseState::Expired:
        m_activateButton->setVisible(true);
        m_activateButton->setText("重新激活");
        m_activateButton->setEnabled(true);
        m_checkLicenseBtn->setEnabled(true);
        m_deactivateBtn->setEnabled(true);
        break;

    case LicenseState::Revoked:
        m_activateButton->setVisible(true);
        m_activateButton->setText("重新激活");
        m_activateButton->setEnabled(true);
        m_checkLicenseBtn->setEnabled(true);
        m_deactivateBtn->setEnabled(true);
        break;
    }
}

void MainWindow::updateLicenseDetails() {
    if (m_licenseState == LicenseState::Inactive || m_licenseState == LicenseState::Activating) {
        m_licTypeValue->setText("--");
        m_expiryValue->setText("--");
        m_capacityValue->setText("--");
        m_lastOnlineValue->setText("--");
        m_sessionIdValue->setText("--");
        return;
    }

    if (!m_cachedExpiresAt.isEmpty()) {
        QDateTime expDate = QDateTime::fromString(m_cachedExpiresAt, "yyyy-MM-dd HH:mm:ss");
        if (expDate.isValid()) {
            qint64 years = expDate.secsTo(QDateTime::currentDateTime()) / (365LL * 24 * 3600);
            m_licTypeValue->setText(years < 0 ? "永久" : "订阅");
        } else {
            m_licTypeValue->setText("--");
        }
    } else {
        m_licTypeValue->setText("--");
    }

    m_expiryValue->setText(m_cachedExpiresAt.isEmpty() ? "--" : m_cachedExpiresAt);
    m_capacityValue->setText(m_cachedCapacity > 0 ? QString::number(m_cachedCapacity) : "--");
    m_lastOnlineValue->setText(m_lastOnlineTime.isEmpty() ? "--" : m_lastOnlineTime);
    m_sessionIdValue->setText(m_cachedSessionId.isEmpty() ? "--" : m_cachedSessionId);
}

void MainWindow::computeExpirySubtitle() {
    if (m_cachedExpiresAt.isEmpty()) {
        m_statusSubtitle->setText("");
        return;
    }
    QDateTime expDate = QDateTime::fromString(m_cachedExpiresAt, "yyyy-MM-dd HH:mm:ss");
    if (!expDate.isValid()) {
        m_statusSubtitle->setText("");
        return;
    }
    QDateTime now = QDateTime::currentDateTime();
    int daysLeft = now.daysTo(expDate);
    QString dateStr = expDate.toString("yyyy-MM-dd");

    if (daysLeft > 0) {
        m_statusSubtitle->setText(QString("剩余 %1 天，有效期至 %2").arg(daysLeft).arg(dateStr));
    } else if (daysLeft == 0) {
        m_statusSubtitle->setText(QString("今天到期，有效期至 %1").arg(dateStr));
    } else {
        m_statusSubtitle->setText(QString("已过期 %1 天，有效期至 %2").arg(-daysLeft).arg(dateStr));
    }
}

int MainWindow::computeProgressPercent() {
    if (m_cachedIssuedAt.isEmpty() || m_cachedExpiresAt.isEmpty()) return 0;
    QDateTime iat = QDateTime::fromString(m_cachedIssuedAt, "yyyy-MM-dd HH:mm:ss");
    QDateTime exp = QDateTime::fromString(m_cachedExpiresAt, "yyyy-MM-dd HH:mm:ss");
    if (!iat.isValid() || !exp.isValid()) return 0;
    qint64 total = iat.secsTo(exp);
    if (total <= 0) return 100;
    qint64 elapsed = iat.secsTo(QDateTime::currentDateTime());
    int pct = qBound(0, (int)((elapsed * 100) / total), 100);
    return pct;
}

// =====================================================================
// Helpers
// =====================================================================

QString MainWindow::assembleDiagnosticInfo() {
    QString info;
    info += "=== iK客户端 诊断信息 ===\n";
    info += "版本: v2.0.0\n";
    info += "服务器: " + m_serverEdit->text().trimmed() + "\n";
    info += "许可证密钥: " + maskKeyCode(m_keyEdit->text().trimmed()) + "\n";
    info += "激活状态: " + stateToString(m_licenseState) + "\n";
    info += "设备指纹: " + m_currentMachineHash + "\n";
    info += "会话ID: " + m_cachedSessionId + "\n";
    info += "到期时间: " + m_cachedExpiresAt + "\n";
    info += "授权类型: " + m_licTypeValue->text() + "\n";
    info += "设备名额: " + m_capacityValue->text() + "\n";
    info += "最后在线: " + m_lastOnlineValue->text() + "\n";
    info += "\n--- 日志 ---\n" + m_logContent;
    return info;
}

QString MainWindow::maskKeyCode(const QString& key) {
    if (key.length() <= 4) return key;
    return key.left(4) + "****" + key.right(4);
}

QString MainWindow::stateToString(LicenseState state) {
    switch (state) {
    case LicenseState::Inactive:    return "未激活";
    case LicenseState::Activating:  return "激活中";
    case LicenseState::Active:      return "已激活";
    case LicenseState::ExpiringSoon:return "即将过期";
    case LicenseState::Expired:     return "已过期";
    case LicenseState::Revoked:     return "授权异常";
    }
    return "未知";
}

QString MainWindow::mapErrorCode(const QString& code, const QString& msg) {
    if (code.isEmpty()) return msg.isEmpty() ? "操作失败" : msg;
    if (code == "KEY_EMPTY") return "请输入许可证密钥";
    if (code == "KEY_FORMAT_INVALID") return "许可证密钥格式不正确";
    if (code == "KEY_INVALID") return "许可证密钥无效，请确认后重试";
    if (code == "SERVER_UNREACHABLE") return "无法连接许可证服务器，请检查服务器地址或网络";
    if (code == "MACHINE_MISMATCH") return "当前设备未被授权，请联系管理员";
    if (code == "LICENSE_EXPIRED") return "许可证已过期，请续期后重新激活";
    if (code == "TIME_ANOMALY") return "本机时间异常，请校准系统时间";
    if (code == "LICENSE_REVOKED") return "当前许可证已被吊销，请联系管理员";
    if (code == "PRODUCT_MISMATCH") return "许可证与当前客户端不匹配";
    if (code == "SIGNATURE_VERIFICATION_FAILED") return "许可证校验失败，请重新激活";
    if (code == "SERVER_ERROR") return "许可证服务器异常，请稍后重试";
    return msg.isEmpty() ? "操作失败" : msg;
}

// =====================================================================
// 本地持久化
// =====================================================================

void MainWindow::loadLocalLicense() {
    if (!LicenseStorage::exists()) return;

    LicenseRecord rec = LicenseStorage::load();
    if (!rec.valid) return;

    m_cachedToken       = rec.token;
    m_cachedPublicKey   = rec.publicKeyPem;
    m_cachedExpiresAt   = rec.expiresAt;
    m_cachedIssuedAt    = rec.issuedAt;
    m_cachedSessionId   = rec.sessionId;
    m_cachedCapacity    = rec.capacity;
    m_lastOnlineTime    = rec.issuedAt;

    if (!rec.serverUrl.isEmpty()) m_serverEdit->setText(rec.serverUrl);
    if (!rec.keyCode.isEmpty())   m_keyEdit->setText(rec.keyCode);

    if (rec.isExpired()) {
        setLicenseState(LicenseState::Expired);
        m_logContent += timestamp() + " 本地许可证已过期。\n";
    } else {
        int days = rec.daysLeft();
        if (days <= 7 && days > 0)
            setLicenseState(LicenseState::ExpiringSoon);
        else
            setLicenseState(LicenseState::Active);

        m_logContent += timestamp() + QString(" 已从本地恢复许可证（剩余 %1 天）。\n").arg(days);
    }

    updateStatusCard();
}

void MainWindow::saveLocalLicense() {
    LicenseRecord rec;
    rec.serverUrl    = m_serverEdit->text().trimmed();
    rec.keyCode      = m_keyEdit->text().trimmed();
    rec.token        = m_cachedToken;
    rec.publicKeyPem = m_cachedPublicKey;
    rec.expiresAt    = m_cachedExpiresAt;
    rec.issuedAt     = m_cachedIssuedAt;
    rec.sessionId    = m_cachedSessionId;
    rec.machineHash  = m_currentMachineHash;
    rec.capacity     = m_cachedCapacity;

    if (LicenseStorage::save(rec)) {
        m_logContent += timestamp() + " 许可证已保存到本地。\n";
    }
}

void MainWindow::clearLocalLicense() {
    LicenseStorage::clear();
    m_logContent += timestamp() + " 本地许可证已清除。\n";
}

QString MainWindow::timestamp() {
    return "[" + QDateTime::currentDateTime().toString("hh:mm:ss") + "] ";
}

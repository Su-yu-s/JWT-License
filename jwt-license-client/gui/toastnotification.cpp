#include "toastnotification.h"
#include <QApplication>
#include <QScreen>
#include <QHBoxLayout>
#include <QFont>
#include <QPalette>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>

bool ToastNotification::s_darkMode = false;

// =====================================================================
// 构造
// =====================================================================

ToastNotification::ToastNotification(QWidget* parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_TranslucentBackground);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(20, 14, 22, 14);
    layout->setSpacing(0);

    m_label = new QLabel(this);
    m_label->setFont(QFont("Microsoft YaHei", 13, QFont::Normal));
    m_label->setWordWrap(false);
    m_label->setStyleSheet("background: transparent;");
    layout->addWidget(m_label);

    // 柔和阴影
    m_shadow = new QGraphicsDropShadowEffect(this);
    m_shadow->setBlurRadius(30);
    m_shadow->setOffset(0, 8);
    m_shadow->setColor(QColor(0, 0, 0, 50));
    setGraphicsEffect(m_shadow);

    // 入场：淡入
    m_animFadeIn = new QPropertyAnimation(this, "windowOpacity");
    m_animFadeIn->setDuration(300);
    m_animFadeIn->setStartValue(0.0);
    m_animFadeIn->setEndValue(1.0);
    m_animFadeIn->setEasingCurve(QEasingCurve::OutCubic);

    // 入场：下滑（通过 move 实现）
    m_animSlideIn = new QPropertyAnimation(this, "pos");
    m_animSlideIn->setDuration(350);
    m_animSlideIn->setEasingCurve(QEasingCurve::OutBack);

    // 出场：淡出
    m_animFadeOut = new QPropertyAnimation(this, "windowOpacity");
    m_animFadeOut->setDuration(200);
    m_animFadeOut->setStartValue(1.0);
    m_animFadeOut->setEndValue(0.0);
    m_animFadeOut->setEasingCurve(QEasingCurve::InCubic);
    connect(m_animFadeOut, &QPropertyAnimation::finished, this, &QWidget::close);

    // 自动消失
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &ToastNotification::dismiss);
}

// =====================================================================
// 配置外观
// =====================================================================

void ToastNotification::configureAppearance(Type type, bool darkMode) {
    QColor bg, text, accent, shadowC;

    if (darkMode) {
        switch (type) {
        case Type::Success:
            accent  = QColor("#22c55e");
            bg      = QColor(22, 28, 22, 245);
            text    = QColor("#dcfce7");
            shadowC = QColor(34, 197, 94, 50);
            break;
        case Type::Warning:
            accent  = QColor("#eab308");
            bg      = QColor(30, 26, 18, 245);
            text    = QColor("#fef9c3");
            shadowC = QColor(234, 179, 8, 50);
            break;
        case Type::Error:
            accent  = QColor("#ef4444");
            bg      = QColor(30, 18, 18, 245);
            text    = QColor("#fee2e2");
            shadowC = QColor(239, 68, 68, 50);
            break;
        case Type::Info:
            accent  = QColor("#3b82f6");
            bg      = QColor(18, 24, 35, 245);
            text    = QColor("#dbeafe");
            shadowC = QColor(59, 130, 246, 50);
            break;
        }
    } else {
        switch (type) {
        case Type::Success:
            accent  = QColor("#16a34a");
            bg      = QColor(240, 253, 244, 252);
            text    = QColor("#166534");
            shadowC = QColor(34, 197, 94, 35);
            break;
        case Type::Warning:
            accent  = QColor("#ca8a04");
            bg      = QColor(255, 252, 235, 252);
            text    = QColor("#854d0e");
            shadowC = QColor(234, 179, 8, 35);
            break;
        case Type::Error:
            accent  = QColor("#dc2626");
            bg      = QColor(255, 242, 242, 252);
            text    = QColor("#991b1b");
            shadowC = QColor(239, 68, 68, 35);
            break;
        case Type::Info:
            accent  = QColor("#2563eb");
            bg      = QColor(239, 246, 255, 252);
            text    = QColor("#1e40af");
            shadowC = QColor(59, 130, 246, 35);
            break;
        }
    }

    m_accentColor = accent;

    // 背景（QPalette + paintEvent 绘制）
    QPalette pal = palette();
    pal.setColor(QPalette::Window, bg);
    setAutoFillBackground(true);
    setPalette(pal);

    // 文字
    m_label->setStyleSheet(QString(
        "color: %1; background: transparent; font-size: 13px; padding-left: 12px;"
    ).arg(text.name()));

    // 阴影
    if (m_shadow) {
        m_shadow->setColor(shadowC);
    }
}

// =====================================================================
// paintEvent：圆角矩形 + 左侧色条
// =====================================================================

void ToastNotification::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF r = rect().adjusted(2, 2, -2, -2);
    qreal radius = 12;

    // 背景
    {
        QPainterPath path;
        path.addRoundedRect(r, radius, radius);
        p.fillPath(path, palette().color(QPalette::Window));
    }

    // 左侧色条（圆角矩形左侧裁剪）
    {
        QPainterPath bar;
        qreal barW = 3.5;
        QRectF barRect(r.left(), r.top() + 10, barW, r.height() - 20);
        bar.addRoundedRect(barRect, 2, 2);
        p.fillPath(bar, m_accentColor);
    }
}

// =====================================================================
// 动画
// =====================================================================

void ToastNotification::animateIn() {
    m_animFadeIn->start(QAbstractAnimation::DeleteWhenStopped);
    m_animSlideIn->start(QAbstractAnimation::DeleteWhenStopped);
}

void ToastNotification::animateOut() {
    m_animFadeOut->start(QAbstractAnimation::DeleteWhenStopped);
}

void ToastNotification::dismiss() {
    m_timer->stop();
    animateOut();
}

// =====================================================================
// 静态 show：居中显示
// =====================================================================

void ToastNotification::show(QWidget* parent, const QString& message, Type type, int durationMs) {
    auto* toast = new ToastNotification(parent);
    toast->configureAppearance(type, s_darkMode);
    toast->m_label->setText(message);

    // 尺寸适配
    toast->adjustSize();
    int w = qMin(toast->width() + 24, 560);
    toast->setFixedWidth(w);

    // 计算居中位置
    QRect ref;  // 参考矩形（父窗口或屏幕）
    if (parent) {
        ref = parent->geometry();
    } else {
        ref = QApplication::primaryScreen()->geometry();
    }

    int targetX = ref.center().x() - w / 2;
    int targetY = ref.top() + 48;   // 距顶部 48px

    // 起始位置（上方 30px，用于下滑动画）
    int startY = targetY - 30;

    toast->move(targetX, startY);
    toast->m_targetY = targetY;

    // 设置下滑动画的起止位置
    toast->m_animSlideIn->setStartValue(QPoint(targetX, startY));
    toast->m_animSlideIn->setEndValue(QPoint(targetX, targetY));

    toast->QWidget::show();
    toast->raise();
    toast->activateWindow();

    toast->animateIn();
    toast->m_timer->start(durationMs);
}

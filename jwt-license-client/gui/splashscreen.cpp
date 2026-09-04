#include "splashscreen.h"

#include <QApplication>
#include <QScreen>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>

// =====================================================================
// 构造 — 圆角方形 iK 开屏
// =====================================================================
SplashScreen::SplashScreen()
    : QSplashScreen()
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::SplashScreen | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);

    // 大方块圆角（不遮罩，靠 QPainterPath 裁剪）
    setFixedSize(380, 380);

    if (auto* screen = QGuiApplication::primaryScreen()) {
        QRect sg = screen->availableGeometry();
        move(sg.center().x() - width() / 2,
             sg.center().y() - height() / 2 + 10);
    }
}

void SplashScreen::showEvent(QShowEvent* event)
{
    QSplashScreen::showEvent(event);
    repaint();
    QApplication::processEvents();
    QTimer::singleShot(40, this, &SplashScreen::animateIn);
}

// =====================================================================
// 入场 — 圆角方形从中心展开 + Logo 淡入放大，共 ~700ms
// =====================================================================
void SplashScreen::animateIn()
{
    // 圆角方形从中心展开 0 → 380
    auto* revealAnim = new QPropertyAnimation(this, "revealRadius");
    revealAnim->setDuration(650);
    revealAnim->setStartValue(0.0);
    revealAnim->setEndValue(380.0);
    revealAnim->setEasingCurve(QEasingCurve::OutQuad);

    // Logo 从 0.5 → 1.0 放大（矢量渲染）
    auto* scaleAnim = new QPropertyAnimation(this, "logoScale");
    scaleAnim->setDuration(550);
    scaleAnim->setStartValue(0.5);
    scaleAnim->setEndValue(1.0);
    scaleAnim->setEasingCurve(QEasingCurve::OutBack);

    // 淡入
    auto* fadeAnim = new QPropertyAnimation(this, "opacity");
    fadeAnim->setDuration(200);
    fadeAnim->setStartValue(0.0);
    fadeAnim->setEndValue(1.0);
    fadeAnim->setEasingCurve(QEasingCurve::OutCubic);

    auto* all = new QParallelAnimationGroup;
    all->addAnimation(revealAnim);
    all->addAnimation(scaleAnim);
    all->addAnimation(fadeAnim);
    all->start(QAbstractAnimation::DeleteWhenStopped);
}

// =====================================================================
// 出场 — 背景向外展开 + 淡出 (300ms)
// Logo 保持 1x 矢量渲染，不缩放、不模糊
// =====================================================================
void SplashScreen::animateOut()
{
    // 背景容器向外展开（Logo 不受影响）
    auto* expandAnim = new QPropertyAnimation(this, "expandScale");
    expandAnim->setDuration(300);
    expandAnim->setStartValue(1.0);
    expandAnim->setEndValue(2.5);
    expandAnim->setEasingCurve(QEasingCurve::OutQuad);

    // 淡出
    auto* fadeOut = new QPropertyAnimation(this, "opacity");
    fadeOut->setDuration(300);
    fadeOut->setStartValue(1.0);
    fadeOut->setEndValue(0.0);
    fadeOut->setEasingCurve(QEasingCurve::InCubic);

    auto* out = new QParallelAnimationGroup;
    out->addAnimation(expandAnim);
    out->addAnimation(fadeOut);
    connect(out, &QParallelAnimationGroup::finished, this, [this]() { close(); });
    out->start(QAbstractAnimation::DeleteWhenStopped);
}

void SplashScreen::finish() { animateOut(); }

// =====================================================================
// Q_PROPERTY 存取器
// =====================================================================
void   SplashScreen::setOpacity(qreal v)      { m_opacity = v; update(); }
qreal  SplashScreen::opacity() const          { return m_opacity; }
void   SplashScreen::setLogoScale(qreal v)    { m_logoScale = v; update(); }
qreal  SplashScreen::logoScale() const        { return m_logoScale; }
void   SplashScreen::setRevealRadius(qreal v) { m_revealRadius = v; update(); }
qreal  SplashScreen::revealRadius() const     { return m_revealRadius; }
void   SplashScreen::setExpandScale(qreal v)  { m_expandScale = v; update(); }
qreal  SplashScreen::expandScale() const      { return m_expandScale; }

// =====================================================================
// 绘制
// =====================================================================
void SplashScreen::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setOpacity(m_opacity);

    // 始终裁剪为圆角方形（32px 圆角）
    QPainterPath clip;
    clip.addRoundedRect(rect(), 32, 32);
    painter.setClipPath(clip);

    // 入场圆角方形展开裁剪（从中心向外放大）
    if (m_revealRadius > 1.0) {
        qreal s = m_revealRadius;
        QRectF revealRect(rect().center().x() - s/2,
                          rect().center().y() - s/2, s, s);
        QPainterPath revealClip;
        revealClip.addRoundedRect(revealRect, 28, 28);
        painter.setClipPath(revealClip);
    }

    // ---- 背景（出场时受 expandScale 控制向外展开）----
    painter.save();
    if (m_expandScale > 1.01) {
        painter.translate(rect().center());
        painter.scale(m_expandScale, m_expandScale);
        painter.translate(-rect().center());
    }
    painter.fillRect(rect(), cBgLight);
    painter.restore();

    // ---- Logo（不受 expandScale 影响，1x 矢量渲染保持清晰）----
    drawLogo(painter);
}

void SplashScreen::drawLogo(QPainter& painter)
{
    if (m_logoScale < 0.01) return;

    painter.save();

    qreal cx = width() / 2.0;
    qreal cy = height() / 2.0;
    painter.translate(cx, cy);
    painter.scale(m_logoScale, m_logoScale);

    // 深色圆角底
    QRectF bg(-80, -80, 160, 160);
    painter.setBrush(QColor(11, 15, 23));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(bg, 36, 36);

    // 白色 iK 文字，在图标内精准居中
    painter.setPen(QColor(255, 255, 255));
    QFont font("Segoe UI", 58, QFont::DemiBold);
    font.setHintingPreference(QFont::PreferNoHinting);
    painter.setFont(font);

    painter.drawText(bg, Qt::AlignCenter, QStringLiteral("iK"));

    painter.restore();
}

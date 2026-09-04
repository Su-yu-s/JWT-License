#ifndef SPLASHSCREEN_H
#define SPLASHSCREEN_H

#include <QSplashScreen>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>

// =====================================================================
// SplashScreen — iK 字母标开屏
// 圆角方形 + 深色底白字 iK Logo + 圆角展开入场 + 背景展开过渡
// =====================================================================

class SplashScreen : public QSplashScreen {
    Q_OBJECT

    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)
    Q_PROPERTY(qreal logoScale READ logoScale WRITE setLogoScale)
    Q_PROPERTY(qreal revealRadius READ revealRadius WRITE setRevealRadius)
    Q_PROPERTY(qreal expandScale READ expandScale WRITE setExpandScale)

public:
    explicit SplashScreen();
    void finish();

protected:
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void drawLogo(QPainter& painter);
    void animateIn();
    void animateOut();

    // Q_PROPERTY 存取器
    void   setOpacity(qreal v);
    qreal  opacity() const;
    void   setLogoScale(qreal v);
    qreal  logoScale() const;
    void   setRevealRadius(qreal v);
    qreal  revealRadius() const;
    void   setExpandScale(qreal v);
    qreal  expandScale() const;

    // 颜色
    static constexpr QColor cBgLight = QColor(248, 250, 252);

    // 状态
    qreal  m_opacity      = 0.0;
    qreal  m_logoScale    = 0.0;
    qreal  m_revealRadius = 0.0;
    qreal  m_expandScale  = 1.0;
};

#endif // SPLASHSCREEN_H

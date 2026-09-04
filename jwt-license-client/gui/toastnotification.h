#pragma once

#include <QWidget>
#include <QLabel>
#include <QPropertyAnimation>
#include <QTimer>
#include <QGraphicsDropShadowEffect>

class ToastNotification : public QWidget {
    Q_OBJECT

public:
    enum class Type { Success, Warning, Error, Info };

    explicit ToastNotification(QWidget* parent = nullptr);

    // MainWindow 切换主题时调用
    static void setDarkMode(bool dark) { s_darkMode = dark; }

    static void show(QWidget* parent, const QString& message, Type type = Type::Info, int durationMs = 3000);
    static void showSuccess(QWidget* parent, const QString& message) { show(parent, message, Type::Success); }
    static void showWarning(QWidget* parent, const QString& message) { show(parent, message, Type::Warning); }
    static void showError(QWidget* parent, const QString& message)   { show(parent, message, Type::Error); }
    static void showInfo(QWidget* parent, const QString& message)    { show(parent, message, Type::Info); }

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void dismiss();

private:
    void configureAppearance(Type type, bool darkMode);
    void animateIn();
    void animateOut();

    QLabel* m_label = nullptr;
    QPropertyAnimation* m_animFadeIn = nullptr;
    QPropertyAnimation* m_animSlideIn = nullptr;
    QPropertyAnimation* m_animFadeOut = nullptr;
    QTimer* m_timer = nullptr;
    QGraphicsDropShadowEffect* m_shadow = nullptr;

    QColor m_accentColor;   // 左侧色条颜色
    int    m_targetY = 0;   // 动画目标 Y 坐标

    static bool s_darkMode;
};

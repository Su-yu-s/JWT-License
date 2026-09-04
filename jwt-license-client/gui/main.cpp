#include "splashscreen.h"
#include "mainwindow.h"

#include <QApplication>
#include <QScreen>
#include <QStyleFactory>
#include <QSurfaceFormat>
#include <QFont>
#include <QTimer>
#include <QPainter>
#include <QIcon>
#include <QPixmap>

// =====================================================================
// 生成应用图标（深色圆角底 + 白色 iK）
// =====================================================================
static QIcon makeAppIcon()
{
    int sz = 256;
    QPixmap px(sz, sz);
    px.fill(Qt::transparent);
    QPainter p(&px);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(QColor(11, 15, 23));
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(0, 0, sz, sz, 48, 48);
    p.setPen(QColor(255, 255, 255));
    QFont f("Segoe UI", 90, QFont::DemiBold);
    f.setHintingPreference(QFont::PreferNoHinting);
    p.setFont(f);
    p.drawText(QRect(0, 0, sz, sz), Qt::AlignCenter, "iK");
    p.end();
    return QIcon(px);
}

int main(int argc, char* argv[])
{
    // High DPI support
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));
    app.setApplicationName("iK客户端");
    app.setApplicationVersion("2.0.0");
    app.setOrganizationName("iKClient");

    // Consistent font
    QFont defaultFont("Segoe UI", 13);
    defaultFont.setHintingPreference(QFont::PreferNoHinting);
    app.setFont(defaultFont);

    // 应用图标
    QIcon appIcon = makeAppIcon();
    app.setWindowIcon(appIcon);

    // ================================================================
    // 无缝衔接：开屏径向展开 → 直接过渡为主窗口
    //
    //   0 ms     开屏显示（圆形径向展开 + Logo 弹跳）
    //   2400 ms  开屏向四周展开放大(350ms) + 淡出
    //   2420 ms  主窗口在同一中心淡入(300ms)，重叠过渡
    //   ~2750 ms 开屏关闭，主窗口完全可见
    // ================================================================

    // 1. 创建并显示开屏
    SplashScreen splash;
    splash.show();
    QApplication::processEvents();

    // 2. 创建主窗口（隐藏状态，与开屏同一中心位置）
    MainWindow window;
    window.setWindowOpacity(0.0);
    if (auto* screen = QGuiApplication::primaryScreen()) {
        QRect sg = screen->availableGeometry();
        QSize ws = window.size();
        window.move(sg.center().x() - ws.width() / 2,
                    sg.center().y() - ws.height() / 2 + 10);
    }

    // 3. 开屏展示后，展开过渡到主窗口
    QTimer::singleShot(2400, [&]() {
        // 开屏开始向外展开 + 淡出（350ms）
        splash.finish();

        // 几乎同时显示主窗口（重叠过渡）
        QTimer::singleShot(20, [&]() {
            window.show();
            window.activateWindow();
            window.raise();
            QTimer::singleShot(20, &window, &MainWindow::fadeIn);
        });
    });

    return app.exec();
}

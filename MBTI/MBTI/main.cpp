#include "MBT_tester.h"
#include <QApplication>
#include <QFont>
#include <QFontDatabase>

// 选择应用的字体族：按优先级检测常见中文/系统字体，返回第一个可用的
static QString chooseAppFontFamily()
{
    const QStringList families = QFontDatabase::families();

    if (families.contains("YouYuan")) return "YouYuan";
    if (families.contains("幼圆")) return "幼圆";
    if (families.contains("Microsoft YaHei UI")) return "Microsoft YaHei UI";
    if (families.contains("Microsoft YaHei")) return "Microsoft YaHei";
    if (families.contains("微软雅黑")) return "微软雅黑";
    if (families.contains("SimHei")) return "SimHei";

    // 若未找到偏好字体，则使用系统默认应用字体
    return QApplication::font().family();
}

int main(int argc, char* argv[])
{
    // 程序入口，创建 QApplication 并设置全局字体
    QApplication a(argc, argv);

    QFont appFont(chooseAppFontFamily());
    appFont.setPointSize(11);
    QApplication::setFont(appFont);

    // 创建主窗口并显示
    MBT_tester w;
    w.show();

    // 进入 Qt 事件循环
    return a.exec();
}
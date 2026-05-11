#pragma once
#ifndef MBT_TESTER_H
#define MBT_TESTER_H

#include <QMainWindow>             // QMainWindow：主窗口基类
#include "mbtiengine.h"           // 引入 MBTI 引擎的声明（题库、答案、计算结果）
#include "quizpage.h"             // 问卷页的声明（显示问题、保存答案）
#include "resultpage.h"           // 结果页的声明（显示类型、昵称、描述、维度可视化）

QT_BEGIN_NAMESPACE
namespace Ui { class MBT_tester; }  // 前向声明：uic 生成的 UI 类，避免在头中包含 ui 文件
QT_END_NAMESPACE

// 主应用窗口类：负责页面创建、页面间切换、以及管理 MBTI 引擎实例
class MBT_tester : public QMainWindow
{
    Q_OBJECT

public:
    MBT_tester(QWidget* parent = nullptr); // 构造函数：创建 UI、初始化引擎与页面
    ~MBT_tester();                         // 析构函数：清理 UI 与子对象

private slots:
    void on_btnStart_clicked();  // 槽：欢迎页的“开始”按钮被点击时调用（展示问卷）
    void onQuizFinished();       // 槽：QuizPage 发出 quizFinished 时调用（计算并显示结果）
    void onRestartQuiz();        // 槽：ResultPage 发出 restartQuiz 时调用（重置并回到问卷）

private:
    void showWelcome();   // 切换到欢迎页（stacked widget 的索引控制）
    void showQuiz();      // 切换到问卷页
    void showResult();    // 切换到结果页
    void applyMainStyle(); // 应用主窗口的统一样式（QSS），控制全局外观

private:
    Ui::MBT_tester* ui;   // 指向 uic 生成的 UI 对象，包含界面控件指针
    MBTIEngine* m_engine; // 指向 MBTI 引擎实例（管理题库、答案与结果计算）
    QuizPage* m_quizPage; // 问卷页面实例指针（在构造中创建并加入 stacked widget）
    ResultPage* m_resultPage; // 结果页面实例指针（在构造中创建并加入 stacked widget）
};

#endif // MBT_TESTER_H
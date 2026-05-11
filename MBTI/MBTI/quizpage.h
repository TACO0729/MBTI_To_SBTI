#ifndef QUIZPAGE_H
#define QUIZPAGE_H

#include <QWidget>        // 基类：所有 QWidget 派生控件的基类
#include <QButtonGroup>   // 用于管理单选按钮组，方便获取选中 id

#include "mbtiengine.h"   // 包含 Question / QuizResult / MBTIEngine 的声明，QuizPage 使用引擎读题写答

QT_BEGIN_NAMESPACE
namespace Ui { class QuizPage; } // 前向声明：uic 生成的 UI 类（在 cpp 中通过 ui 指针使用）
QT_END_NAMESPACE

// QuizPage: 问卷页面类，负责显示题目、选项、进度，并与 MBTI 引擎交互保存答案。
// 该类通过信号/槽与主窗口和 ResultPage 协作（发出 quizFinished 信号通知完成）。
class QuizPage : public QWidget
{
    Q_OBJECT

public:
    // 构造函数：接收一个 MBTI 引擎指针（由上层窗口创建并传递），可选父控件
    explicit QuizPage(MBTIEngine* engine, QWidget* parent = nullptr);
    ~QuizPage();

    // 重启问卷：将当前索引重置并加载第一题（由外部调用，例如在开始/重测时）
    void restartQuiz();

signals:
    void quizFinished();      // 当问卷全部回答完毕时发出（上层会显示结果页）
    void goBackToWelcome();   // 可选：返回欢迎页（如果 UI 需要此行为）

private slots:
    // 槽：上一题按钮点击
    void on_btnPrev_clicked();
    // 槽：下一题 / 查看结果 按钮点击
    void on_btnNext_clicked();

private:
    // 加载指定索引的题目并刷新 UI（题号、题干、选项、恢复答案等）
    void loadQuestion(int index);
    // 更新进度条与进度文本（百分比、已回答数量）
    void updateProgress();
    // 更新按钮的可用性与文本（上一题、下一题/查看结果）
    void updateButtonState();
    // 将当前 UI 的选择保存到引擎（写模型）
    void saveCurrentAnswer();
    // 从引擎恢复指定题目的答案到 UI（读模型）
    void restoreAnswer(int index);
    // 清除单选按钮的 UI 选中状态（并屏蔽信号以避免触发保存）
    void clearRadioSelection();

    // 设置页面样式（字体选择、QSS 等）
    void applyQuizStyle();
    // 从系统字体中选择一个“合适”的字体族用于页面显示
    QString chooseNiceFontFamily() const;

    // 帮助方法：判断当前题是否已作答（委托给引擎）
    bool currentQuestionHasAnswer() const;
    // 查找第一个未回答的问题并跳转（用于提示用户完成所有题）
    void goToFirstUnansweredQuestion();

    // 将 Unicode codepoint 转换为 QString，用于在题号中显示 emoji（兼容 Qt5/Qt6）
    QString emoji(uint codepoint) const;

private:
    Ui::QuizPage* ui;       // 指向由 uic 生成的 UI 对象（包含界面上所有控件指针）
    MBTIEngine* m_engine;   // 引擎指针：提供题库、保存/读取答案、计算结果的接口

    QButtonGroup* m_answerGroup; // 管理单选按钮（便于获取选中 id：1/2/3）
    int m_currentIndex;          // 当前题目的索引（0 基准）
};

#endif // QUIZPAGE_H
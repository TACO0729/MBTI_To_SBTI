#pragma once

#include <QtWidgets/QMainWindow>

#include "ui_SBTI.h"
#include "sbtiengine.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QVector>
#include <QProgressBar>
#include <QScrollArea>
#include <QString>
#include <QWidget>

// SBTI 窗口类头文件
// 说明：该类封装了主窗口 UI 与与 SBTI 引擎的交互逻辑。
//       头文件仅声明成员与方法，具体实现位于 SBTI.cpp。
//       注释以中文说明每个部分的职责，便于快速理解接口。

class SBTI : public QMainWindow
{
    Q_OBJECT

public:
    // 构造与析构
    explicit SBTI(QWidget* parent = nullptr);
    ~SBTI();

private:
    // ============================================================
    // 初始化与界面构建
    // ============================================================
    // 从资源 / JSON 初始化引擎（加载数据并准备题库）
    void initEngine();
    // 创建所有 Qt 控件并布局（不包含样式、文本优化等）
    void buildInterface();

    // ============================================================
    // 页面显示与交互逻辑
    // ============================================================
    // 显示当前题目（从引擎读取并填充题干与选项）
    void showCurrentQuestion();
    // 处理用户选择的答案（将值提交到引擎）
    void handleAnswer(int value);
    // 计算并显示结果页面（包括 HTML 渲染）
    void showResult();
    // 重新开始测试（重置引擎与界面状态）
    void restartTest();
    // 回到上一题（撤销引擎最后一次答案）
    void goToPreviousQuestion();

    // ============================================================
    // 第二阶段 / 第三阶段 UI 优化
    // ============================================================
    // 应用全局样式表（QSS）
    void applyAppStyle();
    // 在主界面上方安装自定义 Header（标题 + 副标题）
    void installTopHeader();
    // 对题目页中常见控件做统一美化（objectName、最小高度等）
    void polishQuestionPage();
    // 对结果页中常见控件做统一美化（滚动区、结果标签等）
    void polishResultPage();
    // 自动规范各按钮的显示文本（根据 objectName 或原始文本匹配）
    void updateButtonTexts();

    // ============================================================
    // 第三阶段 A：页面淡入动画
    // ============================================================
    // 对指定 widget 播放淡入动画（用于提升页面切换体验）
    void playFadeIn(QWidget* widget, int duration = 220);
    // 对当前页面（centralWidget 或窗口）播放淡入
    void playPageFadeIn();

    // ============================================================
    // 第三阶段 B：选项点击体验优化
    // ============================================================
    // 优化选项控件的鼠标光标、最小高度、尺寸策略等
    void enhanceOptionInteractions();

    // ============================================================
    // 第三阶段 C：结果复制按钮
    // ============================================================
    // 安装“复制结果”按钮（若尚未创建）
    void installCopyResultButton();
    // 将上次生成的纯文本结果复制到系统剪贴板
    void copyResultToClipboard();
    // 根据当前引擎结果构建用于复制的纯文本（便于粘贴到聊天/笔记）
    QString buildResultPlainText() ;

    // ============================================================
    // 结果 HTML 构建
    // ============================================================
    // 将 SbtiResult 生成富文本 HTML 以在 QLabel 中显示
    QString buildResultHtml(const SbtiResult& result) const;
    // 构建调试信息的 HTML（目前可能为空，可扩展）
    QString buildDebugHtml() const;
    // 将纯文本做 HTML 转义并把换行替换为 <br>
    QString htmlText(const QString& text) const;

private:
    // ============================================================
    // Qt Designer UI
    // ============================================================
    // 由 Qt Designer 生成的 UI 对象（如果使用 .ui 文件）
    Ui::SBTIClass ui;

    // ============================================================
    // 第三阶段 C：复制结果
    // ============================================================
    // 指向复制按钮的指针（懒创建），用于在结果页显示复制功能
    QPushButton* m_copyResultButton = nullptr;
    // 缓存最后一次生成的纯文本结果，便于复制到剪贴板
    QString m_lastResultPlainText;

    // ============================================================
    // 测试引擎
    // ============================================================
    // 引擎实例，负责题目管理、计分与匹配类型逻辑
    SBTIEngine m_engine;

    // ============================================================
    // 主界面容器
    // ============================================================
    // central widget 与主垂直布局（所有主要部分都被加入到这里）
    QWidget* m_centralWidget = nullptr;
    QVBoxLayout* m_mainLayout = nullptr;

    // ============================================================
    // 顶部标题区域
    // ============================================================
    // 标题、副标题与进度显示（文本 + 进度条）
    QLabel* m_titleLabel = nullptr;
    QLabel* m_subtitleLabel = nullptr;
    QLabel* m_progressLabel = nullptr;
    QProgressBar* m_progressBar = nullptr;

    // ============================================================
    // 内容卡片区域
    // ============================================================
    // 卡片风格的容器（题干 + 选项 + 结果滚动区）
    QFrame* m_cardFrame = nullptr;
    QVBoxLayout* m_cardLayout = nullptr;

    // ============================================================
    // 题目区域
    // ============================================================
    // 显示题干的 QLabel（加粗、大字号）
    QLabel* m_questionLabel = nullptr;

    // ============================================================
    // 结果区域
    // ============================================================
    // 使用 QScrollArea 包裹结果内容，便于展示较长的结果文本
    QScrollArea* m_resultScrollArea = nullptr;
    QWidget* m_resultContainer = nullptr;
    QVBoxLayout* m_resultLayout = nullptr;
    QLabel* m_resultLabel = nullptr;

    // ============================================================
    // 选项按钮
    // ============================================================
    // 预创建的一组 QPushButton（通常为 4 个），用于填充每道题的选项
    QVector<QPushButton*> m_optionButtons;

    // ============================================================
    // 底部按钮
    // ============================================================
    // 底部布局及“上一题”“重新开始”按钮
    QHBoxLayout* m_bottomButtonLayout = nullptr;
    QPushButton* m_previousButton = nullptr;
    QPushButton* m_restartButton = nullptr;
};
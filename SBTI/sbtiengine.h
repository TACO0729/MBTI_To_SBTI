#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QMap>
#include <QJsonObject>

#include "sbtimodels.h"

// sbtiengine.h
// 说明：SBTIEngine 是测试的核心引擎类，负责：
//  - 从 JSON 文件加载配置、题目、维度与类型定义
//  - 管理题目队列与答题流程（打乱、插入特殊题、撤销等）
//  - 根据已答题目计算每个维度分数并匹配最接近的类型（包含特殊规则处理）
// 头文件声明了数据结构用于调试输出与引擎对外暴露的 API。

struct SbtiDebugMatch
{
    QString code;
    QString cn;
    QString pattern;

    int distance = 0;   // 与用户模式的总距离（差异累加）
    int exact = 0;      // 完全匹配维度的数量
    int similarity = 0; // 基于 distance / maxDistance 计算的相似度百分比
};

struct SbtiDebugInfo
{
    QMap<QString, int> dimensionScores;     // 每个维度的数值分数
    QMap<QString, QString> dimensionLevels; // 每个维度的等级（L/M/H）

    QString finalPattern;                   // 根据 levels 组成的最终 pattern 字符串

    QVector<SbtiDebugMatch> topMatches;     // 前几名匹配的类型（用于调试展示）

    QString resultMode;                     // 结果模式：normal / drunk / fallback / empty

    bool drunkTriggered = false;            // 是否触发了 drunk 规则
    bool fallbackTriggered = false;         // 是否触发了 fallback 规则
    bool specialTriggered = false;          // 是否触发了任意特殊规则

    QString specialReason;                  // 触发特殊规则的原因说明（便于调试）

    int fallbackThreshold = 0;              // fallback 相似度阈值（来自配置）
    int maxDistance = 0;                    // 匹配计算时的最大可能距离（来自配置）
};

class SBTIEngine : public QObject
{
    Q_OBJECT

public:
    explicit SBTIEngine(QObject* parent = nullptr);

    // 从四个资源文件加载引擎所需数据（config/questions/dimensions/types）
    bool loadFromFiles(const QString& configPath,
        const QString& questionsPath,
        const QString& dimensionsPath,
        const QString& typesPath);

    // 启动或重置引擎，准备题目队列并清空历史答案
    void start();

    // 队列与索引访问器
    int totalQuestions() const;
    int currentIndex() const;

    SbtiQuestion currentQuestion() const;
    SbtiQuestion questionAt(int index) const;

    bool hasNextQuestion() const;
    // 回答当前题（value 通常为选项对应的数值），并推进索引
    void answerCurrentQuestion(int value);

    // 撤销最后一次回答（用于“上一题”）
    bool undoLastAnswer();

    bool isFinished() const;
    bool isDrunk() const;

    // 根据当前已答题结果计算最终类型（主/次/排名等）
    SbtiResult calculateResult();

    // 生成便于调试的详细信息（分数、等级、top matches、特殊规则等）
    SbtiDebugInfo debugInfo() const;

    QVector<SbtiQuestion> questionQueue() const;
    QMap<QString, int> answers() const;

    // 计算维度分数并将分数映射为等级（L/M/H）
    QMap<QString, int> calcDimensionScores() const;
    QMap<QString, QString> scoresToLevels(const QMap<QString, int>& scores) const;

    // 访问维度顺序与定义（用于构建模式与显示）
    QVector<QString> dimOrder() const;
    QMap<QString, SbtiDimensionDef> dimensionDefinitions() const;

    // 显示文本（来源于 config）
    QString displayTitle() const;
    QString displaySubtitle() const;
    QString displayAuthor() const;
    QString normalFunNote() const;
    QString specialFunNote() const;

private:
    // 文件加载的内部实现
    bool loadConfig(const QString& path);
    bool loadQuestions(const QString& path);
    bool loadDimensions(const QString& path);
    bool loadTypes(const QString& path);

    // 读取并解析 JSON 文件为 QJsonObject（若失败可选返回 ok=false）
    QJsonObject readJsonObject(const QString& path, bool* ok = nullptr) const;

    // 题目队列管理：打乱主题、随机插入饮酒门题等
    QVector<SbtiQuestion> shuffledMainQuestions() const;
    void insertDrinkGateQuestionRandomly();

    // 辅助：移除队列中尚未被回答的指定题目（用于特殊题逻辑）
    void removeUnansweredQuestionById(const QString& questionId);
    // 重新计算是否触发 drunk（饮酒）状态
    void recomputeDrunkState();

    // 模式字符串解析（将 "L-M-H-..." 转为逐字符向量）
    QVector<QString> parsePattern(const QString& pattern) const;

    // 对单个类型进行匹配评分（计算 distance / exact / similarity）
    SbtiType matchOneType(const QMap<QString, QString>& userLevels,
        const SbtiType& type) const;

    // 将用户等级序列按维度顺序拼接成最终 pattern（用于显示/调试）
    QString buildFinalPattern(const QMap<QString, QString>& userLevels) const;

    // 将等级字符映射为数值用于距离计算（L->1, M->2, H->3）
    static int levelToNumber(const QString& level);

private:
    // 题库数据：主题与特殊题（special），以及当前队列（可能插入了特殊题）
    QVector<SbtiQuestion> m_mainQuestions;
    QVector<SbtiQuestion> m_specialQuestions;
    QVector<SbtiQuestion> m_queue;

    // 维度顺序与定义（从 dimensions.json 加载）
    QVector<QString> m_dimOrder;
    QMap<QString, SbtiDimensionDef> m_dimDefs;

    // 类型定义：标准类型与特殊类型（如 DRUNK、HHHH）
    QVector<SbtiType> m_standardTypes;
    QVector<SbtiType> m_specialTypes;

    // 已保存的答案（以 question id 为键，对应 answer value）
    QMap<QString, int> m_answers;

    // 当前索引、完成状态与 drunk 标记
    int m_currentIndex = 0;
    bool m_finished = false;
    bool m_isDrunk = false;

    // 将分数转换等级时使用的阈值（来自 config）
    int m_LMax = 3;
    int m_HMin = 5;
    int m_maxDistance = 30;
    int m_fallbackThreshold = 60;

    // 饮酒门相关配置（默认值可被 config 覆盖）
    QString m_drinkGateQuestionId = "drink_gate_q1";
    int m_drinkGateTriggerValue = 3;
    int m_drunkTriggerValue = 2;

    // 显示文本（从 config 的 display 节点读取）
    QString m_title = "SBTI";
    QString m_subtitle = "";
    QString m_author = "";
    QString m_funNote = "";
    QString m_funNoteSpecial = "";
};
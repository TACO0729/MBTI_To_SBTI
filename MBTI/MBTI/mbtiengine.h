#ifndef MBTIENGINE_H
#define MBTIENGINE_H

#include <QObject>    // Qt 对象基类，支持信号/槽等
#include <QVector>    // Qt 容器，用于存储题目和答案列表
#include <QMap>       // 键值映射，用于静态映射（类型昵称/描述）
#include <QString>    // Qt 字符串类型

// 单个题目的数据结构
struct Question {
    QString text;        // 题干文本
    QString optionA;     // 选项 A 文本
    QString optionB;     // 选项 B 文本
    char dimension;      // 所属维度字符：'E','S','T','J'
    bool isA_LeftSide;   // 标记选项 A 是否属于左侧维度（true: A->E/S/T/J，B->I/N/F/P）
};

// 问卷计算结果的模型
struct QuizResult {
    int scoreE_I = 0;    // E vs I 维度得分：正为 E 偏向，负为 I 偏向
    int scoreS_N = 0;    // S vs N 得分
    int scoreT_F = 0;    // T vs F 得分
    int scoreJ_P = 0;    // J vs P 得分

    QString getTypeCode() const;      // 根据各维度符号生成四字母类型码（如 "INTJ"）
    int getScore(char dim) const;     // 根据维度字符返回对应分数
};

// MBTIEngine：核心逻辑类，负责加载题库、保存答案、计算结果等
class MBTIEngine : public QObject
{
    Q_OBJECT

public:
    explicit MBTIEngine(QObject* parent = nullptr); // 构造函数，可接收父对象

    bool loadQuestions(const QString& jsonPath);   // 从 JSON 文件加载题目（资源或外部文件）

    const QVector<Question>& questions() const { return m_questions; } // 访问题目列表
    int totalQuestions() const { return m_questions.size(); }         // 题目总数

    void answerQuestion(int index, int choice);  // 保存答案：1=A, 2=B, 3=Neutral
    int getAnswer(int index) const;              // 获取指定题目的已保存答案

    bool isAnswered(int index) const;            // 指定题目是否已作答
    int answeredCount() const;                   // 已作答题目数量
    bool allAnswered() const;                    // 是否全部作答
    int firstUnansweredIndex() const;            // 返回第一个未答题索引，找不到返回 -1

    QuizResult calculateResult() const;          // 计算并返回 QuizResult（累加维度分数）
    void reset();                                // 重置答案（全部置为未答）

    // 静态工具方法：提供类型映射与维度文本，便于 UI 显示
    static QString getTypeNickname(const QString& typeCode);
    static QString getTypeDescription(const QString& typeCode);
    static QString getDimensionName(char dim);
    static QString getDimensionLeft(char dim);
    static QString getDimensionRight(char dim);

private:
    QVector<Question> m_questions; // 问题集合
    QVector<int> m_answers;        // 与题目一一对应的答案数组（0 表示未答）

    // 静态映射：从类型码到昵称/描述（在 cpp 中定义并初始化）
    static const QMap<QString, QString> s_nicknames;
    static const QMap<QString, QString> s_descriptions;
};

#endif // MBTIENGINE_H
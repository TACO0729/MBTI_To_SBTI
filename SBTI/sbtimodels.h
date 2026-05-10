#pragma once

#include <QString>
#include <QVector>
#include <QMap>

// sbtimodels.h
// 说明：定义 SBTI 引擎与界面之间使用的轻量数据模型结构。
//       所有结构都是 POD 风格，方便序列化/反序列化与传递。

struct SbtiOption
{
    QString label; // 选项显示文本
    int value = 0; // 选项对应的数值（用于计分）
};

struct SbtiQuestion
{
    QString id;                // 题目唯一 ID（用于答案映射与特殊逻辑判断）
    QString dim;               // 该题目对应的维度 id（用于按维度累加分数）
    QString text;              // 题干文本
    QVector<SbtiOption> options;// 该题的选项集合
    bool special = false;      // 是否为特殊题（非主干问题，如饮酒门等）
};

struct SbtiDimensionDef
{
    QString id;                // 维度 id（与 dimOrder 对应）
    QString name;              // 维度中文/显示名称
    QString model;             // 维度模型描述（可用于展示）
    QMap<QString, QString> levels; // 等级标签映射（L/M/H 对应的文本）
};

struct SbtiType
{
    QString code;    // 类型代码（如 ENFP、DRUNK、HHHH 等）
    QString cn;      // 中文名称
    QString pattern; // 模式字符串（例如 "L-M-H-..."）
    QString intro;   // 简短介绍
    QString desc;    // 详细描述

    // 匹配时的计算结果字段（在 matchOneType 后填充）
    int distance = 0;   // 与用户模式的总距离
    int exact = 0;      // 完全一致维度数
    int similarity = 0; // 相似度百分比（基于 distance / maxDistance）
};

struct SbtiResult
{
    SbtiType primary;           // 主类型（最终显示的类型）
    SbtiType secondary;         // 次类型（若有）
    QVector<SbtiType> rankings; // 按匹配优先级排序的所有标准类型

    QString mode;               // 结果模式（normal/drunk/fallback/empty）
    // topMatches 可用于额外调试或快速展示最相近的几种类型
    QVector<SbtiType> topMatches;
    bool hasSecondary = false;  // 是否存在次要类型
};
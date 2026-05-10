#include "sbtiengine.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QRandomGenerator>
#include <QtMath>
#include <algorithm>
#include <random>

// sbtiengine.cpp
// 说明：实现 SBTI 的核心逻辑引擎，包括从 JSON 文件加载配置/题目/维度/类型，
// 管理题目队列与答题流程，计算分数并匹配类型（包括特殊规则：饮酒门、fallback、drunk 等）.

// 构造函数：QObject 父对象链初始化
SBTIEngine::SBTIEngine(QObject* parent)
    : QObject(parent)
{
}

// 从多个文件加载完整数据（配置、题库、维度定义、类型定义）
// 返回 true 当所有加载都成功。
bool SBTIEngine::loadFromFiles(const QString& configPath,
    const QString& questionsPath,
    const QString& dimensionsPath,
    const QString& typesPath)
{
    return loadConfig(configPath)
        && loadQuestions(questionsPath)
        && loadDimensions(dimensionsPath)
        && loadTypes(typesPath);
}

// 读取给定路径的 JSON 文件并解析为 QJsonObject。
// 若解析或打开失败，通过 ok 输出 false 并返回空对象。
QJsonObject SBTIEngine::readJsonObject(const QString& path, bool* ok) const
{
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (ok) {
            *ok = false;
        }
        return QJsonObject();
    }

    const QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        if (ok) {
            *ok = false;
        }
        return QJsonObject();
    }

    if (ok) {
        *ok = true;
    }

    return doc.object();
}

// 加载配置文件（config.json），提取评分阈值、界面显示文本、饮酒门等规则。
// 将配置保存到成员变量供后续使用。
bool SBTIEngine::loadConfig(const QString& path)
{
    bool ok = false;
    const QJsonObject root = readJsonObject(path, &ok);

    if (!ok) {
        return false;
    }

    const QJsonObject scoring = root.value("scoring").toObject();
    const QJsonObject thresholds = scoring.value("levelThresholds").toObject();

    const QJsonArray l = thresholds.value("L").toArray();
    const QJsonArray h = thresholds.value("H").toArray();

    // L 阈值的上限（如果提供）
    if (l.size() >= 2) {
        m_LMax = l.at(1).toInt(3);
    }

    // H 阈值的下限（如果提供）
    if (h.size() >= 1) {
        m_HMin = h.at(0).toInt(5);
    }

    m_maxDistance = scoring.value("maxDistance").toInt(30);
    m_fallbackThreshold = scoring.value("fallbackThreshold").toInt(60);

    // 显示相关文本
    const QJsonObject display = root.value("display").toObject();
    m_title = display.value("title").toString("SBTI");
    m_subtitle = display.value("subtitle").toString("");
    m_author = display.value("author").toString("");
    m_funNote = display.value("funNote").toString("");
    m_funNoteSpecial = display.value("funNoteSpecial").toString("");

    // 饮酒门相关配置：饮酒判定问题 ID、触发值、drunk 触发值
    const QJsonObject drinkGate = root.value("drinkGate").toObject();
    m_drinkGateQuestionId = drinkGate.value("questionId").toString("drink_gate_q1");
    m_drinkGateTriggerValue = drinkGate.value("triggerValue").toInt(3);
    m_drunkTriggerValue = drinkGate.value("drunkTriggerValue").toInt(2);

    return true;
}

// 加载题库（questions.json）
// 解析 main 与 special 两类题目，返回是否包含主题（至少一题）。
bool SBTIEngine::loadQuestions(const QString& path)
{
    bool ok = false;
    const QJsonObject root = readJsonObject(path, &ok);

    if (!ok) {
        return false;
    }

    // 局部 lambda：把 JSON 数组解析成 QVector<SbtiQuestion>
    auto parseQuestionArray = [](const QJsonArray& arr, bool special) -> QVector<SbtiQuestion> {
        QVector<SbtiQuestion> result;

        for (const QJsonValue& value : arr) {
            const QJsonObject obj = value.toObject();

            SbtiQuestion q;
            q.id = obj.value("id").toString();
            q.dim = obj.value("dim").toString();
            q.text = obj.value("text").toString();
            q.special = special;

            const QJsonArray options = obj.value("options").toArray();

            for (const QJsonValue& optValue : options) {
                const QJsonObject optObj = optValue.toObject();

                SbtiOption opt;
                opt.label = optObj.value("label").toString();
                opt.value = optObj.value("value").toInt();

                q.options.append(opt);
            }

            // 只有当 id 与 text 可用时才加入
            if (!q.id.isEmpty() && !q.text.isEmpty()) {
                result.append(q);
            }
        }

        return result;
        };

    m_mainQuestions = parseQuestionArray(root.value("main").toArray(), false);
    m_specialQuestions = parseQuestionArray(root.value("special").toArray(), true);

    return !m_mainQuestions.isEmpty();
}

// 加载维度定义（dimensions.json）
// 读取维度顺序与每个维度的描述、模型与等级标签
bool SBTIEngine::loadDimensions(const QString& path)
{
    bool ok = false;
    const QJsonObject root = readJsonObject(path, &ok);

    if (!ok) {
        return false;
    }

    m_dimOrder.clear();
    m_dimDefs.clear();

    const QJsonArray order = root.value("order").toArray();

    for (const QJsonValue& value : order) {
        const QString dim = value.toString();

        if (!dim.isEmpty()) {
            m_dimOrder.append(dim);
        }
    }

    const QJsonObject definitions = root.value("definitions").toObject();

    for (auto it = definitions.begin(); it != definitions.end(); ++it) {
        const QString id = it.key();
        const QJsonObject obj = it.value().toObject();

        SbtiDimensionDef def;
        def.id = id;
        def.name = obj.value("name").toString();
        def.model = obj.value("model").toString();

        const QJsonObject levels = obj.value("levels").toObject();
        def.levels["L"] = levels.value("L").toString();
        def.levels["M"] = levels.value("M").toString();
        def.levels["H"] = levels.value("H").toString();

        m_dimDefs[id] = def;
    }

    return !m_dimOrder.isEmpty();
}

// 加载类型定义（types.json）
// standard 为常规类型集合，special 为特殊类型（如 DRUNK、HHHH）
bool SBTIEngine::loadTypes(const QString& path)
{
    bool ok = false;
    const QJsonObject root = readJsonObject(path, &ok);

    if (!ok) {
        return false;
    }

    auto parseTypes = [](const QJsonArray& arr) -> QVector<SbtiType> {
        QVector<SbtiType> result;

        for (const QJsonValue& value : arr) {
            const QJsonObject obj = value.toObject();

            SbtiType type;
            type.code = obj.value("code").toString();
            type.cn = obj.value("cn").toString();
            type.pattern = obj.value("pattern").toString();
            type.intro = obj.value("intro").toString();
            type.desc = obj.value("desc").toString();

            if (!type.code.isEmpty()) {
                result.append(type);
            }
        }

        return result;
        };

    m_standardTypes = parseTypes(root.value("standard").toArray());
    m_specialTypes = parseTypes(root.value("special").toArray());

    return !m_standardTypes.isEmpty();
}

// 启动/重置引擎：清理答案、重置索引、打乱主题队列、插入饮酒门问题。
void SBTIEngine::start()
{
    m_answers.clear();
    m_currentIndex = 0;
    m_finished = false;
    m_isDrunk = false;

    // 随机化主题并形成当前队列
    m_queue = shuffledMainQuestions();
    // 随机位置插入饮酒门问题（若存在 special 中）
    insertDrinkGateQuestionRandomly();
}

// 随机打乱主题顺序，使用 std::shuffle 与随机设备
QVector<SbtiQuestion> SBTIEngine::shuffledMainQuestions() const
{
    QVector<SbtiQuestion> result = m_mainQuestions;

    std::random_device rd;
    std::mt19937 g(rd());

    std::shuffle(result.begin(), result.end(), g);

    return result;
}

// 在队列中随机位置插入饮酒门问题 drink_gate_q1（若在 specialQuestions 中找到）
// 目的是让问卷中可能出现引导饮酒检查的题目。
void SBTIEngine::insertDrinkGateQuestionRandomly()
{
    SbtiQuestion drinkQ;
    bool found = false;

    for (const SbtiQuestion& q : m_specialQuestions) {
        if (q.id == m_drinkGateQuestionId) {
            drinkQ = q;
            found = true;
            break;
        }
    }

    if (!found) {
        return;
    }

    if (m_queue.isEmpty()) {
        m_queue.append(drinkQ);
        return;
    }

    const int pos = QRandomGenerator::global()->bounded(m_queue.size() + 1);
    m_queue.insert(pos, drinkQ);
}

// 基本访问器：队列长度
int SBTIEngine::totalQuestions() const
{
    return m_queue.size();
}

int SBTIEngine::currentIndex() const
{
    return m_currentIndex;
}

// 获取当前题（越界返回空 SbtiQuestion）
SbtiQuestion SBTIEngine::currentQuestion() const
{
    if (m_currentIndex < 0 || m_currentIndex >= m_queue.size()) {
        return SbtiQuestion();
    }

    return m_queue[m_currentIndex];
}

// 获取指定索引题
SbtiQuestion SBTIEngine::questionAt(int index) const
{
    if (index < 0 || index >= m_queue.size()) {
        return SbtiQuestion();
    }

    return m_queue[index];
}

// 是否还有下一题
bool SBTIEngine::hasNextQuestion() const
{
    return m_currentIndex + 1 < m_queue.size();
}

// 回答当前题：将答案保存到 m_answers，并处理与饮酒门相关的特殊逻辑，
// 然后移动当前索引到下一题，并设置 finished 状态。
void SBTIEngine::answerCurrentQuestion(int value)
{
    if (m_currentIndex < 0 || m_currentIndex >= m_queue.size()) {
        return;
    }

    const SbtiQuestion q = m_queue[m_currentIndex];

    m_answers[q.id] = value;

    // 饮酒门逻辑：如果回答触发值，则在下一位置插入后续判断题 drink_gate_q2，
    // 否则移除未被回答的 drink_gate_q2（若存在于队列中）。
    if (q.id == m_drinkGateQuestionId) {
        if (value == m_drinkGateTriggerValue) {
            for (const SbtiQuestion& specialQ : m_specialQuestions) {
                if (specialQ.id == "drink_gate_q2") {
                    bool exists = false;

                    for (const SbtiQuestion& queueQ : m_queue) {
                        if (queueQ.id == "drink_gate_q2") {
                            exists = true;
                            break;
                        }
                    }

                    if (!exists) {
                        m_queue.insert(m_currentIndex + 1, specialQ);
                    }

                    break;
                }
            }
        }
        else {
            removeUnansweredQuestionById("drink_gate_q2");
        }
    }

    // 答题后重新计算是否触发 drunk 状态
    recomputeDrunkState();

    m_currentIndex++;

    if (m_currentIndex >= m_queue.size()) {
        m_finished = true;
    }
    else {
        m_finished = false;
    }
}

// 撤销最后一次回答（用于“上一题”功能）
// 不会重新打乱队列或重建题组，只是删除最后一次保存的答案并回退索引。
// 若撤销的是 drink_gate_q1，需要同步处理 drink_gate_q2 的移除。
bool SBTIEngine::undoLastAnswer()
{
    if (m_currentIndex <= 0) {
        return false;
    }

    if (m_currentIndex > m_queue.size()) {
        m_currentIndex = m_queue.size();
    }

    const int answeredIndex = m_currentIndex - 1;

    if (answeredIndex < 0 || answeredIndex >= m_queue.size()) {
        return false;
    }

    const SbtiQuestion q = m_queue[answeredIndex];

    m_answers.remove(q.id);

    m_currentIndex = answeredIndex;
    m_finished = false;

    if (q.id == m_drinkGateQuestionId) {
        removeUnansweredQuestionById("drink_gate_q2");
    }

    recomputeDrunkState();

    return true;
}

// 从队列中移除尚未被回答的指定问题 ID（如果已回答则不移除）。
// 该函数向后遍历队列以安全地删除多个匹配项，并在必要时调整 m_currentIndex。
void SBTIEngine::removeUnansweredQuestionById(const QString& questionId)
{
    if (m_answers.contains(questionId)) {
        return;
    }

    for (int i = m_queue.size() - 1; i >= 0; --i) {
        if (m_queue[i].id == questionId) {
            if (i < m_currentIndex) {
                m_currentIndex--;
            }

            m_queue.removeAt(i);
        }
    }

    if (m_currentIndex < 0) {
        m_currentIndex = 0;
    }

    if (m_currentIndex > m_queue.size()) {
        m_currentIndex = m_queue.size();
    }

    m_finished = m_currentIndex >= m_queue.size();
}

// 根据已回答的问题判断是否触发 drunk 标记（来自 drink_gate_q2 的触发值）
void SBTIEngine::recomputeDrunkState()
{
    m_isDrunk = false;

    if (m_answers.contains("drink_gate_q2")
        && m_answers.value("drink_gate_q2") == m_drunkTriggerValue) {
        m_isDrunk = true;
    }
}

// 只读访问器
bool SBTIEngine::isFinished() const
{
    return m_finished;
}

bool SBTIEngine::isDrunk() const
{
    return m_isDrunk;
}

QVector<SbtiQuestion> SBTIEngine::questionQueue() const
{
    return m_queue;
}

QMap<QString, int> SBTIEngine::answers() const
{
    return m_answers;
}

// 计算每个维度的总分：对主题（m_mainQuestions）进行累加，
// 未回答的问题会被跳过（不参与统计）。
QMap<QString, int> SBTIEngine::calcDimensionScores() const
{
    QMap<QString, int> scores;

    for (const SbtiQuestion& q : m_mainQuestions) {
        if (!m_answers.contains(q.id)) {
            continue;
        }

        scores[q.dim] += m_answers.value(q.id);
    }

    return scores;
}

// 将数值分数转换为等级（L/M/H），使用配置中的阈值 m_LMax / m_HMin。
// 若维度没有分数记录，会在后续确保至少是 M。
QMap<QString, QString> SBTIEngine::scoresToLevels(const QMap<QString, int>& scores) const
{
    QMap<QString, QString> levels;

    for (auto it = scores.begin(); it != scores.end(); ++it) {
        const QString dim = it.key();
        const int score = it.value();

        if (score <= m_LMax) {
            levels[dim] = "L";
        }
        else if (score >= m_HMin) {
            levels[dim] = "H";
        }
        else {
            levels[dim] = "M";
        }
    }

    // 确保每个定义的维度都有一个 level（缺省为 M）
    for (const QString& dim : m_dimOrder) {
        if (!levels.contains(dim)) {
            levels[dim] = "M";
        }
    }

    return levels;
}

// 将等级字符串转换为数值（L->1, M->2, H->3），用于计算距离/相似度
int SBTIEngine::levelToNumber(const QString& level)
{
    if (level == "L") {
        return 1;
    }

    if (level == "H") {
        return 3;
    }

    return 2;
}

// 将类型模式字符串（例如 "L-M-H-..."）解析为每个维度的字符序列（去掉 '-'）
QVector<QString> SBTIEngine::parsePattern(const QString& pattern) const
{
    QString clean = pattern;
    clean.remove("-");

    QVector<QString> result;

    for (const QChar& ch : clean) {
        result.append(QString(ch));
    }

    return result;
}

// 根据用户每个维度的等级构建最终模式字符串（按 m_dimOrder 的顺序拼接，带 '-' ）
// 该模式可用于展示或调试。
QString SBTIEngine::buildFinalPattern(const QMap<QString, QString>& userLevels) const
{
    QStringList parts;

    for (const QString& dim : m_dimOrder) {
        parts.append(userLevels.value(dim, "M"));
    }

    return parts.join("-");
}

// 对单个类型进行匹配评分：计算与用户 levels 的距离（L/M/H 映射到数字）
// distance：等级差值累加，exact：完全一致项数量，similarity：基于 maxDistance 的百分比相似度。
SbtiType SBTIEngine::matchOneType(const QMap<QString, QString>& userLevels,
    const SbtiType& type) const
{
    SbtiType result = type;

    const QVector<QString> typeLevels = parsePattern(type.pattern);

    int distance = 0;
    int exact = 0;

    for (int i = 0; i < m_dimOrder.size(); ++i) {
        const QString dim = m_dimOrder[i];

        const QString userLevel = userLevels.value(dim, "M");
        const QString typeLevel = i < typeLevels.size() ? typeLevels[i] : "M";

        const int userVal = levelToNumber(userLevel);
        const int typeVal = levelToNumber(typeLevel);

        const int diff = qAbs(userVal - typeVal);

        distance += diff;

        if (diff == 0) {
            exact++;
        }
    }

    const int similarity = qMax(
        0,
        qRound((1.0 - static_cast<double>(distance) / static_cast<double>(m_maxDistance)) * 100.0)
    );

    result.distance = distance;
    result.exact = exact;
    result.similarity = similarity;

    return result;
}

// 计算最终结果：
// 1. 统计每个维度分数 -> 转等级
// 2. 对所有标准类型计算匹配度并排序（优先 distance 小、exact 大、similarity 大）
// 3. 处理特殊规则：若 isDrunk 则返回 DRUNK；若相似度低于 fallbackThreshold 则返回 HHHH；否则返回排名第一、并提供第二名作为次要类型。
SbtiResult SBTIEngine::calculateResult()
{
    SbtiResult result;

    const QMap<QString, int> scores = calcDimensionScores();
    const QMap<QString, QString> levels = scoresToLevels(scores);

    QVector<SbtiType> rankings;

    for (const SbtiType& type : m_standardTypes) {
        rankings.append(matchOneType(levels, type));
    }

    std::sort(rankings.begin(), rankings.end(), [](const SbtiType& a, const SbtiType& b) {
        if (a.distance != b.distance) {
            return a.distance < b.distance;
        }

        if (a.exact != b.exact) {
            return a.exact > b.exact;
        }

        return a.similarity > b.similarity;
        });

    result.rankings = rankings;

    if (rankings.isEmpty()) {
        result.mode = "empty";
        return result;
    }

    const SbtiType best = rankings.first();

    SbtiType drunkType;
    SbtiType fallbackType;

    bool hasDrunk = false;
    bool hasFallback = false;

    // 在 special types 中寻找 DRUNK 与 HHHH（fallback）定义
    for (const SbtiType& type : m_specialTypes) {
        if (type.code == "DRUNK") {
            drunkType = type;
            hasDrunk = true;
        }

        if (type.code == "HHHH") {
            fallbackType = type;
            hasFallback = true;
        }
    }

    // 优先处理 drunk（若用户触发）
    if (m_isDrunk && hasDrunk) {
        drunkType.similarity = best.similarity;
        drunkType.exact = best.exact;
        drunkType.distance = best.distance;

        result.primary = drunkType;
        result.secondary = best;
        result.hasSecondary = true;
        result.mode = "drunk";

        return result;
    }

    // fallback：当最佳相似度低于阈值时，使用 HHHH 替代主结果
    if (best.similarity < m_fallbackThreshold && hasFallback) {
        fallbackType.similarity = best.similarity;
        fallbackType.exact = best.exact;
        fallbackType.distance = best.distance;

        result.primary = fallbackType;
        result.secondary = best;
        result.hasSecondary = true;
        result.mode = "fallback";

        return result;
    }

    // 正常情况：第一名为主，第二名为次（若存在）
    result.primary = best;

    if (rankings.size() >= 2) {
        result.secondary = rankings[1];
        result.hasSecondary = true;
    }

    result.mode = "normal";

    return result;
}

// 生成调试信息：包含分数、等级、最终 pattern、前 N 个匹配与触发的特殊规则解释
SbtiDebugInfo SBTIEngine::debugInfo() const
{
    SbtiDebugInfo debug;

    debug.fallbackThreshold = m_fallbackThreshold;
    debug.maxDistance = m_maxDistance;

    QMap<QString, int> scores = calcDimensionScores();
    for (const QString& dim : m_dimOrder) {
        if (!scores.contains(dim)) {
            scores[dim] = 0;
        }
    }

    const QMap<QString, QString> levels = scoresToLevels(scores);

    debug.dimensionScores = scores;
    debug.dimensionLevels = levels;
    debug.finalPattern = buildFinalPattern(levels);

    QVector<SbtiType> rankings;

    for (const SbtiType& type : m_standardTypes) {
        rankings.append(matchOneType(levels, type));
    }

    std::sort(rankings.begin(), rankings.end(), [](const SbtiType& a, const SbtiType& b) {
        if (a.distance != b.distance) {
            return a.distance < b.distance;
        }

        if (a.exact != b.exact) {
            return a.exact > b.exact;
        }

        return a.similarity > b.similarity;
        });

    const int topCount = qMin(5, rankings.size());

    // 收集 top N 用于调试展示
    for (int i = 0; i < topCount; ++i) {
        const SbtiType& type = rankings[i];

        SbtiDebugMatch match;
        match.code = type.code;
        match.cn = type.cn;
        match.pattern = type.pattern;
        match.distance = type.distance;
        match.exact = type.exact;
        match.similarity = type.similarity;

        debug.topMatches.append(match);
    }

    bool hasDrunk = false;
    bool hasFallback = false;

    for (const SbtiType& type : m_specialTypes) {
        if (type.code == "DRUNK") {
            hasDrunk = true;
        }

        if (type.code == "HHHH") {
            hasFallback = true;
        }
    }

    if (rankings.isEmpty()) {
        debug.resultMode = "empty";
        debug.specialReason = "No standard type rankings.";
        return debug;
    }

    const SbtiType best = rankings.first();

    if (m_isDrunk && hasDrunk) {
        debug.resultMode = "drunk";
        debug.drunkTriggered = true;
        debug.specialTriggered = true;
        debug.specialReason =
            QString("drink_gate_q2 answered with trigger value %1. Primary result is replaced by DRUNK. Original best type is %2, similarity %3%.")
            .arg(m_drunkTriggerValue)
            .arg(best.code)
            .arg(best.similarity);

        return debug;
    }

    if (best.similarity < m_fallbackThreshold && hasFallback) {
        debug.resultMode = "fallback";
        debug.fallbackTriggered = true;
        debug.specialTriggered = true;
        debug.specialReason =
            QString("Best similarity %1% is lower than fallback threshold %2%. Primary result is replaced by HHHH. Original best type is %3.")
            .arg(best.similarity)
            .arg(m_fallbackThreshold)
            .arg(best.code);

        return debug;
    }

    debug.resultMode = "normal";
    debug.specialTriggered = false;
    debug.specialReason =
        QString("No special rule triggered. Best type is %1, similarity %2%.")
        .arg(best.code)
        .arg(best.similarity);

    return debug;
}

// 以下为若干简单的访问器，返回定义顺序、维度定义、显示文本等
QVector<QString> SBTIEngine::dimOrder() const
{
    return m_dimOrder;
}

QMap<QString, SbtiDimensionDef> SBTIEngine::dimensionDefinitions() const
{
    return m_dimDefs;
}

QString SBTIEngine::displayTitle() const
{
    return m_title;
}

QString SBTIEngine::displaySubtitle() const
{
    return m_subtitle;
}

QString SBTIEngine::displayAuthor() const
{
    return m_author;
}

QString SBTIEngine::normalFunNote() const
{
    return m_funNote;
}

QString SBTIEngine::specialFunNote() const
{
    return m_funNoteSpecial;
}
#include "mbtiengine.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

// 静态映射：类型昵称与描述（用于在结果页显示）
const QMap<QString, QString> MBTIEngine::s_nicknames = {
    {"ISTJ", "检查者 (Logistician)"},
    {"ISFJ", "守护者 (Defender)"},
    {"INFJ", "提倡者 (Advocate)"},
    {"INTJ", "建筑师 (Architect)"},
    {"ISTP", "鉴赏家 (Virtuoso)"},
    {"ISFP", "探险家 (Adventurer)"},
    {"INFP", "调停者 (Mediator)"},
    {"INTP", "逻辑学家 (Logician)"},
    {"ESTP", "企业家 (Entrepreneur)"},
    {"ESFP", "表演者 (Entertainer)"},
    {"ENFP", "竞选者 (Campaigner)"},
    {"ENTP", "辩论家 (Debater)"},
    {"ESTJ", "总经理 (Executive)"},
    {"ESFJ", "执政官 (Consul)"},
    {"ENFJ", "主人公 (Protagonist)"},
    {"ENTJ", "指挥官 (Commander)"}
};

const QMap<QString, QString> MBTIEngine::s_descriptions = {
    {"ISTJ", "务实可靠，注重细节和事实，遵守规则和传统，责任心强。喜欢有组织和可预测的环境。"},
    {"ISFJ", "温暖体贴，默默奉献，重视和谐与责任，是值得信赖的伙伴。关注他人的实际需求。"},
    {"INFJ", "理想主义者，富有洞察力，追求意义和深度，渴望帮助他人成长。具有强烈的个人价值观。"},
    {"INTJ", "独立思考，战略眼光，追求知识和效率，善于长远规划。对系统和理论有浓厚兴趣。"},
    {"ISTP", "灵活务实，喜欢动手解决问题，冷静理性，享受当下的体验。善于应对突发状况。"},
    {"ISFP", "敏感艺术，忠于自我价值观，享受当下，追求生活的美感。安静而友善的观察者。"},
    {"INFP", "理想主义，富有同情心，追求内心和谐，重视个人价值和创造力。寻求意义和真实性。"},
    {"INTP", "好奇分析，热爱理论和逻辑，追求知识，喜欢探索可能性。喜欢思考而非行动。"},
    {"ESTP", "活力四射，喜欢冒险和行动，适应力强，善于抓住机遇。活在当下，享受刺激。"},
    {"ESFP", "热情开朗，喜欢成为焦点，享受当下，善于带动气氛。友好、乐观、乐于助人。"},
    {"ENFP", "充满热情，富有创意，善于激励他人，追求自由和可能性。充满好奇心和想象力。"},
    {"ENTP", "机智创新，喜欢辩论和挑战，思维敏捷，善于发现新视角。喜欢智力挑战。"},
    {"ESTJ", "组织能力强，务实高效，重视传统和秩序，天生的管理者。注重结果和执行力。"},
    {"ESFJ", "热心助人，善于合作，重视社会和谐，是优秀的协调者。关注他人的感受和需求。"},
    {"ENFJ", "富有魅力，善于激励和引导他人，追求共同成长和理想。天生的领导者和导师。"},
    {"ENTJ", "果断自信，天生的领导者，追求效率和目标，善于战略规划。喜欢挑战和竞争。"}
};

// 构造函数：目前无需额外初始化
MBTIEngine::MBTIEngine(QObject* parent)
    : QObject(parent)
{
}

// 从 JSON 文件加载题库（文件中应为数组，每项包含 text/optionA/optionB/dimension/isA_LeftSide）
bool MBTIEngine::loadQuestions(const QString& jsonPath)
{
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
        return false;
    }

    QJsonArray arr = doc.array();

    m_questions.clear();
    m_answers.clear();

    for (const auto& val : arr) {
        if (!val.isObject()) {
            continue;
        }

        QJsonObject obj = val.toObject();

        Question q;
        q.text = obj["text"].toString().trimmed();
        q.optionA = obj["optionA"].toString().trimmed();
        q.optionB = obj["optionB"].toString().trimmed();

        QString dimension = obj["dimension"].toString().trimmed();
        if (q.text.isEmpty() || q.optionA.isEmpty() || q.optionB.isEmpty() || dimension.isEmpty()) {
            continue;
        }

        q.dimension = dimension.at(0).toLatin1();

        // 仅接受 E/S/T/J 四个维度字母
        if (q.dimension != 'E' && q.dimension != 'S' && q.dimension != 'T' && q.dimension != 'J') {
            continue;
        }

        // isA_LeftSide 指示选项 A 是否属于维度的左侧（例如 E/S/T/J）
        q.isA_LeftSide = obj["isA_LeftSide"].toBool(true);

        m_questions.append(q);
        m_answers.append(0); // 初始化答案为 0（未答）
    }

    return !m_questions.isEmpty();
}

// 保存对某题的答案（1=A, 2=B, 3=Neutral）
void MBTIEngine::answerQuestion(int index, int choice)
{
    if (index < 0 || index >= m_answers.size()) {
        return;
    }

    if (choice < 0 || choice > 3) {
        return;
    }

    m_answers[index] = choice;
}

int MBTIEngine::getAnswer(int index) const
{
    if (index >= 0 && index < m_answers.size()) {
        return m_answers[index];
    }

    return 0;
}

bool MBTIEngine::isAnswered(int index) const
{
    return index >= 0 && index < m_answers.size() && m_answers[index] != 0;
}

// 统计已回答问题数量
int MBTIEngine::answeredCount() const
{
    int count = 0;

    for (int ans : m_answers) {
        if (ans != 0) {
            ++count;
        }
    }

    return count;
}

// 判断是否全部作答
bool MBTIEngine::allAnswered() const
{
    if (m_questions.isEmpty() || m_answers.size() != m_questions.size()) {
        return false;
    }

    for (int ans : m_answers) {
        if (ans == 0) {
            return false;
        }
    }

    return true;
}

// 返回第一个未回答题目的索引（找不到返回 -1）
int MBTIEngine::firstUnansweredIndex() const
{
    for (int i = 0; i < m_answers.size(); ++i) {
        if (m_answers[i] == 0) {
            return i;
        }
    }

    return -1;
}

// 根据答案计算 QuizResult：遍历题目并根据 isA_LeftSide 与选择决定 +1/-1/0
QuizResult MBTIEngine::calculateResult() const
{
    QuizResult result;

    for (int i = 0; i < m_questions.size() && i < m_answers.size(); ++i) {
        if (m_answers[i] == 0) {
            continue;
        }

        const Question& q = m_questions[i];

        int score = 0;

        if (m_answers[i] == 1) {
            score = q.isA_LeftSide ? 1 : -1;
        }
        else if (m_answers[i] == 2) {
            score = q.isA_LeftSide ? -1 : 1;
        }
        else {
            score = 0;
        }

        // 将分数累加到对应维度
        switch (q.dimension) {
        case 'E':
            result.scoreE_I += score;
            break;
        case 'S':
            result.scoreS_N += score;
            break;
        case 'T':
            result.scoreT_F += score;
            break;
        case 'J':
            result.scoreJ_P += score;
            break;
        default:
            break;
        }
    }

    return result;
}

// 重置答案（全部重置为未答）
void MBTIEngine::reset()
{
    for (int i = 0; i < m_answers.size(); ++i) {
        m_answers[i] = 0;
    }
}

// QuizResult 的辅助方法：生成类型码（例如 "ENTJ"）
QString QuizResult::getTypeCode() const
{
    QString code;

    code += scoreE_I >= 0 ? 'E' : 'I';
    code += scoreS_N >= 0 ? 'S' : 'N';
    code += scoreT_F >= 0 ? 'T' : 'F';
    code += scoreJ_P >= 0 ? 'J' : 'P';

    return code;
}

// 根据维度字母返回对应分数
int QuizResult::getScore(char dim) const
{
    switch (dim) {
    case 'E':
        return scoreE_I;
    case 'S':
        return scoreS_N;
    case 'T':
        return scoreT_F;
    case 'J':
        return scoreJ_P;
    default:
        return 0;
    }
}

// 静态访问：根据类型码查昵称/描述
QString MBTIEngine::getTypeNickname(const QString& typeCode)
{
    return s_nicknames.value(typeCode, "未知类型");
}

QString MBTIEngine::getTypeDescription(const QString& typeCode)
{
    return s_descriptions.value(typeCode, "暂无描述");
}

// 维度名称与左右两侧标签（用于 UI 显示）
QString MBTIEngine::getDimensionName(char dim)
{
    switch (dim) {
    case 'E':
        return "精力来源";
    case 'S':
        return "认知方式";
    case 'T':
        return "决策方式";
    case 'J':
        return "生活方式";
    default:
        return "";
    }
}

QString MBTIEngine::getDimensionLeft(char dim)
{
    switch (dim) {
    case 'E':
        return "外向 (E)";
    case 'S':
        return "实感 (S)";
    case 'T':
        return "思考 (T)";
    case 'J':
        return "判断 (J)";
    default:
        return "";
    }
}

QString MBTIEngine::getDimensionRight(char dim)
{
    switch (dim) {
    case 'E':
        return "内向 (I)";
    case 'S':
        return "直觉 (N)";
    case 'T':
        return "情感 (F)";
    case 'J':
        return "知觉 (P)";
    default:
        return "";
    }
}
#ifndef RESULTPAGE_H
#define RESULTPAGE_H

#include <QWidget>      // QWidget 基类，用于自定义页面控件
#include <QFrame>       // QFrame 用于 DimensionBar 的基类（绘制边框/背景）
#include "mbtiengine.h" // 包含 Question/QuizResult 结构与 MBTI 引擎的声明

QT_BEGIN_NAMESPACE
namespace Ui {
    class ResultPage; // 前向声明：uic 生成的 UI 类（在 cpp 中通过 ui 指针使用）
}
QT_END_NAMESPACE

// DimensionBar：用于在结果页绘制每个维度的可视化进度条（例如 E vs I）
// 这个控件负责自绘制（paintEvent），不持有数据逻辑，仅显示传入的分数信息
class DimensionBar : public QFrame
{
    Q_OBJECT

public:
    // dim: 维度字符 'E'/'S'/'T'/'J'
    // score: 正为左侧（E/S/T/J）倾向，负为右侧（I/N/F/P）倾向
    // maxScore: 用于归一化显示的最大绝对分值
    explicit DimensionBar(char dim, int score, int maxScore, QWidget* parent = nullptr);

protected:
    // 自定义绘制：绘制背景条、填充条和分数字体气泡
    void paintEvent(QPaintEvent* event) override;

private:
    // 返回一个偏好的字体族，用于绘制文字（会在 cpp 中实现字体选择策略）
    QString preferredFontFamily() const;

private:
    char m_dim;   // 维度标识符（'E','S','T','J'）
    int m_score;  // 当前维度分数（正负代表两端）
    int m_maxScore;// 最大可能分数（用于比例计算）
};

// ResultPage：显示测试结果的页面（类型码、昵称、描述、人物图片、维度条等）
class ResultPage : public QWidget
{
    Q_OBJECT

public:
    explicit ResultPage(QWidget* parent = nullptr); // 构造：创建 UI 并设置样式
    ~ResultPage();

    // 使用计算得到的结果填充页面内容（类型码、昵称、描述、维度可视化）
    void setResult(const QuizResult& result);

signals:
    void restartQuiz(); // 用户点击“重测/重启”按钮时发出

private slots:
    void on_btnRestart_clicked(); // 与 UI 中重试按钮绑定的槽

private:
    // 返回推荐字体族（供页面整体样式使用）
    QString preferredFontFamily() const;
    // 根据四字码返回对应主题颜色（用于强调类型显示）
    QString colorForType(const QString& typeCode) const;

private:
    Ui::ResultPage* ui; // 指向由 uic 生成的 UI 对象（包含所有界面控件指针）
};

#endif // RESULTPAGE_H
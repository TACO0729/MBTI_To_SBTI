#include "resultpage.h"
#include "ui_resultpage.h"

#include <QPainter>
#include <QPaintEvent>
#include <QDateTime>
#include <QVBoxLayout>
#include <QLayoutItem>
#include <QPushButton>
#include <QPixmap>
#include <QFontDatabase>
#include <QPalette>
#include <QtMath>

// 辅助：从常见字体中选出一个“好看”的字体族，用于界面显示
static QString chooseNiceFont()
{
    QFontDatabase db;
    QStringList families = db.families();

    if (families.contains("YouYuan")) {
        return "YouYuan";
    }

    if (families.contains("幼圆")) {
        return "幼圆";
    }

    if (families.contains("Microsoft YaHei UI")) {
        return "Microsoft YaHei UI";
    }

    if (families.contains("Microsoft YaHei")) {
        return "Microsoft YaHei";
    }

    if (families.contains("微软雅黑")) {
        return "微软雅黑";
    }

    if (families.contains("SimHei")) {
        return "SimHei";
    }

    if (families.contains("黑体")) {
        return "黑体";
    }

    // 回退到应用默认字体
    return QApplication::font().family();
}

// DimensionBar 构造：表示单个维度的可视化条控件
DimensionBar::DimensionBar(char dim, int score, int maxScore, QWidget* parent)
    : QFrame(parent),
    m_dim(dim),
    m_score(score),
    m_maxScore(maxScore)
{
    setMinimumHeight(58);
    setMaximumHeight(58);
    setFrameStyle(QFrame::NoFrame);

    // 避免被父控件 QSS 边框影响
    setStyleSheet("background: transparent; border: none;");
}

// 返回推荐字体族（委托给静态选择函数）
QString DimensionBar::preferredFontFamily() const
{
    return chooseNiceFont();
}

// DimensionBar 的自定义绘制：绘制背景条、填充条、分数气泡和左右文字
void DimensionBar::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const int w = width();
    const int h = height();

    QColor barColor;

    // 根据维度选择颜色
    switch (m_dim) {
    case 'E':
        barColor = QColor("#7b67c7");
        break;
    case 'S':
        barColor = QColor("#5ca0c4");
        break;
    case 'T':
        barColor = QColor("#e5b13f");
        break;
    case 'J':
        barColor = QColor("#5fa477");
        break;
    default:
        barColor = QColor("#7b67c7");
        break;
    }

    // 获取左右标签文本（来自 MBTI 引擎的静态函数）
    QString leftText = MBTIEngine::getDimensionLeft(m_dim);
    QString rightText = MBTIEngine::getDimensionRight(m_dim);

    QFont textFont(preferredFontFamily());
    textFont.setPointSize(12);
    textFont.setBold(true);
    p.setFont(textFont);
    p.setPen(QColor("#202020"));

    // 布局计算：左右文字区域 + 中间进度条区域
    int leftTextX = 0;
    int leftTextW = 90;
    int rightTextW = 90;

    int barX = leftTextX + leftTextW + 15;
    int barW = w - leftTextW - rightTextW - 35;
    int barH = 16;
    int barY = (h - barH) / 2;

    if (barW < 150) {
        barW = 150;
    }

    // 绘制左右文字、灰色背景条
    p.drawText(leftTextX, 0, leftTextW, h, Qt::AlignVCenter | Qt::AlignRight, leftText);
    p.drawText(barX + barW + 15, 0, rightTextW, h, Qt::AlignVCenter | Qt::AlignLeft, rightText);

    QRectF bgRect(barX, barY, barW, barH);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#e9e9e9"));
    p.drawRoundedRect(bgRect, barH / 2, barH / 2);

    int absScore = qAbs(m_score);

    if (m_maxScore <= 0) {
        m_maxScore = 8;
    }

    double ratio = static_cast<double>(absScore) / static_cast<double>(m_maxScore);
    ratio = qBound(0.0, ratio, 1.0);

    // 计算填充宽度并保证最小可见长度（非 0 分）
    int fillW = static_cast<int>(barW * ratio);

    if (absScore > 0 && fillW < 35) {
        fillW = 35;
    }

    if (fillW > barW) {
        fillW = barW;
    }

    QRectF fillRect;

    // 填充方向：正分向左（从左到右），负分向右（从右到左）
    if (m_score >= 0) {
        fillRect = QRectF(barX, barY, fillW, barH);
    }
    else {
        fillRect = QRectF(barX + barW - fillW, barY, fillW, barH);
    }

    if (absScore > 0) {
        p.setBrush(barColor);
        p.drawRoundedRect(fillRect, barH / 2, barH / 2);
    }

    // 分数气泡：在填充末端或中间显示分数
    QString scoreText = QString::number(absScore);

    QFont scoreFont(preferredFontFamily());
    scoreFont.setPointSize(10);
    scoreFont.setBold(true);
    p.setFont(scoreFont);

    int bubbleW = qMax(38, p.fontMetrics().horizontalAdvance(scoreText) + 18);
    int bubbleH = 24;

    int bubbleCenterX;

    if (m_score >= 0) {
        bubbleCenterX = barX + fillW;
    }
    else {
        bubbleCenterX = barX + barW - fillW;
    }

    // 限制气泡不跑出进度条范围
    bubbleCenterX = qBound(barX + bubbleW / 2, bubbleCenterX, barX + barW - bubbleW / 2);

    QRectF bubbleRect(
        bubbleCenterX - bubbleW / 2,
        barY + barH / 2 - bubbleH / 2,
        bubbleW,
        bubbleH
    );

    // 若分数为 0，则气泡居中显示
    if (absScore == 0) {
        bubbleRect.moveCenter(QPointF(barX + barW / 2, barY + barH / 2));
    }

    p.setBrush(barColor);
    p.setPen(QPen(QColor("#ffffff"), 2));
    p.drawRoundedRect(bubbleRect, bubbleH / 2, bubbleH / 2);

    p.setPen(QColor("#ffffff"));
    p.drawText(bubbleRect, Qt::AlignCenter, scoreText);
}

// ResultPage 构造：设置 UI、字体与样式，并连接重启按钮信号
ResultPage::ResultPage(QWidget* parent)
    : QWidget(parent),
    ui(new Ui::ResultPage)
{
    ui->setupUi(this);

    // 让整个页面都是白色，防止底部按钮两边出现黑色
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::white);
    setPalette(pal);

    // 设置整体字体：优先幼圆，没有就自动用类似字体
    QString fontFamily = preferredFontFamily();
    QFont font(fontFamily);
    setFont(font);

    // 为页面和标签应用全局样式（包含字体族）
    QString globalStyle = QString(
        "QWidget#ResultPage {"
        "background-color: white;"
        "font-family: \"%1\", \"YouYuan\", \"幼圆\", \"Microsoft YaHei\", \"微软雅黑\", \"SimHei\", \"黑体\";"
        "}"
        "QLabel {"
        "font-family: \"%1\", \"YouYuan\", \"幼圆\", \"Microsoft YaHei\", \"微软雅黑\", \"SimHei\", \"黑体\";"
        "}"
    ).arg(fontFamily);

    this->setStyleSheet(this->styleSheet() + globalStyle);

    connect(ui->btnRestart, &QPushButton::clicked,
        this, &ResultPage::on_btnRestart_clicked);
}

ResultPage::~ResultPage()
{
    delete ui;
}

QString ResultPage::preferredFontFamily() const
{
    return chooseNiceFont();
}

// 根据类型码返回对应的主题色（用于视觉强调）
QString ResultPage::colorForType(const QString& typeCode) const
{
    QString t = typeCode.toUpper();

    // NF 组：绿色
    if (t == "ENFJ" || t == "ENFP" ||
        t == "INFJ" || t == "INFP") {
        return "#2e9d57";
    }

    // NT 组：紫色
    if (t == "ENTJ" || t == "ENTP" ||
        t == "INTJ" || t == "INTP") {
        return "#6b5b95";
    }

    // SP 组：黄色
    if (t == "ESFP" || t == "ESTP" ||
        t == "ISFP" || t == "ISTP") {
        return "#e5a927";
    }

    // SJ 组：蓝色
    if (t == "ESFJ" || t == "ESTJ" ||
        t == "ISFJ" || t == "ISTJ") {
        return "#3f7fc4";
    }

    return "#6b5b95";
}

// 填充页面数据：根据 QuizResult 更新 UI（类型、昵称、描述、图片、维度条等）
void ResultPage::setResult(const QuizResult& result)
{
    QString typeCode = result.getTypeCode().toUpper();
    QString fontFamily = preferredFontFamily();

    // 时间（可选显示）
   // ui->labelTime->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));

    // 生成报告编号（时间戳+随机片段）
    QString reportId =
        QDateTime::currentDateTime().toString("yyMMddhhmmss") +
        QString::number(QDateTime::currentMSecsSinceEpoch() % 1000000);

    ui->labelReportId->setText("报告编号：" + reportId);

    // 计算类型颜色并设置类型文本
    QString typeColor = colorForType(typeCode);

    // 最终结果文本（代码后缀“-A”为演示，可按需移除）
    ui->labelTypeCode->setText(typeCode + "-A");

    ui->labelTypeCode->setStyleSheet(QString(
        "QLabel {"
        "color: %1;"
        "font-family: \"%2\", \"YouYuan\", \"幼圆\", \"Microsoft YaHei\", \"微软雅黑\", \"SimHei\", \"黑体\";"
        "font-weight: bold;"
        "}"
    ).arg(typeColor, fontFamily));

    // 昵称与描述来自 MBTI 引擎静态映射
    ui->labelNickname->setText(MBTIEngine::getTypeNickname(typeCode));
    ui->labelDescription->setText(MBTIEngine::getTypeDescription(typeCode));

    ui->labelNickname->setStyleSheet(QString(
        "QLabel {"
        "color: %1;"
        "font-family: \"%2\", \"YouYuan\", \"幼圆\", \"Microsoft YaHei\", \"微软雅黑\", \"SimHei\", \"黑体\";"
        "font-weight: bold;"
        "}"
    ).arg(typeColor, fontFamily));

    ui->labelDescription->setStyleSheet(QString(
        "QLabel {"
        "color: %1;"
        "font-family: \"%2\", \"YouYuan\", \"幼圆\", \"Microsoft YaHei\", \"微软雅黑\", \"SimHei\", \"黑体\";"
        "line-height: 150%;"
        "}"
    ).arg(typeColor, fontFamily));

    // 尝试加载资源图片（路径以类型码的小写拼接）
    QString imagePath = ":/" + typeCode.toLower() + ".png";
    QPixmap pixmap(imagePath);

    if (!pixmap.isNull()) {
        ui->labelImage->setPixmap(
            pixmap.scaled(180, 240, Qt::KeepAspectRatio, Qt::SmoothTransformation)
        );
    }
    else {
        // 图片缺失时显示占位文本
        ui->labelImage->setText("图片缺失");
        ui->labelImage->setStyleSheet(QString(
            "QLabel {"
            "color: #999999;"
            "font-family: \"%1\", \"YouYuan\", \"幼圆\", \"Microsoft YaHei\", \"微软雅黑\";"
            "}"
        ).arg(fontFamily));
    }

    // 清理并重新填充维度可视化条（DimensionBar）
    QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(ui->frameDimensions->layout());

    if (!layout) {
        layout = new QVBoxLayout(ui->frameDimensions);
        ui->frameDimensions->setLayout(layout);
    }

    QLayoutItem* child = nullptr;

    while ((child = layout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            delete child->widget();
        }
        delete child;
    }

    layout->setContentsMargins(5, 15, 5, 15);
    layout->setSpacing(14);

    // maxScore 默认为 8（每维度题目数的最大值）
    int maxScore = 8;

    // 依次添加 4 个维度条
    layout->addWidget(new DimensionBar('E', result.scoreE_I, maxScore, ui->frameDimensions));
    layout->addWidget(new DimensionBar('S', result.scoreS_N, maxScore, ui->frameDimensions));
    layout->addWidget(new DimensionBar('T', result.scoreT_F, maxScore, ui->frameDimensions));
    layout->addWidget(new DimensionBar('J', result.scoreJ_P, maxScore, ui->frameDimensions));
    layout->addStretch();
}

// 重启按钮槽：向外发出 restartQuiz 信号，供上层窗口处理
void ResultPage::on_btnRestart_clicked()
{
    emit restartQuiz();
}
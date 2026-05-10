#include "SBTI.h"

#include <QClipboard>
#include <QDebug>
#include <QMessageBox>
#include <QFont>
#include <QSizePolicy>
#include <QSpacerItem>
#include <QMenuBar>
#include <QStatusBar>
#include <QTimer>
#include <QScrollBar>
#include <QScreen>
#include <QGuiApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QProgressBar>
#include <QGraphicsDropShadowEffect>
#include <QTextBrowser>
#include <QTextEdit>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QRegularExpression>
#include <QEasingCurve>

// 辅助函数：将宽字符常量（wchar_t*）转换为 QString
// 使用时通过 U(L"...") 形式调用，方便写入 Unicode 字符串文字。
static QString U(const wchar_t* text)
{
    return QString::fromWCharArray(text);
}


// 构造函数：初始化 UI 和引擎，设置窗口基础状态
SBTI::SBTI(QWidget* parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    // 隐藏默认菜单和状态栏（程序使用自定义布局）
    if (menuBar()) {
        menuBar()->clear();
        menuBar()->hide();
    }

    if (statusBar()) {
        statusBar()->hide();
    }

    setWindowTitle("SBTI");
    resize(900, 680);

    // 初始化数据引擎（从资源文件加载 json 等）
    initEngine();

    // 构建界面控件树（不包含样式）
    buildInterface();

    // 应用统一样式表（颜色、按钮样式等）
    applyAppStyle();

    // 安装顶部 Header（标题 + 副标题）
    installTopHeader();

    // 扫描并规范按钮文字（无需依赖具体变量名）
    updateButtonTexts();

    // 对题目页与结果页做统一美化（对象名称、尺寸等）
    polishQuestionPage();
    polishResultPage();

    // 优化选项交互体验（鼠标样式、最小高度等）
    enhanceOptionInteractions();

    // 安装“复制结果”按钮（如果不存在）
    installCopyResultButton();

    // 如果有题目则显示当前题
    if (m_engine.totalQuestions() > 0) {
        showCurrentQuestion();
    }
}


// ============================================================
// 第三阶段 A：淡入动画
// 说明：为页面或控件播放一个从透明到不透明的动画，提升体验。
// ============================================================


void SBTI::playFadeIn(QWidget* widget, int duration)
{
    if (!widget) {
        return;
    }

    // 使用 QGraphicsOpacityEffect 控制控件透明度
    QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(widget);
    effect->setOpacity(0.0);
    widget->setGraphicsEffect(effect);

    // 使用 QPropertyAnimation 动画属性 "opacity" 做淡入
    QPropertyAnimation* animation = new QPropertyAnimation(effect, "opacity", widget);
    animation->setDuration(duration);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->setEasingCurve(QEasingCurve::OutCubic);

    // 动画结束时确保透明度正确设置为 1.0
    connect(animation, &QPropertyAnimation::finished, this, [effect]() {
        if (effect) {
            effect->setOpacity(1.0);
        }
        });

    // 自动删除动画对象，避免内存泄漏
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

// 对整页播放淡入，优先作用于 centralWidget（若存在），否则作用于窗口本身
void SBTI::playPageFadeIn()
{
    QWidget* target = nullptr;

    if (centralWidget()) {
        target = centralWidget();
    }
    else {
        target = this;
    }

    playFadeIn(target, 180);
}


// ============================================================
// 第三阶段 B：选项点击体验优化
// 说明：统一设置单选按钮和选项按钮的交互样式与尺寸。
// ============================================================

void SBTI::enhanceOptionInteractions()
{
    // 查找所有 QRadioButton（如果 UI 中存在原生 radio 按钮）
    QList<QRadioButton*> radios = findChildren<QRadioButton*>();

    for (QRadioButton* radio : radios) {
        if (!radio) {
            continue;
        }

        // 鼠标手型、最小高度、确保同组互斥
        radio->setCursor(Qt::PointingHandCursor);
        radio->setMinimumHeight(50);
        radio->setAutoExclusive(true);
    }

    // 对以 QPushButton 实现的选项也做类似处理
    for (QPushButton* button : m_optionButtons) {
        if (!button) {
            continue;
        }

        button->setCursor(Qt::PointingHandCursor);
        button->setMinimumHeight(52);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    }
}

 // ============================================================
 // 第三阶段 C：复制结果按钮
 // 说明：在结果页下添加一个“复制结果到剪贴板”的按钮
 // ============================================================

void SBTI::installCopyResultButton()
{
    // 如果已创建则直接返回（避免重复创建）
    if (m_copyResultButton) {
        return;
    }

    QWidget* root = nullptr;

    if (centralWidget()) {
        root = centralWidget();
    }
    else {
        root = this;
    }

    // 期望根布局为 QVBoxLayout，方便在底部追加按钮
    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(root->layout());

    if (!mainLayout) {
        qDebug() << "[SBTI] installCopyResultButton failed: root layout is not QVBoxLayout";
        return;
    }

    // 创建按钮并设置属性
    m_copyResultButton = new QPushButton(U(L"\u590D\u5236\u7ED3\u679C"), root); // 复制结果
    m_copyResultButton->setObjectName("CopyResultButton");
    m_copyResultButton->setMinimumHeight(44);
    m_copyResultButton->setCursor(Qt::PointingHandCursor);

    // 将按钮添加到主布局（通常在底部）
    mainLayout->addWidget(m_copyResultButton);

    // 点击连接到复制逻辑
    connect(
        m_copyResultButton,
        &QPushButton::clicked,
        this,
        &SBTI::copyResultToClipboard
    );

    // 默认隐藏，只有在有结果时才显示
    m_copyResultButton->hide();
}


// 将最后一次生成的纯文本结果复制到系统剪贴板
void SBTI::copyResultToClipboard()
{
    // 如果没有可复制的纯文本结果，提醒用户
    if (m_lastResultPlainText.trimmed().isEmpty()) {
        QMessageBox::information(
            this,
            U(L"\u63D0\u793A"),                         // 提示
            U(L"\u5F53\u524D\u8FD8\u6CA1\u6709\u53EF\u590D\u5236\u7684\u7ED3\u679C\u3002") // 当前还没有可复制的结果。
        );
        return;
    }

    // 获取系统剪贴板（跨平台通过 QGuiApplication）
    QClipboard* clipboard = QGuiApplication::clipboard();

    if (!clipboard) {
        QMessageBox::warning(
            this,
            U(L"\u63D0\u793A"),                         // 提示
            U(L"\u65E0\u6CD5\u8BBF\u95EE\u526A\u8D34\u677F\u3002") // 无法访问剪贴板。
        );
        return;
    }

    // 写入剪贴板并提示成功
    clipboard->setText(m_lastResultPlainText);

    QMessageBox::information(
        this,
        U(L"\u590D\u5236\u6210\u529F"),                 // 复制成功
        U(L"\u7ED3\u679C\u5DF2\u590D\u5236\u5230\u526A\u8D34\u677F\u3002") // 结果已复制到剪贴板。
    );
}


// 将计算得到的结果组装为纯文本格式（用于复制）
QString SBTI::buildResultPlainText() 
{
    const SbtiResult result = m_engine.calculateResult();

    QString text;

    text += U(L"SBTI \u6D4B\u8BD5\u7ED3\u679C\n");
    text += "====================\n\n";

    text += U(L"\u4E3B\u8981\u7C7B\u578B\uFF1A");
    text += result.primary.code;
    text += " / ";
    text += result.primary.cn;
    text += "\n";

    text += U(L"\u5339\u914D\u5EA6\uFF1A");
    text += QString::number(result.primary.similarity);
    text += "%\n\n";

    if (!result.primary.intro.isEmpty()) {
        text += result.primary.intro;
        text += "\n\n";
    }

    if (!result.primary.desc.isEmpty()) {
        text += result.primary.desc;
        text += "\n\n";
    }

    if (result.hasSecondary) {
        text += U(L"\u6B21\u63A5\u8FD1\u7C7B\u578B\uFF1A");
        text += result.secondary.code;
        text += " / ";
        text += result.secondary.cn;
        text += "\n";

        text += U(L"\u6B21\u63A5\u8FD1\u5339\u914D\u5EA6\uFF1A");
        text += QString::number(result.secondary.similarity);
        text += "%\n";
    }

    return text;
}


// ============================================================
// 按钮文字优化：不依赖具体按钮变量名
// 说明：遍历窗口中的所有 QPushButton，根据名字或文本包含的关键词重写显示文本（支持中英文/大小写）
// ============================================================

void SBTI::updateButtonTexts()
{
    QList<QPushButton*> buttons = findChildren<QPushButton*>();

    for (QPushButton* button : buttons) {
        if (!button) {
            continue;
        }

        const QString name = button->objectName();
        const QString text = button->text();
        const QString key = name + " " + text;

        qDebug() << "[SBTI Button]" << "objectName =" << name << ", text =" << text;

        // 统一最小尺寸，保证视觉一致
        button->setMinimumHeight(44);
        button->setMinimumWidth(120);

        // 根据关键字判断按钮用途，设置中文文本（含箭头）
        if (key.contains("prev", Qt::CaseInsensitive) ||
            key.contains("previous", Qt::CaseInsensitive) ||
            key.contains("back", Qt::CaseInsensitive) ||
            key.contains(U(L"\u4E0A\u4E00"))) {
            button->setText(U(L"\u2190 \u4E0A\u4E00\u9898")); // ← 上一题
        }
        else if (key.contains("next", Qt::CaseInsensitive) ||
            key.contains(U(L"\u4E0B\u4E00"))) {
            button->setText(U(L"\u4E0B\u4E00\u9898 \u2192")); // 下一题 →
        }
        else if (key.contains("restart", Qt::CaseInsensitive) ||
            key.contains("reset", Qt::CaseInsensitive) ||
            key.contains(U(L"\u91CD\u65B0")) ||
            key.contains(U(L"\u5F00\u59CB"))) {
            button->setText(U(L"\u91CD\u65B0\u5F00\u59CB")); // 重新开始
        }
        else if (key.contains("result", Qt::CaseInsensitive) ||
            key.contains("submit", Qt::CaseInsensitive) ||
            key.contains("finish", Qt::CaseInsensitive) ||
            key.contains(U(L"\u67E5\u770B")) ||
            key.contains(U(L"\u7ED3\u679C")) ||
            key.contains(U(L"\u5B8C\u6210"))) {
            button->setText(U(L"\u67E5\u770B\u7ED3\u679C")); // 查看结果
        }
    }
}

 // ============================================================
 // 顶部 Header
 // 说明：创建一个包含标题与副标题的 QFrame，并插入到主布局顶部
 // ============================================================

void SBTI::installTopHeader()
{
    QWidget* root = nullptr;

    if (centralWidget()) {
        root = centralWidget();
    }
    else {
        root = this;
    }

    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(root->layout());

    if (!mainLayout) {
        qDebug() << "[SBTI] installTopHeader failed: root layout is not QVBoxLayout";
        return;
    }

    // 如果已经安装则直接返回（避免重复插入）
    if (root->findChild<QFrame*>("TopHeader")) {
        return;
    }

    // 创建 header frame 与其内部布局
    QFrame* header = new QFrame(root);
    header->setObjectName("TopHeader");

    QVBoxLayout* headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(24, 18, 24, 18);
    headerLayout->setSpacing(6);

    // 标题与副标题（支持中文）
    QLabel* title = new QLabel(U(L"SBTI \u4EBA\u683C\u6D4B\u8BD5"), header); // SBTI 人格测试
    title->setObjectName("HeaderTitle");
    title->setAlignment(Qt::AlignCenter);

    QLabel* subtitle = new QLabel(
        U(L"\u9009\u62E9\u66F4\u7B26\u5408\u4F60\u5F53\u4E0B\u72B6\u6001\u7684\u7B54\u6848"),
        header
    ); // 选择更符合你当下状态的答案
    subtitle->setObjectName("HeaderSubtitle");
    subtitle->setAlignment(Qt::AlignCenter);

    headerLayout->addWidget(title);
    headerLayout->addWidget(subtitle);

    // 插入到主布局的最前面（index 0）
    mainLayout->insertWidget(0, header);
}

 // ============================================================
 // 题目页美化：通用版
 // 说明：为 QLabel、QRadioButton、QProgressBar 等统一设置对象名和常用样式属性
 // ============================================================

void SBTI::polishQuestionPage()
{
    // 为所有标签设置自动换行，并根据对象名/文本设置语义化的 objectName
    QList<QLabel*> labels = findChildren<QLabel*>();

    for (QLabel* label : labels) {
        if (!label) {
            continue;
        }

        const QString name = label->objectName();
        const QString text = label->text();
        const QString key = name + " " + text;

        label->setWordWrap(true);

        // 识别题目文本并设置专用对象名与最小高度
        if (key.contains("question", Qt::CaseInsensitive) ||
            key.contains(U(L"\u9898\u76EE")) ||
            key.contains(U(L"\u95EE\u9898"))) {
            label->setObjectName("QuestionCard");
            label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            label->setMinimumHeight(120);
        }

        // 识别进度文本并设置居中显示
        if (key.contains("progress", Qt::CaseInsensitive) ||
            key.contains(U(L"\u8FDB\u5EA6")) ||
            key.contains("/") ||
            key.contains(U(L"\u7B2C"))) {
            label->setObjectName("ProgressText");
            label->setAlignment(Qt::AlignCenter);
        }
    }

    // 统一处理页面中的所有 QRadioButton（如果 UI 中存在）
    QList<QRadioButton*> radios = findChildren<QRadioButton*>();

    for (QRadioButton* radio : radios) {
        if (!radio) {
            continue;
        }

        radio->setObjectName("OptionButton");
        radio->setMinimumHeight(46);
        radio->setCursor(Qt::PointingHandCursor);
    }

    // 处理进度条样式相关 objectName 与可见性
    QList<QProgressBar*> bars = findChildren<QProgressBar*>();

    for (QProgressBar* bar : bars) {
        if (!bar) {
            continue;
        }

        bar->setObjectName("MainProgressBar");
        bar->setMinimumHeight(14);
        bar->setTextVisible(false);
    }
}

 // ============================================================
 // 结果页美化：通用版
 // 说明：配置结果显示区域的对象名、滚动策略、最小尺寸等
 // ============================================================

void SBTI::polishResultPage()
{
    if (m_cardFrame) {
        m_cardFrame->setObjectName("QuestionCardFrame");
    }

    if (m_resultContainer) {
        m_resultContainer->setObjectName("ResultContainer");
        m_resultContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    }

    if (m_resultScrollArea) {
        m_resultScrollArea->setObjectName("ResultScrollArea");
        m_resultScrollArea->setWidgetResizable(true);
        m_resultScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_resultScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_resultScrollArea->setFrameShape(QFrame::NoFrame);
        m_resultScrollArea->setMinimumHeight(420);
        m_resultScrollArea->show();
    }

    if (m_resultLabel) {
        m_resultLabel->setObjectName("ResultLabel");
        m_resultLabel->setWordWrap(true);
        m_resultLabel->setTextFormat(Qt::RichText);
        m_resultLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        m_resultLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        m_resultLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        m_resultLabel->setMinimumHeight(0);
        m_resultLabel->adjustSize();
        m_resultLabel->show();
    }
}

// ============================================================
// 显示当前题目
// 说明：从引擎读取当前题目，更新进度显示，填充选项按钮并播放淡入动画
// ============================================================

void SBTI::showCurrentQuestion()
{
    const SbtiQuestion q = m_engine.currentQuestion();

    const int current = m_engine.currentIndex() + 1;
    const int total = m_engine.totalQuestions();

    int percent = 0;

    if (total > 0) {
        percent = static_cast<int>((current * 100.0) / total);
    }

    // 更新进度文本
    if (m_progressLabel) {
        m_progressLabel->setText(
            QString("%1 / %2").arg(current).arg(total)
        );
        m_progressLabel->show();
    }

    // 更新进度条
    if (m_progressBar) {
        m_progressBar->setValue(percent);
        m_progressBar->show();
    }

    // 更新题干文本
    if (m_questionLabel) {
        m_questionLabel->show();
        m_questionLabel->setText(q.text);
    }

    // 隐藏结果区域与重启/复制按钮（处于答题阶段）
    if (m_resultScrollArea) {
        m_resultScrollArea->hide();
    }

    if (m_restartButton) {
        m_restartButton->hide();
    }

    if (m_copyResultButton) {
        m_copyResultButton->hide();
    }

    // 如果当前是第一题，则隐藏“上一题”按钮
    if (m_previousButton) {
        if (m_engine.currentIndex() <= 0) {
            m_previousButton->hide();
        }
        else {
            m_previousButton->show();
        }
    }

    // 隐藏并重置所有选项按钮（将在下面逐个填充）
    for (QPushButton* button : m_optionButtons) {
        if (!button) {
            continue;
        }

        button->hide();
        button->setText("");
        button->setProperty("answerValue", 0);
    }

    // 选项前缀字母
    const QStringList optionLetters = {
        "A", "B", "C", "D"
    };

    // 将题目中的选项逐一填充到预创建的按钮中
    for (int i = 0; i < q.options.size() && i < m_optionButtons.size(); ++i) {
        const SbtiOption opt = q.options[i];

        QString buttonText;

        if (i < optionLetters.size()) {
            buttonText = optionLetters[i] + ". " + opt.label;
        }
        else {
            buttonText = opt.label;
        }

        QPushButton* button = m_optionButtons[i];

        if (!button) {
            continue;
        }

        // 将答案对应的数值保存在按钮的 property 中，点击时读取
        button->setText(buttonText);
        button->setProperty("answerValue", opt.value);
        button->show();
    }

    // 播放整页淡入动画
    playPageFadeIn();
}

 // ============================================================
 // 答题
 // 说明：处理用户选择答案后将值传给引擎，并根据是否完成决定显示下一题或结果
 // ============================================================

void SBTI::handleAnswer(int value)
{
    m_engine.answerCurrentQuestion(value);

    if (m_engine.isFinished()) {
        showResult();
    }
    else {
        showCurrentQuestion();
    }
}

 // ============================================================
 // 上一题
 // 说明：安全撤销引擎内部的最后一次答案，不会重新打乱题目顺序或重置引擎
 // ============================================================

void SBTI::goToPreviousQuestion()
{
    /*
        安全上一题：

        不再 start()
        不再 rebuild
        不再重新随机

        只撤销 engine 内部最后一次答案。
    */
    const bool ok = m_engine.undoLastAnswer();

    if (!ok) {
        return;
    }

    showCurrentQuestion();
}

 // ============================================================
 // 显示结果
 // 说明：计算结果、生成 HTML、显示结果区域并展示复制/重启等控件
 // ============================================================

void SBTI::showResult()
{
    const SbtiResult result = m_engine.calculateResult();

    // 生成并缓存纯文本结果，供复制使用
    m_lastResultPlainText = buildResultPlainText();

    if (m_progressLabel) {
        m_progressLabel->setText("测试完成");
    }

    if (m_progressBar) {
        m_progressBar->setValue(100);
    }

    // 隐藏题干与选项，显示结果内容
    if (m_questionLabel) {
        m_questionLabel->clear();
        m_questionLabel->hide();
    }

    for (QPushButton* button : m_optionButtons) {
        if (button) {
            button->hide();
            button->setEnabled(false);
        }
    }

    if (m_resultLabel) {
        m_resultLabel->clear();
        m_resultLabel->setTextFormat(Qt::RichText);
        m_resultLabel->setWordWrap(true);
        m_resultLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        m_resultLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

        // 生成 HTML 并显示（包含主要类型、次接近类型、提示等）
        QString html = buildResultHtml(result);

        const QString debugHtml = buildDebugHtml();
        if (!debugHtml.trimmed().isEmpty()) {
            html += debugHtml;
        }

        m_resultLabel->setText(html);
        m_resultLabel->adjustSize();
        m_resultLabel->show();
    }

    // 配置滚动区域并滚回顶部
    if (m_resultScrollArea) {
        m_resultScrollArea->setWidgetResizable(true);
        m_resultScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_resultScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_resultScrollArea->show();

        if (m_resultScrollArea->verticalScrollBar()) {
            m_resultScrollArea->verticalScrollBar()->setValue(0);
        }
    }

    // 显示并启用“上一题”与“重新开始”按钮
    if (m_previousButton) {
        m_previousButton->show();
        m_previousButton->setEnabled(true);
    }

    if (m_restartButton) {
        m_restartButton->show();
        m_restartButton->setEnabled(true);
    }

    // 确保复制按钮被添加（并在页面中可见）
    installCopyResultButton();
    polishResultPage();
    playPageFadeIn();
}

 // ============================================================
 // 重新开始
 // 说明：重置引擎到开始状态，清理结果显示与缓存，并展示第一题
 // ============================================================

void SBTI::restartTest()
{
    m_engine.start();

    // 将结果滚动区滚到顶部（避免保留之前的位置）
    if (m_resultScrollArea && m_resultScrollArea->verticalScrollBar()) {
        m_resultScrollArea->verticalScrollBar()->setValue(
            m_resultScrollArea->verticalScrollBar()->minimum()
        );
    }

    if (m_copyResultButton) {
        m_copyResultButton->hide();
    }

    // 清除上次生成的纯文本缓存
    m_lastResultPlainText.clear();

    showCurrentQuestion();
}

SBTI::~SBTI()
{
}

// ============================================================
// 引擎初始化
// 说明：从资源文件加载配置与题库等 JSON，若加载失败则弹出错误提示
// ============================================================
void SBTI::initEngine()
{
    const bool ok = m_engine.loadFromFiles(
        ":/data/config.json",
        ":/data/questions.json",
        ":/data/dimensions.json",
        ":/data/types.json"
    );

    if (!ok) {
        QMessageBox::critical(
            this,
            "Load failed",
            "SBTI data files failed to load. Please check resources.qrc and JSON files.",
            QMessageBox::Ok,
            QMessageBox::Ok
        );
        return;
    }

    // 启动引擎（准备题目顺序、计分等）
    m_engine.start();

    // 输出调试信息（方便开发时查看加载结果）
    qDebug() << "SBTI data loaded successfully.";
    qDebug() << "Title:" << m_engine.displayTitle();
    qDebug() << "Subtitle:" << m_engine.displaySubtitle();
    qDebug() << "Author:" << m_engine.displayAuthor();
    qDebug() << "Dimension count:" << m_engine.dimOrder().size();
    qDebug() << "Question count:" << m_engine.totalQuestions();
}

// ============================================================
// 构建界面：创建所有需要用到的 QWidget，并布置到布局中
// 说明：此函数把 UI 元素实例化并保存到成员变量，信号连接也在这里进行
// ============================================================
void SBTI::buildInterface()
{
    // 中央 widget 与主垂直布局
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    m_mainLayout = new QVBoxLayout(m_centralWidget);
    m_mainLayout->setContentsMargins(44, 30, 44, 34);
    m_mainLayout->setSpacing(14);

    // 标题标签（大字号）
    m_titleLabel = new QLabel(this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setWordWrap(true);

    QFont titleFont;
    titleFont.setPointSize(25);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);

    QString title = m_engine.displayTitle();

    if (title.trimmed().isEmpty()) {
        title = "SBTI";
    }

    m_titleLabel->setText(title);

    // 副标题（可能包含作者信息）
    m_subtitleLabel = new QLabel(this);
    m_subtitleLabel->setAlignment(Qt::AlignCenter);
    m_subtitleLabel->setWordWrap(true);

    QString subtitle = m_engine.displaySubtitle();

    if (!m_engine.displayAuthor().trimmed().isEmpty()) {
        subtitle += "\n";
        subtitle += m_engine.displayAuthor();
    }

    m_subtitleLabel->setText(subtitle);

    // 进度文本与进度条
    m_progressLabel = new QLabel(this);
    m_progressLabel->setAlignment(Qt::AlignCenter);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);

    // 将上方信息加入主布局
    m_mainLayout->addWidget(m_titleLabel);
    m_mainLayout->addWidget(m_subtitleLabel);
    m_mainLayout->addWidget(m_progressLabel);
    m_mainLayout->addWidget(m_progressBar);

    // 卡片风格的题目容器
    m_cardFrame = new QFrame(this);
    m_cardFrame->setObjectName("QuestionCardFrame");

    m_cardLayout = new QVBoxLayout(m_cardFrame);
    m_cardLayout->setContentsMargins(30, 28, 30, 28);
    m_cardLayout->setSpacing(16);

    // 题干标签
    m_questionLabel = new QLabel(m_cardFrame);
    m_questionLabel->setWordWrap(true);
    m_questionLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    QFont questionFont;
    questionFont.setPointSize(15);
    questionFont.setBold(true);
    m_questionLabel->setFont(questionFont);

    m_cardLayout->addWidget(m_questionLabel);

    // 创建 4 个选项按钮（预分配，稍后填充文本/值）
    for (int i = 0; i < 4; ++i) {
        QPushButton* button = new QPushButton(m_cardFrame);
        button->setMinimumHeight(52);
        button->setCursor(Qt::PointingHandCursor);
        button->hide();

        // 点击时从 property 读取 answerValue 并传给 handleAnswer
        connect(button, &QPushButton::clicked, this, [this, button]() {
            const int value = button->property("answerValue").toInt();
            handleAnswer(value);
            });

        m_optionButtons.append(button);
        m_cardLayout->addWidget(button);
    }

    // 结果的滚动显示区（初始隐藏）
    m_resultScrollArea = new QScrollArea(m_cardFrame);
    m_resultScrollArea->setWidgetResizable(true);
    m_resultScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_resultScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_resultScrollArea->hide();

    // 结果容器与布局
    m_resultContainer = new QWidget(m_resultScrollArea);
    m_resultLayout = new QVBoxLayout(m_resultContainer);
    m_resultLayout->setContentsMargins(0, 0, 0, 0);
    m_resultLayout->setSpacing(0);

    // 用 QLabel 展示 HTML 结果（RichText），允许选择文本和打开外部链接
    m_resultLabel = new QLabel(m_resultContainer);
    m_resultLabel->setWordWrap(true);
    m_resultLabel->setTextFormat(Qt::RichText);
    m_resultLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_resultLabel->setOpenExternalLinks(true);
    m_resultLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    m_resultLayout->addWidget(m_resultLabel);
    m_resultLayout->addStretch();

    m_resultScrollArea->setWidget(m_resultContainer);

    m_cardLayout->addWidget(m_resultScrollArea);

    // 将卡片加入主布局（stretch 值设为 1 以填充剩余空间）
    m_mainLayout->addWidget(m_cardFrame, 1);

    // 底部按钮布局：上一题 / 重启
    m_bottomButtonLayout = new QHBoxLayout();
    m_bottomButtonLayout->setSpacing(12);

    m_previousButton = new QPushButton(U(L"\u2190 \u4E0A\u4E00\u9898"), this); // ← 上一题
    m_previousButton->setMinimumHeight(44);
    m_previousButton->setCursor(Qt::PointingHandCursor);

    connect(
        m_previousButton,
        &QPushButton::clicked,
        this,
        &SBTI::goToPreviousQuestion
    );

    m_restartButton = new QPushButton(U(L"\u91CD\u65B0\u5F00\u59CB"), this); // 重新开始
    m_restartButton->setMinimumHeight(44);
    m_restartButton->setCursor(Qt::PointingHandCursor);

    connect(
        m_restartButton,
        &QPushButton::clicked,
        this,
        &SBTI::restartTest
    );

    m_bottomButtonLayout->addWidget(m_previousButton);
    m_bottomButtonLayout->addStretch();
    m_bottomButtonLayout->addWidget(m_restartButton);

    m_mainLayout->addLayout(m_bottomButtonLayout);

    // 初始隐藏底部按钮，只有在需要时显示
    m_previousButton->hide();
    m_restartButton->hide();
}

// ============================================================
// 应用样式表（QSS）
// 说明：集中管理颜色、按钮圆角、滚动条等视觉风格
// ============================================================
void SBTI::applyAppStyle()
{
    setStyleSheet(R"(
        QMainWindow {
            background-color: #101114;
        }

        QWidget {
            font-family: "Microsoft YaHei", "Segoe UI", sans-serif;
            color: #f2f2f2;
        }

        QLabel {
            color: #f2f2f2;
        }

        QLabel#HeaderTitle {
            font-size: 28px;
            font-weight: 900;
            color: #ffffff;
        }

        QLabel#HeaderSubtitle {
            font-size: 14px;
            color: #b8bcc8;
        }

        QFrame#TopHeader {
            background-color: #181a20;
            border: 1px solid #2a2d36;
            border-radius: 18px;
        }

        QFrame#QuestionCardFrame {
            background-color: #181a20;
            border: 1px solid #2a2d36;
            border-radius: 20px;
        }

        QPushButton {
            background-color: #242833;
            border: 1px solid #3a4050;
            border-radius: 12px;
            padding: 10px 16px;
            color: #f2f2f2;
            font-size: 15px;
            font-weight: 600;
        }

        QPushButton:hover {
            background-color: #303747;
            border: 1px solid #6677aa;
        }

        QPushButton:pressed {
            background-color: #1d2230;
        }

        QPushButton#CopyResultButton {
            background-color: #2f6fed;
            border: none;
            color: white;
            font-weight: 800;
        }

        QPushButton#CopyResultButton:hover {
            background-color: #4b83f1;
        }

        QProgressBar {
            background-color: #22252d;
            border: none;
            border-radius: 7px;
            height: 14px;
        }

        QProgressBar::chunk {
            background-color: #7bdcff;
            border-radius: 7px;
        }

        QScrollArea {
            background-color: transparent;
            border: none;
        }

        QScrollBar:vertical {
            background-color: transparent;
            width: 10px;
            margin: 0;
        }

        QScrollBar::handle:vertical {
            background-color: #3a4050;
            border-radius: 5px;
            min-height: 30px;
        }

        QScrollBar::add-line:vertical,
        QScrollBar::sub-line:vertical {
            height: 0;
        }

        QFrame {
            background-color: transparent;
        }
    )");
}

// ============================================================
// 将结果构造成 HTML（用于在 QLabel 中以富文本显示）
// 说明：使用内联样式控制字体大小、颜色，并调用 htmlText 进行防 XSS 转义
// ============================================================
QString SBTI::buildResultHtml(const SbtiResult& result) const
{
    QString html;

    html += "<html>";
    html += "<body style='font-family: Microsoft YaHei, Segoe UI, sans-serif; color:#f2f2f2;'>";

    html += "<table width='100%' cellspacing='0' cellpadding='0' style='color:#f2f2f2;'>";

    html += "<tr><td style='padding: 8px 0 18px 0;'>";
    html += "<span style='font-size:34px; font-weight:900; color:#ffffff;'>";
    html += "你的 SBTI 结果";
    html += "</span><br><br>";
    html += "<span style='font-size:16px; color:#cfd3dc;'>";
    html += "以下是根据你的选择生成的结果。";
    html += "</span>";
    html += "</td></tr>";

    // ============================================================
    // 主要类型
    // ============================================================
    html += "<tr><td style='padding: 18px; background-color:#20232c; border:1px solid #3a4050;'>";

    html += "<div style='font-size:20px; font-weight:800; color:#7bdcff;'>";
    html += "主要类型";
    html += "</div>";

    html += "<br>";

    html += "<div style='font-size:42px; font-weight:900; color:#ffffff;'>";
    html += htmlText(result.primary.code);
    html += "</div>";

    html += "<br>";

    html += "<div style='font-size:24px; font-weight:800; color:#ffffff;'>";
    html += htmlText(result.primary.cn);
    html += "</div>";

    html += "<br>";

    html += "<div style='font-size:16px; color:#d6d8df;'>";
    html += "匹配度：";
    html += QString::number(result.primary.similarity);
    html += "%";
    html += "</div>";

    if (!result.primary.intro.trimmed().isEmpty()) {
        html += "<br><br>";
        html += "<div style='font-size:18px; font-weight:800; color:#ffffff;'>";
        html += htmlText(result.primary.intro);
        html += "</div>";
    }

    if (!result.primary.desc.trimmed().isEmpty()) {
        html += "<br>";
        html += "<div style='font-size:15px; line-height:160%; color:#d6d8df;'>";
        html += htmlText(result.primary.desc);
        html += "</div>";
    }

    html += "</td></tr>";

    // ============================================================
    // 次接近类型
    // ============================================================
    if (result.hasSecondary) {
        html += "<tr><td style='height:18px;'></td></tr>";

        html += "<tr><td style='padding: 18px; background-color:#20232c; border:1px solid #3a4050;'>";

        html += "<div style='font-size:20px; font-weight:800; color:#ffd37b;'>";
        html += "次接近类型";
        html += "</div>";

        html += "<br>";

        html += "<div style='font-size:42px; font-weight:900; color:#ffffff;'>";
        html += htmlText(result.secondary.code);
        html += "</div>";

        html += "<br>";

        html += "<div style='font-size:24px; font-weight:800; color:#ffffff;'>";
        html += htmlText(result.secondary.cn);
        html += "</div>";

        html += "<br>";

        html += "<div style='font-size:16px; color:#d6d8df;'>";
        html += "匹配度：";
        html += QString::number(result.secondary.similarity);
        html += "%";
        html += "</div>";

        if (!result.secondary.intro.trimmed().isEmpty()) {
            html += "<br><br>";
            html += "<div style='font-size:18px; font-weight:800; color:#ffffff;'>";
            html += htmlText(result.secondary.intro);
            html += "</div>";
        }

        if (!result.secondary.desc.trimmed().isEmpty()) {
            html += "<br>";
            html += "<div style='font-size:15px; line-height:160%; color:#d6d8df;'>";
            html += htmlText(result.secondary.desc);
            html += "</div>";
        }

        html += "</td></tr>";
    }
    // 如果引擎检测到“drunk”状态（答题波动较大），显示提示语
    if (m_engine.isDrunk()) {
        html += "<tr><td style='height:18px;'></td></tr>";

        html += "<tr><td style='padding: 16px; background-color:#2c1d1d; border:1px solid #7a3c3c; color:#ffb3b3;'>";
        html += "检测到答题结果可能存在较高波动，请以娱乐心态参考。";
        html += "</td></tr>";
    }

    html += "</table>";

    html += "</body>";
    html += "</html>";

    return html;
}

// 调试用的额外 HTML（当前返回空字符串，可按需扩展）
QString SBTI::buildDebugHtml() const
{
    return QString();
}

// 将纯文本做 HTML 转义并把换行替换为 <br>，用于安全地在 RichText 中显示用户/数据文本
QString SBTI::htmlText(const QString& text) const
{
    QString escaped = text.toHtmlEscaped();
    escaped.replace("\r\n", "<br>");
    escaped.replace("\n", "<br>");
    return escaped;
}
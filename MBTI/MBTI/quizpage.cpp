#include "quizpage.h"
#include "ui_quizpage.h"

#include <QFont>
#include <QFontDatabase>
#include <QSignalBlocker>
#include <QAbstractButton>

// 构造函数：初始化 UI、按钮组、样式和信号连接
QuizPage::QuizPage(MBTIEngine* engine, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::QuizPage)
    , m_engine(engine)
    , m_answerGroup(nullptr)
    , m_currentIndex(0)
{
    ui->setupUi(this); // 实例化并绑定由 Qt Designer 生成的 UI

    // 创建用于管理单选按钮的按钮组，并为每个按钮分配 id（1=A, 2=B, 3=中立）
    m_answerGroup = new QButtonGroup(this);
    m_answerGroup->addButton(ui->radioA, 1);      // radioA -> id 1
    m_answerGroup->addButton(ui->radioB, 2);      // radioB -> id 2
    m_answerGroup->addButton(ui->radioNeutral, 3);// radioNeutral -> id 3
    m_answerGroup->setExclusive(true);            // 保证同一时间只有一个选中

    applyQuizStyle(); // 应用页面样式（字体、布局、QSS 等）

    // 连接“上一题”和“下一题”按钮的点击信号到对应槽
    connect(ui->btnPrev, &QPushButton::clicked, this, &QuizPage::on_btnPrev_clicked);
    connect(ui->btnNext, &QPushButton::clicked, this, &QuizPage::on_btnNext_clicked);

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    // Qt 5.15+ 提供 idClicked 信号：按钮点击后返回 id（兼容现代 API）
    connect(m_answerGroup, &QButtonGroup::idClicked, this, [this](int id) {
        if (!m_engine) {
            return; // 引擎无效时直接返回
        }

        // 仅处理合法 id（1/2/3），并立即把选择写入引擎，更新进度与按钮状态
        if (id == 1 || id == 2 || id == 3) {
            m_engine->answerQuestion(m_currentIndex, id);
            updateProgress();
            updateButtonState();
        }
        });
#else
    // 旧版本 Qt 使用 buttonClicked 信号，需要显式类型转换
    connect(m_answerGroup, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked),
        this, [this](int id) {
            if (!m_engine) {
                return;
            }

            if (id == 1 || id == 2 || id == 3) {
                m_engine->answerQuestion(m_currentIndex, id);
                updateProgress();
                updateButtonState();
            }
        });
#endif

    // 额外监听每个单选按钮的 toggled 信号：确保在不同平台/情况也能保存答案
    connect(ui->radioA, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked && m_engine) {
            m_engine->answerQuestion(m_currentIndex, 1);
            updateProgress();
            updateButtonState();
        }
        });

    connect(ui->radioB, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked && m_engine) {
            m_engine->answerQuestion(m_currentIndex, 2);
            updateProgress();
            updateButtonState();
        }
        });

    connect(ui->radioNeutral, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked && m_engine) {
            m_engine->answerQuestion(m_currentIndex, 3);
            updateProgress();
            updateButtonState();
        }
        });

    loadQuestion(0); // 初始加载第 0 题
}

QuizPage::~QuizPage()
{
    delete ui; // 释放 UI 资源
}

// 重启问卷：重置索引并加载第一题
void QuizPage::restartQuiz()
{
    m_currentIndex = 0;
    loadQuestion(0);
}

// 将 Unicode codepoint 转换为 QString（兼容 Qt5/Qt6）
QString QuizPage::emoji(uint codepoint) const
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    char32_t ucs4[1] = { static_cast<char32_t>(codepoint) };
    return QString::fromUcs4(ucs4, 1);
#else
    uint ucs4[1] = { codepoint };
    return QString::fromUcs4(ucs4, 1);
#endif
}

// 选择一个合适的字体族用于页面（优先常见中文字体）
QString QuizPage::chooseNiceFontFamily() const
{
    const QStringList families = QFontDatabase::families();

    if (families.contains("Microsoft YaHei UI")) {
        return "Microsoft YaHei UI";
    }

    if (families.contains("Microsoft YaHei")) {
        return "Microsoft YaHei";
    }

    if (families.contains("SimHei")) {
        return "SimHei";
    }

    if (families.contains("Arial")) {
        return "Arial";
    }

    return font().family(); // 回退到当前字体
}

// 为问卷页面应用统一样式：字体、对齐、鼠标样式、控件最小高度和 QSS
void QuizPage::applyQuizStyle()
{
    const QString fontFamily = chooseNiceFontFamily();

    QFont baseFont(fontFamily);
    baseFont.setPointSize(12);
    setFont(baseFont); // 为整个 QWidget 设置基础字体

    setAutoFillBackground(true); // 允许背景填充

    // 对齐与换行设置，保证题干居中并自动换行
    ui->labelQuestionNum->setAlignment(Qt::AlignCenter);
    ui->labelProgress->setAlignment(Qt::AlignCenter);
    ui->labelQuestionText->setAlignment(Qt::AlignCenter);
    ui->labelQuestionText->setWordWrap(true);

    // 将可点击控件的鼠标指针设为手形，提升交互性
    ui->radioA->setCursor(Qt::PointingHandCursor);
    ui->radioB->setCursor(Qt::PointingHandCursor);
    ui->radioNeutral->setCursor(Qt::PointingHandCursor);
    ui->btnPrev->setCursor(Qt::PointingHandCursor);
    ui->btnNext->setCursor(Qt::PointingHandCursor);

    ui->progressBar->setTextVisible(false); // 隐藏进度条文本（使用 label 显示）

    // 增加选项高度，便于触摸和点击
    ui->radioA->setMinimumHeight(76);
    ui->radioB->setMinimumHeight(76);
    ui->radioNeutral->setMinimumHeight(76);

    // 使用样式表设置视觉风格，注意：不要在此原始字符串内部插入注释或修改
    setStyleSheet(QString(R"(
        QWidget {
            background-color: qlineargradient(
                x1:0, y1:0, x2:1, y2:1,
                stop:0 #fff7fb,
                stop:0.28 #f7f1ff,
                stop:0.62 #eef7ff,
                stop:1 #fffbea
            );
            color: #222222;
            font-family: "%1";
        }

        QLabel#labelQuestionNum {
            background-color: rgba(255, 255, 255, 225);
            color: #241d38;
            font-size: 31px;
            font-weight: 700;
            padding-top: 18px;
            padding-bottom: 14px;
            border-bottom: 1px solid rgba(128, 104, 179, 70);
        }

        QLabel#labelProgress {
            background-color: rgba(255, 255, 255, 195);
            color: #8068b3;
            font-size: 18px;
            font-weight: 500;
            padding-top: 10px;
            padding-bottom: 12px;
            border-bottom: 1px solid rgba(128, 104, 179, 55);
        }

        QLabel#labelQuestionText {
            color: #23202e;
            font-size: 30px;
            font-weight: 600;
            background-color: rgba(255, 255, 255, 245);
            border: 1px solid rgba(128, 104, 179, 95);
            border-top: 12px solid #8068b3;
            border-radius: 26px;
            padding: 38px 42px;
            margin-left: 44px;
            margin-right: 44px;
            margin-top: 24px;
            margin-bottom: 26px;
        }

        QProgressBar {
            height: 22px;
            border: none;
            border-radius: 11px;
            background-color: rgba(225, 217, 239, 230);
            margin-left: 60px;
            margin-right: 60px;
            margin-top: 10px;
            margin-bottom: 10px;
        }

        QProgressBar::chunk {
            border-radius: 11px;
            background-color: qlineargradient(
                x1:0, y1:0, x2:1, y2:0,
                stop:0 #8068b3,
                stop:0.45 #a884d6,
                stop:1 #f0b46c
            );
        }

        QRadioButton {
            color: #241f30;
            background-color: rgba(255, 255, 255, 248);
            border: 2px solid rgba(128, 104, 179, 105);
            border-radius: 22px;

            font-size: 23px;
            font-weight: 500;

            min-height: 62px;
            padding-left: 26px;
            padding-right: 26px;
            padding-top: 8px;
            padding-bottom: 8px;

            margin-left: 72px;
            margin-right: 72px;
            margin-top: 8px;
            margin-bottom: 8px;

            spacing: 18px;
        }

        QRadioButton:hover {
            background-color: #fff8ec;
            border: 2px solid #f0b46c;
            color: #4d3978;
        }

        QRadioButton:checked {
            background-color: #f1eafa;
            border: 3px solid #8068b3;
            color: #4d3978;
            font-weight: 700;
        }

        QRadioButton::indicator {
            width: 26px;
            height: 26px;
        }

        QPushButton {
            min-width: 170px;
            min-height: 54px;
            color: #ffffff;
            background-color: #8068b3;
            border: none;
            border-radius: 18px;
            font-size: 22px;
            font-weight: 700;
            padding: 10px 34px;
            margin: 16px 22px 20px 22px;
        }

        QPushButton:hover {
            background-color: #6f58a5;
        }

        QPushButton:pressed {
            background-color: #5c478d;
        }

        QPushButton:disabled {
            background-color: #c9bfdc;
            color: #ffffff;
        }

        QPushButton#btnPrev {
            background-color: #a995d0;
        }

        QPushButton#btnPrev:hover {
            background-color: #927fbd;
        }

        QPushButton#btnPrev:disabled {
            background-color: #ddd5eb;
            color: #ffffff;
        }

        QPushButton#btnNext {
            background-color: qlineargradient(
                x1:0, y1:0, x2:1, y2:0,
                stop:0 #8068b3,
                stop:1 #f0a84f
            );
        }

        QPushButton#btnNext:hover {
            background-color: qlineargradient(
                x1:0, y1:0, x2:1, y2:0,
                stop:0 #6f58a5,
                stop:1 #e69b3f
            );
        }

        QPushButton#btnNext:disabled {
            background-color: #cfc4e4;
            color: #ffffff;
        }
    )").arg(fontFamily));
}

// 加载指定索引的题目并更新 UI（包含题干、选项、提示、进度等）
void QuizPage::loadQuestion(int index)
{
    // 边界与引擎有效性检查
    if (!m_engine || index < 0 || index >= m_engine->totalQuestions()) {
        return;
    }

    m_currentIndex = index; // 更新当前题目索引

    const Question& q = m_engine->questions()[index]; // 获取题目数据
    const int total = m_engine->totalQuestions();

    // 更新题号显示，带 emoji 美化
    ui->labelQuestionNum->setText(
        QString("%1   问题  %2  /  %3   %4")
        .arg(emoji(0x1F338))
        .arg(index + 1)
        .arg(total)
        .arg(emoji(0x1F338))
    );

    // 显示题干（加上引号）
    ui->labelQuestionText->setText(
        QString("\"%1\"").arg(q.text)
    );

    // 处理选项文本的空值（提供回退文案）
    QString optionAText = q.optionA.trimmed();
    QString optionBText = q.optionB.trimmed();

    if (optionAText.isEmpty()) {
        optionAText = "选项 A";
    }

    if (optionBText.isEmpty()) {
        optionBText = "选项 B";
    }

    // 写入单选按钮文本及 tooltip（便于查看超长文本）
    ui->radioA->setText("A.  " + optionAText);
    ui->radioB->setText("B.  " + optionBText);
    ui->radioNeutral->setText("C.  两者差不多 / 暂时难以选择");

    ui->radioA->setToolTip(ui->radioA->text());
    ui->radioB->setToolTip(ui->radioB->text());
    ui->radioNeutral->setToolTip(ui->radioNeutral->text());

    // 清除 UI 层的选中状态（不影响模型），再从模型恢复已保存答案
    clearRadioSelection();
    restoreAnswer(index);

    // 更新进度条和按钮状态
    updateProgress();
    updateButtonState();
}

// 清除单选按钮的选中状态，同时屏蔽信号避免触发保存逻辑
void QuizPage::clearRadioSelection()
{
    if (!m_answerGroup) {
        return;
    }

    // QSignalBlocker 在对象作用域内屏蔽信号，避免在 setChecked 时触发回调
    QSignalBlocker blockerGroup(m_answerGroup);
    QSignalBlocker blockerA(ui->radioA);
    QSignalBlocker blockerB(ui->radioB);
    QSignalBlocker blockerNeutral(ui->radioNeutral);

    const bool oldExclusive = m_answerGroup->exclusive();

    m_answerGroup->setExclusive(false); // 取消互斥以便逐个取消选中

    ui->radioA->setChecked(false);
    ui->radioB->setChecked(false);
    ui->radioNeutral->setChecked(false);

    m_answerGroup->setExclusive(oldExclusive); // 恢复互斥设置
}

// 从引擎读取已保存答案并在 UI 上恢复（同时屏蔽信号）
void QuizPage::restoreAnswer(int index)
{
    if (!m_engine) {
        return;
    }

    const int ans = m_engine->getAnswer(index);

    QSignalBlocker blockerGroup(m_answerGroup);
    QSignalBlocker blockerA(ui->radioA);
    QSignalBlocker blockerB(ui->radioB);
    QSignalBlocker blockerNeutral(ui->radioNeutral);

    if (ans == 1) {
        ui->radioA->setChecked(true);
    }
    else if (ans == 2) {
        ui->radioB->setChecked(true);
    }
    else if (ans == 3) {
        ui->radioNeutral->setChecked(true);
    }
}

// 判断当前题是否已作答（委托给引擎）
bool QuizPage::currentQuestionHasAnswer() const
{
    if (!m_engine) {
        return false;
    }

    return m_engine->isAnswered(m_currentIndex);
}

// 更新进度条和进度文本（包括已答数量与百分比）
void QuizPage::updateProgress()
{
    if (!m_engine) {
        return;
    }

    const int total = m_engine->totalQuestions();
    const int answered = m_engine->answeredCount();

    // 进度条显示当前题序（从 1 开始）
    ui->progressBar->setRange(0, total);
    ui->progressBar->setValue(m_currentIndex + 1);

    int pagePercent = 0;
    if (total > 0) {
        pagePercent = static_cast<int>(((m_currentIndex + 1) * 100.0 / total) + 0.5);
    }

    // 在 label 中显示进度百分比和已回答/总数
    ui->labelProgress->setText(
        QString("当前进度：%1%        已回答：%2 / %3")
        .arg(pagePercent)
        .arg(answered)
        .arg(total)
    );
}

// 更新上一题/下一题按钮的可用性与文案（到最后一题时显示“查看结果”）
void QuizPage::updateButtonState()
{
    if (!m_engine) {
        return;
    }

    const int total = m_engine->totalQuestions();

    ui->btnPrev->setEnabled(m_currentIndex > 0); // 只有不是第一题才可用

    if (m_currentIndex == total - 1) {
        ui->btnNext->setText("查看结果"); // 最后一题改为查看结果
    }
    else {
        ui->btnNext->setText("下一题");
    }

    // 下一步按钮仅在当前题已作答时可用
    ui->btnNext->setEnabled(currentQuestionHasAnswer());
}

// 将当前 UI 上的选择保存到引擎（兼容按钮组和单个 radio 状态）
void QuizPage::saveCurrentAnswer()
{
    if (!m_engine || !m_answerGroup) {
        return;
    }

    int checkedId = m_answerGroup->checkedId();

    // 有时按钮组的 checkedId 可能没有更新，故再检查具体 radio 的 checked 状态以确定最终 id
    if (ui->radioA->isChecked()) {
        checkedId = 1;
    }
    else if (ui->radioB->isChecked()) {
        checkedId = 2;
    }
    else if (ui->radioNeutral->isChecked()) {
        checkedId = 3;
    }

    if (checkedId == 1 || checkedId == 2 || checkedId == 3) {
        m_engine->answerQuestion(m_currentIndex, checkedId); // 写入模型
    }
}

// “上一题”按钮槽：保存当前答案并加载上一题（若存在）
void QuizPage::on_btnPrev_clicked()
{
    saveCurrentAnswer();

    if (m_currentIndex > 0) {
        loadQuestion(m_currentIndex - 1);
    }
}

// 查找第一个未回答题并跳转（用于当用户尝试提交但未全部回答时）
void QuizPage::goToFirstUnansweredQuestion()
{
    if (!m_engine) {
        return;
    }

    const int firstIndex = m_engine->firstUnansweredIndex();

    if (firstIndex >= 0) {
        loadQuestion(firstIndex);
    }
}

// “下一题/查看结果”按钮槽：保存当前答案并处理翻页或提交逻辑
void QuizPage::on_btnNext_clicked()
{
    saveCurrentAnswer();

    if (!m_engine) {
        return;
    }

    const int total = m_engine->totalQuestions();

    // 如果当前题尚未作答，更新按钮状态并返回（不翻页）
    if (!currentQuestionHasAnswer()) {
        updateButtonState();
        return;
    }

    // 若还有下一题则加载下一题
    if (m_currentIndex < total - 1) {
        loadQuestion(m_currentIndex + 1);
        return;
    }

    // 到达最后一题但未全部回答时跳到第一个未回答题
    if (!m_engine->allAnswered()) {
        goToFirstUnansweredQuestion();
        return;
    }

    // 全部回答完毕，发出 quizFinished 信号给上层窗口处理（通常显示结果页）
    emit quizFinished();
}
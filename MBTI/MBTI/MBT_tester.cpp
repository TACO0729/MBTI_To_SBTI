#include "MBT_tester.h"
#include "ui_MBT_tester.h"

#include <QMessageBox>

// 主窗口构造：创建 MBTI 引擎、加载题库、构建页面并连接信号槽
MBT_tester::MBT_tester(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MBT_tester)
    , m_engine(new MBTIEngine(this))
{
    ui->setupUi(this);

    setWindowTitle("MBTI 人格测试");
    resize(1100, 720);
    setMinimumSize(980, 650);

    applyMainStyle();

    // 题库资源路径（嵌入资源）
    QString jsonPath = ":/questions.json";
    if (!m_engine->loadQuestions(jsonPath)) {
        // 若加载失败，在欢迎页显示错误并禁止开始按钮
        ui->labelWelcome->setText("错误：无法加载题库文件 questions.json\n请检查资源文件配置");
        ui->btnStart->setEnabled(false);
    }

    // 创建问卷页与结果页，并加入堆栈窗口
    m_quizPage = new QuizPage(m_engine, this);
    m_resultPage = new ResultPage(this);

    ui->stackedWidget->addWidget(m_quizPage);
    ui->stackedWidget->addWidget(m_resultPage);

    // 连接按钮与页面间信号
    connect(ui->btnStart, &QPushButton::clicked, this, &MBT_tester::on_btnStart_clicked);
    connect(m_quizPage, &QuizPage::quizFinished, this, &MBT_tester::onQuizFinished);
    connect(m_resultPage, &ResultPage::restartQuiz, this, &MBT_tester::onRestartQuiz);

    // 显示欢迎页面（索引 0）
    ui->stackedWidget->setCurrentIndex(0);
}

MBT_tester::~MBT_tester()
{
    delete ui;
}

// 应用主窗口的样式表：统一按钮、背景、标签等外观
void MBT_tester::applyMainStyle()
{
    setStyleSheet(R"(
        QMainWindow {
            background-color: #f8f4ff;
        }

        QStackedWidget {
            background-color: #f8f4ff;
            border: none;
        }

        QLabel {
            color: #222222;
        }

        QPushButton {
            min-height: 42px;
            border-radius: 14px;
            background-color: #8068b3;
            color: white;
            font-size: 18px;
            padding: 8px 24px;
        }

        QPushButton:hover {
            background-color: #6f58a5;
        }

        QPushButton:pressed {
            background-color: #5d488f;
        }

        QPushButton:disabled {
            background-color: #cfc4e4;
            color: #ffffff;
        }
    )");
}

// 开始按钮槽：重置引擎并展示问卷页
void MBT_tester::on_btnStart_clicked()
{
    m_engine->reset();

    if (m_quizPage) {
        m_quizPage->restartQuiz();
    }

    showQuiz();
}

// 当问卷完成时，计算结果并切换到结果页（上层调用 setResult）
void MBT_tester::onQuizFinished()
{
    QuizResult result = m_engine->calculateResult();
    m_resultPage->setResult(result);
    showResult();
}

// 重启问卷槽：重置引擎并返回欢迎页
void MBT_tester::onRestartQuiz()
{
    m_engine->reset();

    if (m_quizPage) {
        m_quizPage->restartQuiz();
    }

    showWelcome();
}

// 显示欢迎页面
void MBT_tester::showWelcome()
{
    ui->stackedWidget->setCurrentIndex(0);
}

// 显示问卷页面
void MBT_tester::showQuiz()
{
    ui->stackedWidget->setCurrentIndex(1);
}

// 显示结果页面
void MBT_tester::showResult()
{
    ui->stackedWidget->setCurrentIndex(2);
}
// OOP: GUI Presentation Layer (interview screen)
#include "gui/InterviewWidget.h"
#include "gui/SessionContext.h"
#include "core/User.h"
#include "core/InterviewSession.h"
#include "core/FeedbackGenerator.h"
#include "interview/InterviewService.h"
#include "interview/SessionFactory.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QLabel>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QString>

namespace gui {

InterviewWidget::InterviewWidget(SessionContext& context, QWidget* parent)
    : QWidget(parent)
    , context_(context) {
    buildUi();
}

void InterviewWidget::buildUi() {
    QVBoxLayout* root = new QVBoxLayout(this);
    stack_ = new QStackedWidget(this);
    root->addWidget(stack_, 1);
    
    // ---- page 0: setup ----
    QWidget* setupPage = new QWidget(this);
    QFormLayout* setupForm = new QFormLayout(setupPage);
    courseCombo_ = new QComboBox(setupPage);
    subjectCombo_ = new QComboBox(setupPage);
    modeCombo_ = new QComboBox(setupPage);
    
    const std::vector<std::shared_ptr<core::Course>> courses = context_.courses().courses();
    for (std::size_t i = 0; i < courses.size(); ++i) {
        courseCombo_->addItem(QString::fromStdString(courses[i]->name()),
                              QString::fromStdString(courses[i]->id()));
    }
    modeCombo_->addItem(QStringLiteral("Practice (Linear Ramp)"), 0);
    modeCombo_->addItem(QStringLiteral("Mock Interview (Linear Ramp)"), 1);
    modeCombo_->addItem(QStringLiteral("Daily Challenge (Adaptive)"), 2);
    
    setupForm->addRow(tr("Course:"), courseCombo_);
    setupForm->addRow(tr("Subject:"), subjectCombo_);
    setupForm->addRow(tr("Mode:"), modeCombo_);
    
    startButton_ = new QPushButton(tr("Start interview"), setupPage);
    startButton_->setDefault(true);
    setupForm->addRow(startButton_);
    
    QLabel* hint = new QLabel(tr("A session contains 10 questions ramping Easy -> Medium -> Hard."),
                              setupPage);
    hint->setWordWrap(true);
    setupForm->addRow(hint);
    
    stack_->addWidget(setupPage);
    
    // ---- page 1: running ----
    QWidget* runPage = new QWidget(this);
    QVBoxLayout* runLayout = new QVBoxLayout(runPage);
    progressLabel_ = new QLabel(runPage);
    metaLabel_ = new QLabel(runPage);
    questionText_ = new QTextEdit(runPage);
    questionText_->setReadOnly(true);
    answerEdit_ = new QLineEdit(runPage);
    runLayout->addWidget(progressLabel_);
    runLayout->addWidget(metaLabel_);
    runLayout->addWidget(questionText_, 1);
    runLayout->addWidget(answerEdit_);
    
    QHBoxLayout* actions = new QHBoxLayout;
    submitButton_ = new QPushButton(tr("Submit answer"), runPage);
    skipButton_ = new QPushButton(tr("Skip"), runPage);
    finishButton_ = new QPushButton(tr("Finish early"), runPage);
    actions->addWidget(submitButton_, 1);
    actions->addWidget(skipButton_);
    actions->addWidget(finishButton_);
    runLayout->addLayout(actions);
    stack_->addWidget(runPage);
    
    // ---- page 2: report ----
    QWidget* reportPage = new QWidget(this);
    QVBoxLayout* reportLayout = new QVBoxLayout(reportPage);
    QLabel* reportTitle = new QLabel(tr("Interview report"), reportPage);
    reportTitle->setStyleSheet(QStringLiteral("font-weight: bold;"));
    reportLayout->addWidget(reportTitle);
    reportText_ = new QTextEdit(reportPage);
    reportText_->setReadOnly(true);
    reportLayout->addWidget(reportText_, 1);
    againButton_ = new QPushButton(tr("Start another interview"), reportPage);
    reportLayout->addWidget(againButton_);
    stack_->addWidget(reportPage);
    
    refreshSubjects();
    setSetupMode(true);
    
    connect(courseCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &InterviewWidget::onCourseChanged);
    connect(startButton_, &QPushButton::clicked, this, &InterviewWidget::onStart);
    connect(submitButton_, &QPushButton::clicked, this, &InterviewWidget::onSubmit);
    connect(skipButton_, &QPushButton::clicked, this, &InterviewWidget::onSkip);
    connect(finishButton_, &QPushButton::clicked, this, &InterviewWidget::onFinish);
    connect(againButton_, &QPushButton::clicked, this, [this]() { setSetupMode(true); });
}

void InterviewWidget::setUser(const core::UserPtr& user) {
    user_ = user;
    setSetupMode(true);
}

void InterviewWidget::setSetupMode(bool setup) {
    stack_->setCurrentIndex(setup ? 0 : 2);
    if (setup) {
        session_.reset();
        service_.reset();
    }
}

void InterviewWidget::refreshSubjects() {
    subjectCombo_->clear();
    if (courseCombo_->count() == 0) return;
    
    const std::string courseId = courseCombo_->currentData().toString().toStdString();
    const std::vector<std::shared_ptr<core::Subject>> subjects =
        context_.courses().subjectsForCourse(courseId);
    for (std::size_t i = 0; i < subjects.size(); ++i) {
        subjectCombo_->addItem(QString::fromStdString(subjects[i]->name()),
                               QString::fromStdString(subjects[i]->id()));
    }
}

void InterviewWidget::onCourseChanged(int) {
    refreshSubjects();
}

void InterviewWidget::onStart() {
    if (!user_) return;
    if (subjectCombo_->count() == 0) {
        questionText_->setPlainText(tr("Select a subject with questions first."));
        return;
    }
    
    const std::string subject = subjectCombo_->currentData().toString().toStdString();
    const int modeIndex = modeCombo_->currentData().toInt();
    
    if (modeIndex == 2) {
        core::Optional<core::DailyChallenge> challenge =
            context_.progress().ensureDailyChallenge(courseCombo_->currentData().toString().toStdString());
        if (!challenge.has_value()) {
            questionText_->setPlainText(tr("No daily challenge is available for this course today."));
            return;
        }
    }
    
    if (modeIndex == 0) {
        mode_ = core::InterviewSession::Mode::Practice;
        service_ = interview::SessionFactory::createPractice(
            context_.questionRepo(), context_.attemptRepo(), context_.feedbackGenerator());
    } else if (modeIndex == 1) {
        mode_ = core::InterviewSession::Mode::Mock;
        service_ = interview::SessionFactory::createMock(
            context_.questionRepo(), context_.attemptRepo(), context_.feedbackGenerator());
    } else {
        mode_ = core::InterviewSession::Mode::Daily;
        service_ = interview::SessionFactory::createDaily(
            context_.questionRepo(), context_.attemptRepo(), context_.feedbackGenerator());
    }
    
    session_ = service_->startSession(user_, mode_, subject, "");
    if (!session_) {
        questionText_->setPlainText(tr("No questions available for this subject."));
        return;
    }
    
    stack_->setCurrentIndex(1);
    answerEdit_->clear();
    showCurrentQuestion();
}

void InterviewWidget::showCurrentQuestion() {
    if (!session_) return;
    
    const core::QuestionPtr question = session_->currentQuestion();
    if (!question) {
        onFinish();
        return;
    }
    
    progressLabel_->setText(tr("Question %1 of %2")
        .arg(static_cast<int>(session_->currentQuestionIndex()) + 1)
        .arg(static_cast<int>(session_->questions().size())));
    metaLabel_->setText(tr("Difficulty: %1 | Time limit: %2 min | Topic: %3")
        .arg(QString::fromStdString(core::toString(question->difficulty())))
        .arg(core::timeLimitMinutes(question->difficulty()))
        .arg(QString::fromStdString(question->topic())));
    questionText_->setPlainText(QString::fromStdString(question->display()));
    answerEdit_->setFocus();
    timeTakenSeconds_ = core::timeLimitMinutes(question->difficulty()) * 60;
}

void InterviewWidget::onSubmit() {
    if (!session_ || !service_) return;
    
    const std::string answer = answerEdit_->text().toStdString();
    if (answer.empty()) return;
    
    service_->submitAnswer(session_, answer, timeTakenSeconds_);
    answerEdit_->clear();
    showCurrentQuestion();
}

void InterviewWidget::onSkip() {
    if (!session_ || !service_) return;
    service_->skipQuestion(session_, timeTakenSeconds_);
    answerEdit_->clear();
    showCurrentQuestion();
}

void InterviewWidget::onFinish() {
    if (!session_ || !service_) return;
    
    core::FeedbackReport report = service_->finishSession(session_);
    context_.progress().finalizeSession(user_, report);
    showReport(report);
    session_.reset();
    service_.reset();
    emit sessionFinished();
}

void InterviewWidget::showReport(const core::FeedbackReport& report) {
    QString text;
    text += tr("Overall score: %1%\n")
        .arg(QString::number(report.overallScore * 100.0, 'f', 1));
    text += tr("Strongest topic: %1\n").arg(QString::fromStdString(report.strongestTopic));
    text += tr("Weakest topic: %1\n\n").arg(QString::fromStdString(report.weakestTopic));
    text += tr("Tips:\n");
    for (std::size_t i = 0; i < report.tips.size(); ++i) {
        text += tr("  - %1\n").arg(QString::fromStdString(report.tips[i]));
    }
    if (!report.wrongAnswers.empty()) {
        text += tr("\nQuestions to review:\n");
        for (std::size_t i = 0; i < report.wrongAnswers.size(); ++i) {
            text += tr("  - %1\n").arg(QString::fromStdString(report.wrongAnswers[i].questionId.get()));
        }
    }
    reportText_->setPlainText(text);
    stack_->setCurrentIndex(2);
}

} // namespace gui

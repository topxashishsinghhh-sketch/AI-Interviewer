// OOP: GUI Presentation Layer (daily challenge screen)
#include "gui/DailyChallengeWidget.h"
#include "gui/SessionContext.h"
#include "core/User.h"
#include "core/InterviewSession.h"
#include "core/QuestionSelectionStrategy.h"
#include "interview/InterviewService.h"
#include "interview/SessionFactory.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>

namespace gui {

DailyChallengeWidget::DailyChallengeWidget(SessionContext& context, QWidget* parent)
    : QWidget(parent)
    , context_(context) {
    buildUi();
    setBusy(false);
}

void DailyChallengeWidget::buildUi() {
    QVBoxLayout* root = new QVBoxLayout(this);
    
    headerLabel_ = new QLabel(tr("Today's challenge"), this);
    headerLabel_->setStyleSheet(QStringLiteral("font-weight: bold;"));
    root->addWidget(headerLabel_);
    
    questionText_ = new QTextEdit(this);
    questionText_->setReadOnly(true);
    questionText_->setPlaceholderText(tr("Press \"Load today's challenge\" to begin."));
    root->addWidget(questionText_, 1);
    
    answerEdit_ = new QLineEdit(this);
    answerEdit_->setPlaceholderText(tr("Type your answer, or press Skip"));
    root->addWidget(answerEdit_);
    
    QHBoxLayout* actions = new QHBoxLayout;
    loadButton_ = new QPushButton(tr("Load today's challenge"), this);
    submitButton_ = new QPushButton(tr("Submit"), this);
    skipButton_ = new QPushButton(tr("Skip"), this);
    actions->addWidget(loadButton_, 1);
    actions->addWidget(submitButton_, 1);
    actions->addWidget(skipButton_);
    root->addLayout(actions);
    
    resultText_ = new QTextEdit(this);
    resultText_->setReadOnly(true);
    root->addWidget(resultText_);
    
    connect(loadButton_, &QPushButton::clicked, this, &DailyChallengeWidget::onLoad);
    connect(submitButton_, &QPushButton::clicked, this, &DailyChallengeWidget::onSubmit);
    connect(skipButton_, &QPushButton::clicked, this, &DailyChallengeWidget::onSkip);
}

void DailyChallengeWidget::setUser(const core::UserPtr& user) {
    user_ = user;
    loaded_ = false;
    questionText_->clear();
    resultText_->clear();
    answerEdit_->clear();
}

void DailyChallengeWidget::setBusy(bool busy) {
    submitButton_->setEnabled(busy);
    skipButton_->setEnabled(busy);
    answerEdit_->setEnabled(busy);
    loadButton_->setEnabled(!busy);
}

void DailyChallengeWidget::onLoad() {
    if (!user_) return;
    
    const std::vector<std::shared_ptr<core::Course>> courses = context_.courses().courses();
    if (courses.empty()) {
        headerLabel_->setText(tr("No courses available for a daily challenge."));
        return;
    }
    
    core::Optional<core::DailyChallenge> challenge = context_.progress().ensureDailyChallenge(courses[0]->id());
    if (!challenge.has_value()) {
        headerLabel_->setText(tr("No daily challenge is available today."));
        return;
    }
    
    questionId_ = challenge.value().questionId;
    bonusPoints_ = challenge.value().bonusPoints;
    
    core::QuestionPtr question = context_.questionRepo()->findById(questionId_);
    if (!question) {
        headerLabel_->setText(tr("Challenge question is unavailable."));
        return;
    }
    
    headerLabel_->setText(tr("Challenge for %1 - bonus %2 points")
        .arg(QString::fromStdString(context_.progress().todayKey()))
        .arg(bonusPoints_));
    questionText_->setPlainText(QString::fromStdString(question->display()));
    loaded_ = true;
    setBusy(true);
    answerEdit_->setFocus();
}

void DailyChallengeWidget::onSubmit() {
    if (!loaded_ || !user_) return;
    
    const std::string answer = answerEdit_->text().toStdString();
    if (answer.empty()) return;
    
    std::unique_ptr<interview::InterviewService> service = interview::SessionFactory::createDaily(
        context_.questionRepo(), context_.attemptRepo(), context_.feedbackGenerator());
    
    core::QuestionPtr question = context_.questionRepo()->findById(questionId_);
    if (!question) {
        resultText_->setPlainText(tr("Challenge question is unavailable."));
        return;
    }
    
    auto session = service->startSession(user_, core::InterviewSession::Mode::Daily,
                                         question->subject(), "");
    if (!session) {
        resultText_->setPlainText(tr("Could not start the challenge session."));
        return;
    }
    
    const int timeTaken = core::timeLimitMinutes(question->difficulty()) * 60;
    service->submitAnswer(session, answer, timeTaken);
    core::FeedbackReport report = service->finishSession(session);
    
    const double withBonus = report.overallScore + (bonusPoints_ / 100.0);
    context_.progress().finalizeSession(user_, report);
    
    QString tips;
    for (std::size_t i = 0; i < report.tips.size(); ++i) {
        tips += QString::fromStdString(report.tips[i]) + QStringLiteral("\n");
    }
    if (tips.isEmpty()) {
        tips = tr("(no tips)");
    }
    
    resultText_->setPlainText(tr("Score: %1%\nWith bonus: %2%\n\nTips:\n%3")
        .arg(QString::number(report.overallScore * 100.0, 'f', 1))
        .arg(QString::number(withBonus * 100.0, 'f', 1))
        .arg(tips));
    
    setBusy(false);
    loaded_ = false;
    emit challengeCompleted();
}

void DailyChallengeWidget::onSkip() {
    if (!loaded_ || !user_) return;
    
    std::unique_ptr<interview::InterviewService> service = interview::SessionFactory::createDaily(
        context_.questionRepo(), context_.attemptRepo(), context_.feedbackGenerator());
    
    core::QuestionPtr question = context_.questionRepo()->findById(questionId_);
    if (!question) {
        resultText_->setPlainText(tr("Challenge question is unavailable."));
        return;
    }
    
    auto session = service->startSession(user_, core::InterviewSession::Mode::Daily,
                                         question->subject(), "");
    if (!session) {
        resultText_->setPlainText(tr("Could not start the challenge session."));
        return;
    }
    
    const int timeTaken = core::timeLimitMinutes(question->difficulty()) * 60;
    service->skipQuestion(session, timeTaken);
    core::FeedbackReport report = service->finishSession(session);
    context_.progress().finalizeSession(user_, report);
    
    resultText_->setPlainText(tr("Skipped. The bonus is forfeited.\nScore: %1%")
        .arg(QString::number(report.overallScore * 100.0, 'f', 1)));
    
    setBusy(false);
    loaded_ = false;
    emit challengeCompleted();
}

} // namespace gui

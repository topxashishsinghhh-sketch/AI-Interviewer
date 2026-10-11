// OOP: GUI Presentation Layer (interview screen)
#pragma once
#include <QWidget>
#include <memory>
#include "core/User.h"
#include "core/FeedbackGenerator.h"
#include "core/InterviewSession.h"
#include "interview/InterviewService.h"

QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
class QTextEdit;
class QLineEdit;
class QPushButton;
class QStackedWidget;
QT_END_NAMESPACE

namespace core {
    class InterviewSession;
}

namespace gui {

class SessionContext;

// Drives a full interview: course/subject/mode selection, question answering,
// and the final report. All business logic stays in the services.
class InterviewWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit InterviewWidget(SessionContext& context, QWidget* parent = nullptr);
    void setUser(const core::UserPtr& user);
    
signals:
    void sessionFinished();
    
private slots:
    void onCourseChanged(int index);
    void onStart();
    void onSubmit();
    void onSkip();
    void onFinish();
    
private:
    void buildUi();
    void refreshSubjects();
    void showCurrentQuestion();
    void showReport(const core::FeedbackReport& report);
    void setSetupMode(bool setup);
    
    SessionContext& context_;
    core::UserPtr user_;
    std::shared_ptr<core::InterviewSession> session_;
    std::unique_ptr<interview::InterviewService> service_;
    core::InterviewSession::Mode mode_ = core::InterviewSession::Mode::Practice;
    int timeTakenSeconds_ = 0;
    
    QStackedWidget* stack_ = nullptr;
    
    QComboBox* courseCombo_ = nullptr;
    QComboBox* subjectCombo_ = nullptr;
    QComboBox* modeCombo_ = nullptr;
    QPushButton* startButton_ = nullptr;
    
    QLabel* progressLabel_ = nullptr;
    QLabel* metaLabel_ = nullptr;
    QTextEdit* questionText_ = nullptr;
    QLineEdit* answerEdit_ = nullptr;
    QPushButton* submitButton_ = nullptr;
    QPushButton* skipButton_ = nullptr;
    QPushButton* finishButton_ = nullptr;
    QTextEdit* reportText_ = nullptr;
    QPushButton* againButton_ = nullptr;
};

} // namespace gui

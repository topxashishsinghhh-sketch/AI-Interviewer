// OOP: GUI Presentation Layer (daily challenge screen)
#pragma once
#include <QWidget>
#include <memory>
#include "core/User.h"
#include "core/StrongId.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QTextEdit;
class QLineEdit;
class QPushButton;
QT_END_NAMESPACE

namespace core {
    class User;
}

namespace gui {

class SessionContext;

class DailyChallengeWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit DailyChallengeWidget(SessionContext& context, QWidget* parent = nullptr);
    void setUser(const core::UserPtr& user);
    
signals:
    void challengeCompleted();
    
private slots:
    void onLoad();
    void onSubmit();
    void onSkip();
    
private:
    void buildUi();
    void setBusy(bool busy);
    
    SessionContext& context_;
    core::UserPtr user_;
    core::QuestionId questionId_;
    int bonusPoints_ = 0;
    bool loaded_ = false;
    
    QLabel* headerLabel_ = nullptr;
    QTextEdit* questionText_ = nullptr;
    QLineEdit* answerEdit_ = nullptr;
    QPushButton* loadButton_ = nullptr;
    QPushButton* submitButton_ = nullptr;
    QPushButton* skipButton_ = nullptr;
    QTextEdit* resultText_ = nullptr;
};

} // namespace gui

// OOP: GUI Presentation Layer (main window / navigation)
#pragma once
#include <QMainWindow>
#include <memory>
#include "core/User.h"

QT_BEGIN_NAMESPACE
class QStackedWidget;
class QListWidget;
class QLabel;
class QPushButton;
QT_END_NAMESPACE

namespace core {
    class User;
}

namespace gui {

class SessionContext;
class InterviewWidget;
class DailyChallengeWidget;
class ProfileWidget;
class LeaderboardWidget;
class AdminWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
    MainWindow(SessionContext& context, QWidget* parent = nullptr);
    
public slots:
    void showUser(const core::UserPtr& user);
    void showLogin();
    void refreshCurrentScreen();
    
private:
    void buildUi();
    void showScreen(int index);
    
    SessionContext& context_;
    core::UserPtr user_;
    
    QStackedWidget* stack_ = nullptr;
    QListWidget* nav_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QPushButton* logoutButton_ = nullptr;
    
    InterviewWidget* interviewWidget_ = nullptr;
    DailyChallengeWidget* dailyWidget_ = nullptr;
    ProfileWidget* profileWidget_ = nullptr;
    LeaderboardWidget* leaderboardWidget_ = nullptr;
    AdminWidget* adminWidget_ = nullptr;
};

} // namespace gui

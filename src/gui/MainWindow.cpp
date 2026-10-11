// OOP: GUI Presentation Layer (main window / navigation)
#include "gui/MainWindow.h"
#include "gui/SessionContext.h"
#include "gui/LoginDialog.h"
#include "gui/InterviewWidget.h"
#include "gui/DailyChallengeWidget.h"
#include "gui/ProfileWidget.h"
#include "gui/LeaderboardWidget.h"
#include "gui/AdminWidget.h"
#include "core/User.h"

#include <QStackedWidget>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QListWidgetItem>

namespace gui {

MainWindow::MainWindow(SessionContext& context, QWidget* parent)
    : QMainWindow(parent)
    , context_(context) {
    setWindowTitle(tr("AI Interviewer"));
    resize(1024, 720);
    buildUi();
}

void MainWindow::buildUi() {
    QWidget* central = new QWidget(this);
    QHBoxLayout* root = new QHBoxLayout(central);
    
    nav_ = new QListWidget(central);
    nav_->setFixedWidth(190);
    nav_->setStyleSheet(QStringLiteral("QListWidget { font-size: 14px; }"));
    root->addWidget(nav_);
    
    QWidget* content = new QWidget(central);
    QVBoxLayout* contentLayout = new QVBoxLayout(content);
    
    QHBoxLayout* header = new QHBoxLayout;
    statusLabel_ = new QLabel(tr("Not signed in"), content);
    statusLabel_->setStyleSheet(QStringLiteral("font-weight: bold;"));
    header->addWidget(statusLabel_);
    header->addStretch(1);
    logoutButton_ = new QPushButton(tr("Sign out"), content);
    header->addWidget(logoutButton_);
    contentLayout->addLayout(header);
    
    stack_ = new QStackedWidget(content);
    interviewWidget_ = new InterviewWidget(context_, content);
    dailyWidget_ = new DailyChallengeWidget(context_, content);
    profileWidget_ = new ProfileWidget(context_, content);
    leaderboardWidget_ = new LeaderboardWidget(context_, content);
    adminWidget_ = new AdminWidget(context_, content);
    stack_->addWidget(interviewWidget_);
    stack_->addWidget(dailyWidget_);
    stack_->addWidget(profileWidget_);
    stack_->addWidget(leaderboardWidget_);
    stack_->addWidget(adminWidget_);
    contentLayout->addWidget(stack_, 1);
    
    root->addWidget(content, 1);
    setCentralWidget(central);
    
    QStringList items;
    items << tr("Interview") << tr("Daily challenge") << tr("Profile & streak")
          << tr("Leaderboard") << tr("Admin");
    for (const QString& item : items) {
        nav_->addItem(item);
    }
    
    connect(nav_, &QListWidget::currentRowChanged, this, &MainWindow::showScreen);
    connect(logoutButton_, &QPushButton::clicked, this, &MainWindow::showLogin);
}

void MainWindow::showScreen(int index) {
    if (index < 0 || index >= stack_->count()) return;
    stack_->setCurrentIndex(index);
    refreshCurrentScreen();
}

void MainWindow::refreshCurrentScreen() {
    const int index = stack_->currentIndex();
    if (index == 1) {
        dailyWidget_->setUser(user_);
    } else if (index == 2) {
        profileWidget_->refresh();
    } else if (index == 3) {
        leaderboardWidget_->refresh();
    } else if (index == 4) {
        adminWidget_->refresh();
    }
}

void MainWindow::showUser(const core::UserPtr& user) {
    user_ = user;
    if (!user_) return;
    
    statusLabel_->setText(tr("Signed in as %1 (%2)")
        .arg(QString::fromStdString(user_->name()))
        .arg(user_->isAdmin() ? tr("admin") : tr("student")));
    
    interviewWidget_->setUser(user_);
    dailyWidget_->setUser(user_);
    profileWidget_->setUser(user_);
    leaderboardWidget_->setUser(user_);
    
    // Admins get the admin screen first, students land on the interview screen.
    const int startIndex = user_->isAdmin() ? 4 : 0;
    nav_->setCurrentRow(startIndex);
    showScreen(startIndex);
}

void MainWindow::showLogin() {
    hide();
    // The event loop keeps running; closing the window signals a sign-out.
    close();
}

} // namespace gui

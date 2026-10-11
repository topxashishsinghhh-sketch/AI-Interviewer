// OOP: GUI Presentation Layer (admin screen)
#include "gui/AdminWidget.h"
#include "gui/SessionContext.h"
#include "admin/AdminService.h"
#include "core/StrongId.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QHeaderView>
#include <QMessageBox>

namespace gui {

AdminWidget::AdminWidget(SessionContext& context, QWidget* parent)
    : QWidget(parent)
    , context_(context) {
    buildUi();
    refresh();
}

void AdminWidget::buildUi() {
    QVBoxLayout* root = new QVBoxLayout(this);
    
    statsLabel_ = new QLabel(this);
    statsLabel_->setStyleSheet(QStringLiteral("font-weight: bold;"));
    root->addWidget(statsLabel_);
    
    QLabel* usersTitle = new QLabel(tr("Users"), this);
    usersTitle->setStyleSheet(QStringLiteral("font-weight: bold;"));
    root->addWidget(usersTitle);
    userTable_ = new QTableWidget(this);
    userTable_->setColumnCount(5);
    userTable_->setHorizontalHeaderLabels({tr("Email"), tr("Name"), tr("Role"),
                                           tr("Solved"), tr("Streak")});
    userTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    userTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    userTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    userTable_->setAlternatingRowColors(true);
    root->addWidget(userTable_, 1);
    
    QHBoxLayout* actions = new QHBoxLayout;
    actions->addWidget(new QLabel(tr("Badge:"), this));
    badgeEdit_ = new QLineEdit(this);
    badgeEdit_->setPlaceholderText(tr("e.g. Problem Solver"));
    actions->addWidget(badgeEdit_);
    QPushButton* award = new QPushButton(tr("Award"), this);
    QPushButton* reset = new QPushButton(tr("Reset streak"), this);
    QPushButton* remove = new QPushButton(tr("Delete user"), this);
    remove->setStyleSheet(QStringLiteral("color: #b00020;"));
    actions->addWidget(award);
    actions->addWidget(reset);
    actions->addWidget(remove);
    root->addLayout(actions);
    
    QLabel* questionsTitle = new QLabel(tr("Question bank"), this);
    questionsTitle->setStyleSheet(QStringLiteral("font-weight: bold;"));
    root->addWidget(questionsTitle);
    questionTable_ = new QTableWidget(this);
    questionTable_->setColumnCount(5);
    questionTable_->setHorizontalHeaderLabels({tr("Subject"), tr("Easy"), tr("Medium"),
                                               tr("Hard"), tr("Total")});
    questionTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    questionTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    questionTable_->setAlternatingRowColors(true);
    root->addWidget(questionTable_);
    
    connect(award, &QPushButton::clicked, this, &AdminWidget::onAwardBadge);
    connect(reset, &QPushButton::clicked, this, &AdminWidget::onResetStreak);
    connect(remove, &QPushButton::clicked, this, &AdminWidget::onDeleteUser);
}

int AdminWidget::selectedRow() const {
    const QList<QTableWidgetItem*> selected = userTable_->selectedItems();
    if (selected.isEmpty()) return -1;
    return selected.first()->row();
}

void AdminWidget::refresh() {
    admin::PlatformStats stats = context_.admin().platformStats();
    statsLabel_->setText(tr("Users: %1 (students %2, admins %3) | Questions: %4 | "
                            "Attempts: %5 | Average score: %6 | Active streaks: %7")
        .arg(stats.totalUsers).arg(stats.students).arg(stats.admins)
        .arg(stats.totalQuestions).arg(stats.totalAttempts)
        .arg(QString::number(stats.averageScore, 'f', 2))
        .arg(stats.activeStreaks));
    
    std::vector<admin::UserSummary> users = context_.admin().userSummaries();
    userTable_->setRowCount(static_cast<int>(users.size()));
    for (std::size_t i = 0; i < users.size(); ++i) {
        const core::UserPtr& u = users[i].user;
        userTable_->setItem(static_cast<int>(i), 0,
            new QTableWidgetItem(QString::fromStdString(u->email())));
        userTable_->setItem(static_cast<int>(i), 1,
            new QTableWidgetItem(QString::fromStdString(u->name())));
        userTable_->setItem(static_cast<int>(i), 2,
            new QTableWidgetItem(u->isAdmin() ? tr("Admin") : tr("Student")));
        userTable_->setItem(static_cast<int>(i), 3,
            new QTableWidgetItem(QString::number(users[i].solved)));
        userTable_->setItem(static_cast<int>(i), 4,
            new QTableWidgetItem(QString::number(users[i].streak)));
    }
    
    std::vector<admin::QuestionStats> questions = context_.admin().questionStats();
    questionTable_->setRowCount(static_cast<int>(questions.size()));
    for (std::size_t i = 0; i < questions.size(); ++i) {
        const admin::QuestionStats& q = questions[i];
        questionTable_->setItem(static_cast<int>(i), 0,
            new QTableWidgetItem(QString::fromStdString(q.subject)));
        questionTable_->setItem(static_cast<int>(i), 1,
            new QTableWidgetItem(QString::number(q.easy)));
        questionTable_->setItem(static_cast<int>(i), 2,
            new QTableWidgetItem(QString::number(q.medium)));
        questionTable_->setItem(static_cast<int>(i), 3,
            new QTableWidgetItem(QString::number(q.hard)));
        questionTable_->setItem(static_cast<int>(i), 4,
            new QTableWidgetItem(QString::number(q.total)));
    }
}

void AdminWidget::onAwardBadge() {
    const int row = selectedRow();
    if (row < 0) {
        QMessageBox::information(this, tr("Award badge"), tr("Select a user first."));
        return;
    }
    const std::string badge = badgeEdit_->text().trimmed().toStdString();
    if (badge.empty()) {
        QMessageBox::information(this, tr("Award badge"), tr("Enter a badge name."));
        return;
    }
    
    std::vector<admin::UserSummary> users = context_.admin().userSummaries();
    if (row >= static_cast<int>(users.size())) return;
    const core::UserId id = users[static_cast<std::size_t>(row)].user->id();
    
    QMessageBox::information(this, tr("Award badge"),
        context_.admin().awardBadge(id, badge)
            ? tr("Badge awarded.")
            : tr("Only students can receive badges."));
    refresh();
}

void AdminWidget::onResetStreak() {
    const int row = selectedRow();
    if (row < 0) {
        QMessageBox::information(this, tr("Reset streak"), tr("Select a user first."));
        return;
    }
    
    std::vector<admin::UserSummary> users = context_.admin().userSummaries();
    if (row >= static_cast<int>(users.size())) return;
    const core::UserId id = users[static_cast<std::size_t>(row)].user->id();
    
    QMessageBox::information(this, tr("Reset streak"),
        context_.admin().resetStreak(id) ? tr("Streak reset.") : tr("Could not reset streak."));
    refresh();
}

void AdminWidget::onDeleteUser() {
    const int row = selectedRow();
    if (row < 0) {
        QMessageBox::information(this, tr("Delete user"), tr("Select a user first."));
        return;
    }
    
    std::vector<admin::UserSummary> users = context_.admin().userSummaries();
    if (row >= static_cast<int>(users.size())) return;
    const core::UserPtr target = users[static_cast<std::size_t>(row)].user;
    
    if (QMessageBox::question(this, tr("Delete user"),
            tr("Delete %1?").arg(QString::fromStdString(target->email())),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) {
        return;
    }
    
    QMessageBox::information(this, tr("Delete user"),
        context_.admin().removeUser(target->id()) ? tr("User deleted.") : tr("Could not delete user."));
    refresh();
}

} // namespace gui

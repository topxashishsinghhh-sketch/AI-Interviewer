// OOP: GUI Presentation Layer (profile and streak calendar)
#include "gui/ProfileWidget.h"
#include "gui/SessionContext.h"
#include "core/User.h"
#include "core/Student.h"
#include "progress/ProgressService.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPainter>
#include <QDate>
#include <QFont>
#include <QLinearGradient>

namespace gui {

StreakCalendar::StreakCalendar(QWidget* parent)
    : QWidget(parent) {
    setMinimumHeight(210);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void StreakCalendar::setActivityDate(const QString& isoDate) {
    activityDate_ = isoDate;
    update();
}

void StreakCalendar::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    const QRect area = rect().adjusted(8, 8, -8, -8);
    painter.fillRect(area, QColor(250, 250, 252));
    painter.setPen(QPen(QColor(210, 210, 218), 1));
    painter.drawRoundedRect(area, 8, 8);
    
    const QDate today = QDate::currentDate();
    const QDate firstOfMonth(today.year(), today.month(), 1);
    
    QFont headerFont = font();
    headerFont.setBold(true);
    painter.setFont(headerFont);
    painter.setPen(QColor(40, 40, 50));
    painter.drawText(area.adjusted(12, 8, -12, -area.height() + 34),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QLocale().monthName(today.month(), QLocale::LongFormat) + " " +
                     QString::number(today.year()));
    
    const QStringList dayNames = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};
    const int cols = 7;
    const int rows = 6;
    const int headerHeight = 36;
    const int cellWidth = (area.width() - 24) / cols;
    const int cellHeight = (area.height() - headerHeight - 24) / rows;
    
    painter.setPen(QColor(120, 120, 130));
    QFont small = font();
    small.setPointSizeF(small.pointSizeF() - 1);
    painter.setFont(small);
    for (int c = 0; c < cols; ++c) {
        const QRect cell(area.left() + 12 + c * cellWidth,
                         area.top() + headerHeight,
                         cellWidth, 18);
        painter.drawText(cell, Qt::AlignCenter, dayNames.at(c));
    }
    
    const int firstColumn = firstOfMonth.dayOfWeek() % 7; // Sunday = 0
    const int daysInMonth = firstOfMonth.daysInMonth();
    
    QDate activeDate = QDate::fromString(activityDate_, QStringLiteral("yyyy-MM-dd"));
    
    for (int day = 1; day <= daysInMonth; ++day) {
        const QDate date(today.year(), today.month(), day);
        const int index = firstColumn + day - 1;
        const int row = index / cols;
        const int col = index % cols;
        
        QRect cell(area.left() + 12 + col * cellWidth,
                   area.top() + headerHeight + 22 + row * cellHeight,
                   cellWidth - 4, cellHeight - 6);
        cell.adjust(2, 2, -2, -2);
        
        if (date == activeDate) {
            QLinearGradient gradient(cell.topLeft(), cell.bottomRight());
            gradient.setColorAt(0.0, QColor(66, 133, 244));
            gradient.setColorAt(1.0, QColor(21, 101, 192));
            painter.setBrush(gradient);
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(cell, 4, 4);
            painter.setPen(Qt::white);
        } else if (date == today) {
            painter.setBrush(QColor(232, 240, 254));
            painter.setPen(QPen(QColor(66, 133, 244), 1));
            painter.drawRoundedRect(cell, 4, 4);
            painter.setPen(QColor(25, 90, 180));
        } else {
            painter.setPen(QColor(90, 90, 100));
        }
        
        painter.setFont(small);
        painter.drawText(cell, Qt::AlignCenter, QString::number(day));
    }
    
    painter.setPen(QColor(110, 110, 120));
    QString legend = QStringLiteral("Active: %1");
    legend = legend.arg(activityDate_.isEmpty() ? QStringLiteral("none yet") : activityDate_);
    painter.drawText(area.adjusted(12, -6, -12, -6), Qt::AlignRight | Qt::AlignBottom, legend);
}

ProfileWidget::ProfileWidget(SessionContext& context, QWidget* parent)
    : QWidget(parent)
    , context_(context) {
    buildUi();
}

void ProfileWidget::buildUi() {
    QVBoxLayout* root = new QVBoxLayout(this);
    
    QFormLayout* form = new QFormLayout;
    nameValue_ = new QLabel(this);
    emailValue_ = new QLabel(this);
    collegeValue_ = new QLabel(this);
    branchValue_ = new QLabel(this);
    for (QLabel* label : {nameValue_, emailValue_, collegeValue_, branchValue_}) {
        label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    }
    form->addRow(tr("Name:"), nameValue_);
    form->addRow(tr("Email:"), emailValue_);
    form->addRow(tr("College:"), collegeValue_);
    form->addRow(tr("Branch:"), branchValue_);
    root->addLayout(form);
    
    statsLabel_ = new QLabel(this);
    statsLabel_->setStyleSheet(QStringLiteral("font-weight: bold;"));
    root->addWidget(statsLabel_);
    
    badgesLabel_ = new QLabel(this);
    badgesLabel_->setWordWrap(true);
    root->addWidget(badgesLabel_);
    
    QLabel* calendarTitle = new QLabel(tr("Streak calendar"), this);
    calendarTitle->setStyleSheet(QStringLiteral("font-weight: bold;"));
    root->addWidget(calendarTitle);
    calendar_ = new StreakCalendar(this);
    root->addWidget(calendar_, 1);
    
    root->addStretch(1);
}

void ProfileWidget::setUser(const core::UserPtr& user) {
    user_ = user;
    refresh();
}

void ProfileWidget::refresh() {
    if (!user_) return;
    
    nameValue_->setText(QString::fromStdString(user_->name()));
    emailValue_->setText(QString::fromStdString(user_->email()));
    collegeValue_->setText(QString::fromStdString(user_->college()));
    branchValue_->setText(QString::fromStdString(user_->branch()));
    
    QString stats;
    QString badges = tr("Badges: none");
    
    if (auto student = std::dynamic_pointer_cast<core::Student>(user_)) {
        stats = tr("Current streak: %1 days | Longest: %2 | Solved: %3 | Attempts: %4")
            .arg(student->streak())
            .arg(student->longestStreak())
            .arg(student->totalSolved())
            .arg(student->totalAttempts());
        
        if (!student->badges().empty()) {
            QStringList list;
            for (const std::string& badge : student->badges()) {
                list << QString::fromStdString(badge);
            }
            badges = tr("Badges: %1").arg(list.join(", "));
        }
        
        calendar_->setActivityDate(QString::fromStdString(student->lastActivityDate()));
    } else {
        stats = tr("Administrator account");
        calendar_->setActivityDate(QString());
    }
    
    statsLabel_->setText(stats);
    badgesLabel_->setText(badges);
    
    progress::LeaderboardQuery query;
    core::Optional<core::ScoreboardEntry> entry = context_.progress().userEntry(user_->id(), query);
    if (entry.has_value()) {
        statsLabel_->setText(stats + tr("  |  Rank #%1 (score %2)")
            .arg(entry.value().rank)
            .arg(QString::number(entry.value().score, 'f', 1)));
    }
}

} // namespace gui

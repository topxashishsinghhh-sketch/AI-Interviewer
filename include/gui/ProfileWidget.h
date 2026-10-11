// OOP: GUI Presentation Layer (profile and streak calendar)
#pragma once
#include <QWidget>
#include <memory>
#include "core/User.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QGridLayout;
class QTextEdit;
QT_END_NAMESPACE

namespace core {
    class User;
}

namespace gui {

class SessionContext;

// Draws a month grid and highlights days the student was active. Activity is
// derived from the streak/last-activity date reported by the user object.
class StreakCalendar : public QWidget {
    Q_OBJECT
    
public:
    explicit StreakCalendar(QWidget* parent = nullptr);
    void setActivityDate(const QString& isoDate);
    
protected:
    void paintEvent(QPaintEvent* event) override;
    
private:
    QString activityDate_;
};

class ProfileWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit ProfileWidget(SessionContext& context, QWidget* parent = nullptr);
    void setUser(const core::UserPtr& user);
    
public slots:
    void refresh();
    
private:
    void buildUi();
    
    SessionContext& context_;
    core::UserPtr user_;
    QLabel* nameValue_ = nullptr;
    QLabel* emailValue_ = nullptr;
    QLabel* collegeValue_ = nullptr;
    QLabel* branchValue_ = nullptr;
    QLabel* statsLabel_ = nullptr;
    QLabel* badgesLabel_ = nullptr;
    StreakCalendar* calendar_ = nullptr;
};

} // namespace gui

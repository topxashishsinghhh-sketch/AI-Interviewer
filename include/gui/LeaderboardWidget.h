// OOP: GUI Presentation Layer (leaderboard screen)
#pragma once
#include <QWidget>
#include <memory>
#include "core/User.h"

QT_BEGIN_NAMESPACE
class QComboBox;
class QTableWidget;
class QLabel;
QT_END_NAMESPACE

namespace gui {

class SessionContext;

class LeaderboardWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit LeaderboardWidget(SessionContext& context, QWidget* parent = nullptr);
    void setUser(const core::UserPtr& user);
    
public slots:
    void refresh();
    
private slots:
    void onFilterChanged();
    
private:
    void buildUi();
    
    SessionContext& context_;
    core::UserPtr user_;
    QComboBox* scopeCombo_ = nullptr;
    QComboBox* windowCombo_ = nullptr;
    QComboBox* rankingCombo_ = nullptr;
    QTableWidget* table_ = nullptr;
    QLabel* rankLabel_ = nullptr;
};

} // namespace gui

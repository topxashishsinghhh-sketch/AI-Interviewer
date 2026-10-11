// OOP: GUI Presentation Layer (admin screen)
#pragma once
#include <QWidget>

QT_BEGIN_NAMESPACE
class QLabel;
class QTableWidget;
class QPushButton;
class QLineEdit;
class QSpinBox;
QT_END_NAMESPACE

namespace gui {

class SessionContext;

class AdminWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit AdminWidget(SessionContext& context, QWidget* parent = nullptr);
    
public slots:
    void refresh();
    
private slots:
    void onAwardBadge();
    void onResetStreak();
    void onDeleteUser();
    
private:
    void buildUi();
    int selectedRow() const;
    
    SessionContext& context_;
    QLabel* statsLabel_ = nullptr;
    QTableWidget* userTable_ = nullptr;
    QTableWidget* questionTable_ = nullptr;
    QLineEdit* badgeEdit_ = nullptr;
};

} // namespace gui

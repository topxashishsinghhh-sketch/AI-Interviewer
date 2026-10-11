// OOP: GUI Presentation Layer (authentication screen)
#pragma once
#include <QDialog>
#include <memory>
#include "core/User.h"

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPushButton;
class QComboBox;
class QLabel;
QT_END_NAMESPACE

namespace gui {

class SessionContext;

class LoginDialog : public QDialog {
    Q_OBJECT
    
public:
    explicit LoginDialog(SessionContext& context, QWidget* parent = nullptr);
    
    core::UserPtr user() const { return user_; }
    
private slots:
    void onSubmit();
    void onModeChanged(int index);
    
private:
    void buildUi();
    void setStatus(const QString& message, bool isError);
    
    SessionContext& context_;
    core::UserPtr user_;
    
    QComboBox* modeCombo_ = nullptr;
    QLineEdit* emailEdit_ = nullptr;
    QLineEdit* passwordEdit_ = nullptr;
    QLineEdit* nameEdit_ = nullptr;
    QLineEdit* collegeEdit_ = nullptr;
    QLineEdit* branchEdit_ = nullptr;
    QPushButton* submitButton_ = nullptr;
    QLabel* statusLabel_ = nullptr;
};

} // namespace gui

// OOP: GUI Presentation Layer (authentication screen)
#include "gui/LoginDialog.h"
#include "gui/SessionContext.h"
#include "core/User.h"

#include <QFormLayout>
#include <QVBoxLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>

namespace gui {

LoginDialog::LoginDialog(SessionContext& context, QWidget* parent)
    : QDialog(parent)
    , context_(context) {
    setWindowTitle(tr("AI Interviewer - Sign in"));
    setMinimumWidth(420);
    buildUi();
    onModeChanged(0);
}

void LoginDialog::buildUi() {
    QVBoxLayout* root = new QVBoxLayout(this);
    
    QFormLayout* form = new QFormLayout;
    
    modeCombo_ = new QComboBox(this);
    modeCombo_->addItem(tr("Login"));
    modeCombo_->addItem(tr("Register"));
    form->addRow(tr("Mode:"), modeCombo_);
    
    emailEdit_ = new QLineEdit(this);
    emailEdit_->setPlaceholderText(tr("you@example.com"));
    form->addRow(tr("Email:"), emailEdit_);
    
    passwordEdit_ = new QLineEdit(this);
    passwordEdit_->setEchoMode(QLineEdit::Password);
    form->addRow(tr("Password:"), passwordEdit_);
    
    nameEdit_ = new QLineEdit(this);
    form->addRow(tr("Name:"), nameEdit_);
    
    collegeEdit_ = new QLineEdit(this);
    form->addRow(tr("College:"), collegeEdit_);
    
    branchEdit_ = new QLineEdit(this);
    form->addRow(tr("Branch:"), branchEdit_);
    
    root->addLayout(form);
    
    statusLabel_ = new QLabel(this);
    statusLabel_->setWordWrap(true);
    root->addWidget(statusLabel_);
    
    submitButton_ = new QPushButton(tr("Sign in"), this);
    submitButton_->setDefault(true);
    root->addWidget(submitButton_);
    
    connect(modeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LoginDialog::onModeChanged);
    connect(submitButton_, &QPushButton::clicked, this, &LoginDialog::onSubmit);
}

void LoginDialog::onModeChanged(int index) {
    const bool registering = (index == 1);
    nameEdit_->setVisible(registering);
    collegeEdit_->setVisible(registering);
    branchEdit_->setVisible(registering);
    submitButton_->setText(registering ? tr("Create account") : tr("Sign in"));
    setStatus(QString(), false);
    adjustSize();
}

void LoginDialog::setStatus(const QString& message, bool isError) {
    statusLabel_->setText(message);
    statusLabel_->setStyleSheet(isError ? QStringLiteral("color: #b00020;")
                                        : QStringLiteral("color: #1b5e20;"));
}

void LoginDialog::onSubmit() {
    const std::string email = emailEdit_->text().trimmed().toStdString();
    const std::string password = passwordEdit_->text().toStdString();
    
    if (email.empty() || password.empty()) {
        setStatus(tr("Email and password are required."), true);
        return;
    }
    
    if (modeCombo_->currentIndex() == 1) {
        const std::string name = nameEdit_->text().trimmed().toStdString();
        const std::string college = collegeEdit_->text().trimmed().toStdString();
        const std::string branch = branchEdit_->text().trimmed().toStdString();
        
        if (name.empty()) {
            setStatus(tr("Name is required."), true);
            return;
        }
        
        auth::AuthResult result = context_.auth().registerUser(
            email, password, name, core::UserRole::Student,
            college.empty() ? "Unspecified" : college,
            branch.empty() ? "Unspecified" : branch);
        
        if (!result.success) {
            setStatus(QString::fromStdString(result.message), true);
            return;
        }
        
        setStatus(tr("Registered. You can now sign in."), false);
        modeCombo_->setCurrentIndex(0);
        passwordEdit_->clear();
        return;
    }
    
    auth::AuthResult result = context_.auth().login(email, password);
    if (!result.success) {
        setStatus(QString::fromStdString(result.message), true);
        return;
    }
    
    user_ = result.user;
    accept();
}

} // namespace gui

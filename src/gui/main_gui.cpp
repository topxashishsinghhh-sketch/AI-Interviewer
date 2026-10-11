// OOP: Entry point for the Qt GUI
#include "gui/SessionContext.h"
#include "gui/LoginDialog.h"
#include "gui/MainWindow.h"

#include <QApplication>
#include <QMessageBox>
#include <QString>
#include <QStringList>

#include <cstring>
#include <string>

namespace {

std::string argValue(int argc, char* argv[], const std::string& name,
                     const std::string& fallback) {
    for (int i = 1; i < argc; ++i) {
        if (name == argv[i] && i + 1 < argc) {
            return std::string(argv[i + 1]);
        }
    }
    return fallback;
}

bool hasFlag(int argc, char* argv[], const std::string& name) {
    for (int i = 1; i < argc; ++i) {
        if (name == argv[i]) return true;
    }
    return false;
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("AI Interviewer"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));
    
    if (hasFlag(argc, argv, "--version")) {
        QApplication::setApplicationVersion(QStringLiteral("1.0.0"));
        // Version is shown by the main window title; exit after printing it.
        return 0;
    }
    
    const std::string dataDir = argValue(argc, argv, "--data-dir", "./data");
    const std::string mongoDb = argValue(argc, argv, "--db", "ai_interviewer");
    const bool useMongo = hasFlag(argc, argv, "--storage-mongo") ||
                          (argValue(argc, argv, "--storage", "json") == "mongo");
    
    if (hasFlag(argc, argv, "--help")) {
        const QString help =
            QStringLiteral("AI Interviewer GUI\n\n"
                           "Usage: ai_interviewer_gui [options]\n"
                           "  --data-dir <dir>   Location of the JSON data files (default ./data)\n"
                           "  --db <name>        MongoDB database name (default ai_interviewer)\n"
                           "  --storage <b>      Storage backend: json or mongo (default json)\n"
                           "  --version          Print the version and exit\n");
        QMessageBox::information(nullptr, QStringLiteral("AI Interviewer"), help);
        return 0;
    }
    
    try {
        gui::SessionContext context(dataDir,
                                    useMongo ? db::StorageBackend::Mongo
                                             : db::StorageBackend::Json,
                                    mongoDb);
        
        gui::MainWindow window(context);
        
        for (;;) {
            gui::LoginDialog login(context);
            if (login.exec() != QDialog::Accepted || !login.user()) {
                break;
            }
            
            window.showUser(login.user());
            window.show();
            const int result = app.exec();
            window.hide();
            
            if (result != 0) {
                return result;
            }
            // Returning here means the user logged out; loop back to sign in.
            return 0;
        }
        
        return 0;
    } catch (const std::exception& ex) {
        QMessageBox::critical(nullptr, QStringLiteral("AI Interviewer"),
                              QString::fromStdString(ex.what()));
        return 1;
    }
}

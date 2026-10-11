// OOP: Test (Qt Test, headless GUI smoke test)
#include "gui/SessionContext.h"
#include "gui/MainWindow.h"
#include "gui/LoginDialog.h"
#include "gui/InterviewWidget.h"
#include "gui/LeaderboardWidget.h"
#include "gui/ProfileWidget.h"
#include "gui/DailyChallengeWidget.h"
#include "gui/AdminWidget.h"
#include <QApplication>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QTableWidget>
#include <QTextEdit>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QFile>
#include <QTest>
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QDebug>

#include <fstream>

using gui::SessionContext;
using gui::MainWindow;
using gui::LoginDialog;
using gui::InterviewWidget;
using gui::LeaderboardWidget;
using gui::ProfileWidget;
using gui::DailyChallengeWidget;
using gui::AdminWidget;

namespace {

QLineEdit* findLineEdit(QWidget* root, const QString& placeholder) {
    const QList<QLineEdit*> edits = root->findChildren<QLineEdit*>();
    for (QLineEdit* edit : edits) {
        if (edit->placeholderText().contains(placeholder, Qt::CaseInsensitive)) {
            return edit;
        }
    }
    return nullptr;
}

QPushButton* findButton(QWidget* root, const QString& text) {
    const QList<QPushButton*> buttons = root->findChildren<QPushButton*>();
    for (QPushButton* button : buttons) {
        if (button->text().contains(text, Qt::CaseInsensitive)) {
            return button;
        }
    }
    return nullptr;
}

void writeJson(const QString& path, const QByteArray& content) {
    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(content);
        file.close();
    }
}

// Copies the question bank into a temp data dir so tests never touch ./data.
bool prepareDataDir(QTemporaryDir& dir) {
    if (!dir.isValid()) return false;
    
    writeJson(dir.filePath("users.json"), "[]");
    writeJson(dir.filePath("attempts.json"), "[]");
    writeJson(dir.filePath("scoreboard.json"), "{}");
    writeJson(dir.filePath("daily_challenges.json"), "{}");
    
    // Question bank: a small deterministic set for one subject.
    writeJson(dir.filePath("questions.json"), QByteArray(R"([
  {"id":"gui_easy_1","subject":"oop","topic":"encapsulation","difficulty":"Easy","type":"MCQ",
   "text":"What is encapsulation?","options":["Data hiding","Multiple inheritance","Overloading","Pointers"],"answer":0,"explanation":"x"},
  {"id":"gui_easy_2","subject":"oop","topic":"encapsulation","difficulty":"Easy","type":"Text",
   "text":"Private members are accessed how?","keywords":["scope","member"],"minKeywords":1,"explanation":"x"},
  {"id":"gui_easy_3","subject":"oop","topic":"polymorphism","difficulty":"Easy","type":"MCQ",
   "text":"Runtime polymorphism uses?","options":["Virtual functions","Macros","Inlines","Structs"],"answer":0,"explanation":"x"},
  {"id":"gui_easy_4","subject":"oop","topic":"inheritance","difficulty":"Easy","type":"MCQ",
   "text":"Base class members are?","options":["Inherited","Deleted","Hidden","Copied"],"answer":0,"explanation":"x"},
  {"id":"gui_med_1","subject":"oop","topic":"patterns","difficulty":"Medium","type":"MCQ",
   "text":"Singleton guarantees?","options":["One instance","Many instances","No instances","Threads"],"answer":0,"explanation":"x"},
  {"id":"gui_med_2","subject":"oop","topic":"patterns","difficulty":"Medium","type":"Text",
   "text":"Factory pattern creates?","keywords":["objects","instantiation"],"minKeywords":1,"explanation":"x"},
  {"id":"gui_med_3","subject":"oop","topic":"inheritance","difficulty":"Medium","type":"MCQ",
   "text":"Virtual inheritance solves?","options":["Diamond problem","Memory leak","Syntax","Typing"],"answer":0,"explanation":"x"},
  {"id":"gui_med_4","subject":"oop","topic":"polymorphism","difficulty":"Medium","type":"Text",
   "text":"Static polymorphism uses?","keywords":["templates","compile"],"minKeywords":1,"explanation":"x"},
  {"id":"gui_hard_1","subject":"oop","topic":"patterns","difficulty":"Hard","type":"Coding",
   "text":"Implement a Singleton.","templateCode":"class S{};",
   "testCases":[{"input":"1","expectedOutput":"1","isHidden":false},{"input":"2","expectedOutput":"2","isHidden":true}],
   "explanation":"x"},
  {"id":"gui_hard_2","subject":"oop","topic":"patterns","difficulty":"Hard","type":"Text",
   "text":"Explain RAII.","keywords":["resource","deterministic"],"minKeywords":1,"explanation":"x"},
  {"id":"gui_hard_3","subject":"oop","topic":"inheritance","difficulty":"Hard","type":"MCQ",
   "text":"Pure virtual makes a class?","options":["Abstract","Final","Static","Const"],"answer":0,"explanation":"x"},
  {"id":"gui_hard_4","subject":"oop","topic":"patterns","difficulty":"Hard","type":"Text",
   "text":"Explain dependency injection.","keywords":["injection","decouple"],"minKeywords":1,"explanation":"x"}
])"));
    return true;
}

} // namespace

class GuiSmokeTest : public QObject {
    Q_OBJECT
    
private slots:
    void initTestCase();
    void cleanupTestCase();
    void testLoginDialogRegistersAndSignsIn();
    void testMainWindowRoutesStudentAndAdmin();
    void testInterviewWidgetRunsFullSession();
    void testLeaderboardReflectsFinishedSession();
    void testProfileShowsStreak();
    void testAdminSeesPlatformStats();
    void testDailyChallengeLoadsAndScores();
    
private:
    QTemporaryDir dataDir_;
    std::unique_ptr<SessionContext> context_;
    std::unique_ptr<QApplication>* app_ = nullptr;
};

void GuiSmokeTest::initTestCase() {
    QVERIFY(prepareDataDir(dataDir_));
    try {
        context_.reset(new SessionContext(dataDir_.path().toStdString(),
                                          db::StorageBackend::Json,
                                          "ai_interviewer_gui_test"));
    } catch (const std::exception& ex) {
        qWarning() << "SessionContext construction failed:" << ex.what();
        throw;
    }
}

void GuiSmokeTest::cleanupTestCase() {
    context_.reset();
}

void GuiSmokeTest::testLoginDialogRegistersAndSignsIn() {
    LoginDialog login(*context_);
    
    QComboBox* mode = login.findChild<QComboBox*>();
    QVERIFY(mode);
    QCOMPARE(mode->currentIndex(), 0);
    
    // Register mode
    mode->setCurrentIndex(1);
    // The dialog is never shown in a headless run, so qWaitForWindowExposed
    // would always time out; just pump the event loop once.
    QTest::qWait(1);
    
    QLineEdit* email = findLineEdit(&login, "example.com");
    QVERIFY(email);
    
    const QList<QLineEdit*> allEdits = login.findChildren<QLineEdit*>();
    QVERIFY(allEdits.size() >= 5);
    
    // Fill the form: order follows buildUi() (email, password, name, college, branch)
    QTest::keyClicks(allEdits[0], "gui@test.local");
    QTest::keyClicks(allEdits[1], "guipass123");
    QTest::keyClicks(allEdits[2], "GUI Tester");
    QTest::keyClicks(allEdits[3], "GUI College");
    QTest::keyClicks(allEdits[4], "CSE");
    
    QPushButton* submit = findButton(&login, "Create account");
    QVERIFY(submit);
    QTest::mouseClick(submit, Qt::LeftButton);
    
    // After registering the dialog returns to login mode with a success status.
    QCOMPARE(mode->currentIndex(), 0);
    
    QTest::keyClicks(allEdits[0], "");
    QTest::keyClicks(allEdits[1], "");
    allEdits[0]->setText("gui@test.local");
    allEdits[1]->setText("guipass123");
    
    submit = findButton(&login, "Sign in");
    QVERIFY(submit);
    QTest::mouseClick(submit, Qt::LeftButton);
    
    QCOMPARE(login.result(), static_cast<int>(QDialog::Accepted));
    QVERIFY(login.user() != nullptr);
    QCOMPARE(QString::fromStdString(login.user()->email()), QString("gui@test.local"));
}

void GuiSmokeTest::testMainWindowRoutesStudentAndAdmin() {
    // Register a student directly through the service, then verify routing.
    auth::AuthResult student = context_->auth().registerUser(
        "routing@test.local", "routing123", "Routing Student",
        core::UserRole::Student, "College", "Branch");
    QVERIFY(student.success);
    QVERIFY(!student.user->isAdmin());
    
    MainWindow window(*context_);
    window.showUser(student.user);
    
    const QList<QListWidgetItem*> studentNav = window.findChild<QListWidget*>()->findItems("*", Qt::MatchWildcard);
    QVERIFY(studentNav.size() >= 5);
    
    // Admin routing
    auth::AuthResult admin = context_->auth().registerUser(
        "admin-gui@test.local", "admin123", "GUI Admin",
        core::UserRole::Admin, "College", "IT");
    QVERIFY(admin.success);
    QVERIFY(admin.user->isAdmin());
    
    window.showUser(admin.user);
    // Just verify it does not crash and the window has the expected screens.
    QVERIFY(window.findChild<InterviewWidget*>() != nullptr);
    QVERIFY(window.findChild<AdminWidget*>() != nullptr);
}
void GuiSmokeTest::testInterviewWidgetRunsFullSession() {
    auth::AuthResult user = context_->auth().registerUser(
        "interview-gui@test.local", "interview123", "Interview GUI",
        core::UserRole::Student, "College", "CSE");
    QVERIFY(user.success);
    
    InterviewWidget widget(*context_);
    widget.setUser(user.user);
    
    // Select the cse course (index 0) and the oop subject (index 2)
    QComboBox* course = widget.findChild<QComboBox*>();
    QVERIFY(course);
    QCOMPARE(course->currentIndex(), 0);
    QVERIFY(context_->questionRepo()->findAll().size() > 10);
    
    const QList<QComboBox*> combos = widget.findChildren<QComboBox*>();
    QVERIFY(combos.size() >= 3);
    combos[1]->setCurrentIndex(2);  // oop
    combos[2]->setCurrentIndex(0);  // practice
    
    QPushButton* start = findButton(&widget, "Start interview");
    QVERIFY(start);
    QVERIFY(start->isEnabled());
    
    QSignalSpy finished(&widget, &InterviewWidget::sessionFinished);
    
    // The widget is never shown in headless runs, so drive the slots directly
    // (mouse events are not delivered to hidden widgets).
    const bool invoked = QMetaObject::invokeMethod(&widget, "onStart");
    QVERIFY(invoked);
    QTest::qWait(10);
    
    QLabel* progress = nullptr;
    for (QLabel* label : widget.findChildren<QLabel*>()) {
        if (label->text().startsWith("Question")) progress = label;
    }
    QVERIFY2(progress, "interview did not start");
    QCOMPARE(progress->text(), QString("Question 1 of 10"));
    
    // Answer all ten questions.
    QLineEdit* answer = widget.findChild<QLineEdit*>();
    QVERIFY(answer);
    
    for (int i = 0; i < 10; ++i) {
        QTest::keyClicks(answer, QString());
        answer->setText("A");
        QPushButton* submit = findButton(&widget, "Submit answer");
        QVERIFY(submit);
        QVERIFY(submit->isEnabled());
        const bool ok = QMetaObject::invokeMethod(&widget, "onSubmit");
        QVERIFY(ok);
        QTest::qWait(1);
    }
    
    QCOMPARE(finished.count(), 1);
    
    // The report page must be reachable
    QTextEdit* report = nullptr;
    for (QTextEdit* edit : widget.findChildren<QTextEdit*>()) {
        if (edit->toPlainText().contains("Overall score")) {
            report = edit;
            break;
        }
    }
    QVERIFY2(report, "report text was not shown");
    
    // Attempts were persisted
    QFile attemptFile(dataDir_.filePath("attempts.json"));
    QVERIFY(attemptFile.open(QIODevice::ReadOnly));
    const QByteArray raw = attemptFile.readAll();
    attemptFile.close();
    QVERIFY(raw.count("\"userId\"") >= 10);
}

void GuiSmokeTest::testLeaderboardReflectsFinishedSession() {
    LeaderboardWidget widget(*context_);
    auth::AuthResult user = context_->auth().registerUser(
        "leaderboard-gui@test.local", "leaderboard123", "Leader GUI",
        core::UserRole::Student, "College", "CSE");
    QVERIFY(user.success);
    widget.setUser(user.user);
    
    QTableWidget* table = widget.findChild<QTableWidget*>();
    QVERIFY(table);
    QVERIFY(table->rowCount() >= 1);
    QVERIFY(table->columnCount() == 5);
    QCOMPARE(table->horizontalHeaderItem(0)->text(), QString("Rank"));
}

void GuiSmokeTest::testProfileShowsStreak() {
    auth::AuthResult user = context_->auth().registerUser(
        "profile-gui@test.local", "profile123", "Profile GUI",
        core::UserRole::Student, "College", "CSE");
    QVERIFY(user.success);
    
    ProfileWidget widget(*context_);
    widget.setUser(user.user);
    
    QVERIFY(widget.findChild<gui::StreakCalendar*>() != nullptr);
    const QList<QLabel*> labels = widget.findChildren<QLabel*>();
    bool foundStats = false;
    for (QLabel* label : labels) {
        if (label->text().contains("Current streak")) {
            foundStats = true;
            break;
        }
    }
    QVERIFY(foundStats);
}

void GuiSmokeTest::testAdminSeesPlatformStats() {
    AdminWidget widget(*context_);
    QTableWidget* users = widget.findChildren<QTableWidget*>().first();
    QVERIFY(users);
    QVERIFY(users->rowCount() >= 1);
    QCOMPARE(users->horizontalHeaderItem(0)->text(), QString("Email"));
}

void GuiSmokeTest::testDailyChallengeLoadsAndScores() {
    auth::AuthResult user = context_->auth().registerUser(
        "challenge-gui@test.local", "challenge123", "Challenge GUI",
        core::UserRole::Student, "College", "CSE");
    QVERIFY(user.success);
    
    DailyChallengeWidget widget(*context_);
    widget.setUser(user.user);
    QSignalSpy completed(&widget, &DailyChallengeWidget::challengeCompleted);
    
    // Submitting before loading must be a no-op (button disabled).
    QPushButton* submit = findButton(&widget, "Submit");
    QVERIFY(submit);
    QVERIFY(!submit->isEnabled());
    
    const bool loaded = QMetaObject::invokeMethod(&widget, "onLoad");
    QVERIFY(loaded);
    QTest::qWait(10);
    
    submit = findButton(&widget, "Submit");
    QVERIFY(submit);
    QVERIFY(submit->isEnabled());
    
    QLineEdit* answer = widget.findChild<QLineEdit*>();
    QVERIFY(answer);
    answer->setText("A");
    
    const bool submitted = QMetaObject::invokeMethod(&widget, "onSubmit");
    QVERIFY(submitted);
    QTest::qWait(10);
    
    QCOMPARE(completed.count(), 1);
    
    bool hasResult = false;
    for (QTextEdit* edit : widget.findChildren<QTextEdit*>()) {
        if (edit->toPlainText().contains("With bonus")) hasResult = true;
    }
    QVERIFY2(hasResult, "challenge result was not shown");
    
    // The challenge is persisted for today and is deterministic on reload.
    core::Optional<core::DailyChallenge> first = context_->progress().ensureDailyChallenge("cse");
    core::Optional<core::DailyChallenge> second = context_->progress().ensureDailyChallenge("cse");
    QVERIFY(first.has_value());
    QVERIFY(second.has_value());
    QCOMPARE(first.value().questionId.get(), second.value().questionId.get());
}

QTEST_MAIN(GuiSmokeTest)
#include "gui_smoke_test.moc"

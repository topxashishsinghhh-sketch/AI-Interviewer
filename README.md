# AI Interviewer

C++17 console + Qt 6 GUI + MongoDB backend for mock interviews. Graded on OOP principles.

## Quick Start

### Prerequisites
- CMake 3.20+
- C++17 compiler (GCC 10+, Clang 12+, MSVC 19.28+)
- vcpkg
- OpenSSL (for SHA-256)

### Build (Linux/macOS)
```bash
# Install vcpkg packages
vcpkg install nlohmann-json fmt gtest openssl

# Configure and build
cmake --preset=debug
cmake --build build/debug
```

### Build (Windows / MinGW)
```powershell
# No vcpkg required: nlohmann_json is vendored in third_party/json.hpp
cmake -G "MinGW Makefiles" -S . -B build/debug
cmake --build build/debug

# Optional: enable the MongoDB backend (needs mongocxx/bsoncxx)
cmake -G "MinGW Makefiles" -S . -B build/debug -DUSE_MONGO=ON -DBUILD_TOOLS=ON
cmake --build build/debug
```

### Qt GUI (Windows / MinGW)
Qt 6 is prebuilt with the MinGW-w64 GCC 13.1 toolchain, so the GUI is configured in
its own build directory using `cmake/qt-mingw.cmake`:

```powershell
# Option A: use the helper script (configure + build + QtTest + report paths)
powershell -ExecutionPolicy Bypass -File scripts\run_gui_tests.ps1

# Option B: presets
cmake --preset gui
cmake --build --preset build-gui
ctest --test-dir build-gui --output-on-failure

# Run the GUI
build-gui\src\ai_interviewer_gui.exe
build-gui\src\ai_interviewer_gui.exe --storage mongo --db ai_interviewer
```
Adjust `cmake/qt-mingw.cmake` if Qt is installed to a different prefix.

### Run the web app (browser UI + JSON API)
```powershell
# Builds ai_interviewer_web if needed, then serves http://127.0.0.1:8080
powershell -ExecutionPolicy Bypass -File scripts\run_web.ps1

# Options
powershell -ExecutionPolicy Bypass -File scripts\run_web.ps1 -Port 3000
powershell -ExecutionPolicy Bypass -File scripts\run_web.ps1 -Storage mongo
```
The browser UI is plain HTML/CSS/JS, so there is no front-end build step and no
Node dependency. The server binds to `127.0.0.1` by default; pass
`--host 0.0.0.0` to reach it from another device on the same network.

### Run (console)
```bash
./build/debug/ai_interviewer_console
# Or with flags:
./build/debug/ai_interviewer_console --explain
./build/debug/ai_interviewer_console --storage mongo --uri mongodb://localhost:27017 --db ai_interviewer
./build/debug/ai_interviewer_console --version
```
`--uri`/`--db` fall back to the `MONGODB_URI`/`MONGODB_DB` environment variables.
Because the driver is 64-bit, the MongoDB-enabled console lives in `build-gui`
(see `docs/MONGODB.md`); the `build` console is JSON-only.

## Environment Variables
Copy `.env.example` to `.env` and customize:
```
STORAGE_BACKEND=json          # json | mongo
MONGODB_URI=mongodb://localhost:27017
MONGODB_DB=ai_interviewer
APP_DATA_DIR=./data
```

## Project Structure
```
include/           # Headers (one class per .h)
  core/            # Domain models, interfaces
  auth/            # Authentication service
  admin/           # Admin service
  progress/        # Streak / stats / leaderboard service
  course/          # Course management
  interview/       # Interview logic
  feedback/        # Feedback generation
  db/              # Repository interfaces & implementations
  ui/              # Console UI
  gui/             # Qt 6 Widgets screens
src/               # Implementations (one class per .cpp)
  gui/             # Qt screens (linked into ai_interviewer_gui)
  web/             # HTTP server (linked into ai_interviewer_web)
web/               # Browser front-end (static files, no build step)
data/questions/    # Question JSON files
challenges/        # Coding challenge templates
tests/             # Google Test suite
tests/gui/         # QtTest suite (headless)
docs/              # ARCHITECTURE.md, DATA.md
scripts/           # prepare_data / smoke_test / run_gui_tests / verify_all
cmake/             # Toolchain files (qt-mingw.cmake)
```

See `docs/ARCHITECTURE.md` for the layering and data flow, and `docs/DATA.md`
for the question-bank schema and current coverage (105 questions across
9 subjects).

## Current status

| Phase | State |
| --- | --- |
| 1. Core console (JSON) | Done |
| 2. MongoDB backend | Done |
| 3. Admin / daily / streak / leaderboard | Done |
| 4. Qt 6 GUI | Done |
| 5. Polish + docs | Done |
| 6. MongoDB backend verification | Done - driver built from source, live `mongod` end-to-end run |
| 7. Web UI + HTTP API | Done - browser front-end over the same services, JSON or Mongo |
| 8. LeetCode-style practice platform | Done - problem catalogue, solve, submissions, stats, calendar, 12 subjects |
| 9. Real code execution | Done - sandboxed compile + run with test cases, Run/Submit, verdicts |
| 10. Reference-style UI + expanded bank | Done - difficulty sections, podium leaderboard, 261 questions |

Both builds compile with no warnings under `-Wall -Wextra -Wshadow -Wconversion`.

## Features (Phase 1)
- Register/Login with salted SHA-256 (bcrypt/argon2 recommended for production)
- Lockout after 5 failed attempts
- Courses: CSE, ECE, Commerce, Common (Aptitude, Reasoning, HR)
- Question types: MCQ, Text (keyword scoring), Coding (compile+run)
- Interview modes: Practice, Mock (Linear Ramp), Daily (Adaptive)
- 10 questions: 3 Easy (1min), 4 Medium (2min), 3 Hard (4min)
- Weighted scoring: 1/2/3
- Rule-based feedback: scores, strongest/weakest topics, 2-3 tips
- JSON file storage (auto-creates)
- Streak tracking, badges, leaderboards (template)
- `--explain` flag shows OOP pattern per step

## Features (Phase 2 - MongoDB)
- Full MongoDB backend with swap-in repositories behind the same interfaces
- `Database` singleton: pooled client, schema validators, indexes
- MongoDB repositories: User, Question, Attempt, Interview, Feedback, Scoreboard, DailyChallenge
- `RepositoryFactory` selects backend (`--storage json|mongo`) with automatic JSON fallback
- BSON mappers for all domain types
- CLI tools: `seed_db`, `mongo_backup`, `mongo_restore`, `parity_check`
- `docker-compose up -d` provides MongoDB 7

## Features (Phase 3 - Full UX)
- Ranked leaderboard with scope/window/ranking filters (global/college/branch x all-time/week/month x score/solved/streak)
- Streak calendar (calendar month grid) driven by `Student::recordActivity(dateKey)`
- Deterministic daily challenge (auto-created per course per day, bonus points)
- Badge milestones (Problem Solver, Code Crusher, Week Warrior, Monthly Master)
- Topic progression gating via `CourseService::isUnlocked` (3 solved per preceding topic)
- Session finalization persists user stats and leaderboard entry after every interview
- **Admin role**: separate dashboard with platform statistics, user management
  (award badge / reset streak / delete user) and question-bank coverage stats
- Fixed password verification and JSON cache refresh so data is visible immediately
- `ProgressService` centralises streak/stats/badge/leaderboard logic so the console
  and GUI share exactly one implementation (no business logic in any UI)

## Features (Phase 4 - Qt 6 GUI)
- Qt 6 Widgets desktop app (`ai_interviewer_gui`) over the same service layer
- Sign-in / register dialog (`LoginDialog`) using `AuthService`
- Navigation sidebar and stacked screens (`MainWindow`): Interview, Daily challenge,
  Profile & streak, Leaderboard, Admin
- `InterviewWidget`: course/subject/mode selection, question display, answer/skip,
  finish-early, and a formatted report with tips and questions to review
- `StreakCalendar`: custom-painted month grid highlighting the active streak date
- `LeaderboardWidget`: live filter combos (scope x window x ranking) over a table
- `DailyChallengeWidget`: loads today's deterministic challenge, applies the bonus
- `AdminWidget`: platform statistics, user table with badge/streak/delete actions
- `SessionContext` is the DI container: the screens hold no repositories or services
- Students land on the Interview screen, admins on the Admin screen

## Features (Phase 6 - MongoDB backend verification)

mongocxx/bsoncxx built from source with the Qt MinGW toolchain and the `USE_MONGO`
path exercised end to end against a live `mongod`:

- `MONGO_DRIVER_PREFIX` CMake option; `mongo::mongocxx_shared` / `mongo::bsoncxx_shared`
- `Database::ping()` and initialisation fixed so the singleton actually connects
- `RepositoryFactory` takes a separate `path` and `dbName` (a file path was being
  used as the MongoDB database name)
- `BSONMappers` rewritten against the real mongocxx 3.x API; every repository
  compiles and runs
- `Database` creates collections and indexes idempotently with the modern
  `create_collection(name, options)` overload
- Both front-ends initialise the singleton before building Mongo repositories
- CLI tools rebuilt to link the core library: `seed_db`, `mongo_backup`,
  `mongo_restore`, `parity_check`
- Scoreboard and daily-challenge repositories follow the selected backend, so
  data is not split across two stores
- `scripts\verify_mongo.ps1` verifies seed, a full interview, persistence,
  read-back and the JSON fallback

See `docs/MONGODB.md` for the build steps, schema, tools, and the five bugs the
live verification uncovered.

## Features (Phase 7 - Web UI + HTTP API)

The service layer is reusable, so a browser front-end shares all the C++:

- `include/web/`, `src/web/` — HTTP server over the existing services
  (`auth`, `admin`, `course`, `progress`, `interview`) via cpp-httplib
- `web/` — front-end: `index.html`, `app.js`, `style.css` (no build step, no Node)
- One class per concern: `HttpServer` (facade), `SessionStore` (in-memory
  tokens and in-progress interviews), `JsonApi` (domain <-> wire mapping)
- Endpoints: register/login/logout, me, courses and subjects, interviews
  (start / answer / skip / finish), daily challenge, leaderboard, admin stats,
  admin users, admin questions, health
- Bearer-token auth; admin endpoints reject non-admins with 403
- Storage follows the selected backend (JSON or MongoDB); no duplicated logic

## Features (Phase 8 - LeetCode-style practice platform)

The site is now a practice platform rather than a thin wrapper around interviews:

- **Problem catalogue**: browse, filter by subject / difficulty / type / status
  (solved / attempted / unsolved), text search, sort by difficulty or title, and a
  bookmarked-only view
- **Problem pages**: solve any single problem on demand, get an immediate verdict
  with the question's own explanation, and see your previous attempts
- **Submissions history**: every judged answer with verdict, difficulty and time
- **Personal statistics**: solved/total per difficulty, acceptance rate,
  per-subject progress bars, streaks and badges
- **Activity calendar**: GitHub-style contribution grid over the last 18 weeks
- **Recommendations**: unsolved problems, easiest first, on the dashboard
- **Bookmarks** on any problem
- New subjects: **Operating Systems**, **DBMS**, **Computer Networks** (12 total,
  141 questions)

Everything is served by the same C++ services, so the console, Qt GUI and web
site share one implementation of every rule.

## Features (Phase 9 - Real code execution)

Coding problems are compiled and run, not text-matched:

- `execution::CodeExecutor` writes the submission to an isolated scratch
  directory, compiles it, then runs it against every test case
- **Run** executes against the visible samples and records nothing; **Submit**
  records the attempt and updates solved totals, streak and badges
- Verdicts: Accepted, Wrong Answer, Compile Error, Runtime Error, Time Limit
  Exceeded, Memory Limit Exceeded
- Limits: 2 s and 256 MB per case, enforced by a Job Object (`ProcessMemoryLimit`,
  `PerJobUserTimeLimit`) plus a wall-clock `WaitForSingleObject` timeout, with
  `TerminateJobObject` as the kill switch
- stdin/stdout via temp files rather than pipes, so a child that fills its
  stdout cannot deadlock the parent
- Output comparison strips `\r` and trailing blank space, so CRLF from a Windows
  binary matches the LF in the question bank
- `POST /api/problems/{id}/run` returns per-case status, expected vs actual
  output, exit code and timing; the front-end shows a verdict banner plus a
  test-results table
- The problem API also returns `template` (starter code) and `samples` (visible
  cases), so the editor and Run button work without extra round trips

## Features (Phase 10 - Reference-style UI and expanded bank)

The web interface now follows the layout of a dedicated practice platform,
and the question bank is three times larger:

- **Problems page** presents difficulty sections (Beginner / Intermediate /
  Advanced) with a solved-per-section counter, instead of a flat table
- **Leaderboard** shows a podium for the top three (🥇🥈🥉 with solved/total,
  completion rate and achievement tier) and a per-row ✅/⬜ completion strip
- **Achievement tiers** from solved percentage: Newcomer, Beginner,
  Intermediate, Advanced, Expert, Master
- **Per-subject progress** uses 🟩/⬜ blocks with a percentage, matching the
  reference's package-progress style
- Question bank grown from 141 to **261 questions across 12 subjects**
  (21-22 per subject, 8 Easy / 7 Medium / 7 Hard), including 26 coding
  problems with runnable test cases

## OOP Patterns Used
- **Abstraction**: Question, User, Repository interfaces
- **Inheritance**: MCQQuestion, TextQuestion, CodingQuestion; Student, Admin
- **Polymorphism**: evaluate(), display() virtual methods
- **Strategy**: QuestionSelectionStrategy (LinearRamp, Adaptive), FeedbackGenerator
- **Factory**: SessionFactory (Practice, Mock, Daily), RepositoryFactory (JSON/Mongo + fallback)
- **Builder**: InterviewSessionBuilder
- **Composition**: Course → Subject → Topic
- **Template**: Leaderboard<T>, core::Optional<T> (GCC 6.3 compatible)
- **Repository**: IUserRepository, IQuestionRepository, IAttemptRepository, IInterviewRepository, IFeedbackRepository, IScoreboardRepository, IDailyChallengeRepository
- **Singleton**: Database (MongoDB client pool)
- **Facade**: ConsoleUI, AuthService, InterviewService, CourseService, AdminService
- **Dependency Injection**: gui::SessionContext (screens receive services, never construct them)
- **Observer**: Qt signals/slots between GUI screens (sessionFinished, challengeCompleted)
- **Mapper**: web::JsonApi (domain <-> API payloads), db::BSONMappers (domain <-> BSON)
- **Repository (in-memory)**: web::SessionStore (browser tokens, in-progress interviews)
- **Service (catalogue)**: problem::ProblemService (filtering, judging, statistics)
- **Service (sandbox)**: execution::CodeExecutor (isolated compile and run)

## Testing
```bash
cmake --preset=debug
cmake --build build/debug --target test_core
ctest --test-dir build/debug
```

### Full verification (Windows)
Runs data preparation, the console build, the console smoke test and the Qt
build + QtTest suite, and fails on warnings or any test failure:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\verify_all.ps1
```

### MongoDB backend verification
Builds the `USE_MONGO` configuration, starts a local `mongod`, seeds the bank
and drives the app end to end against MongoDB (needs the driver at
`C:\mongo-install`; see `docs/MONGODB.md`):

```powershell
powershell -ExecutionPolicy Bypass -File scripts\verify_mongo.ps1 -SkipBuild
```

### Smoke test (Windows, no GTest required)
End-to-end console flow: register → login → 10-question interview → feedback →
streak → daily challenge → profile → calendar → leaderboard → admin → logout,
then verifies JSON persistence.
```powershell
cmake --build build --config Release
powershell -ExecutionPolicy Bypass -File scripts\smoke_test.ps1
```

### Qt GUI tests (Qt 6 + QtTest, headless/offscreen)
Runs the real screens against a throwaway data directory:
login/register, student vs admin routing, a full 10-question session, leaderboard
entry, profile/streak, admin tables, and daily-challenge determinism.
```powershell
powershell -ExecutionPolicy Bypass -File scripts\run_gui_tests.ps1
# or
cmake --preset gui
cmake --build --preset build-gui
ctest --test-dir build-gui --output-on-failure
```

### Data files
`data/questions/*.json` are the authoritative question banks; they are merged into
`data/questions.json` (the file the repository loads). Regenerate with:
```powershell
powershell -ExecutionPolicy Bypass -File scripts\prepare_data.ps1
```
Runtime data lives in JSON and is auto-created: `users.json`, `attempts.json`,
`scoreboard.json`, `daily_challenges.json`.

### MongoDB tools (needs mongocxx)
These compile only when `USE_MONGO=ON` and mongocxx/bsoncxx are found. See
`docs/MONGODB.md` for the build steps and `scripts\verify_mongo.ps1` for the
automated end-to-end run.
```powershell
build-gui\tools\seed_db.exe --uri mongodb://127.0.0.1:27017 --db ai_interviewer --questions data\questions.json
build-gui\tools\parity_check.exe --uri mongodb://127.0.0.1:27017 --db ai_interviewer --data-dir data
build-gui\tools\mongo_backup.exe --output .\backups
build-gui\tools\mongo_restore.exe --backup-dir .\backups\backup_<timestamp>
```

## Docker (MongoDB)
```bash
docker-compose up -d
# Then set STORAGE_BACKEND=mongo in .env
```

## License
MIT
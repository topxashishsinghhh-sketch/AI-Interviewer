# Architecture

AI Interviewer is a C++17 application with one domain/service layer and two
front-ends (console and Qt 6 Widgets) over interchangeable storage backends
(JSON or MongoDB).

## Layering

Dependencies only ever point downward. Nothing in a lower layer includes from a
higher one, and no UI class contains business logic.

```
+-------------------------------------------------------------+
| ui/ConsoleUI.cpp     gui/* (Qt)      web/HttpServer.cpp      |  presentation
+-------------------------------------------------------------+
| auth/AuthService           admin/AdminService               |
| interview/*                course/CourseService             |  services
| progress/ProgressService   problem/ProblemService           |
+-------------------------------------------------------------+
| core/*  (Question, User, InterviewSession, strategies,      |  domain
|          feedback, repository interfaces)                   |
+-------------------------------------------------------------+
| db/Json*Repository          db/Mongo*Repository             |  repositories
+-------------------------------------------------------------+
| data/*.json                 mongocxx/bsoncxx                |  storage
+-------------------------------------------------------------+
```

### Presentation
- `ui/ConsoleUI` — the console front-end. It formats and reads input only;
  every decision is delegated to a service.
- `gui/*` — the Qt screens. `gui::SessionContext` is the dependency-injection
  container: screens receive the services they need in their constructors and
  never construct repositories or services themselves.

### Services
- `auth::AuthService` — registration, login, lockout, password hashing
- `admin::AdminService` — platform statistics, user management, question-bank stats
- `interview::InterviewService` — session lifecycle (start / answer / skip / finish)
- `interview::SessionFactory` — creates the right service + strategy per mode
- `course::CourseService` — course/subject/topic catalogue and progression gating
- `progress::ProgressService` — streaks, cumulative stats, badges, leaderboard
  entries and the deterministic daily challenge

### Domain
- `core::Question` hierarchy — `MCQQuestion`, `TextQuestion`, `CodingQuestion`,
  each with its own `evaluate()` and `display()` (polymorphism)
- `core::User` hierarchy — `Student`, `Admin`
- `core::InterviewSession` — the interview entity and its state machine
- `core::QuestionSelectionStrategy` — `LinearRampStrategy` (fixed 3/4/3 ramp),
  `AdaptiveStrategy` (reacts to the user's attempt history)
- `core::FeedbackGenerator` — `RuleBasedFeedback` produces the report
- Repository interfaces (`core::I*Repository`) — the storage contract

### Repositories
- JSON implementations (`db/Json*Repository`) are always available and
  auto-create their files.
- MongoDB implementations (`db/Mongo*Repository`) are compiled only when
  `USE_MONGO=ON` and mongocxx/bsoncxx are found.
- `db::RepositoryFactory` picks an implementation and can fall back to JSON when
  MongoDB is unavailable, so the app always runs without a database.

## Data flow: a 10-question interview

### Console and Qt
1. Course → subject → mode selection in the front-end.
2. `SessionFactory::createPractice/Mock/Daily` builds an `InterviewService`
   wired with `LinearRampStrategy` or `AdaptiveStrategy`.
3. `InterviewService::startSession` selects 10 questions via the strategy.
4. Each answer goes to `core::InterviewSession::submitAnswer`, which asks the
   question to `evaluate()` (polymorphism), appends an `Attempt`, and advances
   the index; the session auto-finishes after the last question.
5. `finishSession` builds a `FeedbackReport` from the attempts and persists the
   attempts through `IAttemptRepository`.
6. `ProgressService::finalizeSession` updates the streak, cumulative counters and
   badges, persists the user, and upserts the leaderboard entry.

### Web (same steps, different transport)
HTTP is stateless, so the only extra piece is where the in-flight session lives:

1. `POST /api/interviews` authenticates the bearer token, calls `startSession`,
   and stores the returned `InterviewSession` in `web::SessionStore` under a
   generated id.
2. `POST /api/interviews/{id}/answers` looks the session up, then calls the same
   `submitAnswer` / `skipQuestion` the console uses.
3. `POST /api/interviews/{id}/finish` calls `finishSession`, then the same
   `ProgressService::finalizeSession`, and drops the session.
4. `web::JsonApi` is the only place domain objects are serialised, so the wire
   format cannot drift between endpoints.

`SessionStore` is the sole in-memory state (browser tokens and in-progress
interviews); every durable read and write goes through a repository interface.

## Configuration

| Source | Meaning |
| --- | --- |
| `--storage json|mongo` | storage backend selection (console) |
| `--db <name>` | MongoDB database name |
| `data/questions/*.json` | authored question banks |
| `data/questions.json` | merged question bank the repository loads |
| `data/users.json` etc. | runtime data, auto-created |
| `BACKUP_MONGO_URI` etc. in `.env.example` | used by `docker-compose.yml` |

Config comes from command-line flags and environment variables; no secrets are
committed, and the app works with no database at all via the JSON fallback.

### Problem catalogue
Browsing a problem and solving one are separate from running a ten-question
session, which is why they live in `problem::ProblemService`:

1. `listProblems()` applies the filter (subject, difficulty, type, free text,
   status) and, for each surviving row, the caller's own progress derived from a
   single `findByUser()` call rather than one query per problem.
2. `solveProblem()` asks the question to `evaluate()` itself (polymorphism), then
   records the attempt, updates the student's counters, streak and badges, and
   persists. A repeat attempt never inflates the solved count.
3. `stats()` counts distinct solved question ids per difficulty, so acceptance
   rate stays correct no matter how many retries a problem took.

## Build matrix

| Target | Compiler | Notes |
| --- | --- | --- |
| `ai_interviewer_console` | MinGW GCC 6.3 | console front-end, JSON storage only |
| `ai_interviewer_gui` | Qt's MinGW GCC 13.1 | Qt 6.11 is prebuilt with this compiler, so the GUI uses `cmake/qt-mingw.cmake` |
| `gui_smoke_test` | Qt's MinGW GCC 13.1 | QtTest suite, headless via `QT_QPA_PLATFORM=offscreen` |
| `ai_interviewer_web` | Qt's MinGW GCC 13.1 | HTTP server + browser front-end, JSON or Mongo |

The console is 32-bit and the GUI/web targets are 64-bit, because Qt 6 and
cpp-httplib are only available for the 64-bit toolchain here. That is why the
MongoDB backend and the web server live in `build-gui`.

The console and GUI are configured in separate build directories because of the
compiler difference; `scripts/run_gui_tests.ps1` automates the GUI side.

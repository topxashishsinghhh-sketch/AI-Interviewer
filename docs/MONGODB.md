# MongoDB backend

The application stores everything behind repository interfaces, so switching the
backend is a configuration change rather than a code change.

| Setting | Effect |
| --- | --- |
| `--storage json` (default) | `Json*Repository`; files under `data/`, auto-created |
| `--storage mongo` | `Mongo*Repository`; falls back to JSON if the server is unreachable |
| `--uri` / `MONGODB_URI` | Connection string (default `mongodb://localhost:27017`) |
| `--db` / `MONGODB_DB` | Database name (default `ai_interviewer`) |

No secrets belong in the repository: the URI comes from the command line or
`MONGODB_URI` in `.env`, which is git-ignored.

## Build

MongoDB support is opt-in because mongocxx/bsoncxx are large dependencies:

```powershell
# one-time: build the driver (see "Installing the driver" below)
cmake --preset gui `
  -DCMAKE_TOOLCHAIN_FILE=cmake/qt-mingw.cmake `
  -DCMAKE_PREFIX_PATH="C:/Qt/6.11.2/mingw_64" `
  -DUSE_MONGO=ON -DBUILD_TOOLS=ON -DBUILD_GUI_TESTS=ON `
  -DMONGO_DRIVER_PREFIX="C:/mongo-install" `
  -S . -B build-gui
cmake --build --preset build-gui --config Release
```

Without `-DUSE_MONGO=ON` the application compiles to JSON only, which is why the
normal `scripts\verify_all.ps1` path never needs the driver.

`MONGO_DRIVER_PREFIX` is required because the toolchain file sets
`CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY`, and the mongocxx/bsoncxx config files
in turn `find_dependency()` the C driver packages.

## Installing the driver (this machine)

The driver is not available prebuilt for MinGW, so it is built from source:

```powershell
# mongo-c-driver 1.30.x (no SSL/SASL keeps it self-contained)
cmake -G "MinGW Makefiles" -S C:\mongo-src\mongo-c-driver-1.30.13 -B C:\mongo-build\mongoc `
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=C:/mongo-install `
  -DENABLE_SSL=OFF -DENABLE_SASL=OFF -DENABLE_TESTS=OFF -DENABLE_EXAMPLES=OFF
cmake --build C:\mongo-build\mongoc --config Release --parallel 4
cmake --install C:\mongo-build\mongoc

# mongocxx r3.10.2 — the version file must sit in the build directory before
# configuring, otherwise CMake asks for -DBUILD_VERSION
Copy-Item C:\mongo-src\mongo-cxx-driver-r3.10.2\build\VERSION_CURRENT C:\mongo-build\mongocxx\VERSION_CURRENT
cmake -G "MinGW Makefiles" -S C:\mongo-src\mongo-cxx-driver-r3.10.2 -B C:\mongo-build\mongocxx `
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/mongo-install `
  -DCMAKE_INSTALL_PREFIX=C:/mongo-install -DENABLE_TESTS=OFF -DENABLE_EXAMPLES=OFF
cmake --build C:\mongo-build\mongocxx --config Release --parallel 4
cmake --install C:\mongo-build\mongocxx
```

There are no bundled MinGW triplets in vcpkg, so a plain source build with the
Qt MinGW GCC 13.1 toolchain is the path used here. The installed driver exports
`mongo::mongocxx_shared` and `mongo::bsoncxx_shared`.

**64-bit only.** The console preset uses MinGW.org GCC 6.3 (32-bit), which
cannot link a 64-bit driver, so `USE_MONGO` is only meaningful in the Qt
MinGW build directory (`build-gui`). Running `seed_db` from `build-gui` is
therefore the normal workflow.

## Schema

`db::Database` creates collections and indexes idempotently on first connect:

| Collection | Indexes |
| --- | --- |
| `users` | unique `email` |
| `questions` | `subject + difficulty`, `topic` |
| `attempts` | `userId + timestamp`, `questionId` |
| `interviews` | `userId + startTime` |
| `feedback` | `userId` |
| `scoreboards_<scope>_<window>_<ranking>` | unique `userId` |
| `daily_challenges` | unique `courseId + date` |

Documents are mapped by `db::BSONMappers`, which is the single conversion path
for both backends: `seed_db` turns the JSON bank into BSON and reuses
`QuestionMapper::fromBSON`, so a question can be read from a file or from
MongoDB through the same code.

Identifiers are stored as `_id`. The JSON banks spell the key `id`, so the
mapper accepts either spelling and the seed tool normalises it to `_id`.

## Tools

Built only when `USE_MONGO=ON` and `BUILD_TOOLS=ON`; each links
`ai_interviewer_core` and the driver DLLs are copied next to them.

```powershell
# Load the question bank and create the bootstrap admin.
# Its credentials come from the environment; no default password is committed.
$env:SEED_ADMIN_EMAIL = "admin@interviewer.local"
$env:SEED_ADMIN_PASSWORD = "<choose one>"
build-gui\tools\seed_db.exe --uri mongodb://127.0.0.1:27017 --db ai_interviewer --questions data\questions.json

# Compare MongoDB against the JSON repositories before a cutover
build-gui\tools\parity_check.exe --uri mongodb://127.0.0.1:27017 --db ai_interviewer --data-dir data

# Snapshot to backups\backup_<timestamp> (one extended-JSON doc per line)
build-gui\tools\mongo_backup.exe --output .\backups

# Restore; --skip-existing keeps records already present
build-gui\tools\mongo_restore.exe --backup-dir .\backups\backup_20260101120000
```

## Verification

`scripts\verify_mongo.ps1` runs the whole backend end to end. It builds the
`USE_MONGO` configuration, starts a local `mongod` if none is listening, seeds
the bank, drives the console through registration → login → interview → read
back, and checks the JSON fallback.

```powershell
# assumes a driver is installed at C:\mongo-install and mongod at C:\mongo-server
powershell -ExecutionPolicy Bypass -File scripts\verify_mongo.ps1

# skip the build when build-gui is current
powershell -ExecutionPolicy Bypass -File scripts\verify_mongo.ps1 -SkipBuild
```

What it verifies, measured on this machine:

- the console prints `Using MongoDB backend for ...` for every repository
- registration, login and a full 10-question interview complete
- `attempts`, `users` and `scoreboards_global_alltime_score` gain documents
- a second login reads the persisted name, streak and counters back out
- `parity_check` reports matching question counts against the JSON bank
- with the server stopped the app still starts (JSON fallback)

## Fallback behaviour

Every `*WithFallback` factory probes the server and falls back to JSON when it
cannot be reached, so the app is usable with no database at all. The chosen
backend is printed on startup by `RepositoryFactory`.

The scoreboard and daily-challenge repositories follow the selected backend
rather than always using JSON, so all data lands in one store; the JSON file is
only written when `--storage json` is in force.

## Bugs found by this verification

These were invisible until the backend was actually executed:

1. `Database::ping()` called `acquire()`, which refuses clients while
   `initialized_` is still false — so `initialize()` could never connect.
   `ping()` now uses the pool directly, and `initialized_` is set before the
   schema step instead of after it.
2. `RepositoryFactory::create*(Mongo, path)` passed the JSON file path as the
   MongoDB **database name**, silently writing to a database called
   `./data/scoreboard.json`. The factory now takes separate `path` and `dbName`
   parameters.
3. `seed_db` fed the JSON bank's `id` field to a mapper that reads `_id`, so
   every question was seeded with an empty id and all 105 collapsed into one
   document. The mapper now accepts either spelling and the seed tool
   normalises to `_id`.
4. The console never called `Database::initialize()` under `--storage mongo`,
   so it silently fell back to JSON. Both front-ends now initialise the
   singleton before constructing Mongo repositories.
5. mongocxx 3.x has no `bulk_write_exception::result()`, no
   `document::view::find()` with a const view, and no `to_string()`/`to_int64()`
   on BSON values; the repositories and mappers were rewritten against the real
   API (element copies from a range-for loop, `std::string(string_view)`, and
   duplicate-key detection via server error code 11000).

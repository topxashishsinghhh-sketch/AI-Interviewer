# AI Interviewer: project rules (read first in every task)
What: C++17 console + Qt 6 Widgets GUI + MongoDB backend. Mock interviews
(Easy/Medium/Hard), courses, profiles, streaks, leaderboards, feedback.
Graded on OOP — make it visible.
Tech: CMake, vcpkg, Google Test, nlohmann/json, mongo-cxx-driver, Qt 6.
-Wall -Wextra -Wshadow -Wconversion, zero warnings. Smart pointers only.
One class per .h/.cpp. One-line // OOP: comment per class.
Layers: ui/gui -> services -> domain -> repository interfaces -> db.
No business logic in UI/GUI. Services never touch UI/GUI.
Storage only through repository interfaces (JSON and Mongo impls).
Config via env vars. No secrets in code or git. App must still work with
no database (JSON fallback).
Protocol: 1) short plan 2) code 3) build+test, fix errors 4) report <=15
lines: done, verified, not verified, next. Do only the current phase.
Never paste full files. State assumptions in one line.
Phases: 1 core console (JSON), 2 MongoDB, 3 admin/daily/streak/leaderboard,
4 Qt GUI, 5 final polish and docs.

## Notes for automated agents (added in Phase 5)

- Build the console with `cmake -G "MinGW Makefiles" -S . -B build` (MinGW
  GCC 6.3 on this machine).
- Build the GUI and its tests with `scripts\run_gui_tests.ps1`, or
  `cmake --preset gui && cmake --build --preset build-gui`. Qt 6.11 is prebuilt
  with MinGW GCC 13.1, which is why the GUI uses `cmake/qt-mingw.cmake` and its
  own build directory; do not try to build it with GCC 6.3.
- Run the browser web app with `scripts\run_web.ps1` (builds
  `ai_interviewer_web` if needed and serves `http://127.0.0.1:8080`). It lives in
  `build-gui` because cpp-httplib/Qt are 64-bit only, and takes `-Storage mongo`
  to use MongoDB instead of JSON.
- Before claiming work is done, run both:
  `scripts\smoke_test.ps1` (console) and `scripts\run_gui_tests.ps1` (Qt).
  Both must print a clean pass, and neither build may emit warnings.
- The compiler here is GCC 6.3, so no `<optional>`, no `std::filesystem`, no
  structured bindings, and no `if constexpr`. `core/Optional` is the shim.
- Keep the layers intact: screens (ui/, gui/) only format and read input, all
  decisions go through auth/, admin/, progress/, interview/, course/.
- Question banks live in `data/questions/*.json`; run
  `scripts\prepare_data.ps1` to regenerate `data/questions.json` and confirm the
  4 Easy / 4 Medium / 3 Hard ratios.
- The MongoDB path needs mongocxx/bsoncxx, which are **not** installed by
  default. Build them into `C:\mongo-install` first (exact commands in
  `docs/MONGODB.md`), then configure with
  `-DUSE_MONGO=ON -DMONGO_DRIVER_PREFIX=C:/mongo-install` in `build-gui`.
- Verify MongoDB with `scripts\verify_mongo.ps1 -SkipBuild` once `build-gui` is
  current; it starts its own `mongod` if none is listening. It needs a driver at
  `C:\mongo-install` and a server at `C:\mongo-server`.
- Because the driver is 64-bit and the console preset is 32-bit GCC 6.3, the
  Mongo tools and the Mongo-enabled console live in `build-gui`.
- The code-execution sandbox (`execution::CodeExecutor`) compiles with
  `std::system()` for the compile step and `CreateProcess` in a Job Object for
  the run step, so it needs no extras on either toolchain. It writes to
  `build/exec` / `build-gui/exec` (git-ignored) rather than a hard-coded `/tmp`.
- The web front-end is plain HTML/CSS/JS with no build step, so edit
  `web/app.js` / `web/index.html` / `web/style.css` and reload the browser.

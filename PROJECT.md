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
Protocol: 1) short plan 2) code 3) build+test, fix errors 4) report ≤15
lines: done, verified, not verified, next. Do only the current phase.
Never paste full files. State assumptions in one line.
Phases: 1 core console (JSON), 2 MongoDB, 3 admin/daily/streak/leaderboard,
4 Qt GUI, 5 final polish and docs.
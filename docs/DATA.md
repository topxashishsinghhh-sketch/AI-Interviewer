# Question bank and data files

## Layout

```
data/questions/          authored question banks, one file per subject
  data_structures.json   subject "ds"
  algorithms.json        subject "algo"
  cpp_oop.json           subject "oop"
  signals.json           subject "signals"
  analog.json            subject "analog"
  accounting.json        subject "accounting"
  aptitude.json          subject "aptitude"
  reasoning.json         subject "reasoning"
  hr.json                subject "hr"
data/questions.json      merged bank that JsonQuestionRepository loads (generated)
data/users.json          runtime data, auto-created, git-ignored
data/attempts.json       runtime data, auto-created, git-ignored
data/scoreboard.json     runtime data, auto-created, git-ignored
data/daily_challenges.json  runtime data, auto-created, git-ignored
```

`data/questions.json` is generated. Regenerate it after editing the per-subject files:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\prepare_data.ps1
```

The script also prints the per-subject and per-difficulty coverage, so you can
confirm the ratios below before committing.

## Schema

Every entry in a question file:

| Field | Type | Notes |
| --- | --- | --- |
| `id` | string | globally unique |
| `subject` | string | must match a `core::Subject` id in `CourseService` |
| `topic` | string | free-form; reported in the feedback |
| `difficulty` | string | `Easy`, `Medium` or `Hard` |
| `type` | string | `MCQ`, `Text` or `Coding` |
| `text` | string | the question itself |
| `explanation` | string | shown in the feedback |

Type-specific fields:

| Type | Extra fields |
| --- | --- |
| `MCQ` | `options`: array of strings, `answer`: zero-based index |
| `Text` | `keywords`: array of strings, `minKeywords`: integer |
| `Coding` | `templateCode`: string, `testCases`: array of `{input, expectedOutput, isHidden}` |

`Text` questions are scored by counting how many `keywords` appear in the
answer, compared against `minKeywords`. `Coding` questions compile and run the
submitted program and compare stdout against `expectedOutput`.

## Difficulty ratios

A subject needs at least 4 Easy, 4 Medium and 3 Hard questions so the linear
ramp (3 Easy, 4 Medium, 3 Hard) and the adaptive strategy both have material to
work with. Current coverage (261 questions, 12 subjects):

| Subject | Easy | Medium | Hard | Total |
| --- | --- | --- | --- | --- |
| accounting | 8 | 7 | 7 | 22 |
| algo | 8 | 7 | 7 | 22 |
| analog | 8 | 7 | 7 | 22 |
| aptitude | 8 | 7 | 6 | 21 |
| dbms | 8 | 7 | 7 | 22 |
| ds | 8 | 7 | 7 | 22 |
| hr | 8 | 7 | 6 | 21 |
| networks | 8 | 7 | 7 | 22 |
| oop | 8 | 7 | 6 | 21 |
| os | 8 | 7 | 7 | 22 |
| reasoning | 8 | 7 | 7 | 22 |
| signals | 8 | 7 | 7 | 22 |
| **Total** | **96** | **84** | **81** | **261** |

Question types: 96 MCQ, 139 Text, 26 Coding (each with runnable test cases).

## Weighting

`core::Difficulty` maps to a weight used when scoring:

| Difficulty | Time limit | Weight |
| --- | --- | --- |
| Easy | 1 minute | 1 |
| Medium | 2 minutes | 2 |
| Hard | 4 minutes | 3 |

## Runtime data

The JSON repositories create their files on first use and append/merge on save.
They are git-ignored because they hold user data. To reset the application,
truncate them to the empty form used by `scripts\prepare_data.ps1`:

```json
[]   // users.json, attempts.json
{}   // scoreboard.json, daily_challenges.json
```

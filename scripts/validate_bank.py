# validate_bank.py — sanity-check every generated question file.
#
# Checks the invariants prepare_data and the API rely on:
#   * valid JSON, top-level array
#   * required keys present and non-empty
#   * MCQ: >= 4 options, answer index in range
#   * Text:  >= 1 keyword
#   * difficulty in {Easy, Medium, Hard}
#   * skillTags non-empty; softSkill boolean
# Exits non-zero if any file is bad.
import glob
import io
import json
import os
import sys

REQUIRED = ["id", "subject", "topic", "difficulty", "type", "text",
            "explanation", "objective", "skillTags", "keywords"]
DIFFICULTIES = {"Easy", "Medium", "Hard"}
TYPES = {"MCQ", "Text", "Coding"}


def validate(path):
    errs = []
    try:
        with io.open(path, encoding="utf-8") as fh:
            data = json.load(fh)
    except Exception as exc:  # noqa: BLE001
        return ["parse error: %s" % exc]

    if not isinstance(data, list):
        return ["top level is not an array"]

    for i, item in enumerate(data):
        tag = item.get("id", "<no id @%d>" % i)
        for key in REQUIRED:
            if key not in item:
                errs.append("%s missing %s" % (tag, key))
            elif item[key] in ("", [], None):
                errs.append("%s empty %s" % (tag, key))
        if item.get("difficulty") not in DIFFICULTIES:
            errs.append("%s bad difficulty %r" % (tag, item.get("difficulty")))
        if item.get("type") not in TYPES:
            errs.append("%s bad type %r" % (tag, item.get("type")))
        if not isinstance(item.get("softSkill"), bool):
            errs.append("%s softSkill not boolean" % tag)
        if item.get("type") == "MCQ":
            opts = item.get("options") or []
            if len(opts) < 4:
                errs.append("%s MCQ has %d options" % (tag, len(opts)))
            ans = item.get("answer")
            if not isinstance(ans, int) or not (0 <= ans < len(opts)):
                errs.append("%s MCQ answer out of range: %r" % (tag, ans))
        if item.get("type") == "Text" and not item.get("keywords"):
            errs.append("%s Text has no keywords" % tag)
        if item.get("type") == "Coding" and not item.get("testCases"):
            errs.append("%s Coding has no test cases" % tag)
    return errs


def main():
    patterns = sys.argv[1:] or ["data/questions/*.json"]
    total = 0
    failed = 0
    for pattern in patterns:
        for path in sorted(glob.glob(pattern)):
            errs = validate(path)
            with io.open(path, encoding="utf-8") as fh:
                count = len(json.load(fh))
            total += count
            if errs:
                failed += 1
                print("%s: %d issue(s)" % (path, len(errs)))
                for e in errs[:5]:
                    print("    " + e)
            else:
                print("%-46s %5d OK" % (os.path.basename(path), count))
    print("total questions: %d across %d file(s), %d failed"
          % (total, len(glob.glob(patterns[0])), failed))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())

# enrich_questions.py
#
# Adds Phase 11 metadata to the question banks in place:
#   - objective: shown above the prompt as the thing being demonstrated
#   - skillTags: grouped so the Skills dashboard can show grouped progress
#   - softSkill: true for behavioural/human-skills banks
#   - keywords:   words highlighted in the prompt
#
# Existing content is never rewritten, only new keys are added. Banks that
# already carry an objective are left untouched.
import io
import json
import os
import sys

# Which tag group each question topic belongs to.
TAG_RULES = [
    # (substring of subject or topic, hard_tag, soft_tag)
    ("ds",      "Data Structures",  None),
    ("tree",    "Data Structures",  None),
    ("graph",   "Graphs",           None),
    ("hash",    "Data Structures",  None),
    ("heap",    "Data Structures",  None),
    ("sort",    "Algorithms",       None),
    ("search",  "Algorithms",       None),
    ("dp",      "Dynamic Programming", None),
    ("greedy",  "Algorithms",       None),
    ("regex",   "Strings",          None),
    ("string",  "Strings",          None),
    ("oop",     "OOP",              None),
    ("pattern", "Design Patterns",  None),
    ("contract","OOP",              None),
    ("sql",     "SQL & Databases",  None),
    ("relational", "SQL & Databases", None),
    ("transact", "SQL & Databases",  None),
    ("normal",  "SQL & Databases",  None),
    ("index",   "SQL & Databases",  None),
    ("os",      "Operating Systems", None),
    ("schedul", "Operating Systems", None),
    ("memory",  "Operating Systems", None),
    ("process", "Operating Systems", None),
    ("concurrency", "Concurrency",  None),
    ("network", "Computer Networks", None),
    ("routing", "Computer Networks", None),
    ("transport", "Computer Networks", None),
    ("signal",  "Signal Processing", None),
    ("fourier", "Signal Processing", None),
    ("filters", "Signal Processing", None),
    ("analog",  "Circuit Design",   None),
    ("opamp",   "Circuit Design",   None),
    ("amplifi", "Circuit Design",   None),
    ("transistor", "Circuit Design", None),
    ("diode",   "Circuit Design",   None),
    ("account", "Accounting",       None),
    ("ratio",   "Financial Analysis", None),
    ("cash",    "Financial Analysis", None),
    ("aptitude","Quantitative Aptitude", None),
    ("arith",   "Quantitative Aptitude", None),
    ("percent", "Quantitative Aptitude", None),
    ("probab",  "Quantitative Aptitude", None),
    ("reasoning","Logical Reasoning", None),
    ("syllog",  "Logical Reasoning", None),
    ("puzzle",  "Logical Reasoning", None),
    ("seating", "Logical Reasoning", None),
    ("direction", "Logical Reasoning", None),
    ("blood",   "Logical Reasoning", None),
    ("coding_d", "Logical Reasoning", None),
    ("series",  "Logical Reasoning", None),
    ("hr",      "Behavioural Interviews", "Communication"),
    ("behavio", "Behavioural Interviews", "Communication"),
    ("communica", None,             "Communication"),
    ("teamwork", None,              "Teamwork"),
    ("conflict", None,              "Conflict Resolution"),
    ("career",  None,               "Career Planning"),
    ("negoti",  None,               "Negotiation"),
    ("leadership", None,            "Leadership"),
    ("scenario", None,              "Problem Solving"),
]

# Subjects that are entirely human-skills practice.
SOFT_SUBJECTS = {"hr"}


def tags_for(entry):
    subject = entry.get("subject", "")
    topic = entry.get("topic", "")
    haystack = (subject + " " + topic).lower()

    hard_tags = []
    soft_tags = []
    for needle, hard, soft in TAG_RULES:
        if needle in haystack:
            if hard and hard not in hard_tags:
                hard_tags.append(hard)
            if soft and soft not in soft_tags:
                soft_tags.append(soft)

    if not hard_tags:
        hard_tags.append(subject or "General")

    is_soft = subject in SOFT_SUBJECTS
    if is_soft:
        # Soft subjects keep their own topic tags so the chart is not empty,
        # but every question is marked soft so the tag is classified soft in
        # the skills profile.
        return hard_tags, list(hard_tags), True
    return hard_tags, soft_tags, False


def keywords_for(entry):
    existing = entry.get("keywords")
    if existing:
        return existing
    text = entry.get("text", "")
    words = []
    for w in text.replace("?", " ").replace(":", " ").replace(",", " ").split():
        if len(w) > 6 and w.lower() not in [x.lower() for x in words]:
            words.append(w)
    return words[:4]


def enrich(path):
    with io.open(path, "r", encoding="utf-8") as fh:
        data = json.load(fh)

    changed = 0
    for entry in data:
        hard_tags, soft_tags, is_soft = tags_for(entry)
        if "objective" not in entry:
            entry["objective"] = entry.get("explanation", "")[:160] or "Apply the concept correctly."
            changed += 1
        # Enrichment only fills gaps. Generated banks already carry curated
        # skillTags, and overwriting them here replaced those with the subject
        # name (e.g. "placement"), which polluted the skills profile.
        if not entry.get("skillTags"):
            entry["skillTags"] = hard_tags + soft_tags if soft_tags else hard_tags
        entry.setdefault("keywords", keywords_for(entry))
        if not entry.get("softSkill"):
            entry["softSkill"] = is_soft or bool(soft_tags)
        elif not is_soft and not entry.get("softSkill"):
            entry["softSkill"] = bool(soft_tags)

    with io.open(path, "w", encoding="utf-8") as fh:
        json.dump(data, fh, indent=2, ensure_ascii=False)
        fh.write("\n")
    return len(data), changed


def main():
    qdir = "data/questions"
    total = 0
    for name in sorted(os.listdir(qdir)):
        if not name.endswith(".json"):
            continue
        path = os.path.join(qdir, name)
        count, changed = enrich(path)
        total += count
        print("%-28s %3d questions, %3d objectives added" % (name, count, changed))
    print("total: %d" % total)


if __name__ == "__main__":
    main()

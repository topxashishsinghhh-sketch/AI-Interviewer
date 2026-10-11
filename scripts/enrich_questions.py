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
# Which tag group each topic belongs to. Exact topic names, so a subject tag
# such as "ds" never wins over "Graphs" for a graphs question.
TOPIC_TAGS = {
    # --- accounting ---
    "analysis": "Financial Analysis", "cash": "Financial Analysis",
    "cash_flow": "Financial Analysis", "revenue": "Financial Analysis",
    "ratios": "Financial Analysis", "books": "Accounting",
    "basics": "Accounting", "statements": "Accounting",
    "depreciation": "Depreciation", "inventory": "Inventory",
    # --- algorithms ---
    "amortized": "Complexity", "complexity": "Complexity", "np": "Complexity",
    "recursion": "Recursion", "sorting": "Sorting", "searching": "Searching",
    "dp": "Dynamic Programming", "greedy": "Greedy", "graphs": "Graphs",
    "strings": "Strings", "two_pointers": "Two Pointers",
    # --- analog ---
    "amplifiers": "Circuit Design", "opamps": "Circuit Design",
    "transistors": "Circuit Design", "capacitors": "Circuit Design",
    "diodes": "Circuit Design", "oscillators": "Circuit Design",
    "feedback": "Feedback Systems", "filters": "Circuit Design",
    "stability": "Feedback Systems", "noise": "Noise Analysis",
    # --- aptitude ---
    "arithmetic": "Arithmetic", "numbers": "Number Systems",
    "number_series": "Number Series", "averages": "Averages",
    "percentages": "Percentages", "ratios": "Ratio and Proportion",
    "probability": "Probability", "time_speed": "Time and Speed",
    "time_work": "Time and Work", "work_time": "Time and Work",
    "profit_loss": "Profit and Loss", "mixtures": "Mixtures and Alligation",
    # --- dbms ---
    "relational": "Relational Model", "sql": "SQL", "keys": "Keys and Constraints",
    "normalization": "Normalization", "transactions": "Transactions",
    "concurrency": "Concurrency", "indexing": "Indexing",
    "query": "Query Processing", "optimization": "Query Processing",
    # --- ds ---
    "arrays": "Arrays", "linked_lists": "Linked Lists", "stacks": "Stacks",
    "queues": "Queues", "hashing": "Hashing", "heaps": "Heaps",
    "trees": "Trees", "binary_trees": "Trees", "balanced_trees": "Balanced Trees",
    "graphs": "Graphs", "complexity": "Complexity Analysis",
    "disjoint_sets": "Disjoint Sets",
    # --- networks ---
    "layers": "Network Layers", "addressing": "Addressing",
    "protocols": "Protocols", "transport": "Transport",
    "congestion": "Congestion Control", "routing": "Routing",
    "security": "Network Security",
    # --- oop ---
    "encapsulation": "Encapsulation", "inheritance": "Inheritance",
    "polymorphism": "Polymorphism", "constructors": "Object Lifetime",
    "access_specifiers": "Encapsulation", "contracts": "Design Contracts",
    "patterns": "Design Patterns", "memory": "Object Lifetime",
    "design": "Software Design",
    # --- os ---
    "processes": "Processes", "scheduling": "CPU Scheduling",
    "concurrency": "Concurrency", "synchronization": "Synchronization",
    "deadlock": "Deadlocks", "memory": "Memory Management",
    "storage": "File Systems", "io": "I/O Systems",
    # --- reasoning ---
    "series": "Series and Patterns", "odd_one_out": "Classification",
    "coding": "Coding and Decoding", "coding_decoding": "Coding and Decoding",
    "blood_relations": "Family Relations", "directions": "Directions",
    "seating": "Seating Arrangement", "syllogism": "Syllogisms",
    "puzzles": "Logic Puzzles", "analogies": "Analogies",
    "logical_deduction": "Logical Deduction", "chess": "Board Reasoning",
    # --- signals ---
    "signal_types": "Signal Types", "convolution": "Convolution",
    "fourier": "Fourier Analysis", "dtft_properties": "Fourier Analysis",
    "sampling": "Sampling Theory", "filters": "Digital Filters",
    "z_transform": "Z Transform", "stability": "System Stability",
    "fft": "Fourier Analysis", "lti": "LTI Systems",
    "modulation": "Modulation",
}

# Subject-level fallback, used only when no topic rule matched.
SUBJECT_TAGS = {
    "accounting": "Accounting", "algo": "Algorithms", "analog": "Circuit Design",
    "aptitude": "Quantitative Aptitude", "dbms": "SQL & Databases",
    "ds": "Data Structures", "networks": "Computer Networks",
    "oop": "Object Oriented Programming", "os": "Operating Systems",
    "reasoning": "Logical Reasoning", "signals": "Signal Processing",
    "hr": "Behavioural Interviews", "placement": "Soft Skills",
}

# Behavioural/soft-skill topic rules. These run after the hard-skill rules, so a
# subject can end up in both groups: explaining an algorithm is a Communication
# question that lives in the Algorithms bank.
SOFT_TOPIC_RULES = [
    ("communication", "Communication"),
    ("communica", "Communication"),
    ("teamwork", "Teamwork"),
    ("team", "Teamwork"),
    ("conflict", "Conflict Resolution"),
    ("leadership", "Leadership"),
    ("negoti", "Negotiation"),
    ("career", "Career Planning"),
    ("adaptab", "Adaptability"),
    ("ownership", "Ownership"),
    ("scenario", "Problem Solving"),
    ("situational", "Problem Solving"),
    ("behavioural", "Behavioural Interviews"),
    ("behavio", "Behavioural Interviews"),
]

TAG_RULES = [
    # (substring of subject or topic, hard_tag, soft_tag)
    ("ds",      "Data Structures",  None),    ("tree",    "Data Structures",  None),
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

# Behavioural/soft-skill topic rules. These run after the hard-skill rules, so a
# subject can end up in both groups: explaining an algorithm is a Communication
# question that lives in the Algorithms bank.
SOFT_TOPIC_RULES = [
    ("communication", "Communication"),
    ("communica", "Communication"),
    ("teamwork", "Teamwork"),
    ("team", "Teamwork"),
    ("conflict", "Conflict Resolution"),
    ("leadership", "Leadership"),
    ("negoti", "Negotiation"),
    ("career", "Career Planning"),
    ("adaptab", "Adaptability"),
    ("ownership", "Ownership"),
    ("scenario", "Problem Solving"),
    ("situational", "Problem Solving"),
    ("behavioural", "Behavioural Interviews"),
    ("behavio", "Behavioural Interviews"),
]


def tags_for(entry):
    subject = entry.get("subject", "")
    topic = entry.get("topic", "")
    haystack = (subject + " " + topic).lower()

    is_soft = subject in SOFT_SUBJECTS
    hard_tags = []
    soft_tags = []

    if is_soft:
        # Soft subjects derive a specific behavioural tag from the topic, so the
        # skill chart shows real coverage rather than one generic bucket.
        for tag in soft_tags_for(subject, topic):
            if tag not in soft_tags:
                soft_tags.append(tag)
        if not soft_tags:
            soft_tags.append("Behavioural Interviews")
        return [], soft_tags, True

    # Technical subjects: a specific topic tag first, subject-level fallback.
    hard_tags = hard_tag_for(subject, topic)
    soft_tags = soft_tags_for(subject, topic)
    return hard_tags, soft_tags, False


def hard_tag_for(subject, topic):
    tag = TOPIC_TAGS.get(topic)
    if tag:
        return [tag]
    tag = SUBJECT_TAGS.get(subject)
    if tag:
        return [tag]

    # Word-aware substring fallback for topics not in the table. A plain
    # substring test on "hr" used to match "synchr-onization", which tagged
    # Operating Systems concurrency questions as soft skills.
    words = set(w for w in (subject + " " + topic).lower().replace("_", " ").split() if w)
    for needle, hard, _soft in TAG_RULES:
        if needle in words and hard:
            return [hard]
    return [subject or "General"]


def soft_tags_for(subject, topic):
    words = set(w for w in topic.lower().replace("_", " ").split() if w)
    out = []
    for needle, tag in SOFT_TOPIC_RULES:
        if needle in words and tag not in out:
            out.append(tag)
    return out


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
        # Authoritative, not "fill the gap": a previous run of this script
        # mis-tagged Operating Systems concurrency questions as soft skills, and
        # gap-filling could not undo its own earlier damage.
        entry["skillTags"] = hard_tags + soft_tags if soft_tags else hard_tags
        entry["softSkill"] = bool(is_soft or soft_tags)
        entry.setdefault("keywords", keywords_for(entry))

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

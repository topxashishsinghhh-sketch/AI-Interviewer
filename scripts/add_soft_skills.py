import json

new_hr = [
    {
        "id": "hr_star_conflict",
        "subject": "hr",
        "topic": "conflict_resolution",
        "difficulty": "Medium",
        "type": "Text",
        "text": "Tell me about a time you had a strong disagreement with a colleague or lead regarding a technical decision. How did you handle it and what was the outcome?",
        "keywords": ["situation", "perspective", "data", "trade-off", "listen", "consensus", "compromise", "outcome", "alignment"],
        "minKeywords": 3,
        "explanation": "Use the STAR method: articulate the context, respect alternative perspectives, base arguments on data/benchmarks rather than ego, and focus on team delivery and shared alignment.",
        "objective": "Demonstrate emotional intelligence, objective decision-making, and professional conflict de-escalation using STAR structure.",
        "skillTags": ["Conflict Resolution", "Communication", "Teamwork"],
        "softSkill": True
    },
    {
        "id": "hr_star_deadline_pressure",
        "subject": "hr",
        "topic": "ownership_and_delivery",
        "difficulty": "Medium",
        "type": "Text",
        "text": "Describe a scenario where you faced a tight project deadline with incomplete or changing specifications. How did you prioritize tasks and deliver?",
        "keywords": ["priority", "scope", "mvp", "communicate", "stakeholder", "deadline", "deliver", "trade-off"],
        "minKeywords": 3,
        "explanation": "Highlight proactive communication: clarifying ambiguity with stakeholders, de-scoping nice-to-haves to lock down an MVP, and delivering reliably under pressure.",
        "objective": "Evaluate adaptability, ownership, ruthless prioritization, and proactive stakeholder updates.",
        "skillTags": ["Time Management", "Adaptability", "Ownership"],
        "softSkill": True
    },
    {
        "id": "hr_star_failure_postmortem",
        "subject": "hr",
        "topic": "accountability",
        "difficulty": "Hard",
        "type": "Text",
        "text": "Tell me about a project or incident where something you built broke or missed user expectations. What was your immediate response, how did you fix it, and what safeguards did you put in place?",
        "keywords": ["ownership", "incident", "root cause", "postmortem", "fix", "prevention", "test", "monitor", "safeguard"],
        "minKeywords": 4,
        "explanation": "Take zero-defensiveness ownership. Walk through root-cause analysis, swift containment/rollback, transparent communication, and preventative measures (tests/monitoring).",
        "objective": "Demonstrate blameless accountability, root cause analysis, systemic problem-solving, and continuous learning.",
        "skillTags": ["Accountability", "Problem Solving", "Post-Mortem"],
        "softSkill": True
    },
    {
        "id": "hr_star_mentorship_leadership",
        "subject": "hr",
        "topic": "leadership",
        "difficulty": "Hard",
        "type": "Text",
        "text": "Describe a time you onboarded, mentored, or unblocked a struggling teammate or junior engineer. How did you identify their challenge and guide them to succeed?",
        "keywords": ["empathy", "pair programming", "feedback", "unblock", "guide", "growth", "confidence", "milestone"],
        "minKeywords": 3,
        "explanation": "Focus on empathy, patience, constructive actionable feedback, and empowering the teammate rather than doing their work for them.",
        "objective": "Assess team enablement, mentorship qualities, empathy, and positive engineering culture.",
        "skillTags": ["Mentorship", "Leadership", "Communication"],
        "softSkill": True
    },
    {
        "id": "hr_mcq_priority_triage",
        "subject": "hr",
        "topic": "workplace_scenarios",
        "difficulty": "Easy",
        "type": "MCQ",
        "text": "During a release cycle, QA files a high-severity bug 2 hours before scheduled production launch. What is the most professional response?",
        "options": [
            "Push the release anyway and fix it quietly the next day",
            "Immediately notify the release coordinator/team, triage reproduction steps and severity, and recommend halting or hotfixing",
            "Blame the QA team for discovering the bug too late in the cycle",
            "Mark the ticket as invalid or cannot reproduce to meet the deadline"
        ],
        "answer": 1,
        "explanation": "Integrity and transparent risk mitigation protect customer trust. Never hide defects or rush untested code into production.",
        "objective": "Identify production readiness protocols and transparent team communication.",
        "skillTags": ["Integrity", "Communication", "Risk Management"],
        "keywords": ["release", "severity", "triage", "production"],
        "softSkill": True
    },
    {
        "id": "hr_mcq_disagree_commit",
        "subject": "hr",
        "topic": "collaboration",
        "difficulty": "Medium",
        "type": "MCQ",
        "text": "What does the leadership principle 'Disagree and Commit' mean in software engineering?",
        "options": [
            "Disagree openly in public meetings, but refuse to work on the decision once finalized",
            "Vigorously debate and advocate your viewpoint with data during planning, but once the final team decision is made, fully support and execute it",
            "Never voice disagreement to maintain harmony within the engineering team",
            "Commit code first before discussing disagreements with peers"
        ],
        "answer": 1,
        "explanation": "Disagree and Commit encourages intellectual honesty during decision-making, followed by unified alignment and 100% effort during execution.",
        "objective": "Evaluate team synergy, healthy debate, and execution alignment.",
        "skillTags": ["Collaboration", "Leadership"],
        "keywords": ["disagree", "commit", "consensus", "execution"],
        "softSkill": True
    }
]

with open('data/questions/hr.json', 'r', encoding='utf-8') as f:
    existing = json.load(f)

existing_ids = {q['id'] for q in existing}
added = 0
for q in new_hr:
    if q['id'] not in existing_ids:
        existing.append(q)
        added += 1

with open('data/questions/hr.json', 'w', encoding='utf-8') as f:
    json.dump(existing, f, indent=2)

print(f'Successfully added {added} questions to hr.json. Total count: {len(existing)}')

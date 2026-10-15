// OOP: Service Layer (Business Logic Facade)
#include "course/CourseService.h"
#include "core/Course.h"
#include "core/Student.h"

namespace course {

CourseService::CourseService() {
    initializeCourses();
}

void CourseService::initializeCourses() {
    // CSE Course
    auto cse = std::make_shared<core::Course>("cse", "Computer Science Engineering", "Core CS subjects");
    
    auto ds = std::make_shared<core::Subject>("ds", "Data Structures", "Arrays, Lists, Trees, Graphs");
    ds->addTopic(std::make_shared<core::Topic>("arrays", "Arrays", "Array manipulation"));
    ds->addTopic(std::make_shared<core::Topic>("linked_lists", "Linked Lists", "Singly/doubly linked lists"));
    ds->addTopic(std::make_shared<core::Topic>("trees", "Trees", "Binary trees, BST, AVL"));
    ds->addTopic(std::make_shared<core::Topic>("graphs", "Graphs", "Traversal, shortest path"));
    cse->addSubject(ds);
    
    auto algo = std::make_shared<core::Subject>("algo", "Algorithms", "Sorting, Searching, DP");
    algo->addTopic(std::make_shared<core::Topic>("sorting", "Sorting", "Quick, Merge, Heap sort"));
    algo->addTopic(std::make_shared<core::Topic>("searching", "Searching", "Binary search, BFS, DFS"));
    algo->addTopic(std::make_shared<core::Topic>("dp", "Dynamic Programming", "Memoization, tabulation"));
    cse->addSubject(algo);
    
    auto oop = std::make_shared<core::Subject>("oop", "Object-Oriented Programming", "Classes, Inheritance, Patterns");
    oop->addTopic(std::make_shared<core::Topic>("encapsulation", "Encapsulation", "Data hiding, access modifiers"));
    oop->addTopic(std::make_shared<core::Topic>("inheritance", "Inheritance", "Single, multiple, virtual"));
    oop->addTopic(std::make_shared<core::Topic>("polymorphism", "Polymorphism", "Static, dynamic, templates"));
    oop->addTopic(std::make_shared<core::Topic>("patterns", "Design Patterns", "Creational, structural, behavioral"));
    cse->addSubject(oop);
    
    auto os = std::make_shared<core::Subject>("os", "Operating Systems", "Processes, memory, scheduling");
    os->addTopic(std::make_shared<core::Topic>("processes", "Processes", "Lifecycle, PCB"));
    os->addTopic(std::make_shared<core::Topic>("scheduling", "CPU Scheduling", "FCFS, SJF, MLFQ"));
    os->addTopic(std::make_shared<core::Topic>("synchronization", "Synchronization", "Semaphores, monitors"));
    os->addTopic(std::make_shared<core::Topic>("memory", "Memory", "Paging, virtual memory"));
    cse->addSubject(os);
    
    auto dbms = std::make_shared<core::Subject>("dbms", "Database Management Systems", "Relational model, SQL, transactions");
    dbms->addTopic(std::make_shared<core::Topic>("relational", "Relational Model", "Relations, algebra"));
    dbms->addTopic(std::make_shared<core::Topic>("sql", "SQL", "Queries, joins, aggregation"));
    dbms->addTopic(std::make_shared<core::Topic>("normalization", "Normalization", "1NF through BCNF"));
    dbms->addTopic(std::make_shared<core::Topic>("transactions", "Transactions", "ACID, isolation"));
    cse->addSubject(dbms);
    
    auto networks = std::make_shared<core::Subject>("networks", "Computer Networks", "Layers, protocols, congestion");
    networks->addTopic(std::make_shared<core::Topic>("layers", "Network Layers", "OSI, TCP/IP"));
    networks->addTopic(std::make_shared<core::Topic>("transport", "Transport", "TCP, UDP, flow control"));
    networks->addTopic(std::make_shared<core::Topic>("routing", "Routing", "Distance-vector, link-state"));
    networks->addTopic(std::make_shared<core::Topic>("congestion", "Congestion", "AIMD, slow start"));
    cse->addSubject(networks);
    
    courses_.push_back(cse);
    
    // ECE Course
    auto ece = std::make_shared<core::Course>("ece", "Electronics & Communication", "Core ECE subjects");
    
    auto signals = std::make_shared<core::Subject>("signals", "Signals & Systems", "Continuous/discrete signals");
    signals->addTopic(std::make_shared<core::Topic>("fourier", "Fourier Analysis", "Series, transform"));
    signals->addTopic(std::make_shared<core::Topic>("lti", "LTI Systems", "Convolution, stability"));
    ece->addSubject(signals);
    
    auto analog = std::make_shared<core::Subject>("analog", "Analog Circuits", "Diodes, transistors, op-amps");
    analog->addTopic(std::make_shared<core::Topic>("diodes", "Diodes", "PN junction, rectifiers"));
    analog->addTopic(std::make_shared<core::Topic>("transistors", "Transistors", "BJT, MOSFET"));
    analog->addTopic(std::make_shared<core::Topic>("opamps", "Op-Amps", "Configurations, applications"));
    ece->addSubject(analog);
    
    courses_.push_back(ece);
    
    // Commerce Course
    auto commerce = std::make_shared<core::Course>("commerce", "Commerce", "Business & Finance");
    
    auto accounting = std::make_shared<core::Subject>("accounting", "Accounting", "Financial accounting");
    accounting->addTopic(std::make_shared<core::Topic>("basics", "Basics", "Debits, credits, ledgers"));
    accounting->addTopic(std::make_shared<core::Topic>("statements", "Financial Statements", "Balance sheet, P&L"));
    commerce->addSubject(accounting);
    
    courses_.push_back(commerce);
    
    // Common Course
    auto common = std::make_shared<core::Course>("common", "Common Aptitude", "General aptitude");
    
    auto aptitude = std::make_shared<core::Subject>("aptitude", "Quantitative Aptitude", "Math & logic");
    aptitude->addTopic(std::make_shared<core::Topic>("numbers", "Number Systems", "Divisibility, HCF/LCM"));
    aptitude->addTopic(std::make_shared<core::Topic>("percentages", "Percentages", "Profit/loss, interest"));
    aptitude->addTopic(std::make_shared<core::Topic>("time_work", "Time & Work", "Pipes, cisterns"));
    common->addSubject(aptitude);
    
    auto reasoning = std::make_shared<core::Subject>("reasoning", "Logical Reasoning", "Patterns, puzzles");
    reasoning->addTopic(std::make_shared<core::Topic>("series", "Series", "Number, letter series"));
    reasoning->addTopic(std::make_shared<core::Topic>("puzzles", "Puzzles", "Arrangement, blood relations"));
    common->addSubject(reasoning);
    
    auto hr = std::make_shared<core::Subject>("hr", "HR Questions", "Behavioral questions");
    hr->addTopic(std::make_shared<core::Topic>("behavioral", "Behavioral", "STAR method, examples"));
    hr->addTopic(std::make_shared<core::Topic>("situational", "Situational", "Conflict, leadership"));
    common->addSubject(hr);
    
    courses_.push_back(common);

    // Soft Skills Course — theory plus practice questions per subject
    auto soft = std::make_shared<core::Course>("soft", "Soft Skills", "Communication, teamwork and workplace behaviour");

    auto communication = std::make_shared<core::Subject>("communication", "Communication Skills", "Clarity, listening, feedback");
    communication->addTopic(std::make_shared<core::Topic>("clarity", "Clarity", "Structure, concision, the ask"));
    communication->addTopic(std::make_shared<core::Topic>("active_listening", "Active Listening", "Paraphrasing, confirming understanding"));
    communication->addTopic(std::make_shared<core::Topic>("feedback", "Feedback", "Giving and receiving it"));
    soft->addSubject(communication);

    auto teamwork = std::make_shared<core::Subject>("teamwork", "Teamwork & Collaboration", "Coordination, trust, remote work");
    teamwork->addTopic(std::make_shared<core::Topic>("roles", "Roles", "Handoffs, ownership, shared outcomes"));
    teamwork->addTopic(std::make_shared<core::Topic>("remote_collaboration", "Remote Collaboration", "Visible state, async decisions"));
    teamwork->addTopic(std::make_shared<core::Topic>("trust", "Trust", "Disagreeing well, reliability"));
    soft->addSubject(teamwork);

    auto leadership = std::make_shared<core::Subject>("leadership", "Leadership & Initiative", "Influence, ownership, delegation");
    leadership->addTopic(std::make_shared<core::Topic>("influence", "Influence", "Initiative without authority"));
    leadership->addTopic(std::make_shared<core::Topic>("ownership", "Ownership", "Outcomes, escalation"));
    leadership->addTopic(std::make_shared<core::Topic>("delegation", "Delegation", "Outcomes over instructions"));
    soft->addSubject(leadership);

    auto timeManagement = std::make_shared<core::Subject>("time_management", "Time Management & Productivity", "Priorities, focus, commitments");
    timeManagement->addTopic(std::make_shared<core::Topic>("prioritization", "Prioritization", "Consequence over urgency"));
    timeManagement->addTopic(std::make_shared<core::Topic>("deep_work", "Deep Work", "Protecting focus"));
    timeManagement->addTopic(std::make_shared<core::Topic>("habits", "Habits", "Capacity, commitments, buffers"));
    soft->addSubject(timeManagement);

    auto adaptability = std::make_shared<core::Subject>("adaptability", "Adaptability & Learning Agility", "Change, feedback, unlearning");
    adaptability->addTopic(std::make_shared<core::Topic>("change", "Change", "Ambiguity, invalidated plans"));
    adaptability->addTopic(std::make_shared<core::Topic>("feedback_loop", "Feedback Loop", "Updating on evidence"));
    adaptability->addTopic(std::make_shared<core::Topic>("growth", "Growth", "Unlearning and transfer"));
    soft->addSubject(adaptability);

    auto conflictResolution = std::make_shared<core::Subject>("conflict_resolution", "Conflict Resolution", "Disagreement, negotiation, repair");
    conflictResolution->addTopic(std::make_shared<core::Topic>("disagreement", "Disagreement", "Positions versus interests"));
    conflictResolution->addTopic(std::make_shared<core::Topic>("negotiation", "Negotiation", "De-escalation, criteria"));
    conflictResolution->addTopic(std::make_shared<core::Topic>("repair", "Repair", "Restoring relationships"));
    soft->addSubject(conflictResolution);

    auto emotionalIntelligence = std::make_shared<core::Subject>("emotional_intelligence", "Emotional Intelligence", "Self-awareness, empathy, regulation");
    emotionalIntelligence->addTopic(std::make_shared<core::Topic>("self_awareness", "Self-Awareness", "Noticing your own state"));
    emotionalIntelligence->addTopic(std::make_shared<core::Topic>("empathy", "Empathy", "Checked hypotheses about others"));
    emotionalIntelligence->addTopic(std::make_shared<core::Topic>("regulation", "Regulation", "Composure as a practice"));
    soft->addSubject(emotionalIntelligence);

    auto workplaceEthics = std::make_shared<core::Subject>("workplace_ethics", "Workplace Ethics & Professionalism", "Integrity, boundaries, escalation");
    workplaceEthics->addTopic(std::make_shared<core::Topic>("integrity", "Integrity", "Honest reporting, credit"));
    workplaceEthics->addTopic(std::make_shared<core::Topic>("boundaries", "Boundaries", "Confidentiality, saying no"));
    workplaceEthics->addTopic(std::make_shared<core::Topic>("compliance", "Compliance", "Raising risk concerns"));
    soft->addSubject(workplaceEthics);

    auto careerPlanning = std::make_shared<core::Subject>("career_planning", "Career Planning & Growth", "Direction, networks, interviews");
    careerPlanning->addTopic(std::make_shared<core::Topic>("goals", "Goals", "Choosing a direction, evidence"));
    careerPlanning->addTopic(std::make_shared<core::Topic>("networking", "Networking", "Being useful early"));
    careerPlanning->addTopic(std::make_shared<core::Topic>("interviews", "Interviews", "Demonstrating reasoning"));
    soft->addSubject(careerPlanning);

    auto presentation = std::make_shared<core::Subject>("presentation", "Presentation & Storytelling", "Structure, delivery, visuals");
    presentation->addTopic(std::make_shared<core::Topic>("structure", "Structure", "Situation, problem, resolution"));
    presentation->addTopic(std::make_shared<core::Topic>("delivery", "Delivery", "Preparation and nerves"));
    presentation->addTopic(std::make_shared<core::Topic>("visuals", "Visuals", "One idea per slide"));
    soft->addSubject(presentation);

    courses_.push_back(soft);
}

const std::vector<std::shared_ptr<core::Course>>& CourseService::courses() const {
    return courses_;
}

std::shared_ptr<core::Course> CourseService::findCourse(const std::string& id) const {
    for (const auto& c : courses_) {
        if (c->id() == id) return c;
    }
    return nullptr;
}

std::shared_ptr<core::Subject> CourseService::findSubject(const std::string& courseId, const std::string& subjectId) const {
    auto course = findCourse(courseId);
    if (course) return course->findSubject(subjectId);
    return nullptr;
}

std::shared_ptr<core::Topic> CourseService::findTopic(const std::string& courseId, const std::string& subjectId, const std::string& topicId) const {
    auto subject = findSubject(courseId, subjectId);
    if (subject) return subject->findTopic(topicId);
    return nullptr;
}

std::vector<std::shared_ptr<core::Subject>> CourseService::subjectsForCourse(const std::string& courseId) const {
    auto course = findCourse(courseId);
    if (course) return course->subjects();
    return {};
}

std::vector<std::shared_ptr<core::Topic>> CourseService::topicsForSubject(const std::string& courseId, const std::string& subjectId) const {
    auto subject = findSubject(courseId, subjectId);
    if (subject) return subject->topics();
    return {};
}

bool CourseService::isUnlocked(const core::UserPtr& user, const std::string& courseId, const std::string& subjectId, const std::string& topicId) const {
    auto subject = findSubject(courseId, subjectId);
    if (!subject) return false;

    auto topics = subject->topics();
    if (topics.empty()) return true;

    // First topic is always unlocked
    if (topics[0]->id() == topicId) return true;

    // Unknown topic: allow (generous for subjects without explicit ordering)
    int topicIndex = -1;
    for (size_t i = 0; i < topics.size(); ++i) {
        if (topics[i]->id() == topicId) {
            topicIndex = static_cast<int>(i);
            break;
        }
    }
    if (topicIndex < 0) return true;

    // Unlock threshold: previous topic requires >= 3 solved problems overall
    const int requiredSolved = 3;
    int solved = 0;
    if (user) {
        if (auto student = std::dynamic_pointer_cast<core::Student>(user)) {
            solved = student->totalSolved();
        } else {
            // Admins bypass progression
            return true;
        }
    } else {
        return true;
    }

    return solved >= requiredSolved * topicIndex;
}

} // namespace course
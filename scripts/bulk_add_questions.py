"""
Script to inject a substantial, high-quality question bank across DSA, System Design, 
OOP in C++, and STAR Behavioral/Soft Skills.
"""
import json
import os

QUESTIONS_DIR = os.path.join("data", "questions")

def append_questions(filename, new_items):
    filepath = os.path.join(QUESTIONS_DIR, filename)
    with open(filepath, "r", encoding="utf-8") as f:
        existing = json.load(f)
    existing_ids = {q["id"] for q in existing}
    added = 0
    for item in new_items:
        if item["id"] not in existing_ids:
            existing.append(item)
            added += 1
    with open(filepath, "w", encoding="utf-8") as f:
        json.dump(existing, f, indent=2)
    print(f"[{filename}] Added {added} questions. Total: {len(existing)}")

# 1. Algorithms & LeetCode Coding Questions
ALGO_NEW = [
    {
        "id": "algo_coding_merge_intervals",
        "subject": "algo",
        "topic": "intervals",
        "difficulty": "Medium",
        "type": "Coding",
        "text": "Given an array of intervals where intervals[i] = [starti, endi], merge all overlapping intervals, and return an array of the non-overlapping intervals that cover all the intervals in the input. Format: read N pairs, output merged intervals space separated.",
        "templateCode": """#include <iostream>
#include <vector>
#include <algorithm>

std::vector<std::pair<int, int>> mergeIntervals(std::vector<std::pair<int, int>>& intervals) {
    // TODO: Sort by start time and merge overlaps
    return {};
}

int main() {
    int n;
    if (!(std::cin >> n)) return 0;
    std::vector<std::pair<int, int>> intervals(n);
    for (int i = 0; i < n; ++i) std::cin >> intervals[i].first >> intervals[i].second;
    auto res = mergeIntervals(intervals);
    for (size_t i = 0; i < res.size(); ++i) {
        if (i > 0) std::cout << " ";
        std::cout << res[i].first << " " << res[i].second;
    }
    std::cout << std::endl;
    return 0;
}
""",
        "testCases": [
            {
                "input": "4\n1 3 2 6 8 10 15 18",
                "expectedOutput": "1 6 8 10 15 18",
                "isHidden": False
            },
            {
                "input": "2\n1 4 4 5",
                "expectedOutput": "1 5",
                "isHidden": False
            },
            {
                "input": "3\n1 4 0 2 3 5",
                "expectedOutput": "0 5",
                "isHidden": True
            }
        ],
        "explanation": "Sort intervals by starting time in O(n log n), then iterate through comparing the current interval start with the previous interval end.",
        "objective": "Apply greedy interval scheduling and sorting techniques.",
        "skillTags": ["Intervals", "Greedy", "Sorting"],
        "keywords": ["intervals", "overlapping", "merge", "sorting"],
        "softSkill": False
    },
    {
        "id": "algo_coding_trapping_rain_water",
        "subject": "algo",
        "topic": "two_pointers",
        "difficulty": "Hard",
        "type": "Coding",
        "text": "Given n non-negative integers representing an elevation map where the width of each bar is 1, compute how much water it can trap after raining using the Two Pointers approach in O(n) time and O(1) extra space.",
        "templateCode": """#include <iostream>
#include <vector>
#include <algorithm>

long long trapWater(const std::vector<int>& height) {
    // TODO: Two pointers technique
    return 0;
}

int main() {
    int n;
    if (!(std::cin >> n)) return 0;
    std::vector<int> height(n);
    for (int i = 0; i < n; ++i) std::cin >> height[i];
    std::cout << trapWater(height) << std::endl;
    return 0;
}
""",
        "testCases": [
            {
                "input": "12\n0 1 0 2 1 0 1 3 2 1 2 1",
                "expectedOutput": "6",
                "isHidden": False
            },
            {
                "input": "6\n4 2 0 3 2 5",
                "expectedOutput": "9",
                "isHidden": False
            },
            {
                "input": "3\n3 0 2",
                "expectedOutput": "2",
                "isHidden": True
            }
        ],
        "explanation": "Maintain left and right pointers and tracking variables left_max and right_max. Move the pointer on the shorter boundary inward.",
        "objective": "Master two-pointer elevation boundary scanning in O(1) auxiliary space.",
        "skillTags": ["Two Pointers", "Dynamic Programming", "Arrays"],
        "keywords": ["trapping", "rain water", "two pointers", "elevation"],
        "softSkill": False
    },
    {
        "id": "algo_coding_coin_change",
        "subject": "algo",
        "topic": "dynamic_programming",
        "difficulty": "Medium",
        "type": "Coding",
        "text": "Given an integer array coins representing coins of different denominations and an integer amount, return the fewest number of coins that you need to make up that amount. If that amount cannot be made up, return -1.",
        "templateCode": """#include <iostream>
#include <vector>
#include <algorithm>

int coinChange(const std::vector<int>& coins, int amount) {
    // TODO: DP tabulation
    return -1;
}

int main() {
    int n, amount;
    if (!(std::cin >> n >> amount)) return 0;
    std::vector<int> coins(n);
    for (int i = 0; i < n; ++i) std::cin >> coins[i];
    std::cout << coinChange(coins, amount) << std::endl;
    return 0;
}
""",
        "testCases": [
            {
                "input": "3 11\n1 2 5",
                "expectedOutput": "3",
                "isHidden": False
            },
            {
                "input": "1 3\n2",
                "expectedOutput": "-1",
                "isHidden": False
            },
            {
                "input": "1 0\n1",
                "expectedOutput": "0",
                "isHidden": True
            }
        ],
        "explanation": "Bottom-up tabulation dp[i] represents minimum coins for amount i. Transition: dp[i] = min(dp[i], dp[i - coin] + 1).",
        "objective": "Apply unbounded knapsack recurrence relations.",
        "skillTags": ["Dynamic Programming", "Knapsack"],
        "keywords": ["coin change", "fewest coins", "dynamic programming"],
        "softSkill": False
    },
    {
        "id": "algo_coding_binary_search_rotated",
        "subject": "algo",
        "topic": "binary_search",
        "difficulty": "Medium",
        "type": "Coding",
        "text": "There is an integer array nums sorted in ascending order with distinct values, rotated at an unknown pivot index. Given nums and target, return the index of target, or -1 if not found in O(log n) time.",
        "templateCode": """#include <iostream>
#include <vector>

int searchRotated(const std::vector<int>& nums, int target) {
    // TODO: Modified binary search
    return -1;
}

int main() {
    int n, target;
    if (!(std::cin >> n >> target)) return 0;
    std::vector<int> nums(n);
    for (int i = 0; i < n; ++i) std::cin >> nums[i];
    std::cout << searchRotated(nums, target) << std::endl;
    return 0;
}
""",
        "testCases": [
            {
                "input": "7 0\n4 5 6 7 0 1 2",
                "expectedOutput": "4",
                "isHidden": False
            },
            {
                "input": "7 3\n4 5 6 7 0 1 2",
                "expectedOutput": "-1",
                "isHidden": False
            },
            {
                "input": "1 0\n0",
                "expectedOutput": "0",
                "isHidden": True
            }
        ],
        "explanation": "At any midpoint, at least one half (left or right) is guaranteed to be normally sorted. Determine which half is sorted and test if target falls within it.",
        "objective": "Master logarithmic binary search with rotational invariants.",
        "skillTags": ["Binary Search", "Arrays"],
        "keywords": ["rotated", "binary search", "log n"],
        "softSkill": False
    }
]

# 2. Data Structures
DS_NEW = [
    {
        "id": "ds_coding_reverse_linked_list",
        "subject": "ds",
        "topic": "linked_lists",
        "difficulty": "Easy",
        "type": "Coding",
        "text": "Given the head of a singly linked list represented as space-separated integers, reverse the list iteratively in O(n) time and O(1) space, and print the reversed elements space-separated.",
        "templateCode": """#include <iostream>
#include <vector>

struct ListNode {
    int val;
    ListNode* next;
    ListNode(int x) : val(x), next(nullptr) {}
};

ListNode* reverseList(ListNode* head) {
    // TODO: 3-pointer iterative reversal
    return nullptr;
}

int main() {
    int n;
    if (!(std::cin >> n) || n <= 0) return 0;
    ListNode dummy(0);
    ListNode* tail = &dummy;
    for (int i = 0; i < n; ++i) {
        int v; std::cin >> v;
        tail->next = new ListNode(v);
        tail = tail->next;
    }
    ListNode* rev = reverseList(dummy.next);
    while (rev) {
        std::cout << rev->val << (rev->next ? " " : "");
        rev = rev->next;
    }
    std::cout << std::endl;
    return 0;
}
""",
        "testCases": [
            {
                "input": "5\n1 2 3 4 5",
                "expectedOutput": "5 4 3 2 1",
                "isHidden": False
            },
            {
                "input": "2\n1 2",
                "expectedOutput": "2 1",
                "isHidden": False
            },
            {
                "input": "1\n42",
                "expectedOutput": "42",
                "isHidden": True
            }
        ],
        "explanation": "Use prev, curr, and next pointers to reverse link directions in place without extra memory allocations.",
        "objective": "Pointer manipulation and in-place list transformations.",
        "skillTags": ["Linked Lists", "Pointers"],
        "keywords": ["reverse", "linked list", "iterative", "pointers"],
        "softSkill": False
    },
    {
        "id": "ds_coding_min_stack",
        "subject": "ds",
        "topic": "stacks",
        "difficulty": "Medium",
        "type": "Coding",
        "text": "Design a stack that supports push, pop, top, and retrieving the minimum element in constant O(1) time.",
        "templateCode": """#include <iostream>
#include <stack>
#include <string>

class MinStack {
    std::stack<int> s;
    std::stack<int> min_s;
public:
    void push(int val) {
        // TODO: Push value and maintain minimum
    }
    void pop() {
        // TODO: Pop from stack and min_stack
    }
    int top() {
        return s.top();
    }
    int getMin() {
        return min_s.top();
    }
};

int main() {
    int q;
    if (!(std::cin >> q)) return 0;
    MinStack ms;
    while (q--) {
        std::string op;
        std::cin >> op;
        if (op == "push") {
            int val; std::cin >> val;
            ms.push(val);
        } else if (op == "pop") {
            ms.pop();
        } else if (op == "top") {
            std::cout << ms.top() << std::endl;
        } else if (op == "getMin") {
            std::cout << ms.getMin() << std::endl;
        }
    }
    return 0;
}
""",
        "testCases": [
            {
                "input": "6\npush -2\npush 0\npush -3\ngetMin\npop\ntop",
                "expectedOutput": "-3\n0",
                "isHidden": False
            },
            {
                "input": "4\npush 5\npush 2\ngetMin\npop",
                "expectedOutput": "2",
                "isHidden": True
            }
        ],
        "explanation": "Maintain an auxiliary stack tracking the minimum element seen so far at each height of the main stack.",
        "objective": "Stack tracking patterns and constant-time invariant preservation.",
        "skillTags": ["Stack", "Data Structure Design"],
        "keywords": ["min stack", "constant time", "getMin"],
        "softSkill": False
    },
    {
        "id": "ds_coding_tree_diameter",
        "subject": "ds",
        "topic": "trees",
        "difficulty": "Medium",
        "type": "Coding",
        "text": "Given the root of a binary tree, return the length of the diameter of the tree. The diameter of a binary tree is the length of the longest path between any two nodes in a tree (number of edges).",
        "templateCode": """#include <iostream>
#include <algorithm>

struct TreeNode {
    int val;
    TreeNode *left, *right;
    TreeNode(int x) : val(x), left(nullptr), right(nullptr) {}
};

int depth(TreeNode* root, int& diameter) {
    // TODO: compute depth and update max diameter
    return 0;
}

int diameterOfBinaryTree(TreeNode* root) {
    int diameter = 0;
    depth(root, diameter);
    return diameter;
}

int main() {
    // Construct sample tree: 1 -> (2 -> 4, 5), 3
    TreeNode* root = new TreeNode(1);
    root->left = new TreeNode(2);
    root->right = new TreeNode(3);
    root->left->left = new TreeNode(4);
    root->left->right = new TreeNode(5);
    std::cout << diameterOfBinaryTree(root) << std::endl;
    return 0;
}
""",
        "testCases": [
            {
                "input": "test",
                "expectedOutput": "3",
                "isHidden": False
            }
        ],
        "explanation": "At every node, the longest path passing through it is left_height + right_height. Compute tree height via DFS in O(n) while updating global maximum.",
        "objective": "Tree DFS traversal and depth computation.",
        "skillTags": ["Trees", "Binary Tree", "DFS"],
        "keywords": ["diameter", "binary tree", "longest path", "dfs"],
        "softSkill": False
    }
]

# 3. C++ and OOP Best Practices
OOP_NEW = [
    {
        "id": "oop_coding_singleton_threadsafe",
        "subject": "oop",
        "topic": "design_patterns",
        "difficulty": "Medium",
        "type": "Coding",
        "text": "Implement a thread-safe Meyers Singleton in C++11/17. Demonstrate that calling getInstance() from multiple calls returns a reference to the exact same memory instance.",
        "templateCode": """#include <iostream>

class ConfigurationManager {
private:
    ConfigurationManager() { /* Private constructor */ }
    ConfigurationManager(const ConfigurationManager&) = delete;
    ConfigurationManager& operator=(const ConfigurationManager&) = delete;

public:
    static ConfigurationManager& getInstance() {
        // TODO: Thread-safe Meyer's Singleton using static local
        static ConfigurationManager instance;
        return instance;
    }

    void test() {
        std::cout << "Singleton Active" << std::endl;
    }
};

int main() {
    auto& a = ConfigurationManager::getInstance();
    auto& b = ConfigurationManager::getInstance();
    std::cout << (&a == &b ? "SAME_INSTANCE" : "DIFFERENT") << std::endl;
    return 0;
}
""",
        "testCases": [
            {
                "input": "run",
                "expectedOutput": "SAME_INSTANCE",
                "isHidden": False
            }
        ],
        "explanation": "C++11 standard §6.7 specifies that static local variables are initialized exactly once in a thread-safe manner (Meyers Singleton).",
        "objective": "Implement thread-safe creational design patterns in modern C++.",
        "skillTags": ["Design Patterns", "C++", "Singleton"],
        "keywords": ["singleton", "thread-safe", "meyers singleton", "design patterns"],
        "softSkill": False
    },
    {
        "id": "oop_text_solid_principles",
        "subject": "oop",
        "topic": "solid",
        "difficulty": "Medium",
        "type": "Text",
        "text": "Explain the Liskov Substitution Principle (LSP) and how violating it with a classic Rectangle/Square inheritance hierarchy breaks client code.",
        "keywords": ["preconditions", "postconditions", "invariants", "inheritance", "behavior", "contract", "subtype"],
        "minKeywords": 3,
        "explanation": "LSP states that subtypes must be substitutable for their base types without altering program correctness. A Square cannot cleanly inherit from Rectangle if setting width independently alters height.",
        "objective": "Master Object-Oriented Design principles and polymorphism contracts.",
        "skillTags": ["SOLID", "OOP Architecture", "Inheritance"],
        "softSkill": False
    },
    {
        "id": "oop_text_raii_rule_of_five",
        "subject": "oop",
        "topic": "resource_management",
        "difficulty": "Hard",
        "type": "Text",
        "text": "Explain Resource Acquisition Is Initialization (RAII) and detail the Rule of Five in Modern C++ (C++11 and later).",
        "keywords": ["destructor", "copy constructor", "copy assignment", "move constructor", "move assignment", "resource", "deterministic"],
        "minKeywords": 4,
        "explanation": "RAII binds the lifecycle of a resource to object lifetime. The Rule of Five mandates defining destructor, copy constructor, copy assignment, move constructor, and move assignment when managing raw resources.",
        "objective": "Demonstrate modern C++ memory management and move semantics proficiency.",
        "skillTags": ["RAII", "Rule of 5", "Modern C++"],
        "softSkill": False
    }
]

# 4. Behavioral & Leadership Soft Skills
HR_NEW = [
    {
        "id": "hr_star_cross_functional_influence",
        "subject": "hr",
        "topic": "cross_functional",
        "difficulty": "Medium",
        "type": "Text",
        "text": "Describe a scenario where you had to persuade product managers, designers, or non-technical stakeholders to prioritize critical technical debt or security refactoring over new shiny features.",
        "keywords": ["data", "business impact", "risk", "customer", "metrics", "latency", "reliability", "compromise"],
        "minKeywords": 3,
        "explanation": "Bridge technical risk to business outcomes: frame technical debt in terms of outage risk, feature velocity drag, or infrastructure dollar costs.",
        "objective": "Assess cross-functional communication, empathy, and business alignment.",
        "skillTags": ["Stakeholder Management", "Communication", "Technical Debt"],
        "softSkill": True
    },
    {
        "id": "hr_star_ambiguous_requirements",
        "subject": "hr",
        "topic": "ambiguity",
        "difficulty": "Hard",
        "type": "Text",
        "text": "Tell me about a time you were given a highly ambiguous project requirement with minimal guidance. How did you break it down, validate assumptions, and drive clarity?",
        "keywords": ["assumptions", "prototype", "clarify", "milestones", "feedback loops", "iteration", "discovery"],
        "minKeywords": 3,
        "explanation": "Show bias for action: formulate hypotheses, interview users/stakeholders, build rapid proof-of-concept prototypes, and establish tight feedback loops.",
        "objective": "Evaluate autonomous leadership, ambiguity tolerance, and problem decomposition.",
        "skillTags": ["Autonomous Execution", "Ambiguity", "Leadership"],
        "softSkill": True
    },
    {
        "id": "hr_mcq_code_review_empathy",
        "subject": "hr",
        "topic": "code_review_culture",
        "difficulty": "Easy",
        "type": "MCQ",
        "text": "What is the most constructive way to leave feedback on a pull request when a peer writes suboptimal code?",
        "options": [
            "Reject the PR immediately with 'Rewrite this, it is slow and unreadable'",
            "Explain the rationale kindly, suggest a concrete alternative or link to docs, and phrase critiques around the code rather than the person",
            "Silently approve the PR to avoid hurting their feelings",
            "Take over their branch and rewrite the code yourself"
        ],
        "answer": 1,
        "explanation": "Constructive code reviews focus on mentoring, psychological safety, and objective rationale without personal critiques.",
        "objective": "Promote psychological safety and positive engineering code review culture.",
        "skillTags": ["Code Review", "Team Culture", "Empathy"],
        "keywords": ["code review", "pull request", "constructive", "empathy"],
        "softSkill": True
    }
]

def main():
    append_questions("algorithms.json", ALGO_NEW)
    append_questions("data_structures.json", DS_NEW)
    append_questions("cpp_oop.json", OOP_NEW)
    append_questions("hr.json", HR_NEW)

if __name__ == "__main__":
    main()

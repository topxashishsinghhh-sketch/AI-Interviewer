import json

new_coding_problems = [
    {
        "id": "algo_coding_two_sum",
        "subject": "algo",
        "topic": "arrays_hashing",
        "difficulty": "Easy",
        "type": "Coding",
        "text": "Given an array of integers nums and an integer target, return indices of the two numbers such that they add up to target. You may assume each input has exactly one solution, and you may not use the same element twice. Return the indices separated by a space in ascending order.",
        "templateCode": """#include <iostream>
#include <vector>
#include <unordered_map>

std::pair<int, int> twoSum(const std::vector<int>& nums, int target) {
    // TODO: Use hash map for O(n) lookup
    return {-1, -1};
}

int main() {
    int n, target;
    if (!(std::cin >> n >> target)) return 0;
    std::vector<int> nums(n);
    for (int i = 0; i < n; ++i) std::cin >> nums[i];
    auto res = twoSum(nums, target);
    std::cout << res.first << " " << res.second << std::endl;
    return 0;
}
""",
        "testCases": [
            {
                "input": "4 9\n2 7 11 15",
                "expectedOutput": "0 1",
                "isHidden": False
            },
            {
                "input": "3 6\n3 2 4",
                "expectedOutput": "1 2",
                "isHidden": False
            },
            {
                "input": "2 6\n3 3",
                "expectedOutput": "0 1",
                "isHidden": True
            }
        ],
        "explanation": "Store elements in an unordered_map mapping value to its index. For each number x, check if (target - x) already exists in O(1) average time.",
        "objective": "Achieve O(n) time complexity using hash maps instead of the naive O(n^2) brute force.",
        "skillTags": ["Arrays", "Hash Table", "Two Pointers"],
        "keywords": ["two sum", "target", "hash map", "indices"],
        "softSkill": False
    },
    {
        "id": "algo_coding_lru_cache",
        "subject": "algo",
        "topic": "design_data_structures",
        "difficulty": "Hard",
        "type": "Coding",
        "text": "Design a data structure that follows the constraints of a Least Recently Used (LRU) cache. Implement LRUCache with capacity, get(key), and put(key, value) in O(1) average time complexity.",
        "templateCode": """#include <iostream>
#include <unordered_map>
#include <list>
#include <string>

class LRUCache {
    int capacity;
    std::list<std::pair<int, int>> items; // key, value
    std::unordered_map<int, std::list<std::pair<int, int>>::iterator> cache;
public:
    LRUCache(int cap) : capacity(cap) {}

    int get(int key) {
        // TODO: retrieve value and move node to front of MRU list
        return -1;
    }

    void put(int key, int value) {
        // TODO: update or insert key-value, evicting LRU element if over capacity
    }
};

int main() {
    int cap, q;
    if (!(std::cin >> cap >> q)) return 0;
    LRUCache lru(cap);
    while (q--) {
        std::string op;
        std::cin >> op;
        if (op == "put") {
            int k, v;
            std::cin >> k >> v;
            lru.put(k, v);
        } else if (op == "get") {
            int k;
            std::cin >> k;
            std::cout << lru.get(k) << std::endl;
        }
    }
    return 0;
}
""",
        "testCases": [
            {
                "input": "2 6\nput 1 1\nput 2 2\nget 1\nput 3 3\nget 2\nget 3",
                "expectedOutput": "1\n-1\n3",
                "isHidden": False
            },
            {
                "input": "1 4\nput 10 100\nget 10\nput 20 200\nget 10",
                "expectedOutput": "100\n-1",
                "isHidden": True
            }
        ],
        "explanation": "Use a doubly linked list (std::list) for maintaining access order alongside an unordered_map mapping keys to list iterators to guarantee O(1) operations.",
        "objective": "Demonstrate understanding of compound data structures: combining Hash Table + Doubly Linked List for constant-time eviction.",
        "skillTags": ["Design", "Doubly Linked List", "Hash Table"],
        "keywords": ["lru", "cache", "eviction", "doubly linked list", "o(1)"],
        "softSkill": False
    },
    {
        "id": "algo_coding_valid_parentheses",
        "subject": "algo",
        "topic": "stacks",
        "difficulty": "Easy",
        "type": "Coding",
        "text": "Given a string s containing just the characters '(', ')', '{', '}', '[' and ']', determine if the input string is valid. Open brackets must be closed by the same type of brackets in the correct order. Print 'true' or 'false'.",
        "templateCode": """#include <iostream>
#include <string>
#include <stack>

bool isValid(const std::string& s) {
    // TODO: use stack to validate matching pairs
    return false;
}

int main() {
    std::string s;
    if (!(std::cin >> s)) return 0;
    std::cout << (isValid(s) ? "true" : "false") << std::endl;
    return 0;
}
""",
        "testCases": [
            {
                "input": "()[]{}",
                "expectedOutput": "true",
                "isHidden": False
            },
            {
                "input": "(]",
                "expectedOutput": "false",
                "isHidden": False
            },
            {
                "input": "([{}])",
                "expectedOutput": "true",
                "isHidden": True
            },
            {
                "input": "((",
                "expectedOutput": "false",
                "isHidden": True
            }
        ],
        "explanation": "Push opening brackets onto a stack. When encountering a closing bracket, verify the stack is not empty and the top element matches the corresponding opening bracket.",
        "objective": "Master standard Stack LIFO parsing and handle empty stack boundary conditions.",
        "skillTags": ["Stack", "Strings"],
        "keywords": ["stack", "parentheses", "matching", "brackets"],
        "softSkill": False
    },
    {
        "id": "algo_coding_max_subarray",
        "subject": "algo",
        "topic": "dynamic_programming",
        "difficulty": "Medium",
        "type": "Coding",
        "text": "Given an integer array nums, find the contiguous subarray (containing at least one number) which has the largest sum and print its sum (Kadane's Algorithm).",
        "templateCode": """#include <iostream>
#include <vector>
#include <algorithm>

long long maxSubArray(const std::vector<int>& nums) {
    // TODO: Kadane's algorithm
    return 0;
}

int main() {
    int n;
    if (!(std::cin >> n)) return 0;
    std::vector<int> nums(n);
    for (int i = 0; i < n; ++i) std::cin >> nums[i];
    std::cout << maxSubArray(nums) << std::endl;
    return 0;
}
""",
        "testCases": [
            {
                "input": "9\n-2 1 -3 4 -1 2 1 -5 4",
                "expectedOutput": "6",
                "isHidden": False
            },
            {
                "input": "1\n1",
                "expectedOutput": "1",
                "isHidden": False
            },
            {
                "input": "5\n-5 -4 -3 -2 -1",
                "expectedOutput": "-1",
                "isHidden": True
            }
        ],
        "explanation": "Kadane's algorithm keeps track of current_max = max(x, current_max + x) and updates overall_max in O(n) time and O(1) space.",
        "objective": "Implement Kadane's algorithm with proper handling of all-negative arrays.",
        "skillTags": ["Dynamic Programming", "Arrays", "Kadane"],
        "keywords": ["kadane", "subarray", "maximum sum", "contiguous"],
        "softSkill": False
    }
]

with open('data/questions/algorithms.json', 'r', encoding='utf-8') as f:
    existing = json.load(f)

existing_ids = {q['id'] for q in existing}
added = 0
for q in new_coding_problems:
    if q['id'] not in existing_ids:
        existing.append(q)
        added += 1

with open('data/questions/algorithms.json', 'w', encoding='utf-8') as f:
    json.dump(existing, f, indent=2)

print(f'Successfully added {added} coding problems to algorithms.json. Total count: {len(existing)}')

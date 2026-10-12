#!/usr/bin/env python3
"""
add_subject_questions.py
Adds 26 comprehensive interview questions across OS, DBMS, Networks, C++ OOP, Algorithms, and Data Structures.
Follows exact C++ JsonQuestionRepository schema for Coding questions:
- templateCode (string)
- testCases: [{ "input": str, "expectedOutput": str, "isHidden": bool }]
"""
import json
import os

new_os_questions = [
    {
        "id": "os_med_virtual_memory_tlb",
        "subject": "os",
        "topic": "memory_management",
        "difficulty": "Medium",
        "type": "MCQ",
        "text": "What happens during a Translation Lookaside Buffer (TLB) miss in modern x86-64 virtual memory architectures?",
        "options": [
            "The Memory Management Unit (MMU) walks multi-level page tables in physical RAM to translate the virtual address to physical address",
            "The operating system halts and prompts the CPU to execute a hard reboot",
            "The kernel immediately throws a segmentation fault (SIGSEGV) terminating the process",
            "The CPU swaps the entire process address space onto secondary disk storage"
        ],
        "answer": 0,
        "explanation": "On a TLB miss, a hardware page table walk traverses the multi-level page hierarchy (PML4, PDPT, PD, PT) to resolve the physical page frame address and load the entry into the TLB.",
        "objective": "Understand virtual memory address translation, TLB caching hierarchy, and hardware page table walks.",
        "skillTags": ["Virtual Memory", "TLB", "Memory Management"],
        "keywords": ["tlb", "virtual memory", "mmu", "page table", "address translation"],
        "softSkill": False
    },
    {
        "id": "os_med_mutex_vs_semaphore",
        "subject": "os",
        "topic": "concurrency",
        "difficulty": "Medium",
        "type": "MCQ",
        "text": "What is the primary conceptual difference between a Mutex and a Counting Semaphore in operating system synchronization?",
        "options": [
            "A Mutex has an ownership semantic (only the acquiring thread can release it), whereas a Semaphore can be signaled by any thread",
            "A Mutex allows up to 5 concurrent threads, while a Semaphore allows only 1",
            "A Semaphore operates strictly in user-space while a Mutex always triggers a kernel interrupt",
            "There is no difference; Mutex is simply the POSIX name for Semaphore"
        ],
        "answer": 0,
        "explanation": "A mutex enforces ownership: only the thread that locked the mutex can unlock it. A semaphore maintains an integer count and can be used for signaling across different threads.",
        "objective": "Distinguish between mutual exclusion locks and signaling primitives in multi-threaded programming.",
        "skillTags": ["Concurrency", "Mutex", "Semaphores", "Synchronization"],
        "keywords": ["mutex", "semaphore", "synchronization", "ownership", "concurrency"],
        "softSkill": False
    },
    {
        "id": "os_hard_deadlock_coffman",
        "subject": "os",
        "topic": "deadlock",
        "difficulty": "Hard",
        "type": "MCQ",
        "text": "Which of the following describes the four Coffman conditions necessary for a deadlock to occur in an operating system?",
        "options": [
            "Mutual Exclusion, Hold and Wait, No Preemption, Circular Wait",
            "Preemption, Race Condition, Starvation, Aging",
            "Paging, Segmentation, Swapping, Thrashing",
            "Virtualization, Multiprocessing, Scheduling, Polling"
        ],
        "answer": 0,
        "explanation": "A deadlock can only occur if all four Coffman conditions simultaneously hold: Mutual Exclusion, Hold and Wait, No Preemption, and Circular Wait.",
        "objective": "Analyze resource allocation graphs and deadlock prevention mechanisms using the Coffman conditions.",
        "skillTags": ["Deadlock", "Resource Allocation", "Operating Systems"],
        "keywords": ["deadlock", "coffman", "mutual exclusion", "circular wait", "no preemption"],
        "softSkill": False
    },
    {
        "id": "os_med_cow_fork",
        "subject": "os",
        "topic": "processes",
        "difficulty": "Medium",
        "type": "Text",
        "text": "Explain Copy-on-Write (COW) during a Unix fork() system call. How does it optimize process creation and memory utilization?",
        "explanation": "When fork() is called, the child process receives a copy of the parent's page table entries with write permissions cleared and marked as COW. Physical memory pages are shared between parent and child until one of them writes to a page, at which point a page fault is triggered and the OS duplicates only that modified page.",
        "objective": "Explain process memory virtualization, page fault handling, and resource optimization in Linux/Unix.",
        "skillTags": ["Copy on Write", "fork", "Processes", "Memory Virtualization"],
        "keywords": ["copy on write", "fork", "page fault", "duplicate", "shared memory", "mmu"],
        "softSkill": False
    },
    {
        "id": "os_hard_context_switch_overhead",
        "subject": "os",
        "topic": "scheduling",
        "difficulty": "Hard",
        "type": "Text",
        "text": "What operations occur during an operating system context switch between two different processes, and why is a process context switch costlier than a thread context switch?",
        "explanation": "A process context switch requires saving CPU registers (PC, SP, general purpose), changing kernel stack pointer, updating the page table base register (CR3 on x86) which invalidates/flushes the TLB, and repopulating cache lines. Thread context switches within the same process share the same virtual address space, avoiding TLB flushes and page table swaps.",
        "objective": "Evaluate kernel context switching overhead, CPU cache pollution, and TLB invalidation differences between processes and threads.",
        "skillTags": ["Context Switching", "Threads", "CPU Scheduling", "TLB"],
        "keywords": ["context switch", "registers", "tlb flush", "cr3", "page directory", "cache pollution", "thread"],
        "softSkill": False
    }
]

new_dbms_questions = [
    {
        "id": "dbms_med_bplus_tree_vs_hash",
        "subject": "dbms",
        "topic": "indexing",
        "difficulty": "Medium",
        "type": "MCQ",
        "text": "Why do relational database storage engines (like InnoDB) overwhelmingly prefer B+ Tree indexing over Hash indexing for primary keys?",
        "options": [
            "B+ Trees support efficient range queries, prefix matching, and sorted scans due to linked leaf nodes",
            "Hash indexes require O(N^2) memory and cannot store integers",
            "B+ Trees only consume memory while Hash indexes write directly to floppy drives",
            "Hash indexes do not support exact point lookups"
        ],
        "answer": 0,
        "explanation": "B+ Trees keep keys in sorted order with contiguous leaf node pointers, enabling fast range scans (e.g., WHERE age BETWEEN 20 AND 30) with O(log N) depth. Hash indexes only support O(1) exact equality lookups.",
        "objective": "Understand relational database indexing data structures, tree balancing, and range query efficiency.",
        "skillTags": ["B+ Tree", "Indexing", "Storage Engines", "SQL Optimization"],
        "keywords": ["b+ tree", "indexing", "range queries", "hash index", "innodb"],
        "softSkill": False
    },
    {
        "id": "dbms_hard_acid_isolation_levels",
        "subject": "dbms",
        "topic": "transactions",
        "difficulty": "Hard",
        "type": "MCQ",
        "text": "Which concurrency anomaly can occur in the 'Repeatable Read' isolation level that is prevented by 'Serializable'?",
        "options": [
            "Phantom Reads",
            "Dirty Reads",
            "Non-Repeatable Reads",
            "Lost Updates"
        ],
        "answer": 0,
        "explanation": "Repeatable Read prevents Dirty Reads and Non-Repeatable Reads for existing rows, but in standard SQL specifications allows Phantom Reads (new rows inserted by concurrent transactions matching a range predicate). Serializable prevents all anomalies.",
        "objective": "Analyze ANSI SQL transaction isolation levels, concurrency anomalies, and multi-version concurrency control (MVCC).",
        "skillTags": ["Transactions", "ACID", "Isolation Levels", "MVCC"],
        "keywords": ["phantom read", "repeatable read", "serializable", "dirty read", "isolation"],
        "softSkill": False
    },
    {
        "id": "dbms_med_wal_crash_recovery",
        "subject": "dbms",
        "topic": "crash_recovery",
        "difficulty": "Medium",
        "type": "Text",
        "text": "Explain Write-Ahead Logging (WAL) in database systems. How does it guarantee the Durability property of ACID without synchronous random disk writes?",
        "explanation": "WAL dictates that any modifications must be written sequentially to append-only log files on durable storage before the corresponding dirty data pages are flushed to database disk files. This turns random I/O into fast sequential I/O, allowing the engine to recover state after crashes by replaying redo logs.",
        "objective": "Explain database recovery architectures, append-only logs, and ACID durability enforcement.",
        "skillTags": ["WAL", "Durability", "Crash Recovery", "Transactions"],
        "keywords": ["write ahead logging", "wal", "durability", "redo log", "sequential write", "crash recovery"],
        "softSkill": False
    },
    {
        "id": "dbms_med_bcnf_normalization",
        "subject": "dbms",
        "topic": "normalization",
        "difficulty": "Medium",
        "type": "MCQ",
        "text": "What is the defining condition for a relation to be in Boyce-Codd Normal Form (BCNF)?",
        "options": [
            "For every non-trivial functional dependency X -> Y, X must be a superkey",
            "Every attribute must be indexed using an AVL tree",
            "There must be at least two foreign keys referencing the master table",
            "All columns must allow NULL values to avoid update anomalies"
        ],
        "answer": 0,
        "explanation": "BCNF is a stricter version of 3NF requiring that for every functional dependency X -> Y, the left-hand side X is a superkey of the relation.",
        "objective": "Master database normalization principles and functional dependency decomposition.",
        "skillTags": ["Normalization", "BCNF", "Relational Schema Design"],
        "keywords": ["bcnf", "superkey", "normalization", "functional dependency"],
        "softSkill": False
    },
    {
        "id": "dbms_hard_sharding_vs_replication",
        "subject": "dbms",
        "topic": "distributed_databases",
        "difficulty": "Hard",
        "type": "Text",
        "text": "Compare Database Sharding with Read Replication. When is horizontal sharding necessary, and what complexities does it introduce?",
        "explanation": "Read replication maintains identical copies of the database across secondary nodes to scale read throughput and ensure high availability. Sharding horizontally partitions rows across multiple independent database nodes when dataset volume or write throughput exceeds single-node storage and memory capacity. Complexities include cross-shard joins, distributed two-phase commit transactions, and re-sharding rebalancing.",
        "objective": "Design scalable distributed database architectures and evaluate trade-offs in horizontal partitioning.",
        "skillTags": ["Sharding", "Replication", "Distributed Systems", "Scalability"],
        "keywords": ["sharding", "replication", "horizontal partitioning", "distributed transactions", "two-phase commit", "cross-shard"],
        "softSkill": False
    }
]

new_networks_questions = [
    {
        "id": "net_med_tcp_handshake",
        "subject": "networks",
        "topic": "transport_layer",
        "difficulty": "Medium",
        "type": "MCQ",
        "text": "What is the sequence of flag packets exchanged during a standard TCP three-way handshake?",
        "options": [
            "SYN -> SYN-ACK -> ACK",
            "ACK -> SYN -> FIN",
            "HELLO -> READY -> CONNECT",
            "GET -> POST -> 200 OK"
        ],
        "answer": 0,
        "explanation": "Client sends SYN with initial sequence number, server responds with SYN-ACK acknowledging client's sequence number and presenting its own, and client acknowledges with ACK to establish full-duplex connection.",
        "objective": "Understand TCP connection lifecycle, sequence number synchronization, and handshake fundamentals.",
        "skillTags": ["TCP", "Transport Layer", "Handshake", "Networking"],
        "keywords": ["tcp", "syn", "syn-ack", "ack", "three-way handshake"],
        "softSkill": False
    },
    {
        "id": "net_hard_http2_vs_http3_quic",
        "subject": "networks",
        "topic": "application_layer",
        "difficulty": "Hard",
        "type": "MCQ",
        "text": "What fundamental problem in HTTP/2 did HTTP/3 solve by transitioning from TCP to QUIC (over UDP)?",
        "options": [
            "Head-of-Line (HoL) blocking at the transport (TCP) layer when a packet is dropped",
            "Lack of TLS encryption support",
            "Inability to stream video files larger than 1GB",
            "HTTP/2 only working on Internet Explorer"
        ],
        "answer": 0,
        "explanation": "While HTTP/2 multiplexes streams over a single TCP connection, a single dropped TCP packet stalls all multiplexed streams until retransmission succeeds. QUIC streams are independent over UDP, eliminating TCP-level Head-of-Line blocking.",
        "objective": "Evaluate modern web transport protocols, QUIC multiplexing, and TCP Head-of-Line blocking mitigation.",
        "skillTags": ["HTTP/3", "QUIC", "HTTP/2", "Head-of-Line Blocking"],
        "keywords": ["http/3", "quic", "head of line blocking", "udp", "multiplexing"],
        "softSkill": False
    },
    {
        "id": "net_med_dns_resolution_hierarchy",
        "subject": "networks",
        "topic": "dns",
        "difficulty": "Medium",
        "type": "Text",
        "text": "Describe the hierarchical DNS resolution process when a browser queries an uncached domain name like 'api.github.com'.",
        "explanation": "The local recursive resolver queries Root Nameservers ('.'), which delegate to Top-Level Domain (TLD) servers ('.com'). The TLD server directs the resolver to the Authoritative Nameservers for 'github.com', which returns the authoritative A/AAAA IP address record back to the resolver and client.",
        "objective": "Trace hierarchical DNS architecture from local resolver to authoritative nameservers.",
        "skillTags": ["DNS", "Networking", "Internet Architecture"],
        "keywords": ["dns", "recursive resolver", "root nameserver", "tld", "authoritative nameserver", "a record"],
        "softSkill": False
    },
    {
        "id": "net_hard_tls_13_handshake",
        "subject": "networks",
        "topic": "security",
        "difficulty": "Hard",
        "type": "MCQ",
        "text": "What major latency enhancement was introduced in TLS 1.3 compared to TLS 1.2?",
        "options": [
            "1-RTT handshake by default (and optional 0-RTT resumption) by combining cipher negotiation with Diffie-Hellman key exchange",
            "Removal of asymmetric cryptography entirely",
            "Switching from AES to plain base64 encoding",
            "Elimination of certificate verification authority checks"
        ],
        "answer": 0,
        "explanation": "TLS 1.3 reduced handshake latency from 2 round-trip times (2-RTT) to 1-RTT by having the client send key shares directly in the ClientHello, and allows 0-RTT session resumption for previously visited hosts.",
        "objective": "Analyze modern network security protocols, TLS handshake round-trips, and cryptographic forward secrecy.",
        "skillTags": ["TLS", "HTTPS", "Network Security", "Cryptography"],
        "keywords": ["tls 1.3", "1-rtt", "0-rtt", "diffie-hellman", "handshake latency"],
        "softSkill": False
    },
    {
        "id": "net_med_websockets_vs_sse",
        "subject": "networks",
        "topic": "application_layer",
        "difficulty": "Medium",
        "type": "Text",
        "text": "Compare WebSockets and Server-Sent Events (SSE). When should you prefer SSE over WebSockets in backend web design?",
        "explanation": "WebSockets provide full-duplex bidirectional TCP communication with minimal framing over a custom protocol after an HTTP upgrade. SSE provides unidirectional server-to-client streaming over standard HTTP/1.1 or HTTP/2, natively supporting auto-reconnection, event IDs, and corporate proxies without custom firewall configuration. Prefer SSE for unidirectional live feeds, notification dashboards, and LLM text generation streaming.",
        "objective": "Architect real-time communication systems choosing between full-duplex WebSockets and HTTP SSE.",
        "skillTags": ["WebSockets", "SSE", "Real-Time Systems", "HTTP"],
        "keywords": ["websockets", "server sent events", "sse", "bidirectional", "unidirectional", "http streaming"],
        "softSkill": False
    }
]

new_cpp_questions = [
    {
        "id": "cpp_hard_vtable_vptr_internals",
        "subject": "cpp_oop",
        "topic": "polymorphism",
        "difficulty": "Hard",
        "type": "Text",
        "text": "Explain how virtual function dispatch is implemented under the hood in C++ using VTABLE and VPTR. What memory overhead does it introduce?",
        "explanation": "Every class with at least one virtual function has a static compiler-generated table of function pointers called the VTABLE. Every instantiated object of that class contains an invisible pointer (VPTR) pointing to its class VTABLE, usually at offset 0. When a virtual function is invoked through a base pointer/reference, the program dereferences object->vptr, indexes the appropriate offset in the table, and jumps to the function. Overhead: 8 bytes per object on 64-bit platforms plus one pointer per virtual method per class.",
        "objective": "Explain virtual dispatch mechanics, runtime polymorphism, and memory overhead of virtual method tables.",
        "skillTags": ["VTABLE", "VPTR", "Polymorphism", "C++ Internals"],
        "keywords": ["vtable", "vptr", "virtual function", "dynamic dispatch", "function pointer", "offset"],
        "softSkill": False
    },
    {
        "id": "cpp_med_rvo_nrvo",
        "subject": "cpp_oop",
        "topic": "optimization",
        "difficulty": "Medium",
        "type": "MCQ",
        "text": "What is Return Value Optimization (RVO) in modern C++?",
        "options": [
            "A compiler optimization where the return value is constructed directly in the storage allocated for the caller's target variable, eliding copy/move constructors",
            "An optimization where all functions automatically return void to save stack memory",
            "A runtime garbage collector that frees unused return objects",
            "A technique where function parameters are sorted alphabetically"
        ],
        "answer": 0,
        "explanation": "RVO (and mandatory copy elision since C++17) allows the compiler to construct a temporary return value directly in the memory space allocated by the caller, eliminating unnecessary copy and move operations.",
        "objective": "Understand C++ compiler optimizations, copy elision rules, and value semantics.",
        "skillTags": ["RVO", "Optimization", "Copy Elision", "Modern C++"],
        "keywords": ["rvo", "copy elision", "return value optimization", "move semantics"],
        "softSkill": False
    },
    {
        "id": "cpp_hard_forwarding_references",
        "subject": "cpp_oop",
        "topic": "templates",
        "difficulty": "Hard",
        "type": "MCQ",
        "text": "What is a forwarding reference (universal reference) in C++, and what is the role of std::forward?",
        "options": [
            "An auto&& or T&& reference where T is a deduced template type; std::forward preserves the original lvalue or rvalue value category",
            "A pointer to an address that can never be dereferenced",
            "A reference that forwards all network packets to a remote server",
            "A macro for printing debug logs to std::cout"
        ],
        "answer": 0,
        "explanation": "In template contexts with deduced types, T&& is a forwarding reference that collapses to lvalue reference if passed an lvalue, or rvalue reference if passed an rvalue. std::forward<T> statically casts the parameter to preserve its original value category (perfect forwarding).",
        "objective": "Master perfect forwarding, reference collapsing rules, and generic template design.",
        "skillTags": ["Templates", "Perfect Forwarding", "Move Semantics", "Modern C++"],
        "keywords": ["forwarding reference", "std::forward", "perfect forwarding", "reference collapsing", "value category"],
        "softSkill": False
    },
    {
        "id": "cpp_med_smart_pointer_cycles",
        "subject": "cpp_oop",
        "topic": "memory_management",
        "difficulty": "Medium",
        "type": "Text",
        "text": "How do cyclic references cause memory leaks with std::shared_ptr, and how does std::weak_ptr resolve this issue?",
        "explanation": "If two objects hold std::shared_ptr references to each other, their strong reference counts will never drop to zero even when all external pointers go out of scope, causing a perpetual leak. std::weak_ptr provides non-owning observer access that increments only the weak reference count without preventing deallocation. When access is needed, lock() creates a temporary shared_ptr.",
        "objective": "Prevent cyclic memory leaks in complex object graphs using weak pointers.",
        "skillTags": ["Smart Pointers", "shared_ptr", "weak_ptr", "Memory Safety"],
        "keywords": ["cyclic reference", "memory leak", "shared_ptr", "weak_ptr", "reference count", "lock"],
        "softSkill": False
    },
    {
        "id": "cpp_hard_type_casting",
        "subject": "cpp_oop",
        "topic": "type_system",
        "difficulty": "Hard",
        "type": "MCQ",
        "text": "When downcasting a base class pointer to a derived class pointer in C++, which cast performs runtime RTTI type checking and returns nullptr on failure?",
        "options": [
            "dynamic_cast",
            "static_cast",
            "reinterpret_cast",
            "const_cast"
        ],
        "answer": 0,
        "explanation": "dynamic_cast safely navigates inheritance hierarchies by checking Run-Time Type Information (RTTI) stored in the polymorphic class's VTABLE. It returns nullptr for pointers or throws std::bad_cast for references on mismatch.",
        "objective": "Evaluate C++ cast operators and understand type safety and RTTI mechanics.",
        "skillTags": ["dynamic_cast", "RTTI", "Type Casting", "Polymorphism"],
        "keywords": ["dynamic_cast", "rtti", "static_cast", "downcasting", "type safety"],
        "softSkill": False
    }
]

new_algo_questions = [
    {
        "id": "longest-substring-without-repeating-characters",
        "subject": "algo",
        "topic": "sliding_window",
        "difficulty": "Medium",
        "type": "Coding",
        "text": "Given a string s, find the length of the longest substring without duplicate characters in optimal O(N) time and O(min(N, M)) space using a sliding window.",
        "explanation": "Maintain a sliding window [left, right] along with a hash table recording the most recent index of each character. When a duplicate character is encountered, jump the left pointer past its previous occurrence.",
        "objective": "Master the sliding window technique and two-pointer state tracking.",
        "skillTags": ["Sliding Window", "Hash Table", "Strings", "Algorithms"],
        "keywords": ["longest substring", "sliding window", "two pointers", "hash table"],
        "softSkill": False,
        "templateCode": "#include <iostream>\n#include <string>\n#include <vector>\n#include <algorithm>\n\nint lengthOfLongestSubstring(const std::string& s) {\n    std::vector<int> lastIndex(256, -1);\n    int maxLen = 0, start = 0;\n    for (int i = 0; i < (int)s.size(); ++i) {\n        unsigned char c = s[i];\n        if (lastIndex[c] >= start) {\n            start = lastIndex[c] + 1;\n        }\n        lastIndex[c] = i;\n        maxLen = std::max(maxLen, i - start + 1);\n    }\n    return maxLen;\n}\n\nint main() {\n    std::string s;\n    if (std::cin >> s) {\n        std::cout << lengthOfLongestSubstring(s) << std::endl;\n    } else {\n        std::cout << 0 << std::endl;\n    }\n    return 0;\n}\n",
        "testCases": [
            {"input": "abcabcbb", "expectedOutput": "3", "isHidden": False},
            {"input": "bbbbb", "expectedOutput": "1", "isHidden": False},
            {"input": "pwwkew", "expectedOutput": "3", "isHidden": True}
        ]
    },
    {
        "id": "kth-largest-element-in-an-array",
        "subject": "algo",
        "topic": "heaps_quickselect",
        "difficulty": "Medium",
        "type": "Coding",
        "text": "Given an integer array nums and an integer k, return the kth largest element in the array using a min-heap or Quickselect.",
        "explanation": "Maintain a min-heap of size k. Iterate through nums: if the current element is greater than the heap's top, pop the top and push the element. The heap top at the end will be the kth largest element.",
        "objective": "Master min-heap streaming and Quickselect partitioning.",
        "skillTags": ["Heaps", "Priority Queue", "Quickselect", "Sorting"],
        "keywords": ["kth largest", "min heap", "priority queue", "quickselect"],
        "softSkill": False,
        "templateCode": "#include <iostream>\n#include <vector>\n#include <queue>\n\nint findKthLargest(const std::vector<int>& nums, int k) {\n    std::priority_queue<int, std::vector<int>, std::greater<int>> minHeap;\n    for (int x : nums) {\n        minHeap.push(x);\n        if ((int)minHeap.size() > k) minHeap.pop();\n    }\n    return minHeap.top();\n}\n\nint main() {\n    int n, k;\n    if (!(std::cin >> n >> k)) return 0;\n    std::vector<int> nums(n);\n    for (int i = 0; i < n; ++i) std::cin >> nums[i];\n    std::cout << findKthLargest(nums, k) << std::endl;\n    return 0;\n}\n",
        "testCases": [
            {"input": "6 2\n3 2 1 5 6 4", "expectedOutput": "5", "isHidden": False},
            {"input": "9 4\n3 2 3 1 2 4 5 5 6", "expectedOutput": "4", "isHidden": False},
            {"input": "1 1\n1", "expectedOutput": "1", "isHidden": True}
        ]
    },
    {
        "id": "course-schedule",
        "subject": "algo",
        "topic": "graphs",
        "difficulty": "Medium",
        "type": "Coding",
        "text": "There are a total of numCourses courses labeled 0 to numCourses - 1. You are given an array prerequisites. Determine if you can finish all courses using Kahn's algorithm (topological sort) or cycle detection in a directed graph.",
        "explanation": "Model prerequisites as a directed graph. Track in-degrees of all nodes. Enqueue all nodes with in-degree 0. While the queue is non-empty, dequeue a node, increment processed count, and decrement in-degrees of neighbors. If processed count equals numCourses, the graph is a DAG with no cycles.",
        "objective": "Implement topological sorting and cycle detection in directed graphs.",
        "skillTags": ["Graphs", "Topological Sort", "BFS", "Kahn Algorithm"],
        "keywords": ["topological sort", "course schedule", "cycle detection", "dag", "graphs"],
        "softSkill": False,
        "templateCode": "#include <iostream>\n#include <vector>\n#include <queue>\n\nbool canFinish(int numCourses, const std::vector<std::pair<int,int>>& prerequisites) {\n    std::vector<int> inDegree(numCourses, 0);\n    std::vector<std::vector<int>> adj(numCourses);\n    for (const auto& p : prerequisites) {\n        adj[p.second].push_back(p.first);\n        inDegree[p.first]++;\n    }\n    std::queue<int> q;\n    for (int i = 0; i < numCourses; ++i) if (inDegree[i] == 0) q.push(i);\n    int count = 0;\n    while (!q.empty()) {\n        int u = q.front(); q.pop();\n        count++;\n        for (int v : adj[u]) {\n            if (--inDegree[v] == 0) q.push(v);\n        }\n    }\n    return count == numCourses;\n}\n\nint main() {\n    int n, m;\n    if (!(std::cin >> n >> m)) return 0;\n    std::vector<std::pair<int,int>> prereqs(m);\n    for (int i = 0; i < m; ++i) std::cin >> prereqs[i].first >> prereqs[i].second;\n    std::cout << (canFinish(n, prereqs) ? 1 : 0) << std::endl;\n    return 0;\n}\n",
        "testCases": [
            {"input": "2 1\n1 0", "expectedOutput": "1", "isHidden": False},
            {"input": "2 2\n1 0\n0 1", "expectedOutput": "0", "isHidden": False},
            {"input": "1 0", "expectedOutput": "1", "isHidden": True}
        ]
    },
    {
        "id": "word-break",
        "subject": "algo",
        "topic": "dynamic_programming",
        "difficulty": "Medium",
        "type": "Coding",
        "text": "Given a string s and a dictionary of strings wordDict, return true if s can be segmented into a space-separated sequence of one or more dictionary words. Solve using Dynamic Programming.",
        "explanation": "Define dp[i] as true if s[0...i-1] can be segmented. Base case: dp[0] = true. For each i from 1 to n, and j from 0 to i-1, if dp[j] is true and s[j...i-1] is in wordDict, set dp[i] = true and break.",
        "objective": "Apply 1D dynamic programming with dictionary lookups.",
        "skillTags": ["Dynamic Programming", "Strings", "Hash Set"],
        "keywords": ["word break", "dynamic programming", "dictionary", "memoization"],
        "softSkill": False,
        "templateCode": "#include <iostream>\n#include <string>\n#include <vector>\n#include <unordered_set>\n\nbool wordBreak(const std::string& s, const std::vector<std::string>& wordDict) {\n    std::unordered_set<std::string> dict(wordDict.begin(), wordDict.end());\n    std::vector<bool> dp(s.size() + 1, false);\n    dp[0] = true;\n    for (int i = 1; i <= (int)s.size(); ++i) {\n        for (int j = 0; j < i; ++j) {\n            if (dp[j] && dict.count(s.substr(j, i - j))) {\n                dp[i] = true;\n                break;\n            }\n        }\n    }\n    return dp[s.size()];\n}\n\nint main() {\n    std::string s;\n    int m;\n    if (!(std::cin >> s >> m)) return 0;\n    std::vector<std::string> dict(m);\n    for (int i = 0; i < m; ++i) std::cin >> dict[i];\n    std::cout << (wordBreak(s, dict) ? 1 : 0) << std::endl;\n    return 0;\n}\n",
        "testCases": [
            {"input": "leetcode 2\nleet code", "expectedOutput": "1", "isHidden": False},
            {"input": "catsandog 5\ncats dog sand and cat", "expectedOutput": "0", "isHidden": False},
            {"input": "applepenapple 2\napple pen", "expectedOutput": "1", "isHidden": True}
        ]
    }
]

new_ds_questions = [
    {
        "id": "ds-implement-trie-prefix-tree",
        "subject": "ds",
        "topic": "trees",
        "difficulty": "Medium",
        "type": "Coding",
        "text": "A Trie (prefix tree) is a tree data structure used to efficiently store and retrieve keys in a dataset of strings. Implement a Trie class with insert, search, and startsWith methods.",
        "explanation": "Each TrieNode contains an array of 26 child pointers (for lowercase English letters) and a boolean isEndOfWord flag. Traversal follows character edges in O(L) time where L is string length.",
        "objective": "Implement prefix trees for fast dictionary and autocomplete lookups.",
        "skillTags": ["Trie", "Trees", "Prefix Tree", "Data Structures"],
        "keywords": ["trie", "prefix tree", "autocomplete", "strings"],
        "softSkill": False,
        "templateCode": "#include <iostream>\n#include <string>\n#include <vector>\n\nstruct TrieNode {\n    TrieNode* children[26] = {nullptr};\n    bool isEnd = false;\n};\n\nclass Trie {\n    TrieNode root;\npublic:\n    void insert(const std::string& word) {\n        TrieNode* curr = &root;\n        for (char c : word) {\n            int idx = c - 'a';\n            if (!curr->children[idx]) curr->children[idx] = new TrieNode();\n            curr = curr->children[idx];\n        }\n        curr->isEnd = true;\n    }\n    bool search(const std::string& word) {\n        TrieNode* curr = &root;\n        for (char c : word) {\n            int idx = c - 'a';\n            if (!curr->children[idx]) return false;\n            curr = curr->children[idx];\n        }\n        return curr->isEnd;\n    }\n};\n\nint main() {\n    Trie t;\n    t.insert(\"apple\");\n    std::cout << t.search(\"apple\") << \" \" << t.search(\"app\") << std::endl;\n    return 0;\n}\n",
        "testCases": [
            {"input": "", "expectedOutput": "1 0", "isHidden": False}
        ]
    },
    {
        "id": "ds-lowest-common-ancestor-binary-tree",
        "subject": "ds",
        "topic": "binary_trees",
        "difficulty": "Medium",
        "type": "Coding",
        "text": "Given a binary tree, find the lowest common ancestor (LCA) of two given nodes p and q in O(N) time and O(H) recursion stack space.",
        "explanation": "Recursively traverse the tree: if current node is null or equals p or q, return root. Recursively search left and right subtrees. If both return non-null, the current node is the LCA. Otherwise return the non-null child.",
        "objective": "Master post-order tree traversal for ancestor finding.",
        "skillTags": ["Binary Trees", "Recursion", "LCA", "Data Structures"],
        "keywords": ["lowest common ancestor", "binary tree", "recursion", "post-order"],
        "softSkill": False,
        "templateCode": "#include <iostream>\n\nstruct Node {\n    int val;\n    Node* left = nullptr;\n    Node* right = nullptr;\n    Node(int v) : val(v) {}\n};\n\nNode* lca(Node* root, int p, int q) {\n    if (!root || root->val == p || root->val == q) return root;\n    Node* l = lca(root->left, p, q);\n    Node* r = lca(root->right, p, q);\n    if (l && r) return root;\n    return l ? l : r;\n}\n\nint main() {\n    Node* root = new Node(3);\n    root->left = new Node(5);\n    root->right = new Node(1);\n    Node* res = lca(root, 5, 1);\n    std::cout << (res ? res->val : -1) << std::endl;\n    return 0;\n}\n",
        "testCases": [
            {"input": "", "expectedOutput": "3", "isHidden": False}
        ]
    }
]

def update_file(filepath, questions):
    with open(filepath, 'r', encoding='utf-8') as f:
        data = json.load(f)
    existing_ids = {q['id'] for q in questions}
    # Keep items not in new list, and add updated ones
    data = [q for q in data if q['id'] not in existing_ids]
    data.extend(questions)
    with open(filepath, 'w', encoding='utf-8') as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
    print(f"Updated {filepath} (total: {len(data)})")

def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    q_dir = os.path.join(root, 'data', 'questions')
    
    update_file(os.path.join(q_dir, 'os.json'), new_os_questions)
    update_file(os.path.join(q_dir, 'dbms.json'), new_dbms_questions)
    update_file(os.path.join(q_dir, 'networks.json'), new_networks_questions)
    update_file(os.path.join(q_dir, 'cpp_oop.json'), new_cpp_questions)
    update_file(os.path.join(q_dir, 'algorithms.json'), new_algo_questions)
    update_file(os.path.join(q_dir, 'data_structures.json'), new_ds_questions)

if __name__ == '__main__':
    main()

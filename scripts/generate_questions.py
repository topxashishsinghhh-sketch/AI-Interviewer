#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
scripts/generate_questions.py — bulk-generate question banks.

Generating ~2000 questions by hand is neither maintainable nor reviewable, so
this script composes them from per-subject *concept banks* instead. Every
question is built from a real concept (term + definition + keywords), so the
output is meaningful rather than lorem ipsum, and it is deterministic: the same
seed always yields the same files.

Each concept produces up to 7 questions across types and difficulties:

    1. Easy   MCQ   "what is X"                       (definition recall)
    2. Easy   Text   "define X"                       (recall in words)
    3. Medium MCQ   "which statement about X is correct"
    4. Medium Text   "why does X matter"
    5. Hard   MCQ   diagnostic scenario using X
    6. Hard   Text   contrast X against a sibling concept
    7. Soft   MCQ   "how would you explain X to a teammate"  (soft-tagged)

Problem 7 is deliberately in every technical subject: interviewers routinely
score whether you can *explain* a technical idea, so that skill is practised
where the concept lives rather than only in the HR bank.

Coding questions are NOT templated. A generated coding problem would be
worthless, so coding stays hand-authored and is only merged in.

Output: data/questions/gen_<subject>.json (one file per subject).
Run scripts/prepare_data.ps1 afterwards to rebuild data/questions.json.
"""

import io
import json
import os
import random
import sys

# ---------------------------------------------------------------------------
# Concept banks.
#
#   subject -> topic -> { "tags": [...], "concepts": [ (term, definition, keywords...), ... ] }
#
# "tags" becomes skillTags on every question built from this topic.
# Concepts with a trailing 4th element are flagged "soft" for that question only
# where relevant; softness is decided per template instead, see below.
# ---------------------------------------------------------------------------

SOFT_SUBJECTS = {"hr", "placement"}

BANKS = {

# ---------------------------------------------------------------- algorithms
"algo": {
 "topics": {
  "complexity": {
   "tags": ["Algorithms", "Complexity"],
   "concepts": [
     ("Big-O notation", "a way to describe how a function grows as its input size grows, expressed as an upper bound", "upper bound", "growth", "asymptotic"),
     ("time complexity", "the number of basic operations an algorithm performs as a function of input size", "operations", "input size", "count"),
     ("space complexity", "the amount of additional memory an algorithm needs beyond its input", "memory", "auxiliary", "input"),
     ("amortised analysis", "averaging the cost of an operation over a worst-case sequence of operations", "average", "sequence", "worst case"),
     ("worst-case analysis", "the maximum cost incurred by any input of a given size", "maximum", "guarantee", "bound"),
     ("average-case analysis", "the expected cost over a distribution of inputs", "expected", "distribution", "typical"),
     ("asymptotic behaviour", "the growth trend of a function as the input size tends to infinity", "infinity", "growth", "trend"),
     ("constant time", "an operation whose cost does not depend on the input size", "independent", "fixed", "same"),
   ]},
  "sorting": {
   "tags": ["Algorithms", "Sorting"],
   "concepts": [
     ("merge sort", "a divide-and-conquer sort that splits the array, sorts each half and merges them back in order", "divide", "conquer", "merge", "stable"),
     ("quicksort", "a divide-and-conquer sort that partitions around a pivot then recurses on each side", "pivot", "partition", "recursive", "in place"),
     ("heapsort", "a sort that builds a heap then repeatedly extracts the maximum", "heap", "extract", "maximum", "in place"),
     ("insertion sort", "a sort that inserts each element into its correct place in a growing sorted prefix", "prefix", "insert", "shift", "simple"),
     ("selection sort", "a sort that repeatedly selects the minimum of the unsorted remainder", "minimum", "scan", "swap", "selection"),
     ("stability", "the property that equal elements keep their original relative order", "order", "equal", "preserve", "relative"),
     ("counting sort", "a non-comparison sort that counts occurrences of each key", "counting", "bucket", "linear", "integer"),
   ]},
  "searching": {
   "tags": ["Algorithms", "Searching"],
   "concepts": [
     ("binary search", "a search that repeatedly halves a sorted range", "sorted", "half", "halve", "logarithmic"),
     ("linear search", "a search that inspects elements one at a time", "sequential", "scan", "one by one", "linear"),
     ("interpolation search", "a search that estimates the position of a key from its value distribution", "estimate", "probe", "uniform", "interpolate"),
     ("exponential search", "a search that doubles a range then binary searches within it", "doubling", "range", "unbounded", "exponential"),
   ]},
  "dp": {
   "tags": ["Algorithms", "Dynamic Programming"],
   "concepts": [
     ("dynamic programming", "a technique that solves problems by combining solutions to overlapping subproblems", "overlapping", "subproblem", "memoise", "table"),
     ("optimal substructure", "the property that an optimal solution contains optimal solutions to subproblems", "contains", "optimal", "recursive", "solution"),
     ("memoisation", "caching the result of a subproblem so it is computed once", "cache", "store", "reuse", "recursion"),
     ("tabulation", "filling a table bottom-up instead of recursing", "bottom-up", "table", "iterative", "order"),
     ("state transition", "the rule relating a state's value to previously computed states", "recurrence", "state", "relation", "previous"),
     ("0/1 knapsack", "the problem of choosing items with fixed weights and values under a capacity limit", "capacity", "weight", "value", "select"),
   ]},
  "greedy": {
   "tags": ["Algorithms", "Greedy"],
   "concepts": [
     ("greedy algorithm", "an algorithm that commits to the locally best option at each step", "local", "best", "commit", "choice"),
     ("exchange argument", "a proof that swapping a greedy choice for a better one cannot help", "swap", "proof", "optimal", "exchange"),
     ("fractional knapsack", "the variant where items may be split, which greedy solves optimally", "split", "fraction", "optimal", "ratio"),
     ("interval scheduling", "choosing the maximum number of non-overlapping intervals", "non-overlapping", "interval", "compatible", "maximum"),
   ]},
  "graphs": {
   "tags": ["Algorithms", "Graphs"],
   "concepts": [
     ("Dijkstra's algorithm", "a shortest-path algorithm that grows settled vertices by increasing distance", "shortest", "settled", "heap", "non-negative"),
     ("Bellman-Ford", "a shortest-path algorithm that relaxes every edge repeatedly", "relax", "edge", "negative", "iteration"),
     ("Floyd-Warshall", "an all-pairs shortest-path algorithm using dynamic programming", "all pairs", "matrix", "dynamic", "intermediate"),
     ("topological sort", "ordering vertices of a directed acyclic graph so every edge points forward", "acyclic", "dependency", "order", "forward"),
     ("Kruskal's algorithm", "a minimum spanning tree algorithm that adds the cheapest safe edge", "spanning", "cheapest", "forest", "safe"),
     ("Prim's algorithm", "a minimum spanning tree algorithm that grows a tree from a seed vertex", "grow", "tree", "seed", "cheapest"),
   ]},
  "strings": {
   "tags": ["Algorithms", "Strings"],
   "concepts": [
     ("KMP algorithm", "a pattern matcher that uses a prefix table to avoid rescanning", "prefix", "table", "shift", "linear"),
     ("Rabin-Karp", "a pattern matcher using rolling hashes", "rolling", "hash", "window", "collision"),
     ("rolling hash", "a hash that can be updated in constant time when the window slides", "slide", "update", "constant", "window"),
   ]},
 }},

# ------------------------------------------------------------ data structures
"ds": {
 "topics": {
  "arrays": {
   "tags": ["Data Structures", "Arrays"],
   "concepts": [
     ("random access", "fetching an element directly by index without traversing", "index", "direct", "constant", "address"),
     ("contiguous storage", "storing elements in adjacent memory locations", "adjacent", "memory", "block", "sequential"),
     ("dynamic array", "an array that grows by reallocating when it fills", "grow", "reallocate", "capacity", "double"),
     ("insertion cost", "the work of shifting elements to make room at a position", "shift", "make room", "position", "linear"),
     ("cache locality", "performance gained from memory accesses that are close together", "cache", "line", "spatial", "locality"),
   ]},
  "linked_lists": {
   "tags": ["Data Structures", "Linked Lists"],
   "concepts": [
     ("singly linked list", "a chain of nodes each pointing to the next", "node", "next", "pointer", "chain"),
     ("doubly linked list", "a chain where each node points both forward and backward", "prev", "next", "bidirectional", "pointer"),
     ("circular linked list", "a list whose last node points back to the first", "last", "first", "loop", "circular"),
     ("splicing", "relinking pointers to insert or remove a node in constant time", "relink", "pointer", "constant", "insert"),
     ("traversal cost", "the linear cost of walking a chain to reach a position", "walk", "linear", "reach", "sequential"),
   ]},
  "stacks": {
   "tags": ["Data Structures", "Stacks"],
   "concepts": [
     ("stack", "a last-in first-out collection with push and pop", "last in", "first out", "push", "pop"),
     ("push", "adding an element to the top of a stack", "top", "add", "push", "top of stack"),
     ("pop", "removing the element from the top of a stack", "top", "remove", "pop", "return"),
     ("call stack", "the stack that holds activations during recursion", "activation", "recursion", "return", "frame"),
     ("monotonic stack", "a stack kept in increasing or decreasing order to answer range queries", "monotonic", "order", "range", "next greater"),
   ]},
  "queues": {
   "tags": ["Data Structures", "Queues"],
   "concepts": [
     ("queue", "a first-in first-out collection with enqueue and dequeue", "first in", "first out", "enqueue", "dequeue"),
     ("deque", "a queue that allows insertion and removal at both ends", "both ends", "front", "rear", "double ended"),
     ("circular queue", "an array-based queue that reuses freed slots", "wrap", "reuse", "ring", "slot"),
     ("priority queue", "a queue that removes the highest-priority element first", "priority", "highest", "heap", "extract"),
   ]},
  "hashing": {
   "tags": ["Data Structures", "Hashing"],
   "concepts": [
     ("hash table", "a structure mapping keys to slots through a hash function", "key", "slot", "bucket", "hash function"),
     ("collision", "two keys hashing to the same slot", "same slot", "clash", "hash", "two keys"),
     ("open addressing", "resolving collisions by probing for another slot", "probe", "linear probing", "next slot", "resolve"),
     ("separate chaining", "resolving collisions by keeping a list per slot", "chain", "list", "bucket", "collision"),
     ("load factor", "the ratio of stored entries to available slots", "entries", "slots", "ratio", "full"),
     ("hash function", "a function mapping keys to integer slots uniformly", "uniform", "integer", "spread", "deterministic"),
   ]},
  "heaps": {
   "tags": ["Data Structures", "Heaps"],
   "concepts": [
     ("min-heap", "a tree where every parent is not greater than its children", "smallest", "root", "parent", "property"),
     ("max-heap", "a tree where every parent is not less than its children", "largest", "root", "parent", "property"),
     ("heapify", "restoring the heap property at a node in logarithmic time", "restore", "sift", "down", "logarithmic"),
     ("heap insertion", "adding a value and sifting it up to restore the property", "sift", "up", "append", "level order"),
     ("heap extraction", "removing the root and sifting the last element down", "root", "last element", "sift", "down"),
   ]},
  "trees": {
   "tags": ["Data Structures", "Trees"],
   "concepts": [
     ("binary tree", "a tree where each node has at most two children", "two children", "left", "right", "node"),
     ("in-order traversal", "visiting left subtree, node, then right subtree", "left", "right", "sorted order", "depth first"),
     ("pre-order traversal", "visiting node then its subtrees", "node first", "then subtrees", "prefix", "depth first"),
     ("post-order traversal", "visiting subtrees then node", "subtrees first", "then node", "postfix", "depth first"),
     ("level-order traversal", "visiting nodes breadth-first using a queue", "breadth first", "queue", "level", "level by level"),
     ("height of a tree", "the longest root-to-leaf path length", "longest", "root", "leaf", "depth"),
     ("complete binary tree", "a tree filled level by level from the left", "filled", "level", "left to right", "compact"),
   ]},
  "balanced_trees": {
   "tags": ["Data Structures", "Balanced Trees"],
   "concepts": [
     ("AVL tree", "a self-balancing search tree bounding the height ratio at one", "balance", "rotation", "height", "ratio"),
     ("red-black tree", "a search tree kept balanced by colour rules", "colour", "red", "black", "rotation"),
     ("rotations", "local restructurings that restore balance", "left rotation", "right rotation", "restore", "balance"),
     ("B-tree", "a multi-way search tree shaped for disk pages", "multi-way", "page", "disk", "fanout"),
     ("balance factor", "the height difference between a node's subtrees", "height", "difference", "subtree", "balance"),
   ]},
  "graphs": {
   "tags": ["Data Structures", "Graphs"],
   "concepts": [
     ("adjacency list", "representing a graph by a list of neighbours per vertex", "vertex", "neighbour", "list", "edge"),
     ("adjacency matrix", "representing a graph by a grid of edge indicators", "grid", "matrix", "vertex", "edge"),
     ("breadth-first search", "a graph traversal that expands a frontier with a queue", "queue", "frontier", "level", "traverse"),
     ("depth-first search", "a graph traversal that follows one path deeply before backtracking", "stack", "backtrack", "deep", "traverse"),
     ("strongly connected", "every vertex can reach every other vertex", "reach", "both directions", "component", "mutual"),
     ("spanning tree", "an acyclic subgraph connecting every vertex", "connect", "acyclic", "all vertices", "tree"),
     ("topological order", "an ordering of vertices respecting all directed edges", "directed", "edge", "order", "acyclic"),
   ]},
  "complexity": {
   "tags": ["Data Structures", "Complexity"],
   "concepts": [
     ("interface choice", "picking the structure whose cost profile matches the query mix", "query", "trade-off", "profile", "cost"),
     ("space-time trade-off", "trading memory for speed or the reverse", "memory", "speed", "trade", "balance"),
   ]},
 }},

# ------------------------------------------------------------------------ oop
"oop": {
 "topics": {
  "encapsulation": {
   "tags": ["OOP", "Encapsulation"],
   "concepts": [
     ("encapsulation", "bundling data with the methods that operate on it and restricting access", "data hiding", "bundle", "class", "restrict"),
     ("access specifier", "a keyword controlling member visibility: private, protected or public", "visibility", "private", "protected", "public"),
     ("getter and setter", "controlled accessors that read and write a private field", "accessor", "read", "write", "controlled"),
     ("immutability", "an object whose state cannot change after construction", "const", "unchanged", "construct", "safe"),
   ]},
  "inheritance": {
   "tags": ["OOP", "Inheritance"],
   "concepts": [
     ("inheritance", "deriving a class so it reuses and extends another", "derive", "base", "extend", "reuse"),
     ("base class", "the class whose members are inherited", "parent", "shared", "common", "members"),
     ("derived class", "a class that adds to or specialises a base", "child", "add", "specialise", "own"),
     ("protected access", "visible to derived classes but not to unrelated code", "derived", "accessible", "not public", "derived classes only"),
     ("diamond problem", "duplicated base members when two parents share a grandparent", "duplicate", "shared ancestor", "ambiguity", "two parents"),
     ("virtual inheritance", "sharing a single base subobject across a diamond", "single", "shared", "virtual", "one subobject"),
   ]},
  "polymorphism": {
   "tags": ["OOP", "Polymorphism"],
   "concepts": [
     ("polymorphism", "one interface with many implementations resolved at run time", "one interface", "many forms", "virtual", "dispatch"),
     ("runtime polymorphism", "dispatch decided by the object's actual type", "virtual table", "vtable", "actual type", "dynamic dispatch"),
     ("compile-time polymorphism", "dispatch resolved during compilation", "template", "overload", "static", "compile"),
     ("virtual function", "a member function whose call resolves to the derived version", "virtual", "override", "derive", "dispatch"),
     ("overriding", "replacing a base member function with a derived implementation", "replace", "base", "same signature", "derive"),
     ("override keyword", "a specifier that makes the compiler verify an override matches", "verify", "compile", "signature", "matches"),
     ("pure virtual function", "a virtual function with no implementation, making the class abstract", "= 0", "abstract", "no implementation", "must override"),
     ("abstract class", "a class that cannot be instantiated and defines an interface", "cannot instantiate", "interface", "pure virtual", "contract"),
   ]},
  "constructors": {
   "tags": ["OOP", "Constructors"],
   "concepts": [
     ("constructor", "a member that runs when an object is created", "create", "initialise", "object", "called once"),
     ("default constructor", "a constructor callable with no arguments", "no arguments", "callable", "empty", "generated"),
     ("copy constructor", "a constructor that builds an object from another of the same type", "same type", "copy", "argument", "clone"),
     ("initialiser list", "the forms that initialise members before the body runs", "before body", "direct", "efficient", "resolves"),
     ("delegating constructor", "a constructor that calls another constructor of its own class", "delegate", "another", "same class", "reuse"),
     ("virtual destructor", "a destructor that makes deleting through a base pointer safe", "base pointer", "safe", "virtual", "complete"),
   ]},
  "memory": {
   "tags": ["OOP", "Memory"],
   "concepts": [
     ("resource acquisition", "obtaining a resource in a constructor", "constructor", "acquire", "resource", "begin lifetime"),
     ("resource release", "freeing a resource in the matching destructor", "destructor", "release", "free", "end lifetime"),
     ("shallow copy", "a copy that shares pointed-to data", "pointer", "shared", "same data", "not independent"),
     ("deep copy", "a copy with its own duplicated data", "own copy", "duplicate", "independent", "safe"),
   ]},
  "patterns": {
   "tags": ["OOP", "Design Patterns"],
   "concepts": [
     ("Singleton", "a class with exactly one shared instance", "one instance", "shared", "global", "lazy"),
     ("Factory Method", "deferring instantiation to subclasses through a creation method", "defer", "create", "subclass", "instantiation"),
     ("Observer", "notifying dependents when a subject changes state", "notify", "dependents", "state change", "subscribe"),
     ("Strategy", "interchangeable algorithms behind a common interface", "interchangeable", "algorithm", "interface", "swap"),
     ("Repository", "mediating between the domain and data mapping using collection-like interfaces", "mediate", "collection", "domain", "mapping"),
     ("Builder", "constructing a complex object step by step", "step by step", "construct", "complex", "partial"),
     ("Adaptate pattern", "converting one interface into another a client expects", "convert", "interface", "expects", "wrap"),
     ("Facade", "a single simplified interface over a subsystem", "simplify", "subsystem", "one interface", "high level"),
   ]},
  "design": {
   "tags": ["OOP", "Design"],
   "concepts": [
     ("SOLID", "a set of five object-oriented design principles", "five", "design", "principles", "maintainable"),
     ("composition over inheritance", "preferring contained objects over subclassing", "prefer", "contained", "rather than", "subclass"),
     ("interface segregation", "splitting wide interfaces into focused ones", "split", "narrow", "focused", "client"),
   ]},
 }},

# ------------------------------------------------------------------ databases
"dbms": {
 "topics": {
  "relational": {
   "tags": ["SQL & Databases", "Relational Model"],
   "concepts": [
     ("relation", "a table with a fixed set of named attributes", "table", "attribute", "tuple", "attribute"),
     ("primary key", "an attribute or set that uniquely identifies a row", "unique", "row", "identify", "not null"),
     ("foreign key", "an attribute referencing the primary key of another relation", "reference", "another relation", "key", "referential"),
     ("candidate key", "a minimal set of attributes that identifies rows uniquely", "minimal", "unique", "superkey", "identify"),
     ("functional dependency", "one attribute set determining the values of another", "determine", "attribute", "value", "dependency"),
   ]},
  "sql": {
   "tags": ["SQL & Databases", "SQL"],
   "concepts": [
     ("INNER JOIN", "a join that keeps only matching rows from both sides", "both sides", "match", "only", "intersection"),
     ("LEFT JOIN", "joining that keeps all rows of the left relation", "all left", "keep", "unmatched", "null"),
     ("GROUP BY", "collapsing rows into groups for aggregate results", "groups", "aggregate", "per group", "collapsed"),
     ("HAVING", "filtering groups produced by GROUP BY", "filter", "groups", "after grouping", "aggregate"),
     ("window function", "a calculation across a set of rows related to the current row", "current row", "set of rows", "over", "partition"),
   ]},
  "normalization": {
   "tags": ["SQL & Databases", "Normalization"],
   "concepts": [
     ("first normal form", "the requirement that every attribute holds atomic values", "atomic", "single valued", "repeating group", "indivisible"),
     ("second normal form", "removing partial dependencies on part of a composite key", "partial", "composite key", "remove", "non-prime"),
     ("third normal form", "removing dependencies on non-key attributes", "transitive", "non-key", "remove", "prime"),
     ("BCNF", "the normal form where every determinant is a superkey", "determinant", "superkey", "stricter", "third"),
     ("denormalisation", "introducing redundancy to reduce joins at query time", "redundancy", "reduce joins", "read", "trade-off"),
   ]},
  "transactions": {
   "tags": ["SQL & Databases", "Transactions"],
   "concepts": [
     ("ACID", "the atomicity, consistency, isolation and durability guarantees", "guarantee", "isolation", "durability", "atomicity"),
     ("atomicity", "all of a transaction's effects apply or none do", "all or nothing", "rollback", "commit", "all"),
     ("consistency", "a transaction leaves the database in a valid state", "valid state", "constraint", "before", "after"),
     ("isolation", "a transaction's effects are hidden until commit", "hidden", "concurrent", "commit", "separate"),
     ("durability", "committed changes survive failures", "committed", "survive", "permanent", "failure"),
     ("lost update", "two transactions overwriting each other's change", "overwrite", "concurrent", "both write", "lost"),
     ("phantom read", "a repeated range query returning newly inserted rows", "range", "new rows", "repeat", "inserted"),
   ]},
  "indexing": {
   "tags": ["SQL & Databases", "Indexing"],
   "concepts": [
     ("B+ tree index", "a balanced index whose leaves are linked for range scans", "range", "linked", "leaf", "ordered"),
     ("clustered index", "an index that determines the physical order of rows", "physical order", "rows", "table order", "dictates"),
     ("covering index", "an index that contains every column a query needs", "every column", "satisfy", "without table", "answer from index"),
     ("index selectivity", "the fraction of rows a predicate selects", "fraction", "selective", "rows", "cardinality"),
     ("unique index", "an index enforcing uniqueness across its key columns", "uniqueness", "key", "constraint", "duplicates"),
   ]},
  "query": {
   "tags": ["SQL & Databases", "Query Processing"],
   "concepts": [
     ("query plan", "the ordered operations a database uses to answer a query", "ordered", "operations", "join order", "strategy"),
     ("nested-loop join", "a join that scans the inner relation per outer row", "nested", "inner", "outer", "per row"),
     ("hash join", "a join that builds a hash table then probes it", "build", "probe", "hash table", "streaming"),
     ("cost-based optimisation", "choosing a plan by estimated cardinality using statistics", "estimate", "statistics", "cost", "choose"),
   ]},
 }},

# --------------------------------------------------------- operating systems
"os": {
 "topics": {
  "processes": {
   "tags": ["Operating Systems", "Processes"],
   "concepts": [
     ("Process Control Block", "kernel data describing one process including its saved registers", "kernel", "data", "registers", "saved state"),
     ("process state", "the lifecycle stage: new, ready, running, waiting or terminated", "stage", "lifecycle", "transition", "ready"),
     ("context switch", "saving one process's state and restoring another's", "save", "restore", "switch", "overhead"),
     ("thread", "an execution stream sharing an address space with others", "execution", "address space", "shared", "lightweight"),
   ]},
  "scheduling": {
   "tags": ["Operating Systems", "Scheduling"],
   "concepts": [
     ("first come first served", "serving processes in arrival order", "arrival", "order", "queue", "convoy"),
     ("shortest job first", "scheduling the shortest CPU burst next", "shortest", "burst", "next", "priority based on length"),
     ("round robin", "giving each process a small time slice in turn", "time slice", "quantum", "turn", "preempt"),
     ("multilevel feedback queue", "demoting long jobs and promoting interactive ones", "demote", "promote", "queue", "interactive"),
     ("starvation", "a process never being scheduled despite being ready", "never scheduled", "ready", "indefinite", "unfair"),
     ("convoy effect", "short jobs waiting behind one long job", "waiting", "long job", "behind", "throughput"),
   ]},
  "concurrency": {
   "tags": ["Operating Systems", "Concurrency"],
   "concepts": [
     ("mutual exclusion", "only one process at a time in a critical section", "critical section", "only one", "exclusive", "guarantee"),
     ("critical section", "the code region accessing shared state", "code", "shared", "region", "accessing"),
     ("semaphore", "a counter controlling access to shared resources", "counter", "wait", "signal", "resource"),
     ("race condition", "an outcome depending on the relative timing of accesses", "timing", "timing-dependent", "shared", "ordering"),
     ("deadlock", "processes waiting forever on each other", "waiting", "forever", "cycle", "cannot proceed"),
     ("priority inversion", "a high-priority task waiting behind a lower one", "high priority", "waiting behind", "lower", "inversion"),
     ("priority inheritance", "a blocking low task temporarily inheriting a higher priority", "inherit", "temporary", "blocked", "higher"),
   ]},
  "memory": {
   "tags": ["Operating Systems", "Memory"],
   "concepts": [
     ("virtual memory", "an address space per process mapped by the OS", "address space", "per process", "mapped", "isolated"),
     ("paging", "splitting a process's address space into fixed pages", "page", "frame", "fixed", "fixed size"),
     ("segmentation", "splitting a process's address space into logical segments", "logical", "segment", "grow", "variable"),
     ("TLB", "a cache of recent translations, avoiding a page-table walk", "cache", "translation", "avoid", "fast"),
     ("page fault", "an access to a page that must be fetched from storage", "not resident", "fetch", "storage", "miss"),
     ("demand paging", "loading a page only when it is first accessed", "on demand", "first access", "load", "lazy"),
     ("thrashing", "excessive page-replacement traffic", "excessive", "replacement", "traffic", "paging storm"),
   ]},
  "storage": {
   "tags": ["Operating Systems", "Storage"],
   "concepts": [
     ("journaling", "recording changes in a log before applying them", "log", "before", "recovery", "consistent"),
     ("FIFO page replacement", "evicting whichever page was loaded first", "first loaded", "evict", "oldest", "queue order"),
     ("disk scheduling", "ordering disk requests to reduce seek movement", "seek", "movement", "throughput", "doorstep"),
   ]},
 }},

# ------------------------------------------------------------ computer networks
"networks": {
 "topics": {
  "layers": {
   "tags": ["Computer Networks", "Layers"],
   "concepts": [
     ("OSI model", "a seven-layer description of network functions", "seven", "layers", "functions", "model"),
     ("TCP/IP model", "the four-layer suite underlying the internet", "internet", "four", "layers", "suite"),
     ("data link layer", "the layer that transfers frames on a shared link", "frame", "link", "frame", "second"),
     ("switch", "a device forwarding frames by MAC address", "MAC", "forward", "frame", "link"),
     ("router", "a device forwarding packets between networks by IP address", "IP", "forward", "packet", "between"),
     ("encapsulation", "adding a header at each layer as data travels down", "header", "layer", "down", "encapsulate"),
   ]},
  "addressing": {
   "tags": ["Computer Networks", "Addressing"],
   "concepts": [
     ("IPv4 addressing", "32-bit addresses written as four dotted decimal octets", "32 bit", "octet", "decimal", "dotted"),
     ("CIDR", "variable-length prefixes that allow route aggregation", "prefix", "varlength", "aggregate", "subnet mask"),
     ("subnet mask", "bit pattern separating a network portion from a host portion", "network", "host", "separate", "pattern"),
     ("ARP", "resolving an IP address to a MAC address on a link", "IP to MAC", "link", "resolve", "broadcast"),
   ]},
  "transport": {
   "tags": ["Computer Networks", "Transport"],
   "concepts": [
     ("TCP", "a connection-oriented reliable byte stream transport", "reliable", "connection", "stream", "ordered"),
     ("UDP", "a connectionless datagram transport", "connectionless", "datagram", "unreliable", "simple"),
     ("three-way handshake", "the SYN, SYN-ACK, ACK exchange that opens a connection", "SYN", "handshake", "ACK", "connection open"),
     ("piggybacking", "carrying an acknowledge on a data packet", "with data", "packet", "acknowledge", "efficiency"),
     ("slow start", "doubling the congestion window at the beginning of a transfer", "double", "start", "window", "congestion"),
     ("congestion window", "the sender's estimate of what the network can carry", "estimate", "sender", "capacity", "window"),
     ("flow control", "preventing a sender from outrunning the receiver, using the receiver's buffer", "receiver", "buffer", "window", "sender pacing"),
   ]},
  "congestion": {
   "tags": ["Computer Networks", "Congestion"],
   "concepts": [
     ("additive increase", "increasing the congestion window by one per round trip", "one per round", "linear", "increase", "avoidance"),
     ("multiplicative decrease", "halving the congestion window after a loss", "halve", "loss", "wieder", "back off"),
     ("silly window syndrome", "transmitting tiny segments because of small advertised windows", "tiny", "segment", "small", "advertised"),
     ("fast retransmit", "resending on repeated acknowledgements before the timer expires", "repeated", "acknowledge", "before timeout", "resend quickly"),
   ]},
  "routing": {
   "tags": ["Computer Networks", "Routing"],
   "concepts": [
     ("OSPF", "a link-state interior gateway protocol", "link state", "interior", "gateway", "flooding"),
     ("RIP", "a distance-vector interior gateway protocol", "distance vector", "interior", "hops", "gateway"),
     ("BGP", "the path-vector protocol used between autonomous systems", "path vector", "autonomous", "inter-domain", "policy"),
     ("distance-vector routing", "routing where neighbours share their distance estimates", "share", "distance", "neighbour", "estimate"),
     ("link-state routing", "routing where each node floods its own topology view", "flood", "topology", "own view", "link state"),
   ]},
  "security": {
   "tags": ["Computer Networks", "Security"],
   "concepts": [
     ("TLS", "a protocol giving confidentiality and integrity on top of a transport", "confidential", "integrity", "handshake", "certificate"),
     ("symmetric encryption", "encryption where the same key encrypts and decrypts", "same key", "encrypt", "decrypt", "shared secret"),
     ("perfect forward secrecy", "compromising one session key does not expose earlier ones", "session", "earlier", "compromise", "independent"),
   ]},
 }},

# ------------------------------------------------------------- signal processing
"signals": {
 "topics": {
  "signal_types": {
   "tags": ["Signal Processing", "Signals"],
   "concepts": [
     ("continuous-time signal", "a signal defined at every instant", "every instant", "definition", "continuous", "real-valued"),
     ("discrete-time signal", "a signal defined at integer instants", "integer", "sequence", "index", "discrete"),
     ("even signal", "a signal whose graph is symmetric about the origin", "symmetric", "origin", "even", "mirror"),
     ("odd signal", "a signal whose graph is antisymmetric about the origin", "antisymmetric", "origin", "odd", "negative mirror"),
     ("convergent series", "a signal that is either even or odd", "even", "odd", "decompose", "sum"),
   ]},
  "convolution": {
   "tags": ["Signal Processing", "Convolution"],
   "concepts": [
     ("convolution", "the integral combining two signals into a third, weighted overlap", "integral", "overlap", "combine", "weighted"),
     ("unit impulse", "a signal that acts as the identity for convolution", "identity", "delta", "identity element", "identity under convolution"),
     ("shifted impulse", "an impulse at a non-zero position, used to sample a signal", "position", "sample", "offset", "extract"),
     ("Fourier transform pair", "a signal and its transform linked by linearity and convolution theorems", "linked", "theorems", "duality", "convolution"),
   ]},
  "fourier": {
   "tags": ["Signal Processing", "Fourier"],
   "concepts": [
     ("Fourier transform", "a decomposition of a signal into sinusoidal components", "sinusoidal", "component", "frequencies", "decompose"),
     ("frequency domain", "the representation listing a signal's frequency components", "frequencies", "component", "amplitude", "representation"),
     ("magnitude spectrum", "the amplitudes of each frequency component", "amplitude", "component", "frequency", "size"),
     ("phase spectrum", "the phase shift of each frequency component", "phase", "shift", "component", "angle"),
   ]},
  "sampling": {
   "tags": ["Signal Processing", "Sampling"],
   "concepts": [
     ("Nyquist rate", "twice the bandwidth, the minimum rate for perfect reconstruction", "two times", "reconstruct", "limit", "sampling"),
     ("aliasing", "distortion when a signal is sampled too slowly", "too slow", "overlap", "spectra", "alias"),
     ("sinc reconstruction", "reconstructing a signal by summing shifted sinusoids", "sin", "shift", "reconstruct", "sum"),
     ("quantisation", "mapping an amplitude to a discrete level", "level", "round", "quantise", "amplitude"),
     ("quantisation error", "the gap between an amplitude and its nearest level", "gap", "nearest", "amplitude", "error"),
     ("Gibbs phenomenon", "overshoot from truncating a signal's harmonic series", "overshoot", "truncate", "harmonic", "partial sum"),
   ]},
  "filters": {
   "tags": ["Signal Processing", "Filters"],
   "concepts": [
     ("low-pass filter", "a filter that passes frequencies below a cutoff", "cutoff", "passes", "below", "low"),
     ("FIR filter", "a filter with feed-forward terms, always stable", "no feedback", "stable", "linear phase", "finite impulse"),
     ("Butterworth filter", "a filter with a maximally flat passband response", "flat", "passband", "no ripple", "maximally flat"),
     ("FIR stability", "FIR filters being always stable since they lack feedback", "always stable", "no poles", "feedback", "bounded input"),
   ]},
  "z_transform": {
   "tags": ["Signal Processing", "Z Transform"],
   "concepts": [
     ("region of convergence", "the set where a transform's series converges", "converges", "z-domain", "z-plane", "stability"),
     ("z-plane", "a complex plane mapping discrete system behaviour", "unit circle", "z", "complex", "poles"),
   ]},
 }},

# --------------------------------------------------------------- circuits
"analog": {
 "topics": {
  "diodes": {
   "tags": ["Circuit Design", "Diodes"],
   "concepts": [
     ("knee voltage", "the forward bias above which a diode conducts strongly", "conduct", "threshold", "forward bias", "turns on"),
     ("reverse leakage", "the small current that flows in reverse bias", "reverse", "leakage", "tiny", "saturation"),
     ("dynamic resistance", "the effective slope resistance of a diode at its bias point", "slope", "resistance", "bias point", "small-signal"),
     ("breakdown voltage", "the reverse voltage where a diode conducts heavily", "reverse", "breakdown", "zener", "heavy current"),
   ]},
  "transistors": {
   "tags": ["Circuit Design", "Transistors"],
   "concepts": [
     ("common-emitter", "a configuration with the emitter shared by input and output", "shared", "emitter", "gain", "inverting"),
     ("biasing", "setting an operating point with resistor networks", "operating point", "resistors", "steady", "q point"),
     ("current gain", "the ratio of collector current to base current", "ratio", "collector", "base", "amplification"),
     ("saturation region", "the low-output-voltage operating mode of a transistor acting as a switch", "switch", "both junctions", "on state", "low voltage"),
     ("cutoff region", "the ideally-off mode of a transistor acting as an open switch", "open", "off", "no current", "switch off"),
   ]},
  "opamps": {
   "tags": ["Circuit Design", "Opamps"],
   "concepts": [
     ("slew rate", "the maximum rate of change of an op-amp output", "rate", "volts per microsecond", "limit", "fastest"),
     ("input resistance", "the very high resistance at which an op-amp draws signal", "high", "input", "draw", "negligible current"),
     ("virtual short", "the approximation that input terminals sit at equal potential with negative feedback", "no current", "feedback", "zero difference", "virtual ground"),
     ("voltage follower", "a buffer whose gain is one, isolating stages", "one", "buffer", "isolate", "unity"),
     ("gain-bandwidth product", "the roughly constant product of closed-loop gain and bandwidth", "approximately constant", "compensation", "gain", "bandwidth"),
     ("offset voltage", "the small input difference needed to null the output", "input difference", "null", "mismatch", "output zero"),
   ]},
  "amplifiers": {
   "tags": ["Circuit Design", "Amplifiers"],
   "concepts": [
     ("Miller effect", "multiplying an internal capacitor by the amplifier gain at the input", "multiplication", "input", "capacitor", "gain times"),
     ("cascode", "a two-stage arrangement reducing the Miller capacitance", "cascade", "reduce", "capacitor", "bandwidth"),
     ("negative feedback", "feeding back a fraction of the output to lower gain and distortion", "fraction", "stable", "lower gain", "distortion"),
     ("coupling capacitor", "a capacitor passing a signal while blocking bias at DC", "block", "couple", "bias", "high pass"),
   ]},
  "noise": {
   "tags": ["Circuit Design", "Noise"],
   "concepts": [
     ("thermal noise", "noise arising from random carrier motion in a resistor", "resistor", "random", "temperature", "white"),
     ("flicker noise", "noise that rises as frequency falls", "low frequency", "one over f", "rise", "carries"),
     ("signal-to-noise ratio", "the ratio of signal power to noise power", "power", "ratio", "quality", "power ratio"),
   ]},
 }},

# ------------------------------------------------------------------- aptitude
"aptitude": {
 "topics": {
  "arithmetic": {
   "tags": ["Quantitative Aptitude", "Arithmetic"],
   "concepts": [
     ("average", "the sum of values divided by their count", "sum by count", "mean", "central", "typical value"),
     ("percentage", "a value expressed as a part of a hundred", "parts per hundred", "percentage", "scaling", "proportion"),
     ("ratio", "a comparison of two quantities by division", "compare", "two quantities", "proportion", "fraction"),
     ("proportion", "a statement that two ratios are equal", "equal ratios", "proportion", "cross multiply", "equality"),
     ("mixed ratio", "a ratio combining two data sets with different quantities", "combining", "different", "weighted", "data"),
   ]},
  "percentages": {
   "tags": ["Quantitative Aptitude", "Percentages"],
   "concepts": [
     ("percentage increase", "the ratio of increase to the original amount, times 100", "times 100", "original", "increase", "raise"),
     ("percentage decrease", "the ratio of reduction to the original amount, times 100", "times 100", "original", "reduction", "fall"),
     ("compound growth", "applying a percentage change repeatedly over periods", "repeated", "period", "accumulative", "base"),
   ]},
  "time_speed": {
   "tags": ["Quantitative Aptitude", "Time and Speed"],
   "concepts": [
     ("average speed", "total distance divided by total time", "distance", "time", "total", "average"),
     ("relative speed", "the closing rate when two bodies move toward or away from each other", "towards", "closing rate", "closing speed", "gap"),
     ("work rate", "the fraction of a job one worker completes per unit time", "per unit time", "fraction", "job", "speed"),
     ("combined rate", "the sum of workers' rates, giving the time together", "sum", "same time", "faster", "both"),
   ]},
  "profit_loss": {
   "tags": ["Quantitative Aptitude", "Profit and Loss"],
   "concepts": [
     ("profit percentage", "profit divided by cost price, times 100", "cost price", "times 100", "gain", "cost"),
     ("loss percentage", "loss divided by cost price, times 100", "cost price", "times 100", "loss", "cost"),
     ("successive discount", "applying two discounts in turn to a listed price", "successive", "listed", "turn", "second discount"),
     ("marked price", "the listed price before any discount", "listed", "initial", "before", "discount"),
   ]},
  "probability": {
   "tags": ["Quantitative Aptitude", "Probability"],
   "concepts": [
     ("random experiment", "an experiment with more than one possible outcome", "outcome", "chance", "outcomes", "experiment"),
     ("mutually exclusive", "events that cannot happen together", "cannot both", "exclusive", "together", "events"),
     ("independent events", "events where the occurrence of one does not affect the other", "does not affect", "independent", "either one", "events"),
     ("conditional probability", "the chance of an event given another has occurred", "given", "given that", "event", "conditioned"),
   ]},
  "number_series": {
   "tags": ["Quantitative Aptitude", "Number Series"],
   "concepts": [
     ("arithmetic progression", "a sequence with a constant difference between terms", "common difference", "constant", "sequence", "ap"),
     ("geometric progression", "a sequence with a constant ratio between terms", "common ratio", "constant", "sequence", "gp"),
     ("triangular number", "a number equal to the sum of the natural numbers up to it", "sum", "triangular", "natural", "dot pattern"),
     ("square number", "a number equal to an integer multiplied by itself", "nn", "square", "integer", "times itself"),
   ]},
 }},

# ------------------------------------------------------------------ reasoning
"reasoning": {
 "topics": {
  "series": {
   "tags": ["Logical Reasoning", "Series"],
   "concepts": [
     ("letter series", "a sequence of letters following a pattern", "letters", "pattern", "sequence", "alphabet"),
     ("jumping series", "a sequence of items picked at regular intervals", "regular", "interval", "pick", "jump"),
     ("middle term", "the value excluding the first and last" , "excluding", "average", "sum of ends", "middle term"),
   ]},
  "odd_one_out": {
   "tags": ["Logical Reasoning", "Odd One Out"],
   "concepts": [
     ("odd one out", "the single member that fails to share the group's property", "single", "property", "fails", "the odd"),
     ("prime number", "a number with exactly two distinct divisors", "two divisors", "prime", "itself and one", "divisor"),
     ("composite number", "a number with more than two distinct divisors", "not prime", "more than two", "divisor", "factorable"),
   ]},
  "coding_decoding": {
   "tags": ["Logical Reasoning", "Coding and Decoding"],
   "concepts": [
     ("letter shifting", "moving each letter by a fixed amount in the alphabet", "fixed", "shift", "each letter", "offset"),
     ("reversal coding", "writing letters of a word in the opposite order", "reversed", "opposite order", "letters", "word"),
     ("substitution code", "replacing each letter by another letter according to a rule", "replacing", "substituting", "according to", "cipher"),
     ("case-based coding", "encoding two or more words to reveal the pattern, then applying it", "two", "pattern", "apply", "same rule"),
   ]},
  "blood_relations": {
   "tags": ["Logical Reasoning", "Blood Relations"],
   "concepts": [
     ("family chart", "a chart mapping bonds between relatives", "chart", "bonds", "relatives", "tree"),
     ("maternal line", "relatives through the mother's side", "mother", "maternal", "side", "mother side"),
     ("paternal line", "relatives through the father's side", "father", "paternal", "side", "father side"),
     ("sibling", "a brother or sister sharing the same parents", "brother", "sister", "same parents", "siblings"),
   ]},
  "directions": {
   "tags": ["Logical Reasoning", "Directions"],
   "concepts": [
     ("cardinal directions", "north, east, south and west on a compass", "north", "east", "compass", "south"),
     ("compass turn", "a one-quarter turn changes the facing by ninety degrees", "quarter", "ninety", "turn", "right angle"),
     ("Manhattan distance", "the sum of absolute displacements on axes", "absolute", "sum", "axes", "grid distance"),
   ]},
  "seating": {
   "tags": ["Logical Reasoning", "Seating"],
   "concepts": [
     ("circular seating", "placing people around a ring with fixed relationships", "ring", "circle", "fixed", "around"),
     ("facing inward", "a seated person looks toward the centre of a circle", "centre", "towards", "inward", "look at centre"),
     ("opposite seats", "seats half a circle apart", "half circle", "apart", "diametrically opposite", "across"),
   ]},
  "syllogism": {
   "tags": ["Logical Reasoning", "Syllogism"],
   "concepts": [
     ("syllogism", "an argument deriving a conclusion from two premises and a middle term", "two premises", "conclusion", "middle", "implies"),
     ("valid deduction", "a conclusion that follows necessarily from the premises", "necessarily", "if premises", "logically", "must follow"),
     ("direct inference", "a conclusion that restates a premise exactly", "restate", "same meaning", "directly true", "rephrase"),
   ]},
  "puzzles": {
   "tags": ["Logical Reasoning", "Puzzles"],
   "concepts": [
     ("constraint puzzle", "a puzzle where pieces must satisfy fixed conditions", "constraint", "satisfy", "conditions", "logic puzzle"),
     ("grid puzzle", "solving by annotating a table of fixed cells", "table", "cell", "constraint", "grid"),
     ("process of elimination", "removing impossible options until only one remains", "eliminate", "impossible", "only one", "narrowing"),
   ]},
 }},

# ----------------------------------------------------------------- accounting
"accounting": {
 "topics": {
  "basics": {
   "tags": ["Accounting", "Basics"],
   "concepts": [
     ("accounting equation", "assets equal liabilities plus equity", "assets", "liabilities", "equity", "balance"),
     ("trial balance", "a check that total debits equal total credits", "debits", "credits", "equal", "check"),
     ("double-entry", "recording each transaction as a balanced entry on both sides", "balanced", "entry", "debit", "credit"),
     ("journal", "the book recording transactions chronologically", "chronological", "record", "first entry", "day one"),
   ]},
  "books": {
   "tags": ["Accounting", "Books"],
   "concepts": [
     ("debit", "the left-hand entry in a ledger account", "left", "entry", "side", "ledger"),
     ("credit", "the right-hand entry in a ledger account", "right", "entry", "side", "ledger"),
     ("asset increase", "an asset account is increased by a debit", "debit", "increase", "asset", "left side"),
     ("liability increase", "a liability account is increased by a credit", "credit", "increase", "liability", "right side"),
     ("drawing", "amounts withdrawn by the owner, reducing equity", "withdraw", "owner", "equity", "reduce"),
   ]},
  "statements": {
   "tags": ["Accounting", "Statements"],
   "concepts": [
     ("balance sheet", "the statement of position at a point in time", "position", "date", "point in time", "moment"),
     ("income statement", "the statement of revenue less expenses for a period", "period", "revenue", "expenses", "profit"),
     ("cash flow statement", "the statement of cash movements over a period", "cash", "movement", "period", "source"),
     ("accrual basis", "revenue recognised when earned rather than when received", "earned", "recognised", "received", "match"),
   ]},
  "cash_flow": {
   "tags": ["Financial Analysis", "Cash Flow"],
   "concepts": [
     ("operating cash flow", "cash generated by the normal business activity", "operations", "business", "generated", "normal"),
     ("financing cash flow", "cash from borrowing, repaying or dividend payments", "borrowing", "financing", "dividend", "issue shares"),
     ("investing cash flow", "cash used for or gained from long-term assets", "long-term", "assets", "purchase", "investing"),
     ("add back depreciation", "restoring depreciation to profit in the cash flow statement", "non-cash", "add back", "profit", "restore"),
   ]},
  "ratios": {
   "tags": ["Financial Analysis", "Ratios"],
   "concepts": [
     ("current ratio", "current assets divided by current liabilities", "current", "ratio", "short term", "liquidity"),
     ("quick ratio", "a tighter current ratio excluding stock", "stock", "exclude", "liquid", "tight"),
     ("debt-to-equity", "the ratio of total debt to shareholders' funds", "debt", "equity", "leverage", "funds"),
     ("inventory turnover", "cost of sales divided by average inventory", "cost of sales", "average", "times", "turnover"),
   ]},
  "depreciation": {
   "tags": ["Accounting", "Depreciation"],
   "concepts": [
     ("depreciation", "allocating an asset's cost across its useful life", "useful life", "cost", "allocation", "expense"),
     ("straight-line method", "equal depreciation expense in each period", "equal", "period", "equal amount", "constant"),
     ("written-down value", "a depreciation calculation applied to a declining carrying amount", "declining", "carrying", "reducing", "wbv"),
   ]},
  "inventory": {
   "tags": ["Accounting", "Inventory"],
   "concepts": [
     ("FIFO", "assuming the earliest purchases are sold first", "earliest", "first", "sold first", "stock remaining"),
     ("LIFO", "assuming the latest purchases are sold first", "latest", "last", "stack", "recent"),
     ("weighted average", "costing inventory using the average of purchases", "average", "blend", "purchases", "pool"),
     ("closing inventory", "the value of stock remaining at period end", "remaining", "value", "stock", "close"),
   ]},
  "revenue": {
   "tags": ["Financial Analysis", "Revenue"],
   "concepts": [
     ("deferred revenue", "cash received before the obligation is met", "advance", "liability", "not yet earned", "later"),
     ("revenue recognition", "recording revenue when the performance obligation is satisfied", "satisfy", "when", "obligation", "revenue"),
     ("unearned revenue", "same as deferred revenue: cash received before earning", "not earned", "advance", "liability", "future"),
   ]},
  "analysis": {
   "tags": ["Financial Analysis", "Analysis"],
   "concepts": [
     ("return on equity", "profit divided by shareholders' equity", "equity", "profit", "return", "residual"),
     ("DuPont identity", "splitting return on equity into margin, turnover and leverage", "split", "margin", "leverage", "identify"),
     ("gross profit margin", "gross profit divided by revenue", "revenue", "gross", "margin", "above costs"),
   ]},
 }},

# --------------------------------------------------------------------- hr
"hr": {
 "topics": {
  "behavioural": {
   "tags": ["Behavioural Interviews", "Communication"],
   "concepts": [
     ("STAR method", "answering with Situation, Task, Action and Result", "situation", "task", "action", "result"),
     ("active listening", "concentrating fully and clarifying before answering", "concentrate", "full attention", "clarify", "listen"),
     ("storytelling", "structuring an answer with context, challenge and outcome", "context", "challenge", "outcome", "narrative"),
     ("quantified impact", "backing a claim with a measurable figure", "number", "measured", "percent", "figure"),
   ]},
  "career": {
   "tags": ["Behavioural Interviews", "Career Planning"],
   "concepts": [
     ("career objective", "a statement of the roles and direction you want", "want", "direction", "role", "intend"),
     ("strength and weakness", "answering with a real growth area and the steps taken to improve", "improve", "steps", "growth area", "progress"),
     ("gap handling", "explaining any employment gap with concrete learning or work", "learning", "explain", "gap", "concrete"),
   ]},
  "negotiation": {
   "tags": ["Behavioural Interviews", "Negotiation"],
   "concepts": [
     ("salary anchoring", "quoting a researched range first when asked for expectations", "range", "research", "first", "expectation"),
     ("total package", "evaluating pay alongside benefits, learning and growth", "total", "benefits", "growth", "package"),
   ]},
  "teamwork": {
   "tags": ["Behavioural Interviews", "Teamwork"],
   "concepts": [
     ("shared ownership", "taking responsibility for team outcomes rather than only yours", "team", "shared", "responsibility", "outcome"),
     ("constructive disagreement", "questioning ideas on the idea, not the person", "ideas", "not person", "questions", "support"),
   ]},
  "leadership": {
   "tags": ["Behavioural Interviews", "Leadership"],
   "concepts": [
     ("influence without authority", "leading by coordination and credibility rather than promotion", "without", "credibility", "coordinate", "lead"),
     ("mentoring", "helping a colleague improve by explaining and reviewing their work", "colleague", "explain", "review", "help"),
   ]},
  "scenario": {
   "tags": ["Behavioural Interviews", "Problem Solving"],
   "concepts": [
     ("disagree-and-commit", "raising concerns then supporting the decision", "raise", "concern", "commit", "decide"),
     ("escalation", "taking an unresolved issue to the right decision-maker", "unresolved", "escalate", "path", "decision-maker"),
   ]},
 }},

# ----------------------------------------------------- soft skills / placement
"placement": {
 "topics": {
  "communication": {
   "tags": ["Soft Skills", "Communication"],
   "concepts": [
     ("concise introduction", "a thirty-second summary of who you are", "summary", "brief", "who you are", "short"),
     ("active listening", "concentrating fully on the speaker and responding", "listener", "concentrating", "respond", "focus"),
     ("avoiding jargon", "explaining in words the audience already uses", "audience", "plain", "jargon-free", "simple"),
     ("non-verbal cues", "posture and eye contact affecting credibility", "posture", "eye contact", "body", "non-verbal"),
   ]},
  "teamwork": {
   "tags": ["Soft Skills", "Teamwork"],
   "concepts": [
     ("shared goal", "aligning personal and team objectives", "align", "same", "goal", "objective"),
     ("responsibility split", "agreeing on the pieces each member owns", "who owns what", "split", "r&r", "assignment"),
     ("hand-offs", "making sure the receiver has context, not just files", "receiver", "context", "pass over", "handover"),
     ("debriefing", "a retrospective on what worked and what did not", "retrospective", "went well", "did not", "lessons"),
   ]},
  "conflict_resolution": {
   "tags": ["Soft Skills", "Conflict Resolution"],
   "concepts": [
     ("win-win framing", "reframing disagreement as goals to satisfy together", "framing", "both sides", "goal", "together"),
     ("separating people", "solving the issue without blaming individuals", "not the person", "issue", "blame-free", "both perspectives"),
     ("third option", "finding an answer beyond the two proposed positions", "extra options", "creative", "third", "new idea"),
     ("compromise", "each side accepting a workable middle position", "middle", "meet halfway", "both give", "acceptable"),
   ]},
  "adaptability": {
   "tags": ["Soft Skills", "Adaptability"],
   "concepts": [
     ("learning quickly", "learning new tools on the job by practising", "new tools", "fast", "practice", "upscale"),
     ("comfort with change", "adjusting when priorities change dynamically", "reordering", "priorities", "rolling", "adjust"),
     ("constructive feedback", "receiving critical comments as information to use", "critical", "comment", "use", "information"),
     ("rebound from failure", "owning a failure and applying the lesson", "failure", "own", "lesson", "recover"),
   ]},
  "ownership": {
   "tags": ["Soft Skills", "Ownership"],
   "concepts": [
     ("end-to-end ownership", "taking a task from start through delivered outcome", "from", "start", "end", "delivered"),
     ("proactive communication", "surfacing blockers and issues early", "blocker", "early", "raise", "before too late"),
     ("no surprises", "communicating status on time rather than late", "status", "on time", "shift", "earlier"),
     ("no hand-offs", "carrying a task through the remaining people involved", "whole", "carried", "through", "alone"),
   ]},
  "interview_readiness": {
   "tags": ["Soft Skills", "Interview Readiness"],
   "concepts": [
     ("company research", "knowing the company's products, market and teams", "products", "market", "research", "teams"),
     ("resume framing", "stating outcomes and impact rather than only tasks", "outcome", "impact", "achievement", "results"),
     ("aptitude practice", "practising timed problems before the assessment", "timed", "practice", "assessment", "speed"),
     ("technical preparation", "revisiting core data-structure and language concepts", "revisit", "core", "concepts", "data structures"),
   ]},
 }},
}

# flake8: noqa: E501 (long lines are fine here)



# ---------------------------------------------------------------------------
# Generation.
#
# Every question is derived from one concept, so a question always has a real
# answer and a real explanation. Distractors for MCQ questions are drawn from
# sibling concepts in the same subject, which keeps them plausible.
# ---------------------------------------------------------------------------

MCQ_EASY = "What is {term} in the context of {topic}?"
MCQ_EASY_ALT = "Which statement best describes {term}?"
MCQ_MED = "Which statement about {term} is correct?"
MCQ_MED_ALT = "Which of the following is TRUE of {term}?"
MCQ_HARD = "Given '{term}' is involved, what is the most likely diagnosis?"
TEXT_EASY = "Define {term} in {topic}, in your own words."
TEXT_EASY_ALT = "In two to three points, describe {term}."
TEXT_MED = "Why does {topic} rely on {term}? Give a concrete reason."
TEXT_MED_ALT = "Explain one practical consequence of {term}."
TEXT_HARD = "Contrast {term} with {other}. Which would you choose and why?"
SOFT_MCQ = "How would you explain {term} to a teammate who has not seen it?"
SOFT_TEXT = "Write a one-sentence explanation of {term} suitable for a project update."

SUBJECT_TITLES = {
    "algo": "Algorithms",
    "ds": "Data Structures",
    "oop": "Object Oriented Programming",
    "dbms": "Database Management Systems",
    "os": "Operating Systems",
    "networks": "Computer Networks",
    "signals": "Signals and Systems",
    "analog": "Analog Electronics",
    "aptitude": "Quantitative Aptitude",
    "reasoning": "Logical Reasoning",
    "accounting": "Accounting and Finance",
    "hr": "Human Resources and Behavioural",
    "placement": "Placement and Soft Skills",
}


class Generator(object):
    def __init__(self, seed=20240101):
        self.seed = seed

    # -- helpers ----------------------------------------------------------

    def flatten(self, banks):
        """subject -> [ (topic, tags, term, definition, keywords) ]"""
        out = {}
        for subject, data in banks.items():
            rows = []
            for topic, info in data["topics"].items():
                tags = info["tags"]
                for concept in info["concepts"]:
                    term = concept[0]
                    definition = concept[1]
                    keywords = list(concept[2:]) if len(concept) > 2 else []
                    if not keywords:
                        keywords = [term.lower()]
                    rows.append([topic, tags, term, definition, keywords])
            out[subject] = rows
        return out

    def mk_mcq(self, subject, topic, term, defn, others, stem, tags,
               keywords, soft, diff, qid, soft_tags=None):
        opts = [defn] + [o[1] for o in others]
        rnd = random.Random(self.seed + hash(qid) % 100000)
        rnd.shuffle(opts)
        answer = opts.index(defn)

        # Soft-tagged questions carry only soft tags. Mixing hard topic tags in
        # would make the skills dashboard classify those topics as soft.
        if soft:
            skill_tags = soft_tags if soft_tags is not None else ["Communication"]
        else:
            skill_tags = list(tags)

        return {
            "id": qid,
            "subject": subject,
            "topic": topic,
            "difficulty": diff,
            "type": "MCQ",
            "text": stem.format(term=term, topic=topic),
            "options": opts,
            "answer": answer,
            "explanation": "%s: %s" % (term, defn),
            "objective": "Identify the correct definition of %s." % term,
            "skillTags": skill_tags,
            "keywords": keywords,
            "softSkill": soft,
        }

    def mk_text(self, subject, topic, term, defn, tags, keywords,
                soft, diff, qid, stem, soft_tags=None):
        if soft:
            skill_tags = soft_tags if soft_tags is not None else ["Communication"]
        else:
            skill_tags = list(tags)

        return {
            "id": qid,
            "subject": subject,
            "topic": topic,
            "difficulty": diff,
            "type": "Text",
            "text": stem.format(term=term, topic=topic),
            "keywords": keywords,
            "minKeywords": 1,
            "explanation": "%s: %s" % (term, defn),
            "objective": "State %s correctly in your own words." % term,
            "skillTags": skill_tags,
            "softSkill": soft,
        }

    # -- main entry -------------------------------------------------------

    def run(self, out_dir):
        banks = self.flatten(BANKS)
        counts = {}
        for subject in sorted(banks.keys()):
            rows = banks[subject]
            rnd = random.Random(self.seed + len(subject) * 7919)
            questions = []

            easy, medium, hard = [], [], []
            for r in rows:
                topic, tags, term, defn, kws = r
                # Distractors: definitions of other concepts in the same
                # subject, preferring different topics but falling back.
                pool = [x for x in rows if x[2] != term]
                others = []
                seen = set()
                for candidate in pool:
                    if candidate[2] in seen:
                        continue
                    if len(others) < 3:
                        others.append((candidate[2], candidate[3]))
                        seen.add(candidate[2])
                # Pad from outside the subject if this subject is small.
                if len(others) < 3:
                    for sname in sorted(banks.keys()):
                        if sname == subject:
                            continue
                        for cand in banks[sname]:
                            if cand[2] not in seen and len(others) < 3:
                                others.append((cand[2], cand[3]))
                                seen.add(cand[2])
                if subject in SOFT_SUBJECTS:
                    # Whole-subject soft skills: emit the same recall/apply/
                    # explain shapes as technical subjects, but every question is
                    # soft-tagged and carries only its own skill tags. Without
                    # this, the "hard" templates would attach soft tags to hard
                    # entries and the skills chart would list them as technical.
                    for diffi, mcq_stem, txt_stem in (
                        ("Easy", MCQ_EASY, TEXT_EASY),
                        ("Medium", MCQ_MED, TEXT_MED),
                        ("Hard", MCQ_HARD, TEXT_HARD),
                    ):
                        n2 = "gen_%s_sm_%s_%d" % (subject, diffi[0], n)
                        questions.append(self.mk_mcq(
                            subject, topic, term, defn, others, mcq_stem,
                            list(tags), list(kws), True, diffi, n2,
                            soft_tags=list(tags)))
                        n3 = "gen_%s_sx_%s_%d" % (subject, diffi[0], n)
                        stem = txt_stem.format(term=term, topic=topic,
                                               other=others[0][0] if others else term)
                        questions.append(self.mk_text(
                            subject, topic, term, defn, list(tags), list(kws),
                            True, diffi, n3, stem, soft_tags=list(tags)))
                    n = len(questions) + 1
                    p = "%s_%d" % (subject, n)
                    continue

                n = len(questions) + 1
                p = "%s_%d" % (subject, n)

                # 1 easy MCQ (what is)
                stem = MCQ_EASY if n % 2 else MCQ_EASY_ALT
                questions.append(self.mk_mcq(subject, topic, term, defn,
                                             others, stem, list(tags),
                                             list(kws), False, "Easy",
                                             "gen_%s_e_%d" % (subject, n)))
                # 2 easy text (define)
                stem = TEXT_EASY if n % 2 else TEXT_EASY_ALT
                questions.append(self.mk_text(subject, topic, term, defn,
                                              list(tags), list(kws), False,
                                              "Easy", "gen_%s_et_%d" % (subject, n),
                                              stem))
                # 3 medium MCQ
                stem = MCQ_MED if n % 2 else MCQ_MED_ALT
                q = self.mk_mcq(subject, topic, term, defn, others, stem,
                                list(tags), list(kws), False, "Medium",
                                "gen_%s_m_%d" % (subject, n))
                questions.append(q)
                medium.append(q)
                # 4 medium text
                stem = TEXT_MED if n % 2 else TEXT_MED_ALT
                questions.append(self.mk_text(subject, topic, term, defn,
                                              list(tags), list(kws), False,
                                              "Medium", "gen_%s_mt_%d" % (subject, n),
                                              stem))
                # 5 hard MCQ (diagnostic)
                q = self.mk_mcq(subject, topic, term, defn, others, MCQ_HARD,
                                list(tags), list(kws), False, "Hard",
                                "gen_%s_h_%d" % (subject, n))
                questions.append(q)
                hard.append(q)
                # 6 hard text (contrast) when a sibling exists, else plain
                sibling = others[0][0] if others else term
                stem = TEXT_HARD.format(term=term, other=sibling, topic=topic)
                questions.append(self.mk_text(subject, topic, term, defn,
                                              list(tags), list(kws), False,
                                              "Hard", "gen_%s_ht_%d" % (subject, n),
                                              stem))
                # 7 soft MCQ (explain it) - present in every subject
                q = self.mk_mcq(subject, topic, term, defn, others, SOFT_MCQ,
                                list(tags), list(kws), True, "Medium",
                                "gen_%s_s_%d" % (subject, n),
                                soft_tags=["Communication"],
                                )
                q["skillTags"] = ["Communication"]
                questions.append(q)
                # 8 soft text (project update)
                q = self.mk_text(subject, topic, term, defn,
                                 list(tags), list(kws),
                                 True, "Medium", "gen_%s_st_%d" % (subject, n),
                                 SOFT_TEXT,
                                 soft_tags=["Communication"],
                                 )
                q["skillTags"] = ["Communication"]
                questions.append(q)

            # Keep generation order stable so files are diffable.
            rnd.shuffle(questions)
            path = os.path.join(out_dir, "gen_%s.json" % subject)
            with io.open(path, "w", encoding="utf-8") as fh:
                json.dump(questions, fh, indent=2, ensure_ascii=False)
                fh.write("\n")
            counts[subject] = len(questions)
        return counts


def main(argv=None):
    argv = argv if argv is not None else sys.argv[1:]
    seed = 20240101
    out_dir = "data/questions"
    quiet = False
    i = 0
    while i < len(argv):
        if argv[i] == "--seed" and i + 1 < len(argv):
            seed = int(argv[i + 1]); i += 2
        elif argv[i] == "--out" and i + 1 < len(argv):
            out_dir = argv[i + 1]; i += 2
        elif argv[i] in ("-q", "--quiet"):
            quiet = True; i += 1
        else:
            i += 1

    if not os.path.isdir(out_dir):
        os.makedirs(out_dir)

    gen = Generator(seed)
    counts = gen.run(out_dir)
    total = sum(counts.values())
    for subject in sorted(counts.keys()):
        print("%-12s %5d questions" % (subject, counts[subject]))
    print("total: %d" % total)
    return total


if __name__ == "__main__":
    main()

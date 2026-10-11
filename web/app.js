"use strict";

// ---------------------------------------------------------------------------
// API client. The only place the browser talks to the server.
// ---------------------------------------------------------------------------

const TOKEN_KEY = "ai_interviewer.token";

const state = {
  token: sessionStorage.getItem(TOKEN_KEY) || "",
  user: null,
  view: "login",
  registerMode: false,
  catalog: null,
  problems: [],
  problemsLoaded: 0,
  currentProblem: null,
  interviewId: null,
  interviewTotal: 0,
  lastRunResult: null,
  stats: null
};

async function request(path, options = {}) {
  const headers = Object.assign({}, options.headers);
  if (options.body) headers["Content-Type"] = "application/json";
  if (state.token) headers["Authorization"] = "Bearer " + state.token;

  let response;
  try {
    response = await fetch(path, { method: options.method || "GET", headers, body: options.body });
  } catch (networkError) {
    throw new Error("Cannot reach the server. Is it running?");
  }

  const text = await response.text();
  let payload = null;
  if (text) {
    try { payload = JSON.parse(text); } catch (parseError) { payload = null; }
  }
  if (!response.ok) {
    throw new Error((payload && payload.error) || "Request failed (" + response.status + ")");
  }
  return payload;
}

// ---------------------------------------------------------------------------
// Presentation helpers. No domain decisions here: this file renders what the
// API returns and forwards input to the API.
// ---------------------------------------------------------------------------

const $ = (id) => document.getElementById(id);

const NAV_ITEMS = [
  { group: "Practice" },
  { id: "problems",  label: "Problems",     ico: "#" },
  { id: "collections", label: "Study list", ico: "*" },
  { id: "start",     label: "Interview",    ico: ">" },
  { id: "daily",     label: "Daily",        ico: "!" },
  { group: "Progress" },
  { id: "dashboard", label: "Dashboard",    ico: "~" },
  { id: "profile",   label: "Profile",      ico: "@" },
  { id: "submissions", label: "Submissions", ico: "=" },
  { id: "board",     label: "Leaderboard",  ico: "^" },
  { group: "Admin" },
  { id: "admin",     label: "Admin",        ico: "+" }
];

function escapeHtml(text) {
  return String(text === null || text === undefined ? "" : text)
    .replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;");
}

function setMessage(id, message, kind) {
  const element = $(id);
  if (!element) return;
  element.textContent = message || "";
  element.className = "msg" + (kind ? " " + kind : "");
}

function difficultyPill(difficulty) {
  const key = String(difficulty || "").toLowerCase();
  const cls = key === "easy" ? "easy" : key === "medium" ? "medium" : "hard";
  return '<span class="pill ' + cls + '">' + escapeHtml(difficulty) + "</span>";
}

function show(view) {
  state.view = view;
  document.querySelectorAll(".view").forEach((section) => {
    section.hidden = section.id !== "view-" + view;
  });
  document.querySelectorAll(".side-item, .navbtn, [data-nav]").forEach((element) => {
    if (element.dataset.nav) {
      element.classList.toggle("active", element.dataset.nav === view);
    }
  });
  renderSidebar();

  if (view === "dashboard") loadDashboard();
  if (view === "problems") loadProblems();
  if (view === "submissions") loadSubmissions();
  if (view === "profile") loadProfile();
  if (view === "board") loadLeaderboard();
  if (view === "daily") loadDaily();
  if (view === "admin") loadAdmin();
  if (view === "start") { loadCourses(); loadSubjects(); }
}

document.addEventListener("click", (event) => {
  const trigger = event.target.closest("[data-nav]");
  if (!trigger) return;
  event.preventDefault();
  const view = trigger.dataset.nav;
  const authenticated = Boolean(state.user);
  if (!authenticated && view !== "login") { show("login"); return; }
  show(view);
});

function renderTopbar() {
  const nav = $("nav");
  nav.innerHTML = "";
  if (state.user) {
    const items = [
      { id: "problems", label: "Problems" },
      { id: "start", label: "Interview" },
      { id: "daily", label: "Daily" },
      { id: "board", label: "Leaderboard" }
    ];
    for (const item of items) {
      const button = document.createElement("button");
      button.className = "navbtn" + (state.view === item.id ? " active" : "");
      button.dataset.nav = item.id;
      button.textContent = item.label;
      nav.appendChild(button);
    }
  } else {
    const button = document.createElement("button");
    button.className = "navbtn active";
    button.dataset.nav = "login";
    button.textContent = "Sign in";
    nav.appendChild(button);
  }

  const who = $("who");
  who.innerHTML = "";
  if (state.user) {
    const label = document.createElement("span");
    label.textContent = state.user.name + " · " + state.user.role;
    const out = document.createElement("button");
    out.className = "ghost";
    out.textContent = "Sign out";
    out.addEventListener("click", signOut);
    who.appendChild(label);
    who.appendChild(out);
  }

  const adminEntry = NAV_ITEMS.find((item) => item.id === "admin");
  if (adminEntry) adminEntry.hidden = !(state.user && state.user.role === "Admin");
}

function renderSidebar() {
  const sidebar = $("sidebar");
  sidebar.innerHTML = "";
  if (!state.user) { sidebar.hidden = true; return; }
  sidebar.hidden = false;

  for (const item of NAV_ITEMS) {
    if (item.hidden) continue;
    if (item.group) {
      const label = document.createElement("p");
      label.className = "side-label";
      label.textContent = item.group;
      sidebar.appendChild(label);
      continue;
    }
    if (item.id === "admin" && !(state.user && state.user.role === "Admin")) continue;

    const button = document.createElement("button");
    button.className = "side-item" + (state.view === item.id ? " active" : "");
    button.dataset.nav = item.id;
    let suffix = "";
    if (item.id === "submissions" && state.stats) {
      suffix = "<span class='count'>" + state.stats.attempts + "</span>";
    } else if (item.id === "problems" && state.stats) {
      suffix = "<span class='count'>" + state.problemsLoaded + "</span>";
    }
    button.innerHTML = "<span class='ico'>" + item.ico + "</span>" + escapeHtml(item.label) + suffix;
    sidebar.appendChild(button);
  }
  renderTopbar();
}

// ---------------------------------------------------------------------------
// Auth
// ---------------------------------------------------------------------------

function toggleAuthMode() {
  state.registerMode = !state.registerMode;
  $("authTitle").textContent = state.registerMode ? "Create your account" : "Sign in";
  $("authSub").textContent = state.registerMode
    ? "Track solved problems, streaks and rankings."
    : "Continue to the practice platform.";
  $("authSubmit").textContent = state.registerMode ? "Register" : "Sign in";
  $("authMode").textContent = state.registerMode ? "Have an account? Sign in" : "Need an account? Register";
  $("registerFields").hidden = !state.registerMode;
  setMessage("authMsg", "");
}

$("authMode").addEventListener("click", toggleAuthMode);

$("authForm").addEventListener("submit", async (event) => {
  event.preventDefault();
  const form = event.target;
  const body = { email: form.email.value.trim(), password: form.password.value };
  if (state.registerMode) {
    body.name = form.name.value.trim() || body.email.split("@")[0];
    body.college = form.college.value.trim() || "Unspecified";
    body.branch = form.branch.value.trim() || "Unspecified";
  }
  if (!body.email || !body.password) {
    setMessage("authMsg", "Email and password are required.", "error");
    return;
  }

  setMessage("authMsg", "Working…");
  try {
    const payload = state.registerMode
      ? await request("/api/register", { method: "POST", body: JSON.stringify(body) })
      : await request("/api/login", { method: "POST", body: JSON.stringify(body) });
    state.token = payload.token;
    state.user = payload.user;
    sessionStorage.setItem(TOKEN_KEY, state.token);
    form.reset();
    state.registerMode = false;
    toggleAuthMode();
    await loadStats();
    show("dashboard");
  } catch (error) {
    setMessage("authMsg", error.message, "error");
  }
});

async function signOut() {
  try { await request("/api/logout", { method: "POST" }); } catch (error) { /* ignore */ }
  state.token = "";
  state.user = null;
  state.stats = null;
  state.problemsLoaded = 0;
  sessionStorage.removeItem(TOKEN_KEY);
  show("login");
}

// ---------------------------------------------------------------------------
// Shared data
// ---------------------------------------------------------------------------

async function loadStats() {
  try {
    state.stats = await request("/api/stats/me");
  } catch (error) {
    state.stats = null;
  }
}

async function loadCatalog() {
  if (state.catalog) return state.catalog;
  state.catalog = await request("/api/catalog");
  return state.catalog;
}

// ---------------------------------------------------------------------------
// Dashboard
// ---------------------------------------------------------------------------

async function loadDashboard() {
  await loadStats();
  const stats = state.stats || { solved: { total: 0 }, available: { total: 0 } };

  const solved = stats.solved || {};
  const available = stats.available || {};
  const solvedTotals = [solved.easy || 0, solved.medium || 0, solved.hard || 0];
  const availTotals = [available.easy || 0, available.medium || 0, available.hard || 0];
  const pct = (a, b) => (b > 0 ? Math.round((a / b) * 100) : 0);
  const acceptance = stats.acceptancePermille
    ? (stats.acceptancePermille / 10).toFixed(1) + "%"
    : "—";

  $("dashCards").innerHTML = [
    statCard(solvedTotals[0] + "/" + availTotals[0], "Easy solved"),
    statCard(solvedTotals[1] + "/" + availTotals[1], "Medium solved"),
    statCard(solvedTotals[2] + "/" + availTotals[2], "Hard solved"),
    statCard(solved.total || 0, "Total solved"),
    statCard(stats.attempts || 0, "Submissions"),
    statCard(acceptance, "Acceptance"),
    statCard(stats.streak || 0, "Current streak")
  ].join("");

  $("dashStreak").textContent = (stats.streak || 0) + " day streak · longest " + (stats.longestStreak || 0);

  // Recommended: unsolved, easiest first.
  const problems = await request("/api/problems?status=unsolved&sort=difficulty&limit=6");
  const list = $("recommendList");
  list.innerHTML = problems.length
    ? problems.slice(0, 6).map((problem) =>
        '<li><a data-problem="' + escapeHtml(problem.id) + '">' +
        difficultyPill(problem.difficulty) + "<span>" + escapeHtml(problem.text) + "</span></a></li>"
      ).join("")
    : '<li class="empty">You have solved everything available. Nice work.</li>';

  renderCalendar($("dashCalendar"), stats.activeDates || []);
}

function statCard(value, label) {
  return '<div class="stat"><b>' + escapeHtml(value) + "</b><span>" + escapeHtml(label) + "</span></div>";
}

function renderCalendar(element, activeDates) {
  if (!element) return;
  const active = new Set(activeDates || []);
  const weeks = 18;
  const end = new Date();
  const start = new Date(end);
  start.setDate(start.getDate() - weeks * 7 + 1);
  start.setDate(start.getDate() - start.getDay()); // back to Sunday

  const todayKey = localKey(end);
  let html = "";
  const cursor = new Date(start);
  while (cursor <= end) {
    const key = localKey(cursor);
    const isActive = active.has(key);
    let cls = "cal-cell";
    if (isActive) cls += " l3";
    if (key === todayKey) cls += " today";
    html += '<span class="' + cls + '" data-tip="' + key + '"></span>';
    cursor.setDate(cursor.getDate() + 1);
  }
  element.innerHTML = html +
    '<div class="cal-legend"><span>Less</span>' +
    '<span class="cal-cell"></span><span class="cal-cell l1"></span>' +
    '<span class="cal-cell l2"></span><span class="cal-cell l3"></span>' +
    '<span class="cal-cell l4"></span><span>More</span></div>';
}

function localKey(date) {
  const y = date.getFullYear();
  const m = String(date.getMonth() + 1).padStart(2, "0");
  const d = String(date.getDate()).padStart(2, "0");
  return y + "-" + m + "-" + d;
}

// ---------------------------------------------------------------------------
// Problems list
// ---------------------------------------------------------------------------

async function fillFilters() {
  const catalog = await loadCatalog();
  const subject = $("filterSubject");
  subject.innerHTML = '<option value="">All subjects</option>' +
    catalog.subjects.map((s) =>
      '<option value="' + escapeHtml(s.id) + '">' + escapeHtml(s.id) + " (" + s.total + ")</option>"
    ).join("");
}

async function loadProblems() {
  await fillFilters();
  const params = new URLSearchParams();
  if ($("filterSubject").value) params.set("subject", $("filterSubject").value);
  if ($("filterDifficulty").value) params.set("difficulty", $("filterDifficulty").value);
  if ($("filterStatus").value) params.set("status", $("filterStatus").value);
  if ($("filterType").value) params.set("type", $("filterType").value);
  if ($("filterSort").value) params.set("sort", $("filterSort").value);
  if ($("filterBookmarked").checked) params.set("bookmarked", "1");
  const search = $("problemSearch").value.trim();
  if (search) params.set("q", search);

  let problems;
  try {
    problems = await request("/api/problems?" + params.toString());
  } catch (error) {
    $("problemCount").textContent = error.message;
    return;
  }
  state.problems = problems;
  state.problemsLoaded = problems.length;

  const body = $("problemTable").querySelector("tbody");
  body.innerHTML = problems.map((problem) =>
    "<tr>" +
      '<td class="col-status">' + (problem.solved ? '<span class="tick">✓</span>' : '<span class="tick pending">○</span>') + "</td>" +
      '<td><span class="link" data-problem="' + escapeHtml(problem.id) + '">' + escapeHtml(problem.text) + "</span>" +
        (problem.bookmarked ? ' <span style="color:var(--medium)">★</span>' : "") + "</td>" +
      '<td class="col-subject muted">' + escapeHtml(problem.subject) + "</td>" +
      '<td class="col-topic muted">' + escapeHtml(problem.topic) + "</td>" +
      '<td class="col-diff">' + difficultyPill(problem.difficulty) + "</td>" +
      '<td class="col-act muted">' + (problem.attempts > 0 ? problem.attempts + "×" : "") + "</td>" +
    "</tr>"
  ).join("") || '<tr><td colspan="6" class="muted">No problems match those filters.</td></tr>';

  const counts = { Easy: 0, Medium: 0, Hard: 0 };
  problems.forEach((p) => { if (counts[p.difficulty] !== undefined) counts[p.difficulty]++; });
  $("problemCount").textContent =
    problems.length + " problems · " + counts.Easy + " easy / " + counts.Medium +
    " medium / " + counts.Hard + " hard";

  renderSidebar();
}

for (const id of ["filterSubject", "filterDifficulty", "filterStatus", "filterType",
                  "filterSort", "filterBookmarked"]) {
  $(id).addEventListener("change", loadProblems);
}
let searchTimer = null;
$("problemSearch").addEventListener("input", () => {
  clearTimeout(searchTimer);
  searchTimer = setTimeout(loadProblems, 220);
});

// ---------------------------------------------------------------------------
// Problem detail
// ---------------------------------------------------------------------------

document.addEventListener("click", async (event) => {
  const link = event.target.closest("[data-problem]");
  if (!link) return;
  event.preventDefault();
  openProblem(link.dataset.problem);
});

async function openProblem(id) {
  try {
    const problem = await request("/api/problems/" + encodeURIComponent(id));
    state.currentProblem = problem;
    $("pTitle").textContent = problem.title || problem.id;
    $("pDifficulty").className = "pill " + String(problem.difficulty).toLowerCase();
    $("pDifficulty").textContent = problem.difficulty;
    $("pMeta").textContent = problem.subject + " · " + problem.topic + " · " + problem.type;
    $("pText").textContent = problem.text || "";
    $("pAnswer").value = problem.template || "";
    setProblemMode(problem.type);
    renderSamples(problem.samples || []);
    $("pVerdict").hidden = true;
    $("pTestResults").hidden = true;
    $("pSubmissions").hidden = true;
    setBookmarkButton(problem.bookmarked);
    show("problem");
    await loadProblemSubmissions(id);
  } catch (error) {
    setMessage("startMsg", error.message, "error");
  }
}

// Renders the visible sample cases supplied with a coding problem.
function renderSamples(samples) {
  const panel = $("pSamples");
  if (!samples || samples.length === 0) {
    panel.hidden = true;
    return;
  }
  panel.hidden = false;
  $("pSampleList").innerHTML = samples.map((sample, index) =>
    "<div class='sample'><b>Case " + (index + 1) + "</b>" +
    "<div class='sample-block'><span class='muted'>Input</span><pre>" +
    escapeHtml(sample.input) + "</pre></div>" +
    "<div class='sample-block'><span class='muted'>Expected output</span><pre>" +
    escapeHtml(sample.expected) + "</pre></div></div>"
  ).join("");
}

// Coding problems get an editor plus Run/Submit; other types just answer text.
function setProblemMode(type) {  const isCoding = type === "Coding";
  $("pRun").hidden = !isCoding;
  $("pTestCasePicker").hidden = !isCoding;
  $("pAnswer").rows = isCoding ? 16 : 4;
  $("pEditorHint").textContent = isCoding
    ? "C++17 · stdin/stdout · 2 s and 256 MB per case"
    : "";
  if (isCoding) {
    $("pTypeNote").textContent = "Code";
  } else {
    $("pTypeNote").textContent =
      type === "MCQ" ? "Answer" : "Answer";
  }
}

function setBookmarkButton(bookmarked) {
  const button = $("pBookmark");
  button.textContent = bookmarked ? "★ Bookmarked" : "☆ Bookmark";
  button.classList.toggle("active", Boolean(bookmarked));
}

$("pBookmark").addEventListener("click", async () => {
  const problem = state.currentProblem;
  if (!problem) return;
  try {
    const result = await request("/api/problems/" + encodeURIComponent(problem.id) + "/bookmark",
      { method: "POST" });
    problem.bookmarked = result.bookmarked;
    setBookmarkButton(result.bookmarked);
    await loadStats();
  } catch (error) {
    setMessage("startMsg", error.message, "error");
  }
});

$("pClear").addEventListener("click", () => {
  const template = state.currentProblem ? state.currentProblem.template || "" : "";
  $("pAnswer").value = template;
  $("pAnswer").focus();
});

// "Run" compiles and executes the code against the visible samples. It records
// nothing, so it can be used freely while working on a solution.
$("pRun").addEventListener("click", async () => {
  const problem = state.currentProblem;
  if (!problem) return;
  const code = $("pAnswer").value.trim();
  if (!code) { setMessage("startMsg", "Write some code first.", "error"); return; }

  $("pRun").disabled = true;
  $("pSubmit").disabled = true;
  try {
    renderTestResults(null, "Running…");
    const result = await request("/api/problems/" + encodeURIComponent(problem.id) + "/run",
      { method: "POST", body: JSON.stringify({ code, language: $("pLanguage").value }) });
    renderTestResults(result);
  } catch (error) {
    renderTestResults({ verdict: "Error", compileError: error.message, results: [] });
  } finally {
    $("pRun").disabled = false;
    $("pSubmit").disabled = false;
  }
});

function renderTestResults(result, pendingMessage) {
  const panel = $("pTestResults");
  const verdict = $("pVerdict");
  panel.hidden = false;
  const body = panel.querySelector("tbody");

  if (!result) {
    $("pTestSummary").textContent = pendingMessage || "";
    body.innerHTML = "";
    verdict.hidden = true;
    return;
  }

  $("pTestSummary").textContent =
    (result.verdict || "") + (result.totalCount
      ? "  ·  " + result.passedCount + "/" + result.totalCount + " passed"
      : "");
  verdict.hidden = false;
  verdict.className = "verdict " + (result.allPassed ? "ok" : "bad");
  verdict.innerHTML = "<b>" + escapeHtml(result.verdict || "") + "</b>" +
    (result.compileError ? "<pre class='problem-text'>" + escapeHtml(result.compileError) + "</pre>" : "");

  const sampleOnly = $("pSampleOnly").checked;
  const rows = result.results || [];
  body.innerHTML = rows
    .filter((item, index) => (sampleOnly ? index === 0 : true))
    .map((item) =>
      "<tr><td class='muted'>" + item.index + "</td>" +
      "<td>" + (item.passed
        ? "<span class='pill ok'>Passed</span>"
        : "<span class='pill bad'>" + escapeHtml(item.errorType || "failed") + "</span>") + "</td>" +
      "<td class='muted'>" + escapeHtml(String(item.expected).slice(0, 40)) + "</td>" +
      "<td class='muted'>" + escapeHtml(String(item.output).slice(0, 40)) + "</td>" +
      "<td class='muted'>" + (item.timeMs || 0) + "ms</td></tr>"
    ).join("") || "<tr><td colspan='5' class='muted'>No cases ran.</td></tr>";
}

$("pSampleOnly").addEventListener("change", () => {
  if (state.lastRunResult) renderTestResults(state.lastRunResult);
});

$("pSubmit").addEventListener("click", async () => {
  const problem = state.currentProblem;
  if (!problem) return;
  const answer = $("pAnswer").value.trim();
  if (!answer) {
    setMessage("startMsg", "Write an answer first.", "error");
    return;
  }

  // Coding problems judge by compiling and running, so Submit records the run.
  if (problem.type === "Coding") {
    $("pSubmit").disabled = true;
    try {
      const run = await request("/api/problems/" + encodeURIComponent(problem.id) + "/run",
        { method: "POST", body: JSON.stringify({ code: answer, language: $("pLanguage").value }) });
      state.lastRunResult = run;
      renderTestResults(run);

      const correct = Boolean(run.allPassed);
      const verdict = $("pVerdict");
      verdict.hidden = false;
      verdict.className = "verdict " + (correct ? "ok" : "bad");
      verdict.innerHTML = "<b>" + (correct ? "Accepted" : "Not accepted") + "</b>" +
        escapeHtml(run.verdict === "Accepted"
          ? "All test cases passed."
          : (run.compileError || run.verdict || ""));

      await request("/api/problems/" + encodeURIComponent(problem.id) + "/solve",
        { method: "POST", body: JSON.stringify({ answer }) });
      problem.solved = problem.solved || correct;
      problem.attempts = (problem.attempts || 0) + 1;
      await loadProblemSubmissions(problem.id);
      await loadStats();
      renderSidebar();
    } catch (error) {
      setMessage("startMsg", error.message, "error");
    } finally {
      $("pSubmit").disabled = false;
    }
    return;
  }

  $("pSubmit").disabled = true;
  try {
    const result = await request("/api/problems/" + encodeURIComponent(problem.id) + "/solve",
      { method: "POST", body: JSON.stringify({ answer }) });
    const verdict = $("pVerdict");
    verdict.hidden = false;
    verdict.className = "verdict " + (result.correct ? "ok" : "bad");
    verdict.innerHTML =
      "<b>" + (result.correct ? "Accepted" : "Wrong answer") + "</b>" +
      escapeHtml(result.feedback || "");
    if (result.stats) state.stats = result.stats;
    problem.solved = problem.solved || result.correct;
    problem.attempts = (problem.attempts || 0) + 1;
    await loadProblemSubmissions(problem.id);
    renderSidebar();
  } catch (error) {
    setMessage("startMsg", error.message, "error");
  } finally {
    $("pSubmit").disabled = false;
  }
});

async function loadProblemSubmissions(id) {
  try {
    const rows = await request("/api/problems/" + encodeURIComponent(id) + "/submissions");
    const body = $("pSubmissions").querySelector("tbody");
    $("pSubmissions").hidden = rows.length === 0;
    body.innerHTML = rows.map((row) =>
      "<tr><td>" + (row.correct ? '<span class="pill ok">Accepted</span>' : '<span class="pill bad">Wrong</span>') + "</td>" +
      "<td class='muted'>" + escapeHtml(row.submittedAt) + "</td>" +
      "<td class='muted'>" + escapeHtml(String(row.answer).slice(0, 40)) + "</td></tr>"
    ).join("");
  } catch (error) { /* submissions are optional */ }
}

// ---------------------------------------------------------------------------
// Submissions history
// ---------------------------------------------------------------------------

async function loadSubmissions() {
  try {
    const rows = await request("/api/submissions?limit=100");
    const body = $("submissionTable").querySelector("tbody");
    body.innerHTML = rows.map((row) =>
      "<tr><td>" + (row.correct ? '<span class="pill ok">Accepted</span>' : '<span class="pill bad">Wrong</span>') + "</td>" +
      '<td><span class="link" data-problem="' + escapeHtml(row.questionId) + '">' + escapeHtml(row.title || row.questionId) + "</span></td>" +
      '<td class="muted">' + escapeHtml(row.subject) + "</td>" +
      "<td>" + difficultyPill(row.difficulty) + "</td>" +
      '<td class="muted">' + escapeHtml(row.submittedAt) + "</td></tr>"
    ).join("") || '<tr><td colspan="5" class="muted">No submissions yet.</td></tr>';
  } catch (error) {
    setMessage("problemCount", error.message, "error");
  }
}

// ---------------------------------------------------------------------------
// Interview (10-question session)
// ---------------------------------------------------------------------------

async function loadCourses() {
  try {
    const courses = await request("/api/courses");
    const select = $("courseSelect");
    select.innerHTML = "";
    for (const course of courses) {
      const option = document.createElement("option");
      option.value = course.id;
      option.textContent = course.name;
      select.appendChild(option);
    }
    await loadSubjects();
  } catch (error) {
    setMessage("startMsg", error.message, "error");
  }
}

async function loadSubjects() {
  const courseId = $("courseSelect").value;
  if (!courseId) return;
  try {
    const subjects = await request("/api/courses/" + encodeURIComponent(courseId) + "/subjects");
    const select = $("subjectSelect");
    select.innerHTML = "";
    for (const subject of subjects) {
      if (!subject.questionCount) continue;
      const option = document.createElement("option");
      option.value = subject.id;
      option.textContent = subject.name + " (" + subject.questionCount + ")";
      select.appendChild(option);
    }
    setMessage("startMsg", "");
  } catch (error) {
    setMessage("startMsg", error.message, "error");
  }
}

$("courseSelect").addEventListener("change", loadSubjects);

$("beginBtn").addEventListener("click", async () => {
  const subject = $("subjectSelect").value;
  const mode = $("modeSelect").value;
  if (!subject) { setMessage("startMsg", "No subject has questions yet.", "error"); return; }
  setMessage("startMsg", "Starting…");
  try {
    const session = await request("/api/interviews",
      { method: "POST", body: JSON.stringify({ subject, mode }) });
    state.interviewId = session.id;
    state.interviewTotal = session.total;
    $("interviewTitle").textContent = mode + " interview";
    show("interview");
    renderQuestion(session);
  } catch (error) {
    setMessage("startMsg", error.message, "error");
  }
});

function renderQuestion(session) {
  if (!session.question) { finishInterview(); return; }
  $("questionMeta").textContent =
    "Question " + (session.answered + 1) + " of " + session.total +
    " · " + session.question.difficulty + " · " + session.question.topic;
  $("questionText").textContent = session.question.text || "";
  $("answerInput").value = "";
  $("answerInput").focus();
  $("progress").max = session.total;
  $("progress").value = session.answered;
}

async function answerInterview(skip) {
  if (!state.interviewId) return;
  const answer = skip ? "" : $("answerInput").value.trim();
  if (!skip && !answer) return;
  try {
    const session = await request("/api/interviews/" + state.interviewId + "/answers",
      { method: "POST", body: JSON.stringify(skip ? { skip: 1 } : { answer }) });
    if (session.finished) { await finishInterview(); return; }
    renderQuestion(session);
  } catch (error) {
    $("questionText").textContent = error.message;
  }
}

$("answerSubmit").addEventListener("click", () => answerInterview(false));
$("skipBtn").addEventListener("click", () => answerInterview(true));
$("finishEarlyBtn").addEventListener("click", finishInterview);

async function finishInterview() {
  if (!state.interviewId) { show("report"); return; }
  try {
    const payload = await request("/api/interviews/" + state.interviewId + "/finish",
      { method: "POST" });
    state.interviewId = null;
    renderReport(payload);
    show("report");
  } catch (error) {
    $("reportBox").textContent = error.message;
    show("report");
  }
}

function renderReport(payload) {
  const report = payload.report || {};
  const percent = ((report.overallScore || 0) * 100).toFixed(1);
  $("reportBox").innerHTML =
    "<div class='cards'>" +
    statCard(percent + "%", "Overall score") +
    statCard(escapeHtml(report.strongestTopic || "-"), "Strongest") +
    statCard(escapeHtml(report.weakestTopic || "-"), "Weakest") +
    statCard(payload.streak || 0, "Streak") +
    "</div><h3>Tips</h3><ul>" +
    (report.tips || []).map(t => "<li>" + escapeHtml(t) + "</li>").join("") + "</ul>";
}

// ---------------------------------------------------------------------------
// Daily challenge
// ---------------------------------------------------------------------------

async function loadDaily() {
  $("dailyResult").textContent = "";
  try {
    const challenge = await request("/api/daily-challenge");
    $("dailyMeta").textContent = challenge.date + " · worth " + challenge.bonusPoints + " bonus points";
    $("dailyQuestion").innerHTML = challenge.question
      ? "<p class='meta'>" + difficultyPill(challenge.question.difficulty) + " " +
        escapeHtml(challenge.question.topic) + "</p><p class='qtext'>" +
        escapeHtml(challenge.question.text) + "</p>"
      : "<p class='muted'>No question today.</p>";
    state.dailyQuestionId = challenge.question ? challenge.question.id : null;
  } catch (error) {
    $("dailyMeta").textContent = error.message;
  }
}

$("dailySubmit").addEventListener("click", async () => {
  const answer = $("dailyAnswer").value.trim();
  if (!answer) return;
  try {
    const challenge = await request("/api/daily-challenge");
    const session = await request("/api/interviews",
      { method: "POST", body: JSON.stringify({ subject: challenge.question.subject, mode: "Daily" }) });
    await request("/api/interviews/" + session.id + "/answers",
      { method: "POST", body: JSON.stringify({ answer }) });
    const payload = await request("/api/interviews/" + session.id + "/finish", { method: "POST" });
    $("dailyResult").textContent = "Score " + (((payload.report || {}).overallScore || 0) * 100).toFixed(1) +
      "% · streak " + payload.streak + " day(s)";
    $("dailyAnswer").value = "";
    await loadStats();
  } catch (error) {
    $("dailyResult").textContent = error.message;
  }
});

// ---------------------------------------------------------------------------
// Leaderboard
// ---------------------------------------------------------------------------

async function loadLeaderboard() {
  const params = new URLSearchParams({
    scope: $("boardScope").value,
    window: $("boardWindow").value,
    ranking: $("boardRanking").value
  });
  try {
    const rows = await request("/api/leaderboard?" + params.toString());
    const body = $("boardTable").querySelector("tbody");
    body.innerHTML = rows.map(row =>
      "<tr><td class='muted'>" + row.rank + "</td><td>" + escapeHtml(row.name) + "</td><td>" +
      row.score.toFixed(2) + "</td><td>" + row.solved + "</td><td>" + row.streak + "</td></tr>"
    ).join("") || '<tr><td colspan="5" class="muted">No scores yet.</td></tr>';
  } catch (error) {
    setMessage("problemCount", error.message, "error");
  }
}

for (const id of ["boardScope", "boardWindow", "boardRanking"]) {
  $(id).addEventListener("change", loadLeaderboard);
}

// ---------------------------------------------------------------------------
// Profile
// ---------------------------------------------------------------------------

async function loadProfile() {
  await loadStats();
  const user = await request("/api/me");
  const stats = state.stats || { solved: { total: 0 }, available: { total: 0 } };
  const badges = (user.badges || []).length ? user.badges.join(", ") : "None yet";

  $("profileCard").innerHTML =
    "<div class='cards'>" +
    statCard(user.name, "Name") +
    statCard(user.role, "Role") +
    statCard(stats.solved.total || 0, "Solved") +
    statCard(stats.attempts || 0, "Submissions") +
    statCard(stats.streak || 0, "Streak") +
    statCard(stats.longestStreak || 0, "Longest") +
    "</div><p class='muted'>" + escapeHtml(user.email) + " · " + escapeHtml(user.college) +
    " · " + escapeHtml(user.branch) + "</p><p class='muted'>Badges: " + escapeHtml(badges) + "</p>";

  const rows = stats.bySubject || [];

  // Per-subject solved counts come from the problems endpoint so the bars are
  // accurate rather than re-deriving them client-side.
  const problems = await request("/api/problems");
  const solvedBySubject = {};
  problems.forEach(p => {
    if (!p.solved) return;
    solvedBySubject[p.subject] = (solvedBySubject[p.subject] || 0) + 1;
  });

  $("subjectProgress").innerHTML = rows.map(row => {
    const done = solvedBySubject[row.subject] || 0;
    const pct = row.total > 0 ? Math.round((done / row.total) * 100) : 0;
    return '<div class="bar-row"><span>' + escapeHtml(row.subject) + "</span>" +
      '<div class="bar-track"><div class="bar-fill" style="width:' + pct + '%"></div></div>' +
      '<span class="bar-num">' + done + "/" + row.total + "</span></div>";
  }).join("") || '<p class="muted">No subjects yet.</p>';

  renderCalendar($("profileCalendar"), stats.activeDates || []);
}

// ---------------------------------------------------------------------------
// Admin
// ---------------------------------------------------------------------------

async function loadAdmin() {
  try {
    const stats = await request("/api/admin/stats");
    $("adminStats").innerHTML = [
      statCard(stats.totalUsers, "Users"),
      statCard(stats.students, "Students"),
      statCard(stats.admins, "Admins"),
      statCard(stats.totalQuestions, "Questions"),
      statCard(stats.totalAttempts, "Attempts"),
      statCard((stats.averageScore || 0).toFixed(2), "Average score"),
      statCard(stats.activeStreaks, "Active streaks")
    ].join("");

    const users = await request("/api/admin/users");
    $("usersTable").querySelector("tbody").innerHTML = users.map(u =>
      "<tr><td>" + escapeHtml(u.email) + "</td><td>" + escapeHtml(u.name) + "</td><td>" +
      escapeHtml(u.role) + "</td><td>" + (u.solved || 0) + "</td><td>" + (u.attempts || 0) + "</td></tr>"
    ).join("");

    const questions = await request("/api/admin/questions");
    $("questionStatsTable").querySelector("tbody").innerHTML = questions.map(q =>
      "<tr><td>" + escapeHtml(q.subject) + "</td><td>" + q.easy + "</td><td>" + q.medium +
      "</td><td>" + q.hard + "</td><td>" + q.total + "</td></tr>"
    ).join("");
  } catch (error) {
    setMessage("problemCount", error.message, "error");
  }
}

// ---------------------------------------------------------------------------
// Bootstrap
// ---------------------------------------------------------------------------

(async function init() {
  toggleAuthMode();
  renderTopbar();
  if (state.token) {
    try {
      state.user = await request("/api/me");
      await loadStats();
      renderTopbar();
      show("dashboard");
      return;
    } catch (error) {
      sessionStorage.removeItem(TOKEN_KEY);
      state.token = "";
    }
  }
  show("login");
})();

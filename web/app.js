"use strict";

// ---------------------------------------------------------------------------
// API client. The only place the browser talks to the server.
// ---------------------------------------------------------------------------

const TOKEN_KEY = "ai_interviewer.token";
const THEME_KEY = "ai_interviewer.theme";

// Icon set. Stroke-based so a single color drives both themes.
const ICONS = {
  problems: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/><polyline points="14 2 14 8 20 8"/><line x1="9" y1="13" x2="15" y2="13"/><line x1="9" y1="17" x2="15" y2="17"/></svg>',
  study: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M4 19.5A2.5 2.5 0 0 1 6.5 17H20"/><path d="M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2z"/></svg>',
  interview: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 15a2 2 0 0 1-2 2H7l-4 4V5a2 2 0 0 1 2-2h14a2 2 0 0 1 2 2z"/></svg>',
  daily: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="4" width="18" height="18" rx="2"/><line x1="16" y1="2" x2="16" y2="6"/><line x1="8" y1="2" x2="8" y2="6"/><line x1="3" y1="10" x2="21" y2="10"/></svg>',
  dashboard: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="3" width="7" height="9"/><rect x="14" y="3" width="7" height="5"/><rect x="14" y="12" width="7" height="9"/><rect x="3" y="16" width="7" height="5"/></svg>',
  profile: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2"/><circle cx="12" cy="7" r="4"/></svg>',
  submissions: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="9 11 12 14 22 4"/><path d="M21 12v7a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11"/></svg>',
  board: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="8" r="6"/><path d="M15.5 13 17 22l-5-3-5 3 1.5-9"/></svg>',
  admin: '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="3"/><path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 1 1-2.83 2.83l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 1 1-4 0v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 1 1-2.83-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 1 1 0-4h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 1 1 2.83-2.83l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 1 1 4 0v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 1 1 2.83 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 1 1 0 4h-.09a1.65 1.65 0 0 0-1.51 1z"/></svg>'
};

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
  stats: null,
  theme: localStorage.getItem(THEME_KEY) || "light"
};

// ---------------------------------------------------------------------------
// Theme. The palette lives entirely in CSS tokens, so switching is one
// attribute on <html> plus persistence.
// ---------------------------------------------------------------------------

function applyTheme(theme) {
  document.documentElement.setAttribute("data-theme", theme);
  localStorage.setItem(THEME_KEY, theme);
  state.theme = theme;
}

function toggleTheme() {
  applyTheme(state.theme === "light" ? "dark" : "light");
}

$("themeToggle").addEventListener("click", toggleTheme);

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
  { id: "problems",  label: "Problems",    icon: "problems" },
  { id: "start",     label: "Interview",   icon: "interview" },
  { id: "daily",     label: "Daily",       icon: "daily" },
  { group: "Progress" },
  { id: "dashboard", label: "Dashboard",   icon: "dashboard" },
  { id: "profile",   label: "Profile",     icon: "profile" },
  { id: "submissions", label: "Submissions", icon: "submissions" },
  { id: "board",     label: "Leaderboard", icon: "board" },
  { group: "Admin" },
  { id: "admin",     label: "Admin",       icon: "admin" }
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

// ---------------------------------------------------------------------------
// Command palette. Navigation plus problem search in one surface, driven
// entirely by data the API already returns.
// ---------------------------------------------------------------------------

const cmdkState = { open: false, query: "", results: [], selected: 0 };

function cmdkNavItems() {
  return NAV_ITEMS.filter((item) => item.id)
    .filter((item) => item.id !== "admin" || (state.user && state.user.role === "Admin"));
}

function cmdkRender() {
  const list = $("cmdkList");
  const query = cmdkState.query.toLowerCase();

  // Navigation entries match on label; problems match on id, topic or text.
  const navMatches = cmdkNavItems()
    .filter((item) => item.label.toLowerCase().includes(query))
    .slice(0, 5)
    .map((item) => ({
      kind: "nav",
      icon: item.icon,
      label: item.label,
      hint: "Go to",
      run: () => show(item.id)
    }));

  let problemMatches = [];
  if (query.length > 0) {
    problemMatches = state.problems
      .filter((p) => p.id.toLowerCase().includes(query)
        || (p.topic || "").toLowerCase().includes(query)
        || (p.text || "").toLowerCase().includes(query))
      .slice(0, 8)
      .map((p) => ({
        kind: "problem",
        icon: "problems",
        label: p.text,
        hint: p.difficulty,
        run: () => openProblem(p.id)
      }));
  }

  cmdkState.results = navMatches.concat(problemMatches);
  if (cmdkState.selected >= cmdkState.results.length) {
    cmdkState.selected = Math.max(0, cmdkState.results.length - 1);
  }

  if (cmdkState.results.length === 0) {
    list.innerHTML = '<div class="cmdk-empty">No matches for "' + escapeHtml(cmdkState.query) + '"</div>';
    return;
  }

  list.innerHTML = cmdkState.results.map((item, index) =>
    '<div class="cmdk-item' + (index === cmdkState.selected ? " selected" : "") +
    '" data-index="' + index + '">' +
      '<span class="ico">' + (ICONS[item.icon] || "") + '</span>' +
      '<span>' + escapeHtml(item.label) + '</span>' +
      '<span class="hint">' + escapeHtml(item.hint) + '</span>' +
    '</div>'
  ).join("");
}

function cmdkOpen() {
  cmdkState.open = true;
  cmdkState.query = "";
  cmdkState.selected = 0;
  $("cmdk").hidden = false;
  $("cmdkInput").value = "";
  $("cmdkInput").focus();
  cmdkRender();
}

function cmdkClose() {
  cmdkState.open = false;
  $("cmdk").hidden = true;
}

function cmdkCommit() {
  const item = cmdkState.results[cmdkState.selected];
  if (!item) return;
  cmdkClose();
  item.run();
}

$("cmdkTrigger").addEventListener("click", cmdkOpen);
$("cmdk").addEventListener("click", (event) => {
  if (event.target === $("cmdk")) cmdkClose();
});
$("cmdkInput").addEventListener("input", (event) => {
  cmdkState.query = event.target.value;
  cmdkState.selected = 0;
  cmdkRender();
});
$("cmdkList").addEventListener("click", (event) => {
  const item = event.target.closest("[data-index]");
  if (!item) return;
  cmdkState.selected = parseInt(item.dataset.index, 10);
  cmdkCommit();
});
$("cmdkInput").addEventListener("keydown", (event) => {
  if (event.key === "Escape") {
    cmdkClose();
  } else if (event.key === "ArrowDown") {
    event.preventDefault();
    cmdkState.selected = Math.min(cmdkState.selected + 1, cmdkState.results.length - 1);
    cmdkRender();
  } else if (event.key === "ArrowUp") {
    event.preventDefault();
    cmdkState.selected = Math.max(cmdkState.selected - 1, 0);
    cmdkRender();
  } else if (event.key === "Enter") {
    event.preventDefault();
    cmdkCommit();
  }
});

// ---------------------------------------------------------------------------
// Keyboard shortcuts. "/" jumps to problem search, Ctrl+K opens the command
// palette and Ctrl+Enter submits the current answer.
// ---------------------------------------------------------------------------

document.addEventListener("keydown", (event) => {
  const typing = /^(INPUT|TEXTAREA|SELECT)$/.test(document.activeElement.tagName);

  // Ctrl/Cmd+K toggles the palette from anywhere.
  if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === "k") {
    event.preventDefault();
    if (cmdkState.open) cmdkClose(); else cmdkOpen();
    return;
  }
  if (event.key === "Escape" && cmdkState.open) {
    cmdkClose();
    return;
  }

  // "/" focuses search unless the user is already typing somewhere.
  if (event.key === "/" && !typing && state.view === "problems") {
    event.preventDefault();
    $("problemSearch").focus();
    return;
  }

  // Ctrl/Cmd+Enter submits the problem answer from anywhere on the page.
  if (event.key === "Enter" && (event.ctrlKey || event.metaKey)
      && state.view === "problem" && state.currentProblem) {
    event.preventDefault();
    $("pSubmit").click();
    return;
  }

  // Ctrl/Cmd+Enter also submits the in-progress interview answer.
  if (event.key === "Enter" && (event.ctrlKey || event.metaKey)
      && state.view === "interview" && state.interviewId) {
    event.preventDefault();
    $("answerSubmit").click();
  }
});

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
      { id: "problems", label: "Problems", icon: "problems" },
      { id: "start", label: "Interview", icon: "interview" },
      { id: "daily", label: "Daily", icon: "daily" },
      { id: "board", label: "Leaderboard", icon: "board" }
    ];
    for (const item of items) {
      const button = document.createElement("button");
      button.className = "navbtn" + (state.view === item.id ? " active" : "");
      button.dataset.nav = item.id;
      button.innerHTML = (ICONS[item.icon] || "") + "<span>" + escapeHtml(item.label) + "</span>";
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
    button.innerHTML = "<span class='ico'>" + (ICONS[item.icon] || "") + "</span>" +
      escapeHtml(item.label) + suffix;
    sidebar.appendChild(button);
  }
  renderTopbar();
}

// ---------------------------------------------------------------------------
// Auth
// ---------------------------------------------------------------------------

// Sets the mode explicitly. `toggleAuthMode` flips it, which is why the login
// form used to open in register mode: init flipped a false default into true.
function setAuthMode(registering) {
  state.registerMode = Boolean(registering);
  $("authTitle").textContent = state.registerMode ? "Create your account" : "Sign in";
  $("authSub").textContent = state.registerMode
    ? "Track solved problems, streaks and rankings."
    : "Continue to the practice platform.";
  $("authSubmit").textContent = state.registerMode ? "Register" : "Sign in";
  $("authMode").textContent = state.registerMode ? "Have an account? Sign in" : "Need an account? Register";
  $("registerFields").hidden = !state.registerMode;
  setMessage("authMsg", "");
}

function toggleAuthMode() {
  setAuthMode(!state.registerMode);
}

$("authMode").addEventListener("click", toggleAuthMode);

$("authForm").addEventListener("submit", async (event) => {
  event.preventDefault();
  // form.elements rather than form.email, so the lookup cannot be shadowed by
  // an inherited property of HTMLFormElement.
  const formEl = event.target;
  const form = formEl.elements;
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
    formEl.reset();
    // Land back on the sign-in form so the next visit is not stuck in
    // register mode.
    setAuthMode(false);
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

// Radial progress ring. SVG so it scales cleanly; the dash offset is animated
// in CSS when the value changes.
function ring(pct, label, color) {
  const stroke = color || "var(--accent)";
  const r = 46;
  const c = 2 * Math.PI * r;
  const clamped = Math.max(0, Math.min(100, pct));
  return (
    '<div class="ring">' +
      '<svg viewBox="0 0 108 108" aria-hidden="true">' +
        '<circle class="track" cx="54" cy="54" r="' + r + '"></circle>' +
        '<circle class="bar" cx="54" cy="54" r="' + r + '" stroke="' + stroke + '"' +
          ' stroke-dasharray="' + c.toFixed(1) + '"' +
          ' stroke-dashoffset="' + (c * (1 - clamped / 100)).toFixed(1) + '"></circle>' +
      '</svg>' +
      '<div class="value">' + clamped + '%<small>' + escapeHtml(label) + '</small></div>' +
    '</div>'
  );
}

async function loadDashboard() {
  await loadStats();
  const stats = state.stats || { solved: { total: 0 }, available: { total: 0 } };

  const solved = stats.solved || {};
  const available = stats.available || {};
  const solvedTotals = [solved.easy || 0, solved.medium || 0, solved.hard || 0];
  const availTotals = [available.easy || 0, available.medium || 0, available.hard || 0];
  const pct = (a, b) => (b > 0 ? Math.round((a / b) * 100) : 0);
  const overall = pct(solved.total || 0, available.total || 0);
  const acceptance = stats.acceptancePermille
    ? (stats.acceptancePermille / 10).toFixed(1) + "%"
    : "—";

  // Bento: one wide hero card with the rings, then a row of metrics, then the
  // recommendation and activity cards.
  const bento = document.createElement("div");
  bento.className = "bento";
  bento.innerHTML =
    '<div class="card span-2">' +
      '<div class="bento-title"><h2>Progress</h2>' +
      '<span class="muted">' + (solved.total || 0) + ' of ' + (available.total || 0) + ' solved</span></div>' +
      '<div class="ring-row">' +
        ring(overall, "solved") +
        '<div class="bars" style="flex:1">' +
          '<div class="bar-row"><span>Easy</span>' +
            '<div class="bar-track"><div class="bar-fill" style="width:' + pct(solvedTotals[0], availTotals[0]) + '%"></div></div>' +
            '<span class="bar-num">' + solvedTotals[0] + '/' + availTotals[0] + '</span></div>' +
          '<div class="bar-row"><span>Medium</span>' +
            '<div class="bar-track"><div class="bar-fill" style="width:' + pct(solvedTotals[1], availTotals[1]) + '%"></div></div>' +
            '<span class="bar-num">' + solvedTotals[1] + '/' + availTotals[1] + '</span></div>' +
          '<div class="bar-row"><span>Hard</span>' +
            '<div class="bar-track"><div class="bar-fill" style="width:' + pct(solvedTotals[2], availTotals[2]) + '%"></div></div>' +
            '<span class="bar-num">' + solvedTotals[2] + '/' + availTotals[2] + '</span></div>' +
        '</div>' +
      '</div>' +
    '</div>' +
    statCard(stats.streak || 0, "Current streak", "warn") +
    statCard(acceptance, "Acceptance", "ok") +
    statCard(stats.attempts || 0, "Submissions");

  const cards = $("dashCards");
  cards.className = "bento";
  cards.replaceWith(bento);
  bento.id = "dashCards";

  $("dashStreak").textContent =
    (stats.streak || 0) + " day streak · longest " + (stats.longestStreak || 0);

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

function statCard(value, label, variant) {
  // variant adds a coloured rail to the card edge: "ok", "warn" or "bad".
  const rail = variant ? " accent-" + variant : "";
  return '<div class="stat' + rail + '"><span>' + escapeHtml(label) + '</span><b>' +
    escapeHtml(value) + '</b></div>';
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

// Difficulty groups mirror the reference platform: problems are presented as
// sections rather than a flat table, so a learner sees the suggested path.
const DIFFICULTY_SECTIONS = [
  { key: "Easy",   label: "Beginner",     blurb: "Fundamentals and warm-ups" },
  { key: "Medium", label: "Intermediate", blurb: "Core concepts applied" },
  { key: "Hard",   label: "Advanced",     blurb: "Mastery and edge cases" }
];

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
  renderProblemSections(problems);
  renderSidebar();
}

function renderProblemSections(problems) {
  const host = document.createElement("div");
  host.id = "problemSections";

  DIFFICULTY_SECTIONS.forEach((section) => {
    const group = problems.filter((p) => p.difficulty === section.key);
    if (group.length === 0) return;

    const solvedInGroup = group.filter((p) => p.solved).length;
    const block = document.createElement("div");
    block.className = "difficulty-section";
    block.innerHTML =
      '<div class="section-head">' +
        difficultyPill(section.key) +
        '<div><b>' + escapeHtml(section.label) + '</b>' +
        '<span class="muted"> · ' + escapeHtml(section.blurb) + '</span></div>' +
        '<span class="section-count">' + solvedInGroup + "/" + group.length + '</span>' +
      '</div>';

    const list = document.createElement("div");
    list.className = "challenge-list";
    group.forEach((problem) => {
      const row = document.createElement("div");
      row.className = "challenge" + (problem.solved ? " solved" : "");
      row.dataset.problem = problem.id;
      row.innerHTML =
        '<span class="challenge-state">' +
          (problem.solved ? '<span class="tick">✓</span>' : '<span class="tick pending">○</span>') +
        '</span>' +
        '<span class="challenge-title">' + escapeHtml(problem.text) + '</span>' +
        '<span class="challenge-tag">' + escapeHtml(problem.subject) + '</span>' +
        '<span class="challenge-tag muted">' + escapeHtml(problem.topic) + '</span>' +
        (problem.bookmarked ? '<span class="challenge-star">★</span>' : '') +
        (problem.attempts > 0
          ? '<span class="challenge-attempts">' + problem.attempts + '×</span>'
          : '');
      list.appendChild(row);
    });
    block.appendChild(list);
    host.appendChild(block);
  });

  if (host.children.length === 0) {
    host.innerHTML = '<p class="muted center">No problems match those filters.</p>';
  }

  const counts = { Easy: 0, Medium: 0, Hard: 0 };
  problems.forEach((p) => { if (counts[p.difficulty] !== undefined) counts[p.difficulty]++; });
  $("problemCount").textContent =
    problems.length + " problems · " + counts.Easy + " beginner / " + counts.Medium +
    " intermediate / " + counts.Hard + " advanced";

  // Replace the table with the grouped sections.
  const old = document.getElementById("problemSections");
  const table = $("problemTable");
  if (old) old.replaceWith(host); else table.replaceWith(host);
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

// Achievement tiers, matching the reference platform's thresholds.
function achievementFor(solved, total) {
  if (total <= 0) return "Newcomer";
  const pct = solved / total;
  if (pct >= 1.0) return "Master";
  if (pct >= 0.8) return "Expert";
  if (pct >= 0.5) return "Advanced";
  if (pct >= 0.25) return "Intermediate";
  if (pct > 0) return "Beginner";
  return "Newcomer";
}

async function loadLeaderboard() {
  const params = new URLSearchParams({
    scope: $("boardScope").value,
    window: $("boardWindow").value,
    ranking: $("boardRanking").value
  });
  try {
    const [rows, stats] = await Promise.all([
      request("/api/leaderboard?" + params.toString()),
      request("/api/stats/me").catch(() => null)
    ]);
    const total = stats && stats.available ? stats.available.total : 0;

    // Podium for the top three.
    const podium = $("boardPodium");
    podium.innerHTML = rows.slice(0, 3).map((row, index) => {
      const medal = ["🥇", "🥈", "🥉"][index];
      const rate = total > 0 ? ((row.solved / total) * 100).toFixed(1) : "0.0";
      return (
        '<div class="podium-card rank-' + (index + 1) + '">' +
          '<div class="podium-medal">' + medal + '</div>' +
          '<div class="podium-name">' + escapeHtml(row.name) + '</div>' +
          '<div class="podium-solved"><b>' + row.solved + '</b>/' + total + '</div>' +
          '<div class="podium-rate">' + rate + '%</div>' +
          '<div class="podium-badge">' + achievementFor(row.solved, total) + '</div>' +
        '</div>'
      );
    }).join("");

    const body = $("boardTable").querySelector("tbody");
    body.innerHTML = rows.map((row, index) => {
      const rate = total > 0 ? ((row.solved / total) * 100).toFixed(1) : "0.0";
      // A short completion strip: one tick per roughly a tenth of the bank.
      const cells = Math.min(20, Math.max(1, total > 0 ? Math.ceil(total / 6) : 1));
      let filled = total > 0 ? Math.round((row.solved / total) * cells) : 0;
      // CSS blocks, same component the profile uses, so the two views match.
      let grid = "";
      for (let i = 0; i < cells; i++) {
        grid += '<span class="block ' + (i < filled ? "on" : "") + '"></span>';
      }
      return (
        "<tr><td class='muted'>" + (index + 1) + "</td>" +
        "<td>" + escapeHtml(row.name) + '</td>' +
        '<td class="muted">' + row.solved + '/' + total + '</td>' +
        '<td class="muted">' + rate + '%</td>' +
        '<td>' + achievementFor(row.solved, total) + '</td>' +
        '<td class="progress-grid">' + grid + '</td></tr>'
      );
    }).join("") || '<tr><td colspan="6" class="muted">No scores yet.</td></tr>';

    $("boardSummary").textContent =
      total > 0 ? total + " problems available" : "";
    $("boardEmpty").textContent = rows.length === 0
      ? "Complete an interview to appear on the board."
      : "";
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
    // Ten CSS blocks rather than emoji: no encoding hazards, and they scale
    // with the type size.
    const cells = 10;
    const filled = Math.round((done / Math.max(1, row.total)) * cells);
    let grid = "";
    for (let i = 0; i < cells; i++) {
      grid += '<span class="block ' + (i < filled ? "on" : "") + '"></span>';
    }
    return '<div class="bar-row"><span>' + escapeHtml(row.subject) + '</span>' +
      '<span class="block-grid">' + grid + '</span>' +
      '<span class="bar-num">' + pct + '% · ' + done + '/' + row.total + '</span></div>';
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
  // Restore the saved theme before first paint so there is no flash.
  applyTheme(state.theme);

  // Reads as: what the browser is talking to, so the storage choice is visible.
  $("backendNote").textContent =
    "C++17 core · " + (window.location.protocol === "https:" ? "TLS" : "local HTTP") +
    " · JSON or MongoDB storage";
  // Land on the sign-in form. This used to call toggleAuthMode(), which flipped
  // the false default into register mode.
  setAuthMode(false);
  renderTopbar();
  if (state.token) {
    try {
      state.user = await request("/api/me");
      await loadStats();
      // Warm the palette's problem index so Ctrl+K can search immediately.
      state.problems = await request("/api/problems?limit=200").catch(() => []);
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

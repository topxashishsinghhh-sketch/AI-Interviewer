"use strict";

// ---------------------------------------------------------------------------
// API client. The only place the browser talks to the server; every call goes
// through request() so the auth token and error handling stay in one spot.
// ---------------------------------------------------------------------------

const TOKEN_KEY = "ai_interviewer.token";

const state = {
  token: sessionStorage.getItem(TOKEN_KEY) || "",
  user: null,
  view: "login",
  registerMode: false,
  courses: [],
  interviewId: null,
  interviewTotal: 0,
  dailyQuestionId: null
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
    const message = payload && payload.error ? payload.error : "Request failed (" + response.status + ")";
    throw new Error(message);
  }
  return payload;
}

// ---------------------------------------------------------------------------
// Presentation helpers. No domain decisions here: this file only renders what
// the API returns and forwards user input to the API.
// ---------------------------------------------------------------------------

const $ = (id) => document.getElementById(id);

function show(view) {
  state.view = view;
  document.querySelectorAll(".view").forEach((section) => {
    section.hidden = section.id !== "view-" + view;
  });
  document.querySelectorAll(".navbtn").forEach((button) => {
    const active = button.dataset.view === view;
    button.style.color = active ? "var(--accent)" : "";
  });
  if (view === "board") loadLeaderboard();
  if (view === "profile") loadProfile();
  if (view === "daily") loadDaily();
  if (view === "admin") loadAdmin();
}

function setMessage(id, message, kind) {
  const element = $(id);
  if (!element) return;
  element.textContent = message || "";
  element.className = "msg" + (kind ? " " + kind : "");
}

function escapeHtml(text) {
  return String(text === null || text === undefined ? "" : text)
    .replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;");
}

function renderNav() {
  const nav = $("nav");
  nav.innerHTML = "";
  const authenticated = Boolean(state.user);
  const views = authenticated
    ? ["dashboard", "start", "daily", "profile", "board", "admin"]
    : ["login"];
  for (const view of views) {
    if (view === "admin" && !(state.user && state.user.role === "Admin")) continue;
    const button = document.createElement("button");
    button.className = "navbtn";
    button.dataset.view = view;
    button.textContent = view === "login" ? "Sign in" : view === "board" ? "Leaderboard" : view[0].toUpperCase() + view.slice(1);
    button.addEventListener("click", () => show(view));
    nav.appendChild(button);
  }

  const signOut = document.createElement("button");
  signOut.className = "navbtn";
  signOut.textContent = "Sign out";
  signOut.addEventListener("click", signOutUser);
  if (authenticated) nav.appendChild(signOut);

  $("who").textContent = authenticated
    ? state.user.name + " (" + state.user.role + ")"
    : "";
  $("adminTile").hidden = !(state.user && state.user.role === "Admin");
}

// ---------------------------------------------------------------------------
// Auth
// ---------------------------------------------------------------------------

function toggleAuthMode() {
  state.registerMode = !state.registerMode;
  $("authTitle").textContent = state.registerMode ? "Create an account" : "Sign in";
  $("authSubmit").textContent = state.registerMode ? "Register" : "Sign in";
  $("authMode").textContent = state.registerMode ? "Have an account? Sign in" : "Need an account? Register";
  $("registerFields").hidden = !state.registerMode;
  setMessage("authMsg", "");
}

$("authMode").addEventListener("click", toggleAuthMode);

$("authForm").addEventListener("submit", async (event) => {
  event.preventDefault();
  const form = event.target;
  const body = {
    email: form.email.value.trim(),
    password: form.password.value
  };
  if (state.registerMode) {
    body.name = form.name.value.trim() || body.email;
    body.college = form.college.value.trim() || "Unspecified";
    body.branch = form.branch.value.trim() || "Unspecified";
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
    setMessage("authMsg", "Welcome, " + payload.user.name, "good");
    renderNav();
    show("dashboard");
    await loadCourses();
  } catch (error) {
    setMessage("authMsg", error.message, "error");
  }
});

async function signOutUser() {
  try { await request("/api/logout", { method: "POST" }); } catch (error) { /* ignore */ }
  state.token = "";
  state.user = null;
  state.interviewId = null;
  sessionStorage.removeItem(TOKEN_KEY);
  renderNav();
  show("login");
  setMessage("authMsg", "");
}

// ---------------------------------------------------------------------------
// Catalogue + interview setup
// ---------------------------------------------------------------------------

async function loadCourses() {
  try {
    state.courses = await request("/api/courses");
    const select = $("courseSelect");
    select.innerHTML = "";
    for (const course of state.courses) {
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
      if (!subject.questionCount) continue; // nothing to interview on yet
      const option = document.createElement("option");
      option.value = subject.id;
      option.textContent = subject.name;
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
  if (!subject) {
    setMessage("startMsg", "This course has no interviewable subjects yet.", "error");
    return;
  }
  setMessage("startMsg", "Starting…");
  try {
    const session = await request("/api/interviews", {
      method: "POST",
      body: JSON.stringify({ subject, mode })
    });
    state.interviewId = session.id;
    state.interviewTotal = session.total;
    $("interviewTitle").textContent = mode + " interview";
    show("interview");
    renderQuestion(session);
    setMessage("startMsg", "");
  } catch (error) {
    setMessage("startMsg", error.message, "error");
  }
});

function renderQuestion(session) {
  if (!session.question) {
    show("report");
    return;
  }
  $("questionMeta").textContent =
    "Question " + (session.answered + 1) + " of " + session.total +
    " · " + (session.question.difficulty || "") + " · " + (session.question.topic || "");
  $("questionText").textContent = session.question.text || "";
  $("answerInput").value = "";
  $("answerInput").focus();
  $("progress").value = session.answered;
  $("progress").max = session.total;
}

$("answerForm").addEventListener("submit", async (event) => {
  event.preventDefault();
  const answer = $("answerInput").value.trim();
  if (!answer || !state.interviewId) return;
  try {
    const session = await request("/api/interviews/" + state.interviewId + "/answers", {
      method: "POST",
      body: JSON.stringify({ answer })
    });
    if (session.finished) {
      await finishInterview();
      return;
    }
    renderQuestion(session);
  } catch (error) {
    setMessage("questionText", error.message);
  }
});

$("skipBtn").addEventListener("click", async () => {
  if (!state.interviewId) return;
  try {
    const session = await request("/api/interviews/" + state.interviewId + "/answers", {
      method: "POST",
      body: JSON.stringify({ skip: 1 })
    });
    if (session.finished) {
      await finishInterview();
      return;
    }
    renderQuestion(session);
  } catch (error) {
    setMessage("questionText", error.message);
  }
});

$("finishEarlyBtn").addEventListener("click", finishInterview);

async function finishInterview() {
  if (!state.interviewId) {
    show("report");
    return;
  }
  try {
    const payload = await request("/api/interviews/" + state.interviewId + "/finish", {
      method: "POST"
    });
    state.interviewId = null;
    renderReport(payload);
    show("report");
  } catch (error) {
    show("report");
    $("reportBox").textContent = error.message;
  }
}

function renderReport(payload) {
  const report = payload.report || {};
  const percent = ((report.overallScore || 0) * 100).toFixed(1);
  const parts = [];
  parts.push("<div class='stats'>");
  parts.push("<div class='stat'><b>" + percent + "%</b><span>Overall score</span></div>");
  parts.push("<div class='stat'><b>" + escapeHtml(report.strongestTopic || "-") + "</b><span>Strongest</span></div>");
  parts.push("<div class='stat'><b>" + escapeHtml(report.weakestTopic || "-") + "</b><span>Weakest</span></div>");
  parts.push("<div class='stat'><b>" + (payload.streak || 0) + "</b><span>Streak (days)</span></div>");
  parts.push("</div>");

  const tips = (report.tips || []).map((tip) => "<li>" + escapeHtml(tip) + "</li>").join("");
  parts.push("<h3>Tips</h3><ul>" + tips + "</ul>");

  const wrong = (report.wrongAnswers || []).map(
    (item) => "<li>" + escapeHtml(item.questionId) + " — you said " + escapeHtml(item.answer) + "</li>"
  ).join("");
  if (wrong) parts.push("<h3>Review</h3><ul>" + wrong + "</ul>");

  $("reportBox").innerHTML = parts.join("");
}

$("againBtn").addEventListener("click", () => { show("start"); loadSubjects(); });
$("reportHomeBtn").addEventListener("click", () => show("dashboard"));

// ---------------------------------------------------------------------------
// Daily challenge
// ---------------------------------------------------------------------------

async function loadDaily() {
  $("dailyResult").textContent = "";
  try {
    const challenge = await request("/api/daily-challenge");
    state.dailyQuestionId = challenge.question ? challenge.question.id : null;
    $("dailyMeta").textContent = "For " + challenge.date + " · worth " + challenge.bonusPoints + " bonus points";
    $("dailyQuestion").innerHTML = challenge.question
      ? "<p class='meta'>" + escapeHtml(challenge.question.difficulty) + " · " + escapeHtml(challenge.question.topic) + "</p>" +
        "<p class='qtext'>" + escapeHtml(challenge.question.text) + "</p>"
      : "<p>No question today.</p>";
  } catch (error) {
    $("dailyMeta").textContent = error.message;
  }
}

$("dailyForm").addEventListener("submit", async (event) => {
  event.preventDefault();
  const answer = $("dailyAnswer").value.trim();
  if (!answer) return;
  try {
    const session = await request("/api/interviews", {
      method: "POST",
      body: JSON.stringify({ subject: (state.dailyQuestionId || ""), mode: "Daily" })
    });
    const answered = await request("/api/interviews/" + session.id + "/answers", {
      method: "POST",
      body: JSON.stringify({ answer })
    });
    const payload = await request("/api/interviews/" + session.id + "/finish", { method: "POST" });
    $("dailyResult").textContent =
      "Score " + (((payload.report || {}).overallScore || 0) * 100).toFixed(1) + "% · streak " + payload.streak + " day(s)";
    $("dailyAnswer").value = "";
  } catch (error) {
    $("dailyResult").textContent = error.message;
  }
});

// ---------------------------------------------------------------------------
// Profile
// ---------------------------------------------------------------------------

async function loadProfile() {
  try {
    const user = await request("/api/me");
    state.user = user;
    renderNav();
    const badges = (user.badges || []).length
      ? (user.badges || []).join(", ")
      : "None yet";
    $("profileBox").innerHTML =
      "<div class='stats'>" +
      "<div class='stat'><b>" + escapeHtml(user.name) + "</b><span>" + escapeHtml(user.email) + "</span></div>" +
      "<div class='stat'><b>" + (user.streak || 0) + "</b><span>Current streak</span></div>" +
      "<div class='stat'><b>" + (user.longestStreak || 0) + "</b><span>Longest streak</span></div>" +
      "<div class='stat'><b>" + (user.totalSolved || 0) + "</b><span>Solved</span></div>" +
      "<div class='stat'><b>" + (user.totalAttempts || 0) + "</b><span>Attempts</span></div>" +
      "<div class='stat'><b>" + escapeHtml(user.college || "-") + "</b><span>College</span></div>" +
      "</div><p class='meta'>Badges: " + escapeHtml(badges) + "</p>";
  } catch (error) {
    $("profileBox").textContent = error.message;
  }
}

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
    body.innerHTML = rows.map((entry) =>
      "<tr><td>" + entry.rank + "</td><td>" + escapeHtml(entry.name) + "</td><td>" +
      entry.score.toFixed(2) + "</td><td>" + entry.solved + "</td><td>" + entry.streak + "</td></tr>"
    ).join("");
    if (!rows.length) body.innerHTML = "<tr><td colspan='5'>No scores yet.</td></tr>";
  } catch (error) {
    const body = $("boardTable").querySelector("tbody");
    body.innerHTML = "<tr><td colspan='5'>" + escapeHtml(error.message) + "</td></tr>";
  }
}

$("boardRefresh").addEventListener("click", loadLeaderboard);
$("boardScope").addEventListener("change", loadLeaderboard);
$("boardWindow").addEventListener("change", loadLeaderboard);
$("boardRanking").addEventListener("change", loadLeaderboard);

// ---------------------------------------------------------------------------
// Admin
// ---------------------------------------------------------------------------

async function loadAdmin() {
  try {
    const stats = await request("/api/admin/stats");
    $("adminStats").innerHTML =
      "<div class='stats'>" +
      "<div class='stat'><b>" + stats.totalUsers + "</b><span>Users</span></div>" +
      "<div class='stat'><b>" + stats.students + "</b><span>Students</span></div>" +
      "<div class='stat'><b>" + stats.admins + "</b><span>Admins</span></div>" +
      "<div class='stat'><b>" + stats.totalQuestions + "</b><span>Questions</span></div>" +
      "<div class='stat'><b>" + stats.totalAttempts + "</b><span>Attempts</span></div>" +
      "<div class='stat'><b>" + stats.averageScore.toFixed(2) + "</b><span>Average score</span></div>" +
      "<div class='stat'><b>" + stats.activeStreaks + "</b><span>Active streaks</span></div>" +
      "</div>";

    const users = await request("/api/admin/users");
    $("usersTable").querySelector("tbody").innerHTML = users.map((user) =>
      "<tr><td>" + escapeHtml(user.email) + "</td><td>" + escapeHtml(user.name) + "</td><td>" +
      escapeHtml(user.role) + "</td><td>" + (user.solved || 0) + "</td><td>" + (user.attempts || 0) + "</td></tr>"
    ).join("");

    const questions = await request("/api/admin/questions");
    $("questionStatsTable").querySelector("tbody").innerHTML = questions.map((row) =>
      "<tr><td>" + escapeHtml(row.subject) + "</td><td>" + row.easy + "</td><td>" + row.medium +
      "</td><td>" + row.hard + "</td><td>" + row.total + "</td></tr>"
    ).join("");
  } catch (error) {
    $("adminStats").textContent = error.message;
  }
}

// ---------------------------------------------------------------------------
// Bootstrap
// ---------------------------------------------------------------------------

document.querySelectorAll("[data-view]").forEach((button) => {
  button.addEventListener("click", () => show(button.dataset.view));
});

(async function init() {
  $("backendNote").textContent = "Backend: " + (window.location.protocol === "https:" ? "https" : "http");
  renderNav();
  if (state.token) {
    try {
      state.user = await request("/api/me");
      renderNav();
      show("dashboard");
      await loadCourses();
      return;
    } catch (error) {
      sessionStorage.removeItem(TOKEN_KEY);
      state.token = "";
    }
  }
  show("login");
})();

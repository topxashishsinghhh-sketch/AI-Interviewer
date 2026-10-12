'use strict';

/* ==========================================================================
   AI Interviewer — front-end logic.

   Shape of this file, kept deliberately boring because the previous version
   accreted state and grew features that then broke each other:

     request()    the only place that talks to the server
     $ helpers    tiny DOM helpers
     api.*        one function per endpoint, returns parsed JSON
     render.*     one function per screen, returns an HTML string
     navigate()   the single place that swaps screens
     state        one object; nothing else is mutable global state

   Nothing here knows about the shell's element ids except the ones in
   index.html.
   ========================================================================== */

var state = {
  token: sessionStorage.getItem('ai_interviewer.token') || '',
  user: null,
  view: 'login',
  registerMode: false,
  problems: [],
  problemsLoaded: 0,
  currentProblem: null,
  stats: null,
  interview: null,
  cmdk: { open: false, query: '', items: [], selected: 0 }
};

/* --------------------------------------------------------------------------
   DOM helpers
   -------------------------------------------------------------------------- */

function $(id) { return document.getElementById(id); }

function on(id, event, handler) {
  var el = $(id);
  if (el) el.addEventListener(event, handler);
}

function setHidden(id, hidden) {
  var el = $(id);
  if (el) el.hidden = hidden;
}

function setText(id, text) {
  var el = $(id);
  if (el) el.textContent = text;
}

function setHtml(id, html) {
  var el = $(id);
  if (el) el.innerHTML = html;
}

function esc(text) {
  return String(text === null || text === undefined ? '' : text)
    .replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;');
}

/* --------------------------------------------------------------------------
   Server access. The only function that calls fetch().
   -------------------------------------------------------------------------- */

function request(path, options) {
  options = options || {};
  var headers = {};
  if (options.body) headers['Content-Type'] = 'application/json';
  if (state.token) headers['Authorization'] = 'Bearer ' + state.token;

  return fetch(path, {
    method: options.method || 'GET',
    headers: headers,
    body: options.body
  }).then(function (response) {
    return response.text().then(function (text) {
      var data = null;
      if (text) {
        try { data = JSON.parse(text); } catch (e) { data = null; }
      }
      if (!response.ok) {
        var message = (data && data.error) || ('Request failed (' + response.status + ')');
        var error = new Error(message);
        error.status = response.status;
        throw error;
      }
      return data;
    });
  });
}

/* --------------------------------------------------------------------------
   API. One function per endpoint.
   -------------------------------------------------------------------------- */

var api = {
  login: function (email, password) {
    return request('/api/login', {
      method: 'POST',
      body: JSON.stringify({ email: email, password: password })
    });
  },
  register: function (email, password, name, college, branch) {
    return request('/api/register', {
      method: 'POST',
      body: JSON.stringify({ email: email, password: password, name: name, college: college, branch: branch })
    });
  },
  logout: function () {
    return request('/api/logout', { method: 'POST' });
  },
  me: function () {
    return request('/api/me');
  },
  stats: function () {
    return request('/api/stats/me');
  },
  problems: function (params) {
    var query = new URLSearchParams(params || {}).toString();
    return request('/api/problems' + (query ? '?' + query : ''));
  },
  problem: function (id) {
    return request('/api/problems/' + encodeURIComponent(id));
  },
  solve: function (id, answer) {
    return request('/api/problems/' + encodeURIComponent(id) + '/solve', {
      method: 'POST',
      body: JSON.stringify({ answer: answer })
    });
  },
  runCode: function (id, code, language) {
    return request('/api/problems/' + encodeURIComponent(id) + '/run', {
      method: 'POST',
      body: JSON.stringify({ code: code, language: language })
    });
  },
  bookmark: function (id) {
    return request('/api/problems/' + encodeURIComponent(id) + '/bookmark', { method: 'POST' });
  },
  submissions: function (limit) {
    return request('/api/submissions?limit=' + (limit || 100));
  },
  board: function (scope, window_, ranking) {
    return request('/api/leaderboard?scope=' + scope + '&window=' + window_ + '&ranking=' + ranking);
  },
  startInterview: function (subject, mode) {
    return request('/api/interviews', {
      method: 'POST',
      body: JSON.stringify({ subject: subject, mode: mode })
    });
  },
  answer: function (sessionId, payload) {
    return request('/api/interviews/' + sessionId + '/answers', {
      method: 'POST',
      body: JSON.stringify(payload)
    });
  },
  finishInterview: function (sessionId) {
    return request('/api/interviews/' + sessionId + '/finish', { method: 'POST' });
  },
  courses: function () {
    return request('/api/courses');
  },
  subjects: function (courseId) {
    return request('/api/courses/' + encodeURIComponent(courseId) + '/subjects');
  },
  daily: function () {
    return request('/api/daily-challenge');
  },
  skills: function () {
    return request('/api/skills');
  },
  practice: function (tag) {
    return request('/api/skills/practice?tag=' + encodeURIComponent(tag));
  },
  adminStats: function () {
    return request('/api/admin/stats');
  },
  adminUsers: function () {
    return request('/api/admin/users');
  },
  adminQuestions: function () {
    return request('/api/admin/questions');
  }
};

/* --------------------------------------------------------------------------
   Small render helpers shared by several screens.
   -------------------------------------------------------------------------- */

function difficultyPill(difficulty) {
  var key = String(difficulty || '').toLowerCase();
  var cls = (key === 'easy' || key === 'medium' || key === 'hard') ? key : 'hard';
  return '<span class="pill ' + cls + '">' + esc(difficulty) + '</span>';
}

function resultPill(ok) {
  return ok
    ? '<span class="pill ok">Accepted</span>'
    : '<span class="pill bad">Wrong</span>';
}

function blocks(filled, total) {
  var out = '';
  for (var i = 0; i < total; i++) {
    out += '<span class="block' + (i < filled ? ' on' : '') + '"></span>';
  }
  return '<span class="block-grid">' + out + '</span>';
}

function statCard(value, label, variant) {
  return '<div class="stat' + (variant ? ' ' + variant : '') + '">' +
    '<span class="label">' + esc(label) + '</span>' +
    '<span class="value">' + esc(value) + '</span></div>';
}

// Wraps the server-supplied keywords in <mark> so the things the grader looks
// for are visible while reading. Keywords longer than 3 chars only, so short
// words do not turn the whole prompt yellow.
function highlight(text, keywords) {
  if (!text || !keywords || !keywords.length) return esc(text);
  var order = keywords.slice().sort(function (a, b) { return b.length - a.length; });
  var pattern = order.map(function (k) {
    return k.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
  }).join('|');
  if (!pattern) return esc(text);

  var re = new RegExp('\\b(' + pattern + ')\\b', 'gi');
  var out = '';
  var last = 0;
  var m;
  while ((m = re.exec(text)) !== null) {
    out += esc(text.slice(last, m.index));
    out += '<mark>' + esc(m[0]) + '</mark>';
    last = m.index + m[0].length;
    if (m.index === re.lastIndex) re.lastIndex++; // avoid zero-length loops
  }
  out += esc(text.slice(last));
  return out;
}

// The "what you are being asked to do" banner shown above the prompt.
function objectiveBanner(p) {
  if (!p.objective) return '';
  return '<div class="objective"><span class="objective-label">Objective</span>' +
         '<span class="objective-text">' + esc(p.objective) + '</span></div>';
}

// Skill chips rendered under the objective.
function skillChips(p) {
  if (!p.skillTags || !p.skillTags.length) return '';
  var chips = p.skillTags.map(function (t) {
    return '<span class="chip' + (p.skillKind === 'Soft' ? ' chip-soft' : '') + '">' +
           esc(t) + '</span>';
  });
  return '<div class="chips">' + chips.join('') + '</div>';
}

/* --------------------------------------------------------------------------
   Screens. Each returns an HTML string for #view-<name> plus optional
   header content. Rendering never mutates state.
   -------------------------------------------------------------------------- */

var views = {};

views.problems = function (html) {
  var parts = [];
  parts.push('<div class="filters">');
  parts.push('<input type="text" id="problem-search" placeholder="Search 310+ problems by keyword…" autocomplete="off">');
  parts.push('<select id="filter-subject"><option value="">All subjects (310)</option>' +
             '<option value="algo">Algorithms (34)</option>' +
             '<option value="ds">Data Structures (27)</option>' +
             '<option value="oop">C++ & OOP (29)</option>' +
             '<option value="os">Operating Systems (27)</option>' +
             '<option value="dbms">DBMS & SQL (27)</option>' +
             '<option value="networks">Computer Networks (27)</option>' +
             '<option value="hr">HR & Behavioral (30)</option>' +
             '<option value="aptitude">Quantitative Aptitude (21)</option>' +
             '<option value="reasoning">Logical Reasoning (22)</option></select>');
  parts.push('<select id="filter-difficulty"><option value="">All difficulties</option>' +
             '<option>Easy</option><option>Medium</option><option>Hard</option></select>');
  parts.push('<select id="filter-status"><option value="">All statuses</option>' +
             '<option value="solved">Solved</option><option value="attempted">Attempted</option>' +
             '<option value="unsolved">Unsolved</option></select>');
  parts.push('<select id="filter-type"><option value="">All types</option>' +
             '<option value="MCQ">MCQ</option><option value="Text">Text</option>' +
             '<option value="Coding">Coding</option></select>');
  parts.push('</div>');
  parts.push('<div id="problem-list"></div>');
  return parts.join('');
};

views.problem = function () {
  var p = state.currentProblem || {};
  var isCoding = p.type === 'Coding';
  var isSoft = p.softSkill || p.skillKind === 'Soft';
  var parts = [];

  parts.push('<a href="#" class="back-link" data-nav="problems">&larr; All problems</a>');

  parts.push('<div class="workspace-split">');

  // Left Pane: Description, Objective, STAR / Rubric, Sample Cases
  parts.push('<div class="workspace-left">');
  parts.push('<div class="card">');
  parts.push('<div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:8px">');
  parts.push('<h2 style="font-size:18px;margin:0">' + esc(p.title || p.id) + '</h2>');
  parts.push(difficultyPill(p.difficulty));
  parts.push('</div>');
  parts.push('<p class="muted" style="font-size:12px;margin:0 0 12px 0">' + esc(p.subject) + ' &middot; ' + esc(p.topic) + ' &middot; ' + esc(p.type) + '</p>');
  parts.push(objectiveBanner(p));
  parts.push('<pre class="problem-text" style="white-space:pre-wrap;font-size:13.5px;line-height:1.6">' + highlight(p.text, p.keywords) + '</pre>');
  parts.push(skillChips(p));
  parts.push('</div>');

  if (isSoft) {
    parts.push('<div class="rubric-card">');
    parts.push('<div class="rubric-header"><span>&#10024;</span> Professional Evaluation Rubric (STAR Method)</div>');
    parts.push('<div class="rubric-list">');
    parts.push('<div><b>S - Situation:</b> Set clear context and background.</div>');
    parts.push('<div><b>T - Task:</b> Explain your specific responsibility and core challenge.</div>');
    parts.push('<div><b>A - Action:</b> Detail technical trade-offs, interpersonal initiatives, and proactive execution.</div>');
    parts.push('<div><b>R - Result:</b> Quantify positive impact, learnings, and engineering takeaways.</div>');
    parts.push('</div></div>');
  }

  if (p.samples && p.samples.length) {
    parts.push('<div class="panel" style="margin-top:0"><h3>Sample Test Cases</h3>');
    for (var i = 0; i < p.samples.length; i++) {
      parts.push('<div class="sample"><b>Example ' + (i + 1) + '</b>' +
        '<div class="sample-block"><span class="muted">Input</span><pre>' + esc(p.samples[i].input) + '</pre></div>' +
        '<div class="sample-block"><span class="muted">Expected Output</span><pre>' + esc(p.samples[i].expected) + '</pre></div>' +
        '</div>');
    }
    parts.push('</div>');
  }
  parts.push('</div>'); // end workspace-left

  // Right Pane: Modern IDE / Response Area, Action Bar, Results
  parts.push('<div class="workspace-right">');
  parts.push('<div class="ide-card">');
  parts.push('<div class="ide-header">');
  parts.push('<div class="ide-tabs">');
  parts.push('<button class="ide-tab active" type="button">' + (isCoding ? 'Solution.cpp' : 'Response Form') + '</button>');
  parts.push('</div>');
  if (isCoding) {
    parts.push('<div style="display:flex;align-items:center;gap:8px">');
    parts.push('<span class="muted" style="font-size:11.5px">Language:</span>');
    parts.push('<select id="language" style="width:auto;padding:3px 24px 3px 8px;font-size:12px">' +
               '<option value="cpp">C++17</option><option value="c">C11</option></select>');
    parts.push('</div>');
  }
  parts.push('</div>'); // end ide-header

  parts.push('<div class="code-editor-wrapper">');
  parts.push('<div class="editor-line-numbers" id="line-numbers">1<br>2<br>3<br>4<br>5<br>6<br>7<br>8<br>9<br>10<br>11<br>12</div>');
  parts.push('<textarea id="answer" class="code-textarea" spellcheck="false" placeholder="' +
             (isCoding ? '// Write your solution here...' : 'Compose your structured answer according to the STAR framework...') +
             '">' + esc(p.template || '') + '</textarea>');
  parts.push('</div>'); // end code-editor-wrapper

  parts.push('<div class="ide-footer">');
  parts.push('<div style="display:flex;gap:8px">');
  parts.push('<button class="btn btn-quiet" id="clear">Reset</button>');
  parts.push('<button class="btn btn-quiet" id="bookmark">' + (p.bookmarked ? '★ Bookmarked' : '☆ Bookmark') + '</button>');
  parts.push('</div>');
  parts.push('<div style="display:flex;gap:8px">');
  if (isCoding) {
    parts.push('<button class="btn" id="run">Run Code</button>');
  }
  parts.push('<button class="btn btn-primary" id="submit">Submit Solution</button>');
  parts.push('</div>');
  parts.push('</div>'); // end ide-footer
  parts.push('</div>'); // end ide-card

  parts.push('<div id="verdict" class="verdict" hidden></div>');
  parts.push('<div id="test-results" class="panel" style="margin-top:0" hidden><h3>Test Suite Execution</h3><table>' +
             '<thead><tr><th>#</th><th>Status</th><th>Expected</th><th>Actual Output</th></tr></thead>' +
             '<tbody></tbody></table></div>');

  parts.push('</div>'); // end workspace-right
  parts.push('</div>'); // end workspace-split

  return parts.join('');
};

views.dashboard = function () {
  var s = state.stats || {};
  var solved = s.solved || { total: 0 };
  var available = s.available || { total: 0 };
  var attempts = s.attempts || 0;
  var total = solved.total || 0;
  var totalAvailable = available.total || 0;
  var percent = totalAvailable > 0 ? Math.round((total / totalAvailable) * 100) : 0;
  var acceptance = s.acceptancePermille ? (s.acceptancePermille / 10).toFixed(1) + '%' : '\u2014';

  var parts = [];
  parts.push('<div class="stat-grid">');
  parts.push(statCard(total + ' / ' + totalAvailable, 'Problems solved', 'ok'));
  parts.push(statCard(percent + '%', 'Completion'));
  parts.push(statCard(attempts, 'Submissions'));
  parts.push(statCard(acceptance, 'Acceptance'));
  parts.push(statCard(s.streak || 0, 'Current streak', 'warn'));
  parts.push(statCard(s.longestStreak || 0, 'Longest streak'));
  parts.push('</div>');

  parts.push('<div class="panel"><h3>Progress by subject</h3><div class="bars">');
  var rows = s.bySubject || [];
  for (var i = 0; i < rows.length; i++) {
    var subject = rows[i].subject;
    var subjectTotal = rows[i].total || 0;
    var done = (s.solvedBySubject && s.solvedBySubject[subject]) || 0;
    var filled = subjectTotal > 0 ? Math.round((done / subjectTotal) * 10) : 0;
    parts.push('<div class="bar-row"><span>' + esc(subject) + '</span>' +
               blocks(filled, 10) +
               '<span class="bar-num">' + done + '/' + subjectTotal + '</span></div>');
  }
  parts.push('</div></div>');
  return parts.join('');
};

views.profile = function (user) {
  var s = state.stats || {};
  var parts = [];
  parts.push('<div class="stat-grid">');
  parts.push(statCard(user.name, 'Name'));
  parts.push(statCard(user.role, 'Role'));
  parts.push(statCard(s.streak || 0, 'Current streak', 'warn'));
  parts.push(statCard(s.longestStreak || 0, 'Longest streak'));
  parts.push('</div>');
  parts.push('<div class="panel"><h3>Account</h3>' +
             '<p class="muted">Email: ' + esc(user.email) + '</p>' +
             '<p class="muted">College: ' + esc(user.college) + '</p>' +
             '<p class="muted">Branch: ' + esc(user.branch) + '</p>' +
             '<p class="muted">Badges: ' + esc((user.badges || []).join(', ') || 'None yet') + '</p></div>');
  return parts.join('');
};

views.submissions = function (rows) {
  var parts = [];
  parts.push('<div class="table-wrap"><table><thead><tr>' +
             '<th>Result</th><th>Problem</th><th>Subject</th><th>Difficulty</th><th>When</th>' +
             '</tr></thead><tbody>');
  for (var i = 0; i < rows.length; i++) {
    var r = rows[i];
    parts.push('<tr><td>' + resultPill(r.correct) + '</td>' +
               '<td>' + esc(r.title || r.questionId) + '</td>' +
               '<td>' + esc(r.subject) + '</td>' +
               '<td>' + difficultyPill(r.difficulty) + '</td>' +
               '<td class="muted">' + esc(r.submittedAt) + '</td></tr>');
  }
  parts.push('</tbody></table></div>');
  return parts.join('');
};

views.board = function (rows) {
  var s = state.stats || {};
  var total = (s.available && s.available.total) || 0;
  var parts = [];
  parts.push('<div class="podium">');
  var medals = ['#1', '#2', '#3'];
  for (var i = 0; i < rows.length && i < 3; i++) {
    var r = rows[i];
    parts.push('<div class="podium-card' + (i === 0 ? ' rank-1' : '') + '">' +
               '<div class="meta">' + medals[i] + '</div>' +
               '<div class="value">' + (r.solved || 0) + '/' + total + '</div>' +
               '<div class="muted">' + esc(r.name) + '</div></div>');
  }
  parts.push('</div>');

  parts.push('<div class="table-wrap"><table><thead><tr>' +
             '<th>#</th><th>Developer</th><th>Solved</th><th>Rate</th><th>Achievement</th><th>Progress</th>' +
             '</tr></thead><tbody>');
  for (var j = 0; j < rows.length; j++) {
    var row = rows[j];
    var rate = total > 0 ? ((row.solved / total) * 100).toFixed(1) + '%' : '0%';
    var achievement = row.solved >= total && total > 0 ? 'Master' : 'Practitioner';
    parts.push('<tr><td class="muted">' + (j + 1) + '</td><td>' + esc(row.name) + '</td>' +
               '<td>' + (row.solved || 0) + '/' + total + '</td>' +
               '<td>' + rate + '</td><td>' + achievement + '</td>' +
               '<td>' + blocks(total > 0 ? Math.round((row.solved / total) * 10) : 0, 10) + '</td></tr>');
  }
  parts.push('</tbody></table></div>');
  return parts.join('');
};

// Skill development screen: hard skills and soft skills tracked separately,
// plus the weakest tags to practise next.
views.skills = function (profile) {
  var parts = [];

  parts.push('<div class="stat-grid">');
  parts.push(statCard(Math.round((profile.hardMastery || 0) * 100) + '%', 'Technical mastery', 'ok'));
  parts.push(statCard(Math.round((profile.softMastery || 0) * 100) + '%', 'Behavioural mastery', 'warn'));
  parts.push(statCard(profile.hard.length, 'Hard skills'));
  parts.push(statCard(profile.soft.length, 'Soft skills'));
  parts.push('</div>');

  var renderSection = function (title, rows, extraClass) {
    if (!rows.length) return '';
    var html = '<div class="panel"><h3>' + esc(title) + '</h3><div class="bars">';
    for (var i = 0; i < rows.length; i++) {
      var row = rows[i];
      var pct = Math.round(row.mastery * 100);
      html += '<div class="bar-row' + (extraClass ? ' ' + extraClass : '') + '">' +
              '<span>' + esc(row.tag) + '</span>' +
              '<div class="bar-track"><div class="bar-fill" style="width:' + pct + '%"></div></div>' +
              '<span class="bar-num">' + row.solved + '/' + row.available + '</span></div>';
    }
    html += '</div></div>';
    return html;
  };

  parts.push(renderSection('Technical skills', profile.hard, 'bar-hard'));
  parts.push(renderSection('Behavioural skills', profile.soft, 'bar-soft'));

  if (profile.gaps && profile.gaps.length) {
    parts.push('<div class="panel"><h3>Practise next</h3>');
    parts.push('<p class="muted">Weakest first, based on what you have solved.</p>');
    parts.push('<div class="gap-list">');
    for (var g = 0; g < profile.gaps.length; g++) {
      var gap = profile.gaps[g];
      parts.push('<div class="gap-item' + (gap.kind === 'Soft' ? ' gap-soft' : '') + '" ' +
                 'data-practice="' + esc(gap.tag) + '">' +
                 '<span class="gap-tag">' + esc(gap.tag) + '</span>' +
                 '<span class="chip' + (gap.kind === 'Soft' ? ' chip-soft' : '') + '">' +
                 esc(gap.kind) + '</span>' +
                 '<span class="gap-bar">' +
                   blocks(gap.available > 0 ? Math.round(gap.mastery * 10) : 0, 10) +
                 '</span></div>');
    }
    parts.push('</div></div>');
  }
  return parts.join('');
};

views.admin = function (stats, users, questions) {
  var parts = [];
  parts.push('<div class="stat-grid">');
  parts.push(statCard(stats.totalUsers, 'Users'));
  parts.push(statCard(stats.students, 'Students'));
  parts.push(statCard(stats.totalQuestions, 'Questions', 'ok'));
  parts.push(statCard(stats.totalAttempts, 'Attempts'));
  parts.push(statCard(stats.activeStreaks, 'Active streaks', 'warn'));
  parts.push('</div>');
  parts.push('<div class="panel"><h3>Users</h3><div class="table-wrap"><table><thead><tr>' +
             '<th>Email</th><th>Name</th><th>Role</th><th>Solved</th><th>Attempts</th></tr></thead><tbody>');
  for (var i = 0; i < users.length; i++) {
    parts.push('<tr><td>' + esc(users[i].email) + '</td><td>' + esc(users[i].name) + '</td>' +
               '<td>' + esc(users[i].role) + '</td><td>' + (users[i].solved || 0) + '</td>' +
               '<td>' + (users[i].attempts || 0) + '</td></tr>');
  }
  parts.push('</tbody></table></div></div>');
  parts.push('<div class="panel"><h3>Question bank</h3><div class="table-wrap"><table><thead><tr>' +
             '<th>Subject</th><th>Easy</th><th>Medium</th><th>Hard</th><th>Total</th></tr></thead><tbody>');
  for (var j = 0; j < questions.length; j++) {
    var q = questions[j];
    parts.push('<tr><td>' + esc(q.subject) + '</td><td>' + q.easy + '</td><td>' + q.medium +
               '</td><td>' + q.hard + '</td><td>' + q.total + '</td></tr>');
  }
  parts.push('</tbody></table></div></div>');
  return parts.join('');
};

/* --------------------------------------------------------------------------
   Navigation — the single place that shows a screen.
   -------------------------------------------------------------------------- */

var NAV = [
  { group: 'Practice' },
  { id: 'problems', label: 'Problems', icon: 'bi-code-slash', nav: true },
  { id: 'start', label: 'Mock Interview', icon: 'bi-mic-fill', nav: true },
  { id: 'daily', label: 'Daily Challenge', icon: 'bi-calendar-check', nav: true },
  { group: 'Analytics' },
  { id: 'dashboard', label: 'Dashboard', icon: 'bi-speedometer2', nav: true },
  { id: 'skills', label: 'Skill Matrix', icon: 'bi-mortarboard', nav: true },
  { id: 'submissions', label: 'Submissions', icon: 'bi-clock-history', nav: true },
  { id: 'board', label: 'Leaderboard', icon: 'bi-trophy', nav: true },
  { group: 'Account' },
  { id: 'profile', label: 'Profile', icon: 'bi-person', nav: true }
];

function navigate(view) {
  state.view = view;
  setHidden('landing-view', true);
  setHidden('app', false);

  var screens = document.querySelectorAll('.view');
  for (var i = 0; i < screens.length; i++) {
    if (screens[i].id === 'view-' + view) {
      screens[i].hidden = false;
    } else {
      screens[i].hidden = true;
    }
  }

  renderNav();
  renderHeader(view);

  if (view === 'problems') { loadProblems(); }
  if (view === 'dashboard') { loadDashboard(); }
  if (view === 'skills') { loadSkills(); }
  if (view === 'submissions') { loadSubmissions(); }
  if (view === 'board') { loadBoard(); }
  if (view === 'profile') { loadProfile(); }
  if (view === 'admin') { loadAdmin(); }
  if (view === 'start') { loadStart(); }
  if (view === 'daily') { loadDaily(); }
}

function renderNav() {
  var nav = $('nav');
  if (!nav) return;
  nav.innerHTML = '';
  for (var i = 0; i < NAV.length; i++) {
    var item = NAV[i];
    if (item.group) {
      var label = document.createElement('p');
      label.className = 'nav-label';
      label.textContent = item.group;
      nav.appendChild(label);
      continue;
    }
    if (item.id === 'admin' && !(state.user && state.user.role === 'Admin')) continue;
    var button = document.createElement('button');
    button.className = 'nav-item' + (state.view === item.id ? ' active' : '');
    button.innerHTML = '<i class="bi ' + (item.icon || 'bi-circle') + ' me-2"></i>' + esc(item.label);
    if (state.stats && item.id === 'problems' && state.problemsLoaded) {
      button.innerHTML += ' <span class="muted" style="font-size:11px;margin-left:auto">(' + state.problemsLoaded + ')</span>';
    }
    button.addEventListener('click', function () {
      navigate(this.dataset.view);
    });
    button.dataset.view = item.id;
    nav.appendChild(button);
  }
}

function renderHeader(view) {
  var titles = {
    problems: 'Problems',
    problem: 'Problem',
    dashboard: 'Dashboard',
    skills: 'Skill development',
    profile: 'Profile',
    submissions: 'Submissions',
    board: 'Leaderboard',
    admin: 'Administration',
    interview: 'Interview',
    report: 'Report',
    daily: 'Daily challenge',
    start: 'Start an interview'
  };
  var head = $('view-head');
  if (!head) return;
  head.innerHTML = '<div class="title">' + (titles[view] || view) + '</div>';
  if (view === 'problems') {
    head.innerHTML += '<div class="actions"><button class="btn btn-primary" id="nav-start">Start interview</button></div>';
    var b = $('nav-start');
    if (b) b.addEventListener('click', function () { navigate('start'); });
  }
}

/* --------------------------------------------------------------------------
   Login / register.
   ------------------------------------------------------------------------- */

function updateAuthUI(isLoggedIn) {
  setHidden('login-view', true);
  if (isLoggedIn && state.user) {
    setHidden('guest-actions', true);
    setHidden('user-actions', false);
    setText('who', state.user.name || 'Candidate');
    var avatar = $('user-avatar');
    if (avatar) avatar.textContent = (state.user.name || 'U').charAt(0).toUpperCase();
    var authItems = document.querySelectorAll('.auth-only');
    for (var i = 0; i < authItems.length; i++) authItems[i].hidden = false;
  } else {
    setHidden('guest-actions', false);
    setHidden('user-actions', true);
    var authItems = document.querySelectorAll('.auth-only');
    for (var j = 0; j < authItems.length; j++) authItems[j].hidden = true;
  }
}

function setLoginMode(registering) {
  state.registerMode = Boolean(registering);
  var title = $('auth-title');
  if (title) title.textContent = state.registerMode ? 'Create New Account' : 'Welcome to AI Interviewer';
  setText('login-sub', state.registerMode
    ? 'Track solved problems, streaks and rankings.'
    : 'Sign in or jump straight in with an instant guest pass.');
  setText('login-submit', state.registerMode ? 'Create account' : 'Sign in');
  setText('login-toggle', state.registerMode
    ? 'Have an account? Sign in'
    : 'Need an account? Register');
  setHidden('register-fields', !state.registerMode);
  setHidden('login-error', true);
}

function quickDemo(targetView, problemId, initialSubject) {
  var rand = Math.floor(1000 + Math.random() * 9000);
  var email = 'candidate.' + rand + '@interview.dev';
  var pass = 'DemoPass123!';
  return api.register(email, pass, 'Demo Candidate', 'Engineering Academy', 'Computer Science')
    .then(function (data) {
      state.token = data.token;
      state.user = data.user;
      sessionStorage.setItem('ai_interviewer.token', state.token);
      updateAuthUI(true);
      return loadStats().then(function () {
        return api.problems({ limit: 0 }).then(function (list) {
          state.problems = list || [];
          state.problemsLoaded = state.problems.length;
        });
      }).then(function () {
        if (problemId) {
          loadProblem(problemId);
        } else {
          navigate(targetView || 'problems');
          if ((!targetView || targetView === 'problems') && initialSubject) {
            loadProblems(initialSubject);
          }
        }
      });
    }).catch(function (err) {
      // If error occurs, fall back to opening auth modal
      setHidden('login-view', false);
    });
}

function enterPractice(targetView, problemId, initialSubject) {
  if (state.token && state.user) {
    setHidden('landing-view', true);
    setHidden('app', false);
    if (problemId) {
      loadProblem(problemId);
    } else {
      navigate(targetView || 'problems');
      if ((!targetView || targetView === 'problems') && initialSubject) {
        loadProblems(initialSubject);
      }
    }
  } else {
    quickDemo(targetView || 'problems', problemId, initialSubject);
  }
}

on('login-toggle', 'click', function () {
  setLoginMode(!state.registerMode);
});

on('btn-open-login', 'click', function () {
  setHidden('login-view', false);
});

on('modal-close-btn', 'click', function () {
  setHidden('login-view', true);
});

on('login-view', 'click', function (e) {
  if (e.target === $('login-view')) setHidden('login-view', true);
});

on('btn-modal-guest', 'click', function () {
  quickDemo('problems');
});

on('btn-instant-demo', 'click', function () {
  enterPractice('problems');
});

on('hero-start-btn', 'click', function () {
  enterPractice('problems');
});

on('hero-demo-btn', 'click', function () {
  quickDemo('problems');
});

on('mentor-cta-btn', 'click', function () {
  enterPractice('start');
});

on('view-all-challenges-btn', 'click', function () {
  enterPractice('problems');
});

on('brand-home', 'click', function (e) {
  e.preventDefault();
  setHidden('app', true);
  setHidden('landing-view', false);
  window.scrollTo({ top: 0, behavior: 'smooth' });
});

on('nav-link-home', 'click', function (e) {
  e.preventDefault();
  setHidden('app', true);
  setHidden('landing-view', false);
  window.scrollTo({ top: 0, behavior: 'smooth' });
});

function scrollToLanding(sectionId) {
  setHidden('app', true);
  setHidden('landing-view', false);
  var el = $(sectionId);
  if (el) el.scrollIntoView({ behavior: 'smooth' });
}

on('nav-link-subjects', 'click', function (e) {
  e.preventDefault();
  scrollToLanding('subjects');
});

on('nav-link-mentor', 'click', function (e) {
  e.preventDefault();
  scrollToLanding('ai-mentor');
});

on('nav-link-challenges', 'click', function (e) {
  e.preventDefault();
  scrollToLanding('challenges');
});

on('nav-link-features', 'click', function (e) {
  e.preventDefault();
  scrollToLanding('features');
});

on('login-form', 'submit', function (event) {
  event.preventDefault();
  var form = event.target.elements;
  var email = form.email.value.trim();
  var password = form.password.value;

  if (!email || !password) {
    setText('login-error', 'Email and password are required.');
    setHidden('login-error', false);
    return;
  }

  var promise;
  if (state.registerMode) {
    promise = api.register(
      email,
      password,
      form.name.value.trim() || email.split('@')[0],
      form.college.value.trim() || 'Unspecified',
      form.branch.value.trim() || 'Unspecified'
    );
  } else {
    promise = api.login(email, password);
  }

  promise.then(function (data) {
    state.token = data.token;
    state.user = data.user;
    sessionStorage.setItem('ai_interviewer.token', state.token);
    event.target.reset();
    setLoginMode(false);
    updateAuthUI(true);
    setHidden('landing-view', true);
    setHidden('app', false);
    return loadStats().then(function () {
      return api.problems({ limit: 200 }).then(function (list) {
        state.problems = list || [];
        state.problemsLoaded = state.problems.length;
      });
    }).then(function () {
      navigate('dashboard');
    });
  }).catch(function (error) {
    setText('login-error', error.message);
    setHidden('login-error', false);
  });
});

on('sign-out', 'click', function () {
  api.logout().catch(function () { /* ignore */ }).then(function () {
    state.token = '';
    state.user = null;
    state.stats = null;
    sessionStorage.removeItem('ai_interviewer.token');
    updateAuthUI(false);
    setHidden('app', true);
    setHidden('landing-view', false);
    window.scrollTo({ top: 0, behavior: 'smooth' });
  });
});

/* --------------------------------------------------------------------------
   Data loaders.
   -------------------------------------------------------------------------- */

function loadStats() {
  return api.stats().then(function (s) {
    state.stats = s;
    return s;
  }).catch(function () {
    state.stats = null;
    return null;
  });
}

function loadProblems(initialSubject) {
  setHtml('view-problems', views.problems());
  var search = $('problem-search');
  var subject = $('filter-subject');
  var difficulty = $('filter-difficulty');
  var status = $('filter-status');
  var type = $('filter-type');

  if (initialSubject && subject) {
    subject.value = initialSubject;
  }

  function params() {
    var p = {};
    if (subject && subject.value) p.subject = subject.value;
    if (difficulty && difficulty.value) p.difficulty = difficulty.value;
    if (status && status.value) p.status = status.value;
    if (type && type.value) p.type = type.value;
    if (search && search.value.trim()) p.q = search.value.trim();
    return p;
  }

  var apply = function () {
    api.problems(params()).then(function (list) {
      state.problems = list || [];
      state.problemsLoaded = state.problems.length;
      renderProblemList();
      renderNav();
    });
  };

  if (search) search.addEventListener('input', apply);
  if (subject) subject.addEventListener('change', apply);
  if (difficulty) difficulty.addEventListener('change', apply);
  if (status) status.addEventListener('change', apply);
  if (type) type.addEventListener('change', apply);
  apply();
}

function renderProblemList() {
  var host = $('problem-list');
  if (!host) return;
  if (!state.problems.length) {
    host.innerHTML = '<p class="notice">No problems match those filters.</p>';
    return;
  }

  // Group by difficulty, beginner first.
  var groups = [
    { key: 'Easy', label: 'Beginner' },
    { key: 'Medium', label: 'Intermediate' },
    { key: 'Hard', label: 'Advanced' }
  ];
  var html = '';
  for (var g = 0; g < groups.length; g++) {
    var rows = [];
    for (var i = 0; i < state.problems.length; i++) {
      if (state.problems[i].difficulty === groups[g].key) rows.push(state.problems[i]);
    }
    if (!rows.length) continue;
    var solved = 0;
    for (var j = 0; j < rows.length; j++) if (rows[j].solved) solved++;
    html += '<div class="difficulty-section">';
    html += '<div class="section-head">' + difficultyPill(groups[g].key) +
            '<b>' + groups[g].label + '</b>' +
            '<span class="count">' + solved + '/' + rows.length + '</span></div>';
    for (var k = 0; k < rows.length; k++) {
      html += '<div class="challenge" data-problem="' + esc(rows[k].id) + '" style="cursor:pointer">' +
              '<span class="state">' + (rows[k].solved ? '<span class="tick">&#10003;</span>'
                                                       : '<span class="tick pending">&#9675;</span>') + '</span>' +
              '<span class="title">' + esc(rows[k].text) + '</span>' +
              '<span class="tag">' + esc(rows[k].subject) + '</span>' +
              '<span class="tag">' + esc(rows[k].topic) + '</span></div>';
    }
    html += '</div>';
  }
  host.innerHTML = html;
}

function loadProblem(id) {
  api.problem(id).then(function (p) {
    state.currentProblem = p;
    setHtml('view-problem', views.problem());
    navigate('problem');

    var submit = $('submit');
    var clear = $('clear');
    var run = $('run');
    var bookmarkButton = $('bookmark');
    var answer = $('answer');
    var lineNumbers = $('line-numbers');

    function updateLineNumbers() {
      if (!lineNumbers || !answer) return;
      var count = (answer.value.split('\n').length || 1);
      var nums = [];
      for (var n = 1; n <= Math.max(count, 12); n++) nums.push(n);
      lineNumbers.innerHTML = nums.join('<br>');
    }
    if (answer) {
      answer.addEventListener('input', updateLineNumbers);
      answer.addEventListener('scroll', function () {
        if (lineNumbers) lineNumbers.scrollTop = answer.scrollTop;
      });
      // Professional IDE enhancement: Tab key indents by 4 spaces instead of losing focus
      answer.addEventListener('keydown', function (e) {
        if (e.key === 'Tab') {
          e.preventDefault();
          var start = this.selectionStart;
          var end = this.selectionEnd;
          this.value = this.value.substring(0, start) + '    ' + this.value.substring(end);
          this.selectionStart = this.selectionEnd = start + 4;
          updateLineNumbers();
        } else if ((e.ctrlKey || e.metaKey) && e.key === 'Enter') {
          e.preventDefault();
          if (p.type === 'Coding' && run) {
            run.click();
          } else if (submit) {
            submit.click();
          }
        }
      });
      updateLineNumbers();
    }

    if (submit) {
      submit.addEventListener('click', function () {
        var text = answer.value.trim();
        if (!text) return;
        if (p.type === 'Coding') {
          api.runCode(p.id, text, $('language').value).then(function (r) {
            renderVerdict(r);
            return api.solve(p.id, text).then(function () {
              p.solved = p.solved || r.allPassed;
              loadStats();
            });
          }).catch(function (e) {
            setText('verdict', e.message);
            setHidden('verdict', false);
          });
        } else {
          api.solve(p.id, text).then(function (r) {
            renderVerdict(r);
            p.solved = p.solved || r.correct;
            loadStats();
          }).catch(function (e) {
            setText('verdict', e.message);
            setHidden('verdict', false);
          });
        }
      });
    }
    if (run) {
      run.addEventListener('click', function () {
        var text = answer.value.trim();
        if (!text) return;
        run.disabled = true;
        api.runCode(p.id, text, $('language').value).then(function (r) {
          renderVerdict(r);
        }).catch(function (e) {
          setText('verdict', e.message);
          setHidden('verdict', false);
        }).then(function () {
          run.disabled = false;
        });
      });
    }
    if (clear) {
      clear.addEventListener('click', function () {
        answer.value = state.currentProblem.template || '';
        answer.focus();
      });
    }
    if (bookmarkButton) {
      bookmarkButton.addEventListener('click', function () {
        api.bookmark(p.id).then(function (r) {
          p.bookmarked = r.bookmarked;
          bookmarkButton.textContent = r.bookmarked ? 'Bookmarked' : 'Bookmark';
        });
      });
    }
  });
}

function renderVerdict(r) {
  var verdict = $('verdict');
  setHidden('verdict', false);
  verdict.className = 'verdict ' + (r.allPassed || r.correct ? 'ok' : 'bad');
  
  var statusBadge = (r.allPassed || r.correct) 
    ? '<span class="pill ok" style="margin-bottom:6px">✔ All Checks Passed</span>'
    : '<span class="pill bad" style="margin-bottom:6px">✖ Checks Failed</span>';
    
  verdict.innerHTML = statusBadge + '<div style="margin-top:4px"><b>' + esc(r.verdict) + '</b>' + esc(r.feedback || '') + '</div>';
  if (!r.results) return;

  var host = $('test-results');
  setHidden('test-results', false);
  var rows = '';
  for (var i = 0; i < r.results.length; i++) {
    var t = r.results[i];
    rows += '<tr><td class="muted">' + (i + 1) + '</td>' +
            '<td>' + resultPill(t.passed) + '</td>' +
            '<td><code style="font-family:var(--mono);font-size:12px;background:var(--surface-2);padding:2px 6px;border-radius:4px">' + esc(t.expected) + '</code></td>' +
            '<td><code style="font-family:var(--mono);font-size:12px;background:var(--surface-2);padding:2px 6px;border-radius:4px">' + esc(t.output) + '</code></td></tr>';
  }
  host.querySelector('tbody').innerHTML = rows;
}

function loadDashboard() {
  loadStats().then(function (s) {
    // limit=0 asks for every row: the dashboard needs the full set to compute
    // per-subject solved counts. The problem list itself stays paged.
    return api.problems({ limit: 0 }).then(function (list) {
      var perSubject = {};
      for (var i = 0; i < list.length; i++) {
        if (!list[i].solved) continue;
        perSubject[list[i].subject] = (perSubject[list[i].subject] || 0) + 1;
      }
      if (s) s.solvedBySubject = perSubject;
      setHtml('view-dashboard', views.dashboard());
    });
  });
}

function loadSkills() {
  api.skills().then(function (profile) {
    setHtml('view-skills', views.skills(profile));
    // Clicking a gap row jumps to a focused practice list.
    var items = document.querySelectorAll('[data-practice]');
    for (var i = 0; i < items.length; i++) {
      items[i].addEventListener('click', function () {
        var tag = this.dataset.practice;
        api.practice(tag).then(function (list) {
          state.problems = list || [];
          state.problemsLoaded = state.problems.length;
          renderProblemList();
          navigate('problems');
        });
      });
    }
  }).catch(function (e) {
    setHtml('view-skills', '<p class="notice">' + esc(e.message) + '</p>');
  });
}

function loadSubmissions() {
  api.submissions().then(function (rows) {
    setHtml('view-submissions', views.submissions(rows || []));
  }).catch(function (e) {
    setHtml('view-submissions', '<p class="notice">' + esc(e.message) + '</p>');
  });
}

function loadBoard() {
  api.board('global', 'alltime', 'score').then(function (rows) {
    setHtml('view-board', views.board(rows || []));
  }).catch(function (e) {
    setHtml('view-board', '<p class="notice">' + esc(e.message) + '</p>');
  });
}

function loadProfile() {
  api.me().then(function (user) {
    state.user = user;
    setHtml('view-profile', views.profile(user));
  });
}

function loadAdmin() {
  Promise.all([api.adminStats(), api.adminUsers(), api.adminQuestions()])
    .then(function (results) {
      setHtml('view-admin', views.admin(results[0], results[1] || [], results[2] || []));
    })
    .catch(function (e) {
      setHtml('view-admin', '<p class="notice">' + esc(e.message) + '</p>');
    });
}

function loadStart() {
  api.courses().then(function (courses) {
    var html = '<div class="card"><div class="field"><span>Subject</span>' +
               '<select id="start-subject">';
    for (var i = 0; i < courses.length; i++) {
      var subs = null;
      html += '<option value="' + esc(courses[i].id) + '" disabled>' + esc(courses[i].name) + '</option>';
      subs = subjectsForCourse(courses[i].id);
      for (var j = 0; j < subs.length; j++) {
        html += '<option value="' + esc(subs[j]) + '">&nbsp;&nbsp;' + esc(subs[j]) + '</option>';
      }
    }
    html += '</select></div>';
    html += '<div class="field" style="margin-top:12px"><span>Mode</span><select id="start-mode">' +
            '<option value="Practice">Practice</option>' +
            '<option value="Mock">Mock interview</option>' +
            '<option value="Daily">Daily challenge</option></select></div>';
    html += '<button class="btn btn-primary" style="margin-top:14px" id="start-go">Begin</button></div>';
    setHtml('view-start', html);

    $('start-go').addEventListener('click', function () {
      var subject = $('start-subject').value;
      var mode = $('start-mode').value;
      api.startInterview(subject, mode).then(function (session) {
        state.interview = session;
        renderInterview();
      });
    });
  });
}

function subjectsForCourse(courseId) {
  // Only used to build the picker; each already warmed in state.problems.
  var seen = {};
  var out = [];
  for (var i = 0; i < state.problems.length; i++) {
    var s = state.problems[i].subject;
    if (!seen[s]) { seen[s] = true; out.push(s); }
  }
  return out;
}

var ivTimerInterval = null;
var ivSecondsElapsed = 0;

function renderInterview() {
  var s = state.interview;
  ivSecondsElapsed = 0;
  if (ivTimerInterval) clearInterval(ivTimerInterval);

  var html = '<div class="interview-hud">' +
    '<div><div class="meta">Live Evaluation Session</div>' +
    '<h3 style="margin:2px 0 0;font-size:16px" id="iv-hud-title">Interview Simulation</h3></div>' +
    '<div class="interview-timer"><span style="color:var(--muted)">⏱</span> <span id="iv-timer-display">00:00</span></div>' +
    '</div>' +
    '<progress id="iv-progress" max="100" value="0"></progress>' +
    '<div class="workspace-split">' +
    '<div class="workspace-left">' +
    '<div class="card">' +
    '<div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:8px">' +
    '<span class="meta" id="iv-meta">Question 1</span>' +
    '<span id="iv-difficulty-pill"></span>' +
    '</div>' +
    '<p class="question-text" id="iv-text" style="font-size:15px;line-height:1.6;font-weight:550"></p>' +
    '</div>' +
    '<div class="rubric-card">' +
    '<div class="rubric-header"><span>🎯</span> Evaluation Criteria</div>' +
    '<div class="rubric-list">' +
    '<div>• Technical Accuracy & Logical Completeness</div>' +
    '<div>• Time & Space Complexity Consideration</div>' +
    '<div>• STAR Methodology Structure (Situation, Task, Action, Result)</div>' +
    '</div></div>' +
    '</div>' +
    '<div class="workspace-right">' +
    '<div class="ide-card">' +
    '<div class="ide-header">' +
    '<span style="font-size:12px;font-weight:600;color:var(--text)">Candidate Response</span>' +
    '</div>' +
    '<div class="code-editor-wrapper" style="min-height:280px">' +
    '<textarea id="iv-answer" class="code-textarea" spellcheck="false" placeholder="Formulate and type your answer clearly..."></textarea>' +
    '</div>' +
    '<div class="ide-footer">' +
    '<div style="display:flex;align-items:center;gap:8px">' +
    '<button class="btn btn-quiet" id="iv-finish">End Interview</button>' +
    '<button class="btn btn-mic" id="iv-voice-btn" type="button" title="Speak Answer (Voice AI)">🎤 <span>Voice Answer</span></button>' +
    '</div>' +
    '<div style="display:flex;gap:8px">' +
    '<button class="btn" id="iv-skip">Skip Question</button>' +
    '<button class="btn btn-primary" id="iv-submit">Submit Answer</button>' +
    '</div></div>' +
    '</div></div></div>';

  setHtml('view-interview', html);
  navigate('interview');

  // Speech Recognition (Web Speech API)
  var voiceBtn = $('iv-voice-btn');
  var isRecording = false;
  var recognition = null;
  if (window.SpeechRecognition || window.webkitSpeechRecognition) {
    var SpeechRec = window.SpeechRecognition || window.webkitSpeechRecognition;
    recognition = new SpeechRec();
    recognition.continuous = true;
    recognition.interimResults = true;
    recognition.lang = 'en-US';

    recognition.onresult = function (event) {
      var transcript = '';
      for (var i = event.resultIndex; i < event.results.length; ++i) {
        transcript += event.results[i][0].transcript;
      }
      var ans = $('iv-answer');
      if (ans && transcript) {
        ans.value = (ans.value ? ans.value + ' ' : '') + transcript;
      }
    };
    recognition.onerror = function () {
      isRecording = false;
      if (voiceBtn) {
        voiceBtn.classList.remove('recording');
        voiceBtn.querySelector('span').textContent = 'Voice Answer';
      }
    };
  } else if (voiceBtn) {
    voiceBtn.title = 'Speech Recognition not supported in this browser';
  }

  if (voiceBtn && recognition) {
    voiceBtn.addEventListener('click', function () {
      if (!isRecording) {
        try {
          recognition.start();
          isRecording = true;
          voiceBtn.classList.add('recording');
          voiceBtn.querySelector('span').textContent = 'Listening...';
        } catch (e) { isRecording = false; }
      } else {
        recognition.stop();
        isRecording = false;
        voiceBtn.classList.remove('recording');
        voiceBtn.querySelector('span').textContent = 'Voice Answer';
      }
    });
  }

  ivTimerInterval = setInterval(function () {
    ivSecondsElapsed++;
    var mins = String(Math.floor(ivSecondsElapsed / 60)).padStart(2, '0');
    var secs = String(ivSecondsElapsed % 60).padStart(2, '0');
    var disp = $('iv-timer-display');
    if (disp) disp.textContent = mins + ':' + secs;
  }, 1000);

  function paint() {
    var q = state.interview.question;
    setText('iv-meta', 'Question ' + (state.interview.answered + 1) + ' of ' + state.interview.total);
    setHtml('iv-difficulty-pill', difficultyPill(q.difficulty));
    setText('iv-text', q.text);
    setText('iv-hud-title', (q.topic || 'General') + ' · ' + (q.type || 'Question'));
    var prog = $('iv-progress');
    if (prog) {
      prog.setAttribute('value', state.interview.answered);
      prog.setAttribute('max', state.interview.total);
    }
    var ans = $('iv-answer');
    if (ans) {
      ans.value = '';
      ans.focus();
    }
  }

  $('iv-submit').addEventListener('click', function () {
    if (isRecording && recognition) { recognition.stop(); isRecording = false; }
    var text = $('iv-answer').value.trim();
    if (!text) return;
    api.answer(state.interview.id, { answer: text }).then(function (session) {
      state.interview = session;
      if (session.finished) { finishAndReport(); } else { paint(); }
    });
  });

  $('iv-skip').addEventListener('click', function () {
    if (isRecording && recognition) { recognition.stop(); isRecording = false; }
    api.answer(state.interview.id, { skip: 1 }).then(function (session) {
      state.interview = session;
      if (session.finished) { finishAndReport(); } else { paint(); }
    });
  });

  $('iv-finish').addEventListener('click', function () {
    if (isRecording && recognition) { recognition.stop(); isRecording = false; }
    finishAndReport();
  });

  function finishAndReport() {
    if (ivTimerInterval) { clearInterval(ivTimerInterval); ivTimerInterval = null; }
    if (isRecording && recognition) { recognition.stop(); isRecording = false; }
    api.finishInterview(state.interview.id).then(function (payload) {
      state.interview = null;
      var report = payload.report || {};
      var pct = Math.round((report.overallScore || 0) * 100);
      var grade = pct >= 80 ? 'Distinction' : (pct >= 60 ? 'Proficient' : 'Needs Practice');
      var mins = Math.floor(ivSecondsElapsed / 60);
      var secs = ivSecondsElapsed % 60;
      var timeStr = mins + 'm ' + secs + 's';

      var commScore = Math.min(100, Math.max(30, Math.round(pct * 0.95 + 10)));
      var logicScore = Math.min(100, Math.max(40, Math.round(pct * 1.05)));
      var edgeScore = Math.min(100, Math.max(25, Math.round(pct * 0.85)));
      var starScore = Math.min(100, Math.max(35, Math.round(pct * 0.9 + 5)));

      var reportHtml = '<div style="margin-bottom:20px">' +
        '<h2>Candidate Performance Evaluation</h2>' +
        '<p class="muted">Comprehensive rubric evaluation based on your interview session.</p>' +
        '</div>' +
        '<div class="stat-grid">' +
        statCard(pct + '%', 'Evaluation Score', pct >= 60 ? 'ok' : 'warn') +
        statCard(grade, 'Competency Rating') +
        statCard(timeStr, 'Session Duration') +
        statCard(payload.streak || 0, 'Current Streak', 'warn') +
        '</div>' +
        '<div class="panel"><h3>AI Competency Breakdown</h3>' +
        '<div class="competency-grid">' +
        '<div class="competency-item"><div><b>Logical Structure & Problem Decomposition</b> <span class="muted" style="float:right">' + logicScore + '%</span></div><div class="competency-bar-track"><div class="competency-bar-fill" style="width:' + logicScore + '%"></div></div></div>' +
        '<div class="competency-item"><div><b>Communication & Articulation</b> <span class="muted" style="float:right">' + commScore + '%</span></div><div class="competency-bar-track"><div class="competency-bar-fill" style="width:' + commScore + '%"></div></div></div>' +
        '<div class="competency-item"><div><b>STAR Method Framing</b> <span class="muted" style="float:right">' + starScore + '%</span></div><div class="competency-bar-track"><div class="competency-bar-fill" style="width:' + starScore + '%"></div></div></div>' +
        '<div class="competency-item"><div><b>Edge Cases & Computational Complexity</b> <span class="muted" style="float:right">' + edgeScore + '%</span></div><div class="competency-bar-track"><div class="competency-bar-fill" style="width:' + edgeScore + '%"></div></div></div>' +
        '</div></div>' +
        '<div style="display:grid;grid-template-columns:1fr 1fr;gap:14px;margin-bottom:14px">' +
        '<div class="card">' +
        '<div class="meta" style="color:var(--ok)">Key Strengths & Mastery</div>' +
        '<p style="font-size:16px;font-weight:600;margin:6px 0 0">' + esc(report.strongestTopic || 'Structured Analytical Thinking') + '</p>' +
        '<p class="muted" style="font-size:12.5px;margin-top:4px">Demonstrated solid analytical reasoning and structured responses.</p>' +
        '</div>' +
        '<div class="card">' +
        '<div class="meta" style="color:var(--warn)">Areas For Growth</div>' +
        '<p style="font-size:16px;font-weight:600;margin:6px 0 0">' + esc(report.weakestTopic || 'Edge Cases & Trade-offs') + '</p>' +
        '<p class="muted" style="font-size:12.5px;margin-top:4px">Focus on framing trade-offs and edge-case boundary conditions.</p>' +
        '</div></div>' +
        '<div class="panel"><h3>Actionable Interview Coaching Tips</h3><ul style="line-height:1.7;font-size:13.5px">' +
        (report.tips && report.tips.length ? report.tips : [
          'Structure situational answers clearly using the STAR methodology (Situation, Task, Action, Result).',
          'Mention time and space complexities explicitly when describing algorithms.',
          'Review edge cases and boundary inputs before submitting.'
        ]).map(function (t) { return '<li>' + esc(t) + '</li>'; }).join('') +
        '</ul></div>' +
        '<div style="margin-top:18px"><button class="btn btn-primary" onclick="navigate(\'problems\')">Back to Problem Practice</button></div>';

      setHtml('view-report', reportHtml);
      navigate('report');
      loadStats();
    });
  }

  paint();
}

function loadDaily() {
  api.daily().then(function (challenge) {
    var q = challenge.question;
    setHtml('view-daily',
      '<p class="meta">' + esc(challenge.date) + ' · worth ' + challenge.bonusPoints + ' bonus points</p>' +
      '<div class="card" style="margin-top:12px"><p class="meta">' +
      difficultyPill(q.difficulty) + ' ' + esc(q.topic) + '</p>' +
      '<p class="question-text">' + esc(q.text) + '</p></div>' +
      '<div class="field" style="margin-top:12px"><span>Answer</span>' +
      '<input type="text" id="daily-answer"></div>' +
      '<button class="btn btn-primary" style="margin-top:10px" id="daily-submit">Submit</button>');

    $('daily-submit').addEventListener('click', function () {
      var text = $('daily-answer').value.trim();
      if (!text) return;
      api.startInterview(q.subject, 'Daily').then(function (session) {
        return api.answer(session.id, { answer: text });
      }).then(function (session) {
        return api.finishInterview(session.id);
      }).then(function (payload) {
        $('daily-answer').value = '';
        var report = payload.report || {};
        setHtml('view-daily',
          '<div class="stat-grid">' +
          statCard(((report.overallScore || 0) * 100).toFixed(1) + '%', 'Score', 'ok') +
          statCard(payload.streak || 0, 'Streak', 'warn') + '</div>');
        loadStats();
      });
    });
  });
}

/* --------------------------------------------------------------------------
   Command palette
   -------------------------------------------------------------------------- */

function cmdkItems() {
  var query = state.cmdk.query.toLowerCase();
  var items = [];

  // Navigation
  var navViews = [
    { id: 'problems', label: 'Problems' },
    { id: 'start', label: 'Start interview' },
    { id: 'daily', label: 'Daily challenge' },
    { id: 'dashboard', label: 'Dashboard' },
    { id: 'skills', label: 'Skill development' },
    { id: 'submissions', label: 'Submissions' },
    { id: 'board', label: 'Leaderboard' },
    { id: 'profile', label: 'Profile' },
    { id: 'admin', label: 'Administration' }
  ];
  for (var i = 0; i < navViews.length; i++) {
    if (navViews[i].id === 'admin' && !(state.user && state.user.role === 'Admin')) continue;
    if (navViews[i].label.toLowerCase().indexOf(query) >= 0) {
      items.push({ label: navViews[i].label, hint: 'Go to', run: makeNavRun(navViews[i].id) });
    }
  }

  // Problems
  if (query) {
    for (var j = 0; j < state.problems.length; j++) {
      var p = state.problems[j];
      var hay = (p.id + ' ' + p.topic + ' ' + p.text).toLowerCase();
      if (hay.indexOf(query) >= 0) {
        items.push({ label: p.text, hint: p.difficulty, run: makeProblemRun(p.id) });
        if (items.length > 12) break;
      }
    }
  }
  return items;
}

function makeNavRun(view) {
  return function () { navigate(view); };
}

function makeProblemRun(id) {
  return function () { loadProblem(id); };
}

function cmdkRender() {
  var items = cmdkItems();
  state.cmdk.items = items;
  if (state.cmdk.selected >= items.length) {
    state.cmdk.selected = Math.max(0, items.length - 1);
  }

  var list = $('cmdk-list');
  if (!items.length) {
    list.innerHTML = '<div class="cmdk-empty">No matches for "' + esc(state.cmdk.query) + '"</div>';
    return;
  }
  var html = '';
  for (var i = 0; i < items.length; i++) {
    html += '<div class="cmdk-item' + (i === state.cmdk.selected ? ' selected' : '') +
            '" data-index="' + i + '"><span>' + esc(items[i].label) + '</span>' +
            '<span class="hint">' + esc(items[i].hint) + '</span></div>';
  }
  list.innerHTML = html;
}

function cmdkOpen() {
  state.cmdk.open = true;
  state.cmdk.query = '';
  state.cmdk.selected = 0;
  setHidden('cmdk', false);
  $('cmdk-input').value = '';
  cmdkRender();
  $('cmdk-input').focus();
}

function cmdkClose() {
  state.cmdk.open = false;
  setHidden('cmdk', true);
}

function cmdkCommit() {
  var item = state.cmdk.items[state.cmdk.selected];
  cmdkClose();
  if (item) item.run();
}

on('cmdk-trigger', 'click', cmdkOpen);
on('cmdk', 'click', function (event) {
  if (event.target === $('cmdk')) cmdkClose();
});
on('cmdk-input', 'input', function (event) {
  state.cmdk.query = event.target.value;
  state.cmdk.selected = 0;
  cmdkRender();
});
on('cmdk-list', 'click', function (event) {
  var el = event.target.closest('[data-index]');
  if (!el) return;
  state.cmdk.selected = parseInt(el.dataset.index, 10);
  cmdkCommit();
});
on('cmdk-input', 'keydown', function (event) {
  if (event.key === 'Escape') { cmdkClose(); }
  else if (event.key === 'ArrowDown') {
    event.preventDefault();
    state.cmdk.selected = Math.min(state.cmdk.selected + 1, state.cmdk.items.length - 1);
    cmdkRender();
  } else if (event.key === 'ArrowUp') {
    event.preventDefault();
    state.cmdk.selected = Math.max(state.cmdk.selected - 1, 0);
    cmdkRender();
  } else if (event.key === 'Enter') {
    event.preventDefault();
    cmdkCommit();
  }
});

/* --------------------------------------------------------------------------
   Global key handling. One handler for the whole document.
   -------------------------------------------------------------------------- */

document.addEventListener('keydown', function (event) {
  // Ctrl/Cmd+K toggles the palette from anywhere.
  if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === 'k') {
    event.preventDefault();
    if (state.cmdk.open) { cmdkClose(); } else { cmdkOpen(); }
    return;
  }
  if (event.key === 'Escape' && state.cmdk.open) {
    cmdkClose();
    return;
  }

  // Ctrl/Cmd+Enter submits the current answer.
  if ((event.ctrlKey || event.metaKey) && event.key === 'Enter') {
    if (state.view === 'problem' && $('submit')) { $('submit').click(); }
    if (state.view === 'interview' && $('iv-submit')) { $('iv-submit').click(); }
  }
});

/* Click handling for anything that opens a problem row or preview card. */
document.addEventListener('click', function (event) {
  // Track tab filters on landing page
  var trackTab = event.target.closest('.track-tab');
  if (trackTab) {
    var tabs = document.querySelectorAll('.track-tab');
    for (var t = 0; t < tabs.length; t++) tabs[t].classList.remove('active');
    trackTab.classList.add('active');
    var track = trackTab.dataset.track;
    var cards = document.querySelectorAll('.preview-card');
    for (var c = 0; c < cards.length; c++) {
      var trackEl = cards[c].querySelector('.preview-track');
      var cardTrack = trackEl ? trackEl.textContent.trim() : '';
      if (track === 'all' || cardTrack.toLowerCase().indexOf(track.toLowerCase().slice(0, 4)) !== -1) {
        cards[c].style.display = 'flex';
      } else {
        cards[c].style.display = 'none';
      }
    }
    return;
  }

  // Explore subject button on landing page
  var subjectBtn = event.target.closest('[data-explore-subject]');
  if (subjectBtn) {
    event.preventDefault();
    var subj = subjectBtn.dataset.exploreSubject;
    enterPractice('problems', null, subj);
    return;
  }

  // Preview card on landing page
  var previewCard = event.target.closest('.preview-card');
  if (previewCard) {
    event.preventDefault();
    var probId = previewCard.dataset.problem;
    enterPractice('problem', probId);
    return;
  }

  // Standard nav link
  var nav = event.target.closest('[data-nav]');
  if (nav) {
    event.preventDefault();
    navigate(nav.dataset.nav);
    return;
  }

  // Problem row in workspace
  var row = event.target.closest('[data-problem]');
  if (row) {
    event.preventDefault();
    loadProblem(row.dataset.problem);
  }
});

/* --------------------------------------------------------------------------
   Theme. One attribute on <html>; the palette lives in CSS.
   -------------------------------------------------------------------------- */

on('theme-toggle', 'click', function () {
  var current = document.documentElement.getAttribute('data-theme');
  var next = current === 'dark' ? 'light' : 'dark';
  document.documentElement.setAttribute('data-theme', next);
  var icon = $('theme-icon');
  if (icon) {
    icon.className = next === 'dark' ? 'bi bi-moon-stars' : 'bi bi-sun';
  }
});

/* --------------------------------------------------------------------------
   Boot.
   ------------------------------------------------------------------------- */

function showBootError(message) {
  var banner = $('boot-error');
  if (!banner) return;
  setText('boot-error-detail', message);
  banner.hidden = false;
}

window.addEventListener('error', function (event) {
  showBootError(String(event.message || 'unknown error') +
    (event.filename ? '  (' + event.filename + ':' + event.lineno + ')' : ''));
});

window.addEventListener('unhandledrejection', function (event) {
  showBootError('Unhandled promise rejection: ' +
    String(event.reason && event.reason.message ? event.reason.message : event.reason));
});

(function boot() {
  try {
    setLoginMode(false);
    if (state.token) {
      request('/api/me').then(function (user) {
        state.user = user;
        updateAuthUI(true);
        setHidden('landing-view', true);
        setHidden('app', false);
        return loadStats().then(function () {
          return api.problems({ limit: 0 }).then(function (list) {
            state.problems = list || [];
            state.problemsLoaded = state.problems.length;
            navigate('dashboard');
          });
        });
      }).catch(function () {
        state.token = '';
        sessionStorage.removeItem('ai_interviewer.token');
        updateAuthUI(false);
        setHidden('landing-view', false);
        setHidden('app', true);
      });
    } else {
      updateAuthUI(false);
      setHidden('landing-view', false);
      setHidden('app', true);
    }
  } catch (error) {
    showBootError(String(error && error.stack ? error.stack : error));
  }
})();

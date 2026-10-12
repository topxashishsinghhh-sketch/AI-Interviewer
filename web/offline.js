/**
 * offline.js — Static / GitHub Pages offline adapter for AI Interviewer.
 *
 * Intercepts every fetch() call made by app.js and handles it entirely
 * in the browser using:
 *   • web/data/questions.json   — the full 310-question bank (bundled at deploy)
 *   • localStorage              — solved/bookmarked state, user profile, sessions
 *
 * No server, no login required.  The app boots as a guest automatically.
 */

(function () {
  'use strict';

  /* -------------------------------------------------------------------------
     Storage helpers
     ---------------------------------------------------------------------- */
  var LS_KEY = 'ai_interviewer_offline';

  function load() {
    try { return JSON.parse(localStorage.getItem(LS_KEY) || '{}'); }
    catch (e) { return {}; }
  }

  function save(data) {
    try { localStorage.setItem(LS_KEY, JSON.stringify(data)); }
    catch (e) { /* quota exceeded – ignore */ }
  }

  function getStore() {
    var d = load();
    if (!d.solved)       d.solved       = {};   // { questionId: true }
    if (!d.bookmarks)    d.bookmarks    = {};   // { questionId: true }
    if (!d.sessions)     d.sessions     = [];   // [{id, subject, mode, answers[], finished}]
    if (!d.streak)       d.streak       = 0;
    if (!d.lastDay)      d.lastDay      = '';
    return d;
  }

  /* -------------------------------------------------------------------------
     In-memory questions cache
     ---------------------------------------------------------------------- */
  var _questions = null;   // array, loaded once from questions.json

  function fetchQuestions() {
    if (_questions) return Promise.resolve(_questions);
    // Relative path works both on GitHub Pages and localhost /web
    return fetch('./data/questions.json')
      .then(function (r) { return r.json(); })
      .then(function (arr) {
        _questions = arr;
        return arr;
      });
  }

  /* -------------------------------------------------------------------------
     Transform a raw question record → the shape app.js expects
     ---------------------------------------------------------------------- */
  function toClientQuestion(q, store) {
    return {
      id:         q.id,
      subject:    q.subject,
      topic:      q.topic,
      difficulty: q.difficulty,
      type:       q.type,
      text:       q.text,
      solved:     !!(store.solved[q.id]),
      bookmarked: !!(store.bookmarks[q.id]),
      // Extra fields used by the problem detail view
      options:    q.options     || null,
      answer:     q.answer      !== undefined ? q.answer : null,
      keywords:   q.keywords    || [],
      explanation:q.explanation || '',
      template:   q.templateCode || q.template || '',
      testCases:  q.testCases   || []
    };
  }

  /* -------------------------------------------------------------------------
     Route table  — maps   METHOD + pathname pattern  →  handler
     ---------------------------------------------------------------------- */

  function handleLogin(body) {
    // Offline: any credentials are accepted; we just create a guest session
    var name = (body && body.email) ? body.email.split('@')[0] : 'Guest';
    var token = 'offline-' + Date.now();
    return ok({ token: token, user: guestUser(name) });
  }

  function handleRegister(body) {
    var name = (body && body.name) ? body.name : 'Candidate';
    var token = 'offline-' + Date.now();
    return ok({ token: token, user: guestUser(name) });
  }

  function guestUser(name) {
    return {
      id: 'guest',
      name: name || 'Guest',
      email: 'guest@offline.dev',
      college: 'N/A',
      branch: 'N/A',
      role: 'Candidate',
      streak: 0,
      score: 0
    };
  }

  function handleMe() {
    return ok(guestUser('Guest'));
  }

  function handleLogout() {
    return ok({ ok: true });
  }

  function handleStats() {
    var store = getStore();
    var solved = Object.keys(store.solved).length;
    return ok({
      solved: solved,
      total: _questions ? _questions.length : 310,
      streak: store.streak,
      score: solved * 10,
      rank: null,
      solvedBySubject: {}
    });
  }

  function handleProblems(searchParams) {
    return fetchQuestions().then(function (qs) {
      var store = getStore();
      var subject    = searchParams.get('subject')    || '';
      var difficulty = searchParams.get('difficulty') || '';
      var type       = searchParams.get('type')       || '';
      var status     = searchParams.get('status')     || '';
      var q          = (searchParams.get('q') || '').toLowerCase();
      var limit      = parseInt(searchParams.get('limit') || '200', 10);

      var filtered = qs.filter(function (item) {
        if (subject    && item.subject    !== subject)    return false;
        if (difficulty && item.difficulty !== difficulty) return false;
        if (type       && item.type       !== type)       return false;
        if (status === 'solved'   && !store.solved[item.id])  return false;
        if (status === 'unsolved' &&  store.solved[item.id])  return false;
        if (q) {
          var hay = (item.id + ' ' + item.topic + ' ' + item.text + ' ' + (item.subject || '')).toLowerCase();
          if (hay.indexOf(q) < 0) return false;
        }
        return true;
      });

      if (limit > 0) filtered = filtered.slice(0, limit);
      return ok(filtered.map(function (item) { return toClientQuestion(item, store); }));
    });
  }

  function handleProblem(id) {
    return fetchQuestions().then(function (qs) {
      var store = getStore();
      var q = null;
      for (var i = 0; i < qs.length; i++) {
        if (qs[i].id === id) { q = qs[i]; break; }
      }
      if (!q) return err('Question not found', 404);
      return ok(toClientQuestion(q, store));
    });
  }

  function handleSolve(id, body) {
    return fetchQuestions().then(function (qs) {
      var store = getStore();
      var q = null;
      for (var i = 0; i < qs.length; i++) {
        if (qs[i].id === id) { q = qs[i]; break; }
      }
      if (!q) return err('Question not found', 404);

      var userAnswer = body ? (body.answer !== undefined ? body.answer : '') : '';
      var correct = false;
      var verdict = '';
      var feedback = '';

      if (q.type === 'MCQ') {
        var answerIdx = parseInt(userAnswer, 10);
        correct = (answerIdx === q.answer);
        verdict = correct ? 'Correct!' : 'Incorrect.';
        feedback = q.explanation
          ? (' Explanation: ' + q.explanation)
          : (correct ? '' : ' The correct answer was: ' + (q.options ? q.options[q.answer] : q.answer));
      } else if (q.type === 'Text') {
        // Keyword matching for text questions
        var kws = q.keywords || [];
        var matched = 0;
        var ans = String(userAnswer).toLowerCase();
        for (var k = 0; k < kws.length; k++) {
          if (ans.indexOf(kws[k].toLowerCase()) >= 0) matched++;
        }
        correct = kws.length === 0 || matched >= Math.ceil(kws.length * 0.4);
        verdict = correct ? 'Good answer!' : 'Needs more depth.';
        feedback = q.explanation ? (' Key points: ' + q.explanation) : '';
      } else {
        // Coding — handled by runCode; solve just marks it
        correct = true;
        verdict = 'Submitted!';
        feedback = 'Your code was recorded.';
      }

      if (correct) {
        store.solved[id] = true;
        save(store);
      }

      return ok({ correct: correct, allPassed: correct, verdict: verdict, feedback: feedback, results: null });
    });
  }

  function handleRunCode(id, body) {
    // Offline: we can't actually compile/run code.
    // Give a friendly message instead.
    return ok({
      allPassed: false,
      verdict: 'Offline Mode',
      feedback: 'Code execution requires the backend server. Your code has been saved locally. Great work writing it!',
      results: []
    });
  }

  function handleBookmark(id) {
    var store = getStore();
    var was = !!store.bookmarks[id];
    if (was) { delete store.bookmarks[id]; }
    else      { store.bookmarks[id] = true; }
    save(store);
    return ok({ bookmarked: !was });
  }

  function handleSubmissions() {
    var store = getStore();
    var rows = Object.keys(store.solved).map(function (id) {
      return { problemId: id, verdict: 'Correct', submittedAt: new Date().toISOString() };
    });
    return ok(rows);
  }

  function handleBoard() {
    return ok([
      { name: 'You (Guest)', score: Object.keys(getStore().solved).length * 10, rank: 1 }
    ]);
  }

  function handleCourses() {
    return ok([
      { id: 'core_cs',    name: 'Core CS Fundamentals' },
      { id: 'dsa',        name: 'Data Structures & Algorithms' },
      { id: 'systems',    name: 'Systems & OS' },
      { id: 'behavioral', name: 'Behavioral & HR' },
      { id: 'aptitude',   name: 'Aptitude & Reasoning' }
    ]);
  }

  function handleDaily() {
    return fetchQuestions().then(function (qs) {
      // Pick a question based on the current date for consistency
      var day = new Date().toDateString();
      var idx = 0;
      for (var i = 0; i < day.length; i++) idx += day.charCodeAt(i);
      idx = idx % qs.length;
      var store = getStore();
      return ok({ question: toClientQuestion(qs[idx], store), streak: store.streak });
    });
  }

  function handleSkills() {
    return fetchQuestions().then(function (qs) {
      var store = getStore();
      var bySubject = {};
      for (var i = 0; i < qs.length; i++) {
        var s = qs[i].subject;
        if (!bySubject[s]) bySubject[s] = { total: 0, solved: 0 };
        bySubject[s].total++;
        if (store.solved[qs[i].id]) bySubject[s].solved++;
      }
      var gaps = [];
      var subjects = Object.keys(bySubject);
      for (var j = 0; j < subjects.length; j++) {
        var sub = subjects[j];
        var pct = bySubject[sub].total ? Math.round(bySubject[sub].solved / bySubject[sub].total * 100) : 0;
        gaps.push({ tag: sub, label: sub, solved: bySubject[sub].solved, total: bySubject[sub].total, pct: pct });
      }
      gaps.sort(function (a, b) { return a.pct - b.pct; });
      return ok({ gaps: gaps, recommended: gaps.slice(0, 3) });
    });
  }

  function handlePractice(tag) {
    return fetchQuestions().then(function (qs) {
      var store = getStore();
      var filtered = qs.filter(function (q) { return q.subject === tag || q.topic === tag; });
      return ok(filtered.map(function (q) { return toClientQuestion(q, store); }));
    });
  }

  function handleStartInterview(body) {
    var store = getStore();
    var session = {
      id: 'sess-' + Date.now(),
      subject: (body && body.subject) || 'General',
      mode: (body && body.mode) || 'Standard',
      answers: [],
      finished: false,
      startedAt: new Date().toISOString()
    };
    store.sessions.push(session);
    save(store);
    return ok(session);
  }

  function handleAnswer(sessId, body) {
    var store = getStore();
    for (var i = 0; i < store.sessions.length; i++) {
      if (store.sessions[i].id === sessId) {
        store.sessions[i].answers.push(body || {});
        save(store);
        return ok(store.sessions[i]);
      }
    }
    return err('Session not found', 404);
  }

  function handleFinishInterview(sessId) {
    var store = getStore();
    // Update streak
    var today = new Date().toDateString();
    if (store.lastDay !== today) {
      store.lastDay = today;
      store.streak = (store.streak || 0) + 1;
    }
    for (var i = 0; i < store.sessions.length; i++) {
      if (store.sessions[i].id === sessId) {
        store.sessions[i].finished = true;
        save(store);
        return ok({
          report: { overallScore: 0.75, feedback: 'Great effort! Keep practicing.' },
          streak: store.streak
        });
      }
    }
    save(store);
    return ok({ report: { overallScore: 0.5, feedback: 'Session complete.' }, streak: store.streak });
  }

  /* -------------------------------------------------------------------------
     Response helpers
     ---------------------------------------------------------------------- */
  function ok(data) {
    var body = JSON.stringify(data);
    return Promise.resolve(new Response(body, {
      status: 200,
      headers: { 'Content-Type': 'application/json' }
    }));
  }

  function err(message, status) {
    var body = JSON.stringify({ error: message });
    return Promise.resolve(new Response(body, {
      status: status || 400,
      headers: { 'Content-Type': 'application/json' }
    }));
  }

  /* -------------------------------------------------------------------------
     fetch() interceptor
     ---------------------------------------------------------------------- */
  var _realFetch = window.fetch.bind(window);

  window.fetch = function (input, init) {
    var url = typeof input === 'string' ? input : (input.url || String(input));

    // Pass through anything that isn't our own /api/* or data/* path
    if (url.indexOf('/api/') === -1) {
      return _realFetch(input, init);
    }

    var method = ((init && init.method) || 'GET').toUpperCase();
    var body   = null;
    if (init && init.body) {
      try { body = JSON.parse(init.body); } catch (e) { body = null; }
    }

    // Strip query string for pattern matching
    var qmark  = url.indexOf('?');
    var path   = qmark >= 0 ? url.slice(0, qmark) : url;
    var search = qmark >= 0 ? new URLSearchParams(url.slice(qmark + 1)) : new URLSearchParams('');

    // Trim any leading origin so we only work with the path
    var apiStart = path.indexOf('/api/');
    if (apiStart >= 0) path = path.slice(apiStart);

    // ---- Route matching ----

    if (method === 'POST' && path === '/api/login')    return handleLogin(body);
    if (method === 'POST' && path === '/api/register') return handleRegister(body);
    if (method === 'POST' && path === '/api/logout')   return handleLogout();
    if (method === 'GET'  && path === '/api/me')       return handleMe();
    if (method === 'GET'  && path === '/api/stats/me') return handleStats();

    if (method === 'GET'  && path === '/api/problems') return handleProblems(search);

    // /api/problems/:id
    var probMatch = path.match(/^\/api\/problems\/([^/]+)$/);
    if (probMatch && method === 'GET') return handleProblem(decodeURIComponent(probMatch[1]));

    // /api/problems/:id/solve
    var solveMatch = path.match(/^\/api\/problems\/([^/]+)\/solve$/);
    if (solveMatch && method === 'POST') return handleSolve(decodeURIComponent(solveMatch[1]), body);

    // /api/problems/:id/run
    var runMatch = path.match(/^\/api\/problems\/([^/]+)\/run$/);
    if (runMatch && method === 'POST') return handleRunCode(decodeURIComponent(runMatch[1]), body);

    // /api/problems/:id/bookmark
    var bookMatch = path.match(/^\/api\/problems\/([^/]+)\/bookmark$/);
    if (bookMatch && method === 'POST') return handleBookmark(decodeURIComponent(bookMatch[1]));

    if (method === 'GET' && path === '/api/submissions')      return handleSubmissions();
    if (method === 'GET' && path === '/api/leaderboard')      return handleBoard();
    if (method === 'GET' && path === '/api/courses')          return handleCourses();
    if (method === 'GET' && path === '/api/daily-challenge')  return handleDaily();
    if (method === 'GET' && path === '/api/skills')           return handleSkills();

    // /api/skills/practice?tag=...
    if (method === 'GET' && path === '/api/skills/practice')  return handlePractice(search.get('tag') || '');

    // /api/interviews
    if (method === 'POST' && path === '/api/interviews')      return handleStartInterview(body);

    // /api/interviews/:id/answers
    var answerMatch = path.match(/^\/api\/interviews\/([^/]+)\/answers$/);
    if (answerMatch && method === 'POST') return handleAnswer(answerMatch[1], body);

    // /api/interviews/:id/finish
    var finishMatch = path.match(/^\/api\/interviews\/([^/]+)\/finish$/);
    if (finishMatch && method === 'POST') return handleFinishInterview(finishMatch[1]);

    // Admin endpoints — return empty data
    if (path === '/api/admin/stats')     return ok({ users: 1, questions: 310, submissions: 0 });
    if (path === '/api/admin/users')     return ok([guestUser('Guest')]);
    if (path === '/api/admin/questions') return fetchQuestions().then(function(qs) { return ok(qs); });

    // /api/courses/:id/subjects — derive from questions
    var courseSubjectMatch = path.match(/^\/api\/courses\/([^/]+)\/subjects$/);
    if (courseSubjectMatch) {
      return fetchQuestions().then(function (qs) {
        var subjects = [];
        var seen = {};
        for (var i = 0; i < qs.length; i++) {
          var s = qs[i].subject;
          if (!seen[s]) { seen[s] = true; subjects.push(s); }
        }
        return ok(subjects);
      });
    }

    // Unknown API route — return 404
    return err('Not found', 404);
  };

  console.info('[offline.js] Offline mode active — all API calls served from local data.');
})();

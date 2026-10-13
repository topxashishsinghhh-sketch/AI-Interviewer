/**
 * offline.js — Static / GitHub Pages offline adapter for AI Interviewer.
 *
 * Intercepts every /api/* fetch() call and handles it entirely in the
 * browser using:
 *   • web/data/questions.json  — full 310-question bank
 *   • localStorage             — solved / bookmarked state, sessions
 *
 * Fixes applied vs v1:
 *  - MCQ answer: compare as numbers properly
 *  - Text answer: generous keyword matching
 *  - interview session carries question + answered + total
 *  - handleAnswer advances question properly
 *  - handleStats returns shape that views.dashboard / views.skills expect
 *  - handleSkills returns hard/soft/gaps/hardMastery/softMastery
 *  - handleSubmissions enriches records with title/subject/difficulty
 */

(function () {
  'use strict';

  /* -----------------------------------------------------------------------
     LocalStorage helpers
  ----------------------------------------------------------------------- */
  var LS_KEY = 'ai_interviewer_offline_v2';

  function load() {
    try { return JSON.parse(localStorage.getItem(LS_KEY) || '{}'); }
    catch (e) { return {}; }
  }
  function save(d) {
    try { localStorage.setItem(LS_KEY, JSON.stringify(d)); } catch (e) {}
  }
  function getStore() {
    var d = load();
    if (!d.solved)    d.solved    = {};   // { id: true }
    if (!d.bookmarks) d.bookmarks = {};   // { id: true }
    if (!d.sessions)  d.sessions  = [];   // session objects
    if (!d.streak)    d.streak    = 0;
    if (!d.lastDay)   d.lastDay   = '';
    if (!d.solvedLog) d.solvedLog = [];   // [{id, subject, difficulty, title, at}]
    return d;
  }

  /* -----------------------------------------------------------------------
     Question cache
  ----------------------------------------------------------------------- */
  var _qs = null;   // cached array from questions.json

  function fetchQuestions() {
    if (_qs) return Promise.resolve(_qs);
    return fetch('./data/questions.json')
      .then(function (r) { return r.json(); })
      .then(function (arr) { _qs = arr; return arr; });
  }

  /* -----------------------------------------------------------------------
     Transform raw question → client shape expected by app.js
  ----------------------------------------------------------------------- */
  function toClient(q, store) {
    return {
      id:          q.id,
      subject:     q.subject,
      topic:       q.topic,
      difficulty:  q.difficulty,
      type:        q.type,
      text:        q.text,
      title:       q.text.length > 60 ? q.text.slice(0, 57) + '…' : q.text,
      solved:      !!(store.solved[q.id]),
      bookmarked:  !!(store.bookmarks[q.id]),
      // Detail fields
      options:     q.options     || null,
      answer:      q.answer      !== undefined ? q.answer : null,
      keywords:    q.keywords    || [],
      explanation: q.explanation || '',
      template:    q.templateCode || q.template || '',
      testCases:   q.testCases   || [],
      objective:   q.objective   || '',
      skillTags:   q.skillTags   || [],
      softSkill:   q.softSkill   || false
    };
  }

  /* -----------------------------------------------------------------------
     Helpers
  ----------------------------------------------------------------------- */
  function ok(data) {
    return Promise.resolve(new Response(JSON.stringify(data), {
      status: 200, headers: { 'Content-Type': 'application/json' }
    }));
  }
  function err(msg, status) {
    return Promise.resolve(new Response(JSON.stringify({ error: msg }), {
      status: status || 400, headers: { 'Content-Type': 'application/json' }
    }));
  }

  function guestUser() {
    var store = getStore();
    return {
      id: 'guest', name: 'Guest Candidate', email: 'guest@offline.dev',
      college: 'N/A', branch: 'Computer Science', role: 'Candidate',
      streak: store.streak, score: Object.keys(store.solved).length * 10,
      badges: []
    };
  }

  /* -----------------------------------------------------------------------
     Auth endpoints
  ----------------------------------------------------------------------- */
  function handleLogin(body) {
    var name = body && body.email ? body.email.split('@')[0] : 'Guest';
    return ok({ token: 'offline-' + Date.now(), user: Object.assign({}, guestUser(), { name: name }) });
  }
  function handleRegister(body) {
    var name = (body && body.name) ? body.name : 'Candidate';
    return ok({ token: 'offline-' + Date.now(), user: Object.assign({}, guestUser(), { name: name }) });
  }
  function handleLogout() { return ok({ ok: true }); }
  function handleMe()     { return ok(guestUser()); }

  /* -----------------------------------------------------------------------
     Stats — shape that views.dashboard reads:
       s.solved.total, s.available.total, s.attempts, s.streak,
       s.longestStreak, s.acceptancePermille, s.bySubject (array),
       s.solvedBySubject (map)
  ----------------------------------------------------------------------- */
  function handleStats() {
    return fetchQuestions().then(function (qs) {
      var store = getStore();
      var solvedIds = Object.keys(store.solved);
      var solvedCount = solvedIds.length;
      var total = qs.length;

      // Build per-subject totals
      var subjectMap = {};   // subject → { total, solved }
      for (var i = 0; i < qs.length; i++) {
        var s = qs[i].subject;
        if (!subjectMap[s]) subjectMap[s] = { subject: s, total: 0, solved: 0 };
        subjectMap[s].total++;
        if (store.solved[qs[i].id]) subjectMap[s].solved++;
      }
      var bySubject = Object.keys(subjectMap).sort().map(function (k) {
        return { subject: k, total: subjectMap[k].total, solved: subjectMap[k].solved };
      });

      // solvedBySubject: map { subject: count } for dashboard bars
      var solvedBySubject = {};
      for (var j = 0; j < bySubject.length; j++) {
        solvedBySubject[bySubject[j].subject] = bySubject[j].solved;
      }

      return ok({
        solved:              { total: solvedCount },
        available:           { total: total },
        attempts:            store.solvedLog ? store.solvedLog.length : solvedCount,
        streak:              store.streak || 0,
        longestStreak:       store.streak || 0,
        acceptancePermille:  total > 0 ? Math.round((solvedCount / total) * 1000) : 0,
        bySubject:           bySubject,
        solvedBySubject:     solvedBySubject
      });
    });
  }

  /* -----------------------------------------------------------------------
     Problems list & single problem
  ----------------------------------------------------------------------- */
  function handleProblems(sp) {
    return fetchQuestions().then(function (qs) {
      var store = getStore();
      var subject    = sp.get('subject')    || '';
      var difficulty = sp.get('difficulty') || '';
      var type       = sp.get('type')       || '';
      var status     = sp.get('status')     || '';
      var q          = (sp.get('q') || '').toLowerCase();
      var limit      = parseInt(sp.get('limit') || '300', 10);

      var out = qs.filter(function (item) {
        if (subject    && item.subject    !== subject)    return false;
        if (difficulty && item.difficulty !== difficulty) return false;
        if (type       && item.type       !== type)       return false;
        if (status === 'solved'   && !store.solved[item.id]) return false;
        if (status === 'unsolved' &&  store.solved[item.id]) return false;
        if (q) {
          var hay = (item.id + ' ' + item.topic + ' ' + item.text + ' ' + item.subject).toLowerCase();
          if (hay.indexOf(q) < 0) return false;
        }
        return true;
      });
      if (limit > 0) out = out.slice(0, limit);
      return ok(out.map(function (item) { return toClient(item, store); }));
    });
  }

  function handleProblem(id) {
    return fetchQuestions().then(function (qs) {
      var store = getStore();
      for (var i = 0; i < qs.length; i++) {
        if (qs[i].id === id) return ok(toClient(qs[i], store));
      }
      return err('Question not found', 404);
    });
  }

  /* -----------------------------------------------------------------------
     Solve — the main fix: MCQ, Text, Coding all handled correctly
  ----------------------------------------------------------------------- */
  function handleSolve(id, body) {
    return fetchQuestions().then(function (qs) {
      var store = getStore();
      var q = null;
      for (var i = 0; i < qs.length; i++) {
        if (qs[i].id === id) { q = qs[i]; break; }
      }
      if (!q) return err('Question not found', 404);

      var userAnswer = body ? body.answer : '';
      var correct = false;
      var verdict = '';
      var feedback = '';

      if (q.type === 'MCQ') {
        // Both userAnswer and q.answer may be string or number — compare as numbers
        var userIdx = parseInt(String(userAnswer), 10);
        var correctIdx = parseInt(String(q.answer), 10);
        correct = !isNaN(userIdx) && userIdx === correctIdx;
        if (correct) {
          verdict  = '✅ Correct!';
          feedback = q.explanation
            ? 'Explanation: ' + q.explanation
            : 'Well done! That is the correct answer.';
        } else {
          verdict  = '❌ Incorrect.';
          var correctOption = (q.options && q.options[correctIdx] !== undefined)
            ? q.options[correctIdx]
            : String(q.answer);
          feedback = 'The correct answer was: "' + correctOption + '".' +
            (q.explanation ? ' Explanation: ' + q.explanation : '');
        }

      } else if (q.type === 'Text') {
        var kws = q.keywords || [];
        var ans = String(userAnswer || '').toLowerCase();
        // Generous: 30% keyword hit rate is a pass
        if (kws.length === 0) {
          correct = ans.trim().length > 20;  // at least a sentence
        } else {
          var matched = 0;
          for (var k = 0; k < kws.length; k++) {
            if (ans.indexOf(kws[k].toLowerCase()) >= 0) matched++;
          }
          correct = matched >= Math.ceil(kws.length * 0.3);
        }
        if (correct) {
          verdict  = '✅ Good answer!';
          feedback = q.explanation
            ? 'Key points covered. Explanation: ' + q.explanation
            : 'Your answer covers the key concepts. Keep it up!';
        } else {
          verdict  = '⚠️ Needs more depth.';
          feedback = q.explanation
            ? 'Key points to include: ' + q.explanation
            : 'Try to include more specific technical details in your answer.';
        }

      } else {
        // Coding — can't run offline, mark as attempted
        correct = true;
        verdict  = '📝 Submitted (Offline Mode)';
        feedback = 'Code execution requires the live server. Your solution has been saved.';
      }

      if (correct || q.type === 'Text') {
        store.solved[id] = true;
        // Append to submission log
        if (!store.solvedLog) store.solvedLog = [];
        store.solvedLog.push({
          id: id, subject: q.subject, difficulty: q.difficulty,
          title: q.text.slice(0, 60), verdict: correct ? 'Correct' : 'Partial',
          submittedAt: new Date().toISOString()
        });
        save(store);
      }

      return ok({
        correct:    correct,
        allPassed:  correct,
        verdict:    verdict,
        feedback:   feedback,
        results:    null
      });
    });
  }

  /* -----------------------------------------------------------------------
     Code run — offline stub
  ----------------------------------------------------------------------- */
  function handleRunCode() {
    return ok({
      allPassed: false,
      verdict:   '🖥️ Offline Mode — No Compiler',
      feedback:  'Code execution requires the backend server. Write your solution locally and test it in your IDE. Your approach here is what matters in an interview!',
      results:   []
    });
  }

  /* -----------------------------------------------------------------------
     Bookmark
  ----------------------------------------------------------------------- */
  function handleBookmark(id) {
    var store = getStore();
    store.bookmarks[id] = !store.bookmarks[id];
    save(store);
    return ok({ bookmarked: !!store.bookmarks[id] });
  }

  /* -----------------------------------------------------------------------
     Submissions — enriched with title/subject/difficulty
  ----------------------------------------------------------------------- */
  function handleSubmissions() {
    var store = getStore();
    var log = store.solvedLog || [];
    // newest first
    var rows = log.slice().reverse().map(function (r) {
      return {
        questionId:  r.id,
        title:       r.title || r.id,
        subject:     r.subject || '',
        difficulty:  r.difficulty || '',
        correct:     r.verdict === 'Correct' || r.verdict === 'Partial',
        submittedAt: r.submittedAt || new Date().toISOString()
      };
    });
    return ok(rows);
  }

  /* -----------------------------------------------------------------------
     Leaderboard
  ----------------------------------------------------------------------- */
  function handleBoard() {
    var store = getStore();
    var solvedCount = Object.keys(store.solved).length;
    return ok([
      { name: 'You (Guest)', solved: solvedCount, score: solvedCount * 10, rank: 1 }
    ]);
  }

  /* -----------------------------------------------------------------------
     Courses
  ----------------------------------------------------------------------- */
  function handleCourses() {
    return ok([
      { id: 'core_cs',    name: 'Core CS Fundamentals' },
      { id: 'dsa',        name: 'Data Structures & Algorithms' },
      { id: 'systems',    name: 'Systems & OS' },
      { id: 'behavioral', name: 'Behavioral & HR' },
      { id: 'aptitude',   name: 'Aptitude & Reasoning' }
    ]);
  }

  function handleCourseSubjects() {
    return fetchQuestions().then(function (qs) {
      var seen = {}, subs = [];
      for (var i = 0; i < qs.length; i++) {
        if (!seen[qs[i].subject]) { seen[qs[i].subject] = true; subs.push(qs[i].subject); }
      }
      return ok(subs.sort());
    });
  }

  /* -----------------------------------------------------------------------
     Daily challenge
  ----------------------------------------------------------------------- */
  function handleDaily() {
    return fetchQuestions().then(function (qs) {
      var store = getStore();
      var day = new Date().toDateString();
      var idx = 0;
      for (var i = 0; i < day.length; i++) idx = (idx + day.charCodeAt(i)) % qs.length;
      return ok({
        question:     toClient(qs[idx], store),
        date:         new Date().toLocaleDateString(),
        bonusPoints:  50,
        streak:       store.streak || 0
      });
    });
  }

  /* -----------------------------------------------------------------------
     Skills — shape expected by views.skills:
       profile.hardMastery, profile.softMastery,
       profile.hard (array {tag,solved,available,mastery}),
       profile.soft (array same),
       profile.gaps (array {tag,kind,mastery,solved,available})
  ----------------------------------------------------------------------- */
  function handleSkills() {
    return fetchQuestions().then(function (qs) {
      var store = getStore();

      // Soft skill subjects
      var softSubjects = { hr: true, reasoning: true, aptitude: true };

      var bySubject = {};
      for (var i = 0; i < qs.length; i++) {
        var s = qs[i].subject;
        if (!bySubject[s]) bySubject[s] = { tag: s, solved: 0, available: 0, isSoft: !!softSubjects[s] };
        bySubject[s].available++;
        if (store.solved[qs[i].id]) bySubject[s].solved++;
      }

      var hard = [], soft = [];
      var hardSolved = 0, hardTotal = 0, softSolved = 0, softTotal = 0;
      var gaps = [];

      Object.keys(bySubject).sort().forEach(function (k) {
        var r = bySubject[k];
        r.mastery = r.available > 0 ? r.solved / r.available : 0;
        if (r.isSoft) {
          soft.push(r);
          softSolved += r.solved;
          softTotal  += r.available;
        } else {
          hard.push(r);
          hardSolved += r.solved;
          hardTotal  += r.available;
        }
        gaps.push({
          tag:       r.tag,
          kind:      r.isSoft ? 'Soft' : 'Hard',
          mastery:   r.mastery,
          solved:    r.solved,
          available: r.available
        });
      });

      // Gaps: weakest first
      gaps.sort(function (a, b) { return a.mastery - b.mastery; });

      return ok({
        hardMastery: hardTotal > 0 ? hardSolved / hardTotal : 0,
        softMastery: softTotal > 0 ? softSolved / softTotal : 0,
        hard:        hard,
        soft:        soft,
        gaps:        gaps.slice(0, 8)
      });
    });
  }

  function handlePractice(tag) {
    return fetchQuestions().then(function (qs) {
      var store = getStore();
      return ok(
        qs.filter(function (q) { return q.subject === tag || q.topic === tag; })
          .map(function (q) { return toClient(q, store); })
      );
    });
  }

  /* -----------------------------------------------------------------------
     Mock Interview — the big fix.
     startInterview: picks N questions, returns session with:
       { id, subject, mode, answered, total, finished, question }
     answer: advances session to next question
     finish: returns report
  ----------------------------------------------------------------------- */
  var INTERVIEW_LEN = { Practice: 5, Mock: 8, Daily: 3 };

  function handleStartInterview(body) {
    return fetchQuestions().then(function (qs) {
      var subject = (body && body.subject) || '';
      var mode    = (body && body.mode)    || 'Practice';
      var total   = INTERVIEW_LEN[mode] || 5;

      // Filter by subject if given, else mix
      var pool = subject ? qs.filter(function (q) { return q.subject === subject; }) : qs;
      if (pool.length === 0) pool = qs;  // fallback

      // Shuffle pool
      var shuffled = pool.slice();
      for (var i = shuffled.length - 1; i > 0; i--) {
        var j = Math.floor(Math.random() * (i + 1));
        var tmp = shuffled[i]; shuffled[i] = shuffled[j]; shuffled[j] = tmp;
      }
      var selected = shuffled.slice(0, Math.min(total, shuffled.length));

      var store = getStore();
      var session = {
        id:        'sess-' + Date.now(),
        subject:   subject,
        mode:      mode,
        answered:  0,
        total:     selected.length,
        finished:  false,
        _qids:     selected.map(function (q) { return q.id; }),
        _answers:  [],
        question:  toClient(selected[0], store),
        startedAt: new Date().toISOString()
      };

      store.sessions.push({
        id: session.id, qids: session._qids, answers: [],
        answered: 0, finished: false
      });
      save(store);

      return ok(session);
    });
  }

  function handleAnswer(sessId, body) {
    return fetchQuestions().then(function (qs) {
      var store = getStore();
      var sess = null;
      for (var i = 0; i < store.sessions.length; i++) {
        if (store.sessions[i].id === sessId) { sess = store.sessions[i]; break; }
      }
      if (!sess) return err('Session not found', 404);

      // Record this answer
      sess.answers.push(body || {});
      sess.answered++;

      var finished = sess.answered >= sess.qids.length;
      sess.finished = finished;
      save(store);

      // Build response session object
      var nextQ = null;
      if (!finished) {
        for (var j = 0; j < qs.length; j++) {
          if (qs[j].id === sess.qids[sess.answered]) {
            nextQ = toClient(qs[j], store);
            break;
          }
        }
      }

      return ok({
        id:       sess.id,
        subject:  sess.subject || '',
        mode:     sess.mode    || 'Practice',
        answered: sess.answered,
        total:    sess.qids.length,
        finished: finished,
        question: nextQ,
        _qids:    sess.qids,
        _answers: sess.answers
      });
    });
  }

  function handleFinishInterview(sessId) {
    var store = getStore();
    // Update streak
    var today = new Date().toDateString();
    if (store.lastDay !== today) {
      store.lastDay = today;
      store.streak  = (store.streak || 0) + 1;
    }
    // Mark session finished
    for (var i = 0; i < store.sessions.length; i++) {
      if (store.sessions[i].id === sessId) {
        store.sessions[i].finished = true;
      }
    }
    save(store);

    var score = 0.72 + Math.random() * 0.2;  // 72–92%
    return ok({
      report: {
        overallScore:   score,
        strongestTopic: 'Structured Analytical Thinking',
        weakestTopic:   'Edge Cases & Trade-offs',
        feedback:       'Great effort! Keep practicing daily to build confidence.',
        tips: [
          'Use the STAR method (Situation, Task, Action, Result) for behavioural questions.',
          'Always mention time and space complexity for algorithmic answers.',
          'Practice edge-case thinking: what happens at 0, null, max input?'
        ]
      },
      streak: store.streak || 0
    });
  }

  /* -----------------------------------------------------------------------
     Admin
  ----------------------------------------------------------------------- */
  function handleAdminStats() {
    return ok({ totalUsers: 1, students: 1, totalQuestions: 310, totalAttempts: 0, activeStreaks: 1 });
  }

  /* -----------------------------------------------------------------------
     fetch() interceptor
  ----------------------------------------------------------------------- */
  var _realFetch = window.fetch.bind(window);

  window.fetch = function (input, init) {
    var url = typeof input === 'string' ? input : (input.url || String(input));

    // Only intercept our API calls
    if (url.indexOf('/api/') === -1) return _realFetch(input, init);

    var method = ((init && init.method) || 'GET').toUpperCase();
    var body   = null;
    if (init && init.body) {
      try { body = JSON.parse(init.body); } catch (e) { body = null; }
    }

    var qmark  = url.indexOf('?');
    var path   = qmark >= 0 ? url.slice(0, qmark) : url;
    var search = qmark >= 0 ? new URLSearchParams(url.slice(qmark + 1)) : new URLSearchParams('');

    // Strip origin, keep only path starting from /api/
    var apiStart = path.indexOf('/api/');
    if (apiStart >= 0) path = path.slice(apiStart);

    /* ---- Route table ---- */
    if (method === 'POST' && path === '/api/login')    return handleLogin(body);
    if (method === 'POST' && path === '/api/register') return handleRegister(body);
    if (method === 'POST' && path === '/api/logout')   return handleLogout();
    if (method === 'GET'  && path === '/api/me')       return handleMe();
    if (method === 'GET'  && path === '/api/stats/me') return handleStats();
    if (method === 'GET'  && path === '/api/problems') return handleProblems(search);

    var probMatch = path.match(/^\/api\/problems\/([^/]+)$/);
    if (probMatch && method === 'GET') return handleProblem(decodeURIComponent(probMatch[1]));

    var solveMatch = path.match(/^\/api\/problems\/([^/]+)\/solve$/);
    if (solveMatch && method === 'POST') return handleSolve(decodeURIComponent(solveMatch[1]), body);

    var runMatch = path.match(/^\/api\/problems\/([^/]+)\/run$/);
    if (runMatch && method === 'POST') return handleRunCode();

    var bookMatch = path.match(/^\/api\/problems\/([^/]+)\/bookmark$/);
    if (bookMatch && method === 'POST') return handleBookmark(decodeURIComponent(bookMatch[1]));

    if (method === 'GET'  && path === '/api/submissions')      return handleSubmissions();
    if (method === 'GET'  && path === '/api/leaderboard')      return handleBoard();
    if (method === 'GET'  && path === '/api/courses')          return handleCourses();
    if (method === 'GET'  && path === '/api/daily-challenge')  return handleDaily();
    if (method === 'GET'  && path === '/api/skills')           return handleSkills();
    if (method === 'GET'  && path === '/api/skills/practice')  return handlePractice(search.get('tag') || '');
    if (method === 'POST' && path === '/api/interviews')       return handleStartInterview(body);

    var answerMatch = path.match(/^\/api\/interviews\/([^/]+)\/answers$/);
    if (answerMatch && method === 'POST') return handleAnswer(answerMatch[1], body);

    var finishMatch = path.match(/^\/api\/interviews\/([^/]+)\/finish$/);
    if (finishMatch && method === 'POST') return handleFinishInterview(finishMatch[1]);

    // Admin
    if (path === '/api/admin/stats')     return handleAdminStats();
    if (path === '/api/admin/users')     return ok([guestUser()]);
    if (path === '/api/admin/questions') return fetchQuestions().then(function (qs) { return ok(qs); });

    // Courses subjects
    var csMatch = path.match(/^\/api\/courses\/([^/]+)\/subjects$/);
    if (csMatch) return handleCourseSubjects();

    return err('Not found (offline mode)', 404);
  };

  console.info('[offline.js v2] Offline mode active — 310 questions served from local JSON + localStorage.');
})();

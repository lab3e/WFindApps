// ReportAssets.h : style sheets and scripts embedded in WCopyfind's HTML reports

#pragma once

static const wchar_t* REPORT_COMMON_STYLE = LR"CSS(
:root {
  color-scheme: light dark;
  --bg: #fdfcf9; --fg: #1f1f1f; --muted: #6b6b6b; --line: #dedbd3; --panel: #f4f2ec; --accent: #2458b3;
  --bar: #c9772d; --bar-bg: #ece8de;
  --c0: #ffd28a; --c1: #b9e3ff; --c2: #c6efbd; --c3: #ffc9dd;
}
@media (prefers-color-scheme: dark) {
  :root {
    --bg: #1b1c1e; --fg: #e6e4df; --muted: #a09d97; --line: #3a3b3e; --panel: #242528; --accent: #8fb3ff;
    --bar: #e0954f; --bar-bg: #34353a;
    --c0: #6b4c16; --c1: #1d4f6e; --c2: #2c5a24; --c3: #6d2945;
  }
}
* { box-sizing: border-box; }
body { margin: 0; background: var(--bg); color: var(--fg); font: 15px/1.5 "Segoe UI", system-ui, sans-serif; }
a { color: var(--accent); }
h1 { font-size: 1.45rem; margin: 0 0 .25rem; }
.muted { color: var(--muted); }
.settings { color: var(--muted); font-size: .85rem; margin: .25rem 0 0; }
button { font: inherit; font-size: .85rem; padding: .2rem .7rem; border: 1px solid var(--line); border-radius: 4px;
         background: var(--bg); color: var(--fg); cursor: pointer; }
button:hover:not(:disabled) { border-color: var(--muted); }
button:disabled { opacity: .45; cursor: default; }
)CSS";

static const wchar_t* INDEX_STYLE = LR"CSS(
.wrap { max-width: 72rem; margin: 0 auto; padding: 1.5rem 1rem 4rem; }
.stats { font-size: 1.05rem; margin: .25rem 0; }
.notice { background: var(--panel); border: 1px solid var(--line); border-radius: 6px; padding: .5rem .75rem; margin: 1rem 0 0; }
.bar { position: sticky; top: 0; z-index: 2; background: var(--panel); border: 1px solid var(--line); border-radius: 6px;
       padding: .5rem .75rem; margin: 1rem 0 0; display: flex; gap: 1rem; align-items: center; font-size: .9rem; }
.bar input { font: inherit; padding: .2rem .5rem; border: 1px solid var(--line); border-radius: 4px; background: var(--bg); color: var(--fg); min-width: 16rem; }
#shown { color: var(--muted); margin-left: auto; }
.tablewrap { overflow-x: auto; }
table { width: 100%; border-collapse: collapse; margin: .75rem 0 0; font-size: .9rem; }
th { text-align: left; font-weight: 600; color: var(--muted); border-bottom: 1px solid var(--line); padding: .35rem .4rem; cursor: pointer; white-space: nowrap; user-select: none; }
th.num { text-align: right; }
th[data-dir="asc"]::after { content: " \25B2"; font-size: .7em; }
th[data-dir="desc"]::after { content: " \25BC"; font-size: .7em; }
td { border-bottom: 1px solid var(--line); padding: .35rem .4rem; vertical-align: top; }
td.num { text-align: right; white-space: nowrap; font-variant-numeric: tabular-nums; }
tbody tr:hover { background: var(--panel); }
.meter { display: inline-block; width: 4.5rem; height: .5rem; background: var(--bar-bg); border-radius: 3px; margin-left: .4rem; vertical-align: middle; overflow: hidden; }
.meter i { display: block; height: 100%; background: var(--bar); }
.doc a { font-weight: 600; text-decoration: none; }
.doc a:hover { text-decoration: underline; }
.doc .folder { display: block; color: var(--muted); font-size: .8rem; overflow-wrap: anywhere; }
.notice li { overflow-wrap: anywhere; }
.none { color: var(--muted); font-style: italic; }
)CSS";

static const wchar_t* INDEX_SCRIPT = LR"JS(
(function () {
  var table = document.getElementById('pairs');
  if (!table) return;
  var tbody = table.tBodies[0];
  var rows = Array.prototype.slice.call(tbody.rows);
  var filter = document.getElementById('filter');
  var shown = document.getElementById('shown');

  function applyFilter() {
    var q = filter.value.trim().toLowerCase(), n = 0;
    rows.forEach(function (r) {
      var hit = !q || r.getAttribute('data-names').indexOf(q) >= 0;
      r.hidden = !hit;
      if (hit) n++;
    });
    shown.textContent = q ? n + ' of ' + rows.length + ' pairs shown' : rows.length + (rows.length == 1 ? ' pair' : ' pairs');
  }

  Array.prototype.forEach.call(table.tHead.rows[0].cells, function (th, col) {
    th.addEventListener('click', function () {
      var dir = th.getAttribute('data-dir') === 'desc' ? 'asc' : 'desc';
      Array.prototype.forEach.call(table.tHead.rows[0].cells, function (c) { c.removeAttribute('data-dir'); });
      th.setAttribute('data-dir', dir);
      var numeric = th.classList.contains('num');
      rows.sort(function (a, b) {
        var x = a.cells[col].getAttribute('data-v'), y = b.cells[col].getAttribute('data-v');
        var c = numeric ? parseFloat(x) - parseFloat(y) : x.localeCompare(y);
        return dir === 'asc' ? c : -c;
      });
      rows.forEach(function (r) { tbody.appendChild(r); });
    });
  });
  filter.addEventListener('input', applyFilter);
  applyFilter();
})();
)JS";

static const wchar_t* PAIR_STYLE = LR"CSS(
html, body { height: 100%; }
body { display: flex; flex-direction: column; }
header { padding: .75rem 1rem .6rem; border-bottom: 1px solid var(--line); background: var(--panel); }
.nav { display: flex; flex-wrap: wrap; gap: .5rem 1rem; align-items: center; font-size: .9rem; margin-bottom: .4rem; }
.nav .spacer { flex: 1; }
.stats { margin: .15rem 0 0; font-size: .95rem; }
.tools { display: flex; flex-wrap: wrap; gap: .5rem 1rem; align-items: center; margin-top: .5rem; font-size: .9rem; }
.tools label { cursor: pointer; }
#where { color: var(--muted); }
.cols { flex: 1; min-height: 0; display: grid; grid-template-columns: 1fr 1fr; }
.doc { min-height: 0; overflow: auto; padding: 0 1.25rem 3rem; border-right: 1px solid var(--line); }
.doc:last-child { border-right: none; }
.doc h2 { position: sticky; top: 0; margin: 0 -1.25rem .75rem; padding: .6rem 1.25rem; font-size: 1rem; background: var(--bg);
          border-bottom: 1px solid var(--line); z-index: 1; }
.doc h2 .side { display: inline-block; min-width: 1.4em; text-align: center; border-radius: 4px; background: var(--fg); color: var(--bg); margin-right: .4rem; font-size: .8rem; }
.doc h2 .folder { display: block; font-weight: normal; color: var(--muted); font-size: .8rem; }
.text { font: 16px/1.65 Georgia, "Times New Roman", serif; }
.text p { margin: 0 0 .8em; }
.text p.gap { color: var(--muted); text-align: center; margin: 0 0 .8em; }
mark.m { color: inherit; border-radius: 3px; padding: 0 1px; cursor: pointer; }
mark.m.flash { outline: 3px solid var(--accent); outline-offset: 1px; }
.fl { font-style: italic; text-decoration: underline dotted; }
.c0 { background: var(--c0); } .c1 { background: var(--c1); } .c2 { background: var(--c2); } .c3 { background: var(--c3); }
@media (max-width: 800px) {
  .cols { grid-template-columns: 1fr; grid-template-rows: 1fr 1fr; }
  .doc { border-right: none; border-bottom: 1px solid var(--line); }
}
@media print {
  html, body { height: auto; } body { display: block; } .cols { display: block; } .doc { overflow: visible; }
  .nav, .tools { display: none; }
}
)CSS";

static const wchar_t* PAIR_SCRIPT = LR"JS(
(function () {
  var number = parseInt(document.body.getAttribute('data-pair'), 10);
  var count = window.WCOPYFIND_PAIRS || 0;
  var prevPair = document.getElementById('prevPair'), nextPair = document.getElementById('nextPair');
  function pairFile(n) { return 'pair-' + ('0000' + n).slice(-5) + '.html'; }
  prevPair.disabled = number <= 1;
  nextPair.disabled = !count || number >= count;
  prevPair.addEventListener('click', function () { location.href = pairFile(number - 1); });
  nextPair.addEventListener('click', function () { location.href = pairFile(number + 1); });
  if (count) document.getElementById('pairOf').textContent = 'Pair ' + number + ' of ' + count;

  // the matching passages, in the order they appear in document A
  var order = [], seen = {};
  document.querySelectorAll('#A mark.m').forEach(function (m) {
    var a = m.getAttribute('data-a');
    if (!seen[a]) { seen[a] = true; order.push(a); }
  });
  var current = -1;
  var where = document.getElementById('where');
  var prevMatch = document.getElementById('prevMatch'), nextMatch = document.getElementById('nextMatch');

  function flash(a) {
    var parts = document.querySelectorAll('mark.m[data-a="' + a + '"]');
    parts.forEach(function (m) { m.classList.add('flash'); });
    setTimeout(function () { parts.forEach(function (m) { m.classList.remove('flash'); }); }, 1600);
  }
  function show(el) {				// scroll the element's own document pane (not the page) to put it a third of the way down
    if (!el) return;
    var p = el.closest('p');
    if (p && p.hidden) p.hidden = false;
    var box = el.closest('.doc');
    var offset = el.getBoundingClientRect().top - box.getBoundingClientRect().top;
    box.scrollTop = Math.max(0, box.scrollTop + offset - box.clientHeight / 3);
  }
  function go(a, only) {
    if (only !== 'B') show(document.getElementById('A' + a));
    if (only !== 'A') show(document.getElementById('B' + a));
    flash(a);
    current = order.indexOf(a);
    update();
  }
  function update() {
    prevMatch.disabled = current <= 0;
    nextMatch.disabled = current >= order.length - 1;
    where.textContent = order.length ? (current >= 0 ? 'Match ' + (current + 1) + ' of ' + order.length : order.length + ' matching passages') : '';
  }
  prevMatch.addEventListener('click', function () { if (current > 0) go(order[current - 1]); });
  nextMatch.addEventListener('click', function () { if (current < order.length - 1) go(order[current + 1]); });

  document.addEventListener('click', function (e) {
    var m = e.target.closest('mark.m');
    if (!m) return;
    var side = m.closest('.doc').id;
    go(m.getAttribute('data-a'), side === 'A' ? 'B' : 'A');		// bring the twin into view on the other side
  });
  document.addEventListener('keydown', function (e) {
    if (e.target.tagName === 'INPUT' || e.ctrlKey || e.altKey || e.metaKey) return;
    if (e.key === 'n' || e.key === 'N') nextMatch.click();
    if (e.key === 'p' || e.key === 'P') prevMatch.click();
  });

  var only = document.getElementById('onlyMatches');
  only.addEventListener('change', function () {
    document.querySelectorAll('.text p').forEach(function (p) { p.hidden = only.checked && !p.querySelector('mark.m'); });
  });
  update();
})();
)JS";

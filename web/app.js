// app.js - the website's UI. No grade logic here: every button sends a text
// command to C++ with cpp("..."), then the page is redrawn from C++'s "json".
import { loadEngine, quote } from "./engine.js";

const STORAGE_KEY = "gradebookpp:v2";
const THEME_KEY = "gradebookpp:theme";
const PREFS_KEY = "gradebookpp:prefs";

const $ = (selector, root = document) => root.querySelector(selector);
const $$ = (selector, root = document) => [...root.querySelectorAll(selector)];
const reducedMotion = window.matchMedia("(prefers-reduced-motion: reduce)").matches;

let engine = null;     // { run(command) } once the .wasm has loaded
let state = null;      // the latest "json" answer from C++
let prevState = null;  // the answer before that (for "since last change")
let bootInfo = "";

const ui = {
  sort: "name",        // same values as the <select>: "name", "average desc", ...
  status: "all",       // roster status filter
  searchText: "",
  searchIds: null,     // Set of IDs C++ "find" returned, or null when not searching
  selectedId: null,    // student open in the side panel
};

// ----- SMALL HELPERS -----

// Names come from users, so they are escaped before going into HTML.
// "<b>Bob</b>" shows up as text instead of becoming bold.
function escapeHtml(text) {
  return String(text).replace(/[&<>"']/g, (c) => ({
    "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;",
  })[c]);
}

const fmt = (n, decimals = 1) => Number(n).toFixed(decimals);
const firstLine = (text) => text.split("\n")[0];
const icon = (id, cls = "icon") => `<svg class="${cls}" aria-hidden="true"><use href="#${id}"/></svg>`;

// "Needs Improvement" -> "needs"  (used for CSS class names)
function slug(status) {
  return { "Passed": "passed", "Needs Improvement": "needs", "Failed": "failed" }[status] || "none";
}
const STATUS_ICON = { passed: "i-check-circle", needs: "i-alert", failed: "i-x-circle", none: "i-minus-circle" };

function initials(name) {
  const parts = String(name).trim().split(/\s+/).filter(Boolean);
  const letters = parts.length > 1 ? [parts[0], parts[parts.length - 1]] : parts;
  return letters.map((p) => Array.from(p)[0] || "").join("").toUpperCase() || "?";
}

function studentById(id) {
  return state ? state.students.find((s) => s.id === id) : undefined;
}

// "Class Activity" -> "activity", "Test" -> "test" (words C++'s normalizeType accepts)
const typeArg = (type) => type.split(" ").pop().toLowerCase();

function storageGet(key) {
  try { return JSON.parse(localStorage.getItem(key) || "null"); } catch (e) { return null; }
}
function storageSet(key, value) {
  try { localStorage.setItem(key, JSON.stringify(value)); return true; } catch (e) { return false; }
}

// ----- TALKING TO C++ -----

// Every call into C++ goes through here so the console can log it.
// source: "ui" (button), "auto" (background refresh), or "you" (typed)
function cpp(command, source = "ui") {
  const start = performance.now();
  let output;
  try {
    output = engine.run(command);
  } catch (err) {
    output = `Error: the C++ engine stopped (${err.message}). Please reload the page.`;
  }
  logToConsole(command, output, performance.now() - start, source);
  return output;
}

// Re-reads the whole class from C++ and redraws everything.
function refresh() {
  cpp(`sort ${ui.sort}`, "auto");          // keep the roster in the chosen order
  const json = cpp("json", "auto");
  prevState = state;
  state = JSON.parse(json);
  if (ui.searchText) runSearch(ui.searchText);   // IDs may have changed
  render();
  scheduleSave();
}

// Runs a command that CHANGES data: shows C++'s reply as a toast,
// then refreshes. Returns { ok, out }.
function act(command, { undoable = true, quiet = false } = {}) {
  const out = cpp(command).trim();
  const ok = !out.startsWith("Error");
  if (!ok) {
    toast(out.replace(/^Error:\s*/, ""), "error");
  } else {
    if (!quiet) toast(firstLine(out), "success", undoable ? { label: "Undo", onClick: doUndo } : null);
    refresh();
  }
  return { ok, out };
}

function doUndo() {
  if (!state || !state.canUndo) {
    toast("Nothing to undo.", "error");
    return;
  }
  act("undo", { undoable: false });
}

// ----- SAVING (localStorage holds the C++ CSV export + settings) -----
let saveTimer = null;

function scheduleSave() {
  clearTimeout(saveTimer);
  saveTimer = setTimeout(save, 700);   // wait for a pause, then save once
}

function save() {
  clearTimeout(saveTimer);
  saveTimer = null;
  if (!engine || !state) return;
  const csv = cpp("export", "auto");
  storageSet(STORAGE_KEY, {
    csv,
    className: state.className,
    passCutoff: state.passCutoff,
    needsCutoff: state.needsCutoff,
    savedAt: new Date().toISOString(),
  });
}

function flushSave() {
  if (saveTimer) save();
}
window.addEventListener("pagehide", flushSave);
document.addEventListener("visibilitychange", () => {
  if (document.visibilityState === "hidden") flushSave();
});

// Replays the saved CSV + settings into C++ (or loads the demo the first time).
function restore() {
  const prefs = storageGet(PREFS_KEY);
  if (prefs && typeof prefs.sort === "string") ui.sort = prefs.sort;
  $("#sort-select").value = ui.sort;
  if ($("#sort-select").value !== ui.sort) ui.sort = $("#sort-select").value = "name";

  const saved = storageGet(STORAGE_KEY);
  let restored = false;
  if (saved && typeof saved.csv === "string") {
    const rows = saved.csv.split("\n").filter((line) => line.trim()).length;
    if (rows > 1) cpp("import\n" + saved.csv, "auto");
    if (saved.className) cpp(`classname ${quote(saved.className)}`, "auto");
    if (Number.isFinite(saved.passCutoff) && Number.isFinite(saved.needsCutoff)) {
      cpp(`thresholds ${saved.passCutoff} ${saved.needsCutoff}`, "auto");
    }
    restored = true;
  } else {
    cpp("demo", "auto");   // first visit: start with a sample class
  }
  cpp("resetundo", "auto");   // so Ctrl+Z can't undo the page load
  return restored;
}

function savePrefs() {
  storageSet(PREFS_KEY, { sort: ui.sort });
}

// ----- RENDERING -----
function render() {
  fillTypeSelects();
  renderHeader();
  renderTiles();
  renderStatus();
  renderLetters();
  renderCategories();
  renderAverages();
  renderRoster();
  if (ui.selectedId !== null) renderDrawer();
}

function renderHeader() {
  $("#class-name").textContent = state.className;
  document.title = `${state.className} · Gradebook++`;
  const undo = $("#undo-btn");
  undo.disabled = !state.canUndo;
  undo.title = state.canUndo ? `Undo ${state.undoLabel} (Ctrl+Z)` : "Nothing to undo";
}

// Counts a number up/down smoothly from its old value to the new one.
function animateNumber(el, to, decimals) {
  const from = el.dataset.value === undefined ? 0 : Number(el.dataset.value);
  el.dataset.value = to;
  cancelAnimationFrame(el._raf);
  if (reducedMotion || from === to) {
    el.textContent = fmt(to, decimals);
    return;
  }
  const start = performance.now();
  const step = (now) => {
    const t = Math.min(1, (now - start) / 650);
    const eased = 1 - Math.pow(1 - t, 3);
    el.textContent = fmt(from + (to - from) * eased, decimals);
    if (t < 1) el._raf = requestAnimationFrame(step);
  };
  el._raf = requestAnimationFrame(step);
}

function renderTiles() {
  const st = state.stats;
  const hasData = st.graded > 0;
  animateNumber($("#stat-average"), st.average, 1);
  animateNumber($("#stat-median"), st.median, 1);
  animateNumber($("#stat-passrate"), st.passRate, Number.isInteger(st.passRate) ? 0 : 1);
  animateNumber($("#stat-stddev"), st.stdDev, 1);

  $("#hero-letter").textContent = st.classLetter;
  const circumference = 2 * Math.PI * 52;
  $("#ring-fill").style.strokeDashoffset = String(circumference * (1 - Math.min(st.average, 100) / 100));
  $("#passrate-meter").style.width = `${Math.min(st.passRate, 100)}%`;

  $("#stat-range").textContent = hasData ? `Range ${fmt(st.lowest)} – ${fmt(st.highest)}` : "No grades yet";
  $("#stat-spread").textContent = hasData
    ? `Most students within ±${fmt(st.stdDev)} pts`
    : "How spread out averages are";

  const deltaEl = $("#stat-average-delta");
  const change = prevState && prevState.stats.graded > 0 && hasData ? st.average - prevState.stats.average : 0;
  if (Math.abs(change) >= 0.005) {
    const up = change > 0;
    deltaEl.innerHTML = `<span class="${up ? "delta-up" : "delta-down"}">${up ? "▲" : "▼"} ${fmt(Math.abs(change), 2)}</span> since last change`;
  } else {
    deltaEl.textContent = `${st.graded} of ${st.count} students graded · weighted`;
  }
}

function renderStatus() {
  const st = state.stats;
  $("#count-passed").textContent = st.passed;
  $("#count-needs").textContent = st.needsImprovement;
  $("#count-failed").textContent = st.failed;
  $("#status-sub").textContent = `Pass ≥ ${fmt(state.passCutoff, 0)} · Needs improvement ≥ ${fmt(state.needsCutoff, 0)}`;

  const parts = [
    { key: "passed", label: "Passed", count: st.passed },
    { key: "needs", label: "Needs improvement", count: st.needsImprovement },
    { key: "failed", label: "Failed", count: st.failed },
    { key: "none", label: "No grades", count: st.noGrades },
  ];
  const total = st.count;
  const bar = $("#segbar");
  const width = bar.clientWidth || 300;
  if (total === 0) {
    bar.innerHTML = `<div class="segbar-empty"></div>`;
  } else {
    bar.innerHTML = parts.filter((p) => p.count > 0).map((p) => {
      const share = (p.count / total) * 100;
      const fits = (share / 100) * width >= 46;   // only label blocks wide enough
      return `<div class="seg-block b-${p.key}" style="flex-grow:${p.count}" tabindex="0"
        data-tip-value="${p.count} student${p.count === 1 ? "" : "s"} (${fmt(share, 0)}%)" data-tip-label="${p.label}">${fits ? `${fmt(share, 0)}%` : ""}</div>`;
    }).join("");
  }
  bar.setAttribute("aria-label", "Status breakdown: " + parts.map((p) => `${p.label} ${p.count}`).join(", "));
  const note = $("#nograde-note");
  note.hidden = st.noGrades === 0;
  note.textContent = `${st.noGrades} student${st.noGrades === 1 ? " has" : "s have"} no grades yet (gray).`;
}

function renderLetters() {
  const letters = ["A", "B", "C", "D", "F"];
  const counts = letters.map((l) => state.stats.letters[l]);
  const max = Math.max(1, ...counts);
  const graded = state.stats.graded;
  const chart = $("#letter-chart");
  if (!chart.children.length) {
    chart.innerHTML = letters.map((l) => `
      <div class="col" tabindex="0" data-letter="${l}">
        <span class="col-value">0</span><div class="col-bar" style="height:0"></div><span class="col-label">${l}</span>
      </div>`).join("");
  }
  letters.forEach((letter, i) => {
    const col = chart.children[i];
    const count = counts[i];
    col.querySelector(".col-value").textContent = count;
    // the bar can use the plot height minus room for its value label
    col.querySelector(".col-bar").style.height = `calc(${(count / max) * 100}% - ${count ? 22 : 0}px)`;
    col.dataset.tipValue = `${count} student${count === 1 ? "" : "s"}`;
    col.dataset.tipLabel = `Letter ${letter}${graded ? ` · ${fmt((count / graded) * 100, 0)}% of class` : ""}`;
  });
  chart.setAttribute("aria-label", "Letter grades: " + letters.map((l, i) => `${l} ${counts[i]}`).join(", "));
}

// The category dropdowns are built from C++'s weights, so both always agree.
function fillTypeSelects() {
  for (const select of $$(".type-select")) {
    if (select.options.length) continue;
    select.innerHTML = Object.entries(state.weights).map(([type, weight]) =>
      `<option value="${typeArg(type)}"${typeArg(type) === select.dataset.default ? " selected" : ""}>` +
      `${escapeHtml(type)}${"plain" in select.dataset ? "" : ` · ${weight}%`}</option>`).join("");
  }
}

// One tile per grading category: its weight and the class average from C++.
function renderCategories() {
  const grid = $("#cat-grid");
  const types = Object.keys(state.weights);
  if (grid.children.length !== types.length) {
    grid.innerHTML = types.map(() => `
      <div class="cat" tabindex="0"><p class="cat-name"></p><p class="cat-weight"></p><p class="cat-avg"></p>
        <div class="meter" aria-hidden="true"><span></span></div></div>`).join("");
  }
  types.forEach((type, i) => {
    const tile = grid.children[i];
    const weight = state.weights[type];
    const avg = state.stats.categories[type];
    tile.classList.toggle("no-grades", avg === null);
    tile.querySelector(".cat-name").textContent = type;
    tile.querySelector(".cat-weight").innerHTML = `${weight}<span class="unit">%</span>`;
    tile.querySelector(".cat-avg").innerHTML = avg === null ? "No grades yet" : `Class avg <strong>${fmt(avg)}%</strong>`;
    tile.querySelector(".meter span").style.width = `${avg === null ? 0 : Math.min(avg, 100)}%`;
    tile.dataset.tipValue = avg === null ? "No grades yet" : `${fmt(avg)}% class average`;
    tile.dataset.tipLabel = `${type} · ${weight}% of the final grade`;
  });
}

// Horizontal bars, one per student. Rows are reused between renders so
// the bars can slide to their new length (try applying a curve!).
const barRows = new Map();
function renderAverages() {
  const chart = $("#avg-chart");
  const graded = state.students;
  if (!graded.length) {
    chart.innerHTML = `<p class="hbars-empty">Add students to see their averages here.</p>`;
    barRows.clear();
    return;
  }
  const highest = Math.max(100, ...graded.map((s) => s.average));
  const domain = highest > 100 ? 120 : 100;
  const pct = (v) => `${(Math.min(v, domain) / domain) * 100}%`;

  let grid = chart.querySelector(".hbar-grid");
  let rows = chart.querySelector(".hbar-rows");
  if (!grid) {
    chart.innerHTML = `<div class="hbar-grid" aria-hidden="true"></div><div class="hbar-rows"></div>`;
    grid = chart.querySelector(".hbar-grid");
    rows = chart.querySelector(".hbar-rows");
    barRows.clear();
  }
  const ticks = domain === 120 ? [0, 25, 50, 75, 100, 120] : [0, 25, 50, 75, 100];
  grid.innerHTML = ticks.map((t) => `<div class="gl" style="left:${pct(t)}"><span>${t}</span></div>`).join("") +
    `<div class="cut cut-needs" style="left:${pct(state.needsCutoff)}"><span>NI ${fmt(state.needsCutoff, 0)}</span></div>` +
    `<div class="cut cut-pass" style="left:${pct(state.passCutoff)}"><span>Pass ${fmt(state.passCutoff, 0)}</span></div>`;

  const seen = new Set();
  for (const s of graded) {
    seen.add(s.id);
    let row = barRows.get(s.id);
    const isNew = !row;
    if (isNew) {
      row = document.createElement("button");
      row.type = "button";
      row.className = "hbar-row";
      row.innerHTML = `<span class="hbar-name"></span><span class="hbar-track"><span class="hbar-fill" style="width:0"></span><span class="hbar-value" style="left:0"></span></span>`;
      row.addEventListener("click", () => openDrawer(s.id));
      barRows.set(s.id, row);
    }
    const st = slug(s.status);
    row.classList.toggle("selected", s.id === ui.selectedId);
    row.querySelector(".hbar-name").textContent = s.name;   // textContent = always safe
    const fill = row.querySelector(".hbar-fill");
    const value = row.querySelector(".hbar-value");
    fill.className = `hbar-fill f-${st}`;
    value.textContent = s.hasGrades ? fmt(s.average) : "–";
    row.dataset.tipValue = s.hasGrades ? `${fmt(s.average)}%` : "No grades";
    row.dataset.tipLabel = `${s.name} · ${s.status}`;
    row.setAttribute("aria-label", `${s.name}: ${s.hasGrades ? fmt(s.average) + "%" : "no grades"}, ${s.status}`);
    const target = s.hasGrades ? pct(s.average) : "0%";
    rows.appendChild(row);   // (re)appending puts rows in roster order
    if (isNew && !reducedMotion) {
      requestAnimationFrame(() => requestAnimationFrame(() => { fill.style.width = target; value.style.left = target; }));
    } else {
      fill.style.width = target;
      value.style.left = target;
    }
  }
  for (const [id, row] of barRows) {
    if (!seen.has(id)) { row.remove(); barRows.delete(id); }
  }
}

function trendHtml(trend) {
  if (trend === "up") return `<span class="trend trend-up" title="Latest grade is above their earlier average">${icon("i-up", "icon icon-sm")}<span class="visually-hidden">Trending up</span></span>`;
  if (trend === "down") return `<span class="trend trend-down" title="Latest grade is below their earlier average">${icon("i-down", "icon icon-sm")}<span class="visually-hidden">Trending down</span></span>`;
  if (trend === "steady") return `<span class="trend" title="Steady">${icon("i-flat", "icon icon-sm")}<span class="visually-hidden">Steady</span></span>`;
  return `<span class="trend" title="Needs 2+ grades">–</span>`;
}

function chipHtml(status) {
  const s = slug(status);
  return `<span class="chip chip-${s}">${icon(STATUS_ICON[s])}${escapeHtml(status)}</span>`;
}

function renderRoster() {
  const st = state.stats;
  const counts = { all: st.count, Passed: st.passed, "Needs Improvement": st.needsImprovement, Failed: st.failed };
  $$("[data-count]").forEach((el) => { el.textContent = counts[el.dataset.count]; });
  $$("#status-filter .seg").forEach((b) => b.classList.toggle("active", b.dataset.status === ui.status));

  const [field, dir] = ui.sort.split(" ");
  $$(".th-sort").forEach((b) => {
    b.classList.toggle("sorted", b.dataset.sort === field);
    b.classList.toggle("desc", b.dataset.sort === field && dir === "desc");
  });

  const visible = state.students.filter((s) =>
    (ui.status === "all" || s.status === ui.status) &&
    (ui.searchIds === null || ui.searchIds.has(s.id)));

  const filtered = ui.status !== "all" || ui.searchIds !== null;
  $("#roster-sub").textContent = filtered
    ? `Showing ${visible.length} of ${st.count} students`
    : `${st.count} student${st.count === 1 ? "" : "s"} · ${st.graded} graded`;

  $("#roster-body").innerHTML = visible.map((s) => {
    const isTop = s.id === st.topId;
    const isRisk = s.id === st.riskId;
    const sl = slug(s.status);
    return `<tr data-id="${s.id}" class="${isTop ? "is-top" : ""} ${isRisk ? "is-risk" : ""} ${s.id === ui.selectedId ? "selected" : ""}" tabindex="0" aria-label="Open ${escapeHtml(s.name)}">
      <td><div class="who"><span class="avatar" aria-hidden="true">${escapeHtml(initials(s.name))}</span>
        <div><div class="who-name">${escapeHtml(s.name)}
          ${isTop ? `<span class="badge badge-top">${icon("i-star")}Top</span>` : ""}
          ${isRisk ? `<span class="badge badge-risk">${icon("i-alert")}At risk</span>` : ""}</div>
        <div class="who-sub">${s.grades.length} grade${s.grades.length === 1 ? "" : "s"}</div></div></div></td>
      <td class="num"><span class="avg-cell"><span class="avg-num">${s.hasGrades ? fmt(s.average) : "–"}</span>
        <span class="mini-meter" aria-hidden="true"><span class="f-${sl}" style="width:${Math.min(s.average, 100)}%"></span></span></span></td>
      <td class="center"><span class="letter">${escapeHtml(s.letter)}</span></td>
      <td class="num hide-sm">${s.hasGrades ? fmt(s.gpa) : "–"}</td>
      <td class="center hide-sm">${trendHtml(s.trend)}</td>
      <td>${chipHtml(s.status)}</td>
      <td class="num hide-md muted">${s.id}</td>
      <td class="row-go">${icon("i-chevron-right")}</td>
    </tr>`;
  }).join("");

  const empty = $("#roster-empty");
  empty.hidden = visible.length > 0;
  if (!visible.length) {
    const noStudents = st.count === 0;
    $("#empty-title").textContent = noStudents ? "No students yet" : "No matches";
    $("#empty-sub").textContent = noStudents
      ? "Add your first student, load the demo class, or import a CSV."
      : "No students match this search and filter.";
    $("#empty-actions").innerHTML = noStudents
      ? `<button class="btn btn-primary" type="button" data-action="add">Add student</button>
         <button class="btn btn-secondary" type="button" data-action="demo">Load demo class</button>
         <button class="btn btn-ghost" type="button" data-action="import">Import CSV</button>`
      : `<button class="btn btn-secondary" type="button" data-action="clear-filters">Clear search &amp; filter</button>`;
  }
}

// ----- SEARCH (asks C++ "find" and keeps the IDs it lists) -----
function runSearch(text) {
  ui.searchText = text.trim();
  if (!ui.searchText) {
    ui.searchIds = null;
    return;
  }
  const out = cpp(`find ${quote(ui.searchText)}`);
  if (out.startsWith("Error")) {
    ui.searchIds = null;
    return;
  }
  // C++ prints lines like "  [1004] Diego Ramirez - 63.1% (D)"
  ui.searchIds = new Set([...out.matchAll(/\[(\d+)\]/g)].map((m) => Number(m[1])));
}

// ----- STUDENT DETAIL DRAWER -----
const drawer = $("#drawer");
let drawerReturnFocus = null;

function openDrawer(id) {
  if (ui.selectedId === null) drawerReturnFocus = document.activeElement;
  ui.selectedId = id;
  $("#need-result").hidden = true;
  $("#rename-form").hidden = true;
  drawer.classList.add("open");
  drawer.setAttribute("aria-hidden", "false");
  $("#scrim").hidden = false;
  renderDrawer();
  renderRoster();
  renderAverages();
  drawer.focus({ preventScroll: true });
}

function closeDrawer() {
  if (ui.selectedId === null) return;
  ui.selectedId = null;
  drawer.classList.remove("open");
  drawer.setAttribute("aria-hidden", "true");
  $("#scrim").hidden = true;
  hideTip();
  if (state) { renderRoster(); renderAverages(); }
  if (drawerReturnFocus && document.contains(drawerReturnFocus)) drawerReturnFocus.focus({ preventScroll: true });
}

function renderDrawer() {
  const s = studentById(ui.selectedId);
  if (!s) { closeDrawer(); return; }
  $("#d-avatar").textContent = initials(s.name);
  $("#d-name").textContent = s.name;
  $("#d-id").textContent = `Student ID ${s.id}`;
  $("#d-average").textContent = s.hasGrades ? fmt(s.average) : "–";
  $("#d-letter").textContent = s.letter;
  $("#d-gpa").textContent = s.hasGrades ? fmt(s.gpa) : "–";
  $("#d-trend").innerHTML = trendHtml(s.trend) + ` <span class="muted">${escapeHtml(s.trend === "none" ? "" : s.trend)}</span>`;
  $("#d-status").innerHTML = chipHtml(s.status);
  $("#d-count").textContent = `(${s.grades.length})`;

  $("#d-cats").innerHTML = Object.entries(state.weights).map(([type, weight]) => {
    const avg = s.categories[type];
    return `<tr class="${avg === null ? "no-grades" : ""}"><td>${escapeHtml(type)}</td><td class="num">${weight}%</td>
      <td class="num">${avg === null ? "–" : fmt(avg) + "%"}</td></tr>`;
  }).join("");

  $("#d-grades").innerHTML = s.grades.length
    ? s.grades.map((g, i) => `<li>
        <span class="g-num">#${i + 1}</span>
        <span class="g-main"><span class="g-label">${escapeHtml(g.label)}</span>
          <span class="g-type">${escapeHtml(g.type)}</span></span>
        <span class="g-score">${fmt(g.score)}</span>
        <span class="g-actions">
          <button class="btn btn-ghost btn-icon" type="button" data-edit="${i + 1}" aria-label="Edit grade ${i + 1}">${icon("i-pencil", "icon icon-sm")}</button>
          <button class="btn btn-ghost btn-icon" type="button" data-del="${i + 1}" aria-label="Delete grade ${i + 1}">${icon("i-trash", "icon icon-sm")}</button>
        </span></li>`).join("")
    : `<li class="muted" style="grid-template-columns:1fr">No grades yet.</li>`;
  drawLineChart();
}

// ----- the grade history line chart (SVG) -----
let chartPoints = [];
function drawLineChart() {
  const box = $("#d-chart");
  const s = studentById(ui.selectedId);
  if (!s) return;
  if (!s.grades.length) {
    box.innerHTML = `<div class="lc-empty">No grades yet. Add one below.</div>`;
    chartPoints = [];
    return;
  }
  const W = Math.max(box.clientWidth, 260);
  const H = box.clientHeight || 210;
  const m = { l: 32, r: 46, t: 14, b: 24 };
  const n = s.grades.length;
  const maxY = Math.max(100, ...s.grades.map((g) => g.score)) > 100 ? 120 : 100;
  const x = (i) => (n === 1 ? (m.l + W - m.r) / 2 : m.l + (i * (W - m.l - m.r)) / (n - 1));
  const y = (v) => m.t + (1 - v / maxY) * (H - m.t - m.b);

  const ticks = maxY === 120 ? [0, 50, 100, 120] : [0, 25, 50, 75, 100];
  let svg = ticks.map((t) =>
    `<line class="lc-grid" x1="${m.l}" x2="${W - m.r}" y1="${y(t)}" y2="${y(t)}"/><text class="lc-axis" x="${m.l - 8}" y="${y(t) + 4}" text-anchor="end">${t}</text>`).join("");
  svg += `<line class="lc-cut" x1="${m.l}" x2="${W - m.r}" y1="${y(state.passCutoff)}" y2="${y(state.passCutoff)}"/>`;
  svg += `<text class="lc-cut-label" x="${W - m.r + 6}" y="${y(state.passCutoff) + 4}">Pass</text>`;

  chartPoints = s.grades.map((g, i) => ({ x: x(i), ys: y(g.score), yr: y(g.running), g, i }));
  const scorePath = chartPoints.map((p, i) => `${i ? "L" : "M"}${p.x},${p.ys}`).join("");
  const runPath = chartPoints.map((p, i) => `${i ? "L" : "M"}${p.x},${p.yr}`).join("");
  if (n > 1) {
    svg += `<path class="lc-area" d="${scorePath}L${chartPoints[n - 1].x},${y(0)}L${chartPoints[0].x},${y(0)}Z"/>`;
  }
  svg += `<path class="lc-running" d="${runPath}"/><path class="lc-score" d="${scorePath}"/>`;
  if (n <= 40) svg += chartPoints.map((p) => `<circle class="lc-dot" cx="${p.x}" cy="${p.ys}" r="4"/>`).join("");
  const last = chartPoints[n - 1];
  svg += `<text class="lc-end" x="${last.x + 8}" y="${last.yr + 4}">${fmt(s.average)}</text>`;
  const labelEvery = Math.ceil(n / 8);
  svg += chartPoints.filter((p) => p.i % labelEvery === 0 || p.i === n - 1)
    .map((p) => `<text class="lc-axis" x="${p.x}" y="${H - 6}" text-anchor="middle">#${p.i + 1}</text>`).join("");
  svg += `<g class="lc-hover" visibility="hidden"><line class="lc-cross" y1="${m.t}" y2="${H - m.b}"/>
    <circle class="lc-hover-dot run" r="5"/><circle class="lc-hover-dot score" r="5.5"/></g>`;
  svg += `<rect class="lc-hit" x="0" y="0" width="${W}" height="${H}" fill="transparent"/>`;

  box.innerHTML = `<svg viewBox="0 0 ${W} ${H}" role="img" aria-label="Grade history for ${escapeHtml(s.name)}: ${n} grades, latest ${fmt(last.g.score)}">${svg}</svg>`;
}

// Crosshair: snap to the nearest grade and list both series in one tooltip.
$("#d-chart").addEventListener("pointermove", (e) => {
  if (!chartPoints.length) return;
  const svg = $("#d-chart svg");
  const rect = svg.getBoundingClientRect();
  const px = e.clientX - rect.left;
  let best = chartPoints[0];
  for (const p of chartPoints) if (Math.abs(p.x - px) < Math.abs(best.x - px)) best = p;
  const hover = svg.querySelector(".lc-hover");
  hover.setAttribute("visibility", "visible");
  hover.querySelector(".lc-cross").setAttribute("x1", best.x);
  hover.querySelector(".lc-cross").setAttribute("x2", best.x);
  hover.querySelector(".score").setAttribute("cx", best.x);
  hover.querySelector(".score").setAttribute("cy", best.ys);
  hover.querySelector(".run").setAttribute("cx", best.x);
  hover.querySelector(".run").setAttribute("cy", best.yr);
  showTip(`<span class="tt-label">#${best.i + 1} · ${escapeHtml(best.g.label)} (${escapeHtml(best.g.type)})</span>
    <span class="tt-row"><i class="tt-key" style="border-color:var(--accent)"></i><strong>${fmt(best.g.score)}</strong> score</span>
    <span class="tt-row"><i class="tt-key" style="border-color:var(--series-2)"></i><strong>${fmt(best.g.running)}</strong> running average</span>`,
  rect.left + best.x, rect.top + best.ys);
});
$("#d-chart").addEventListener("pointerleave", () => {
  const hover = $("#d-chart .lc-hover");
  if (hover) hover.setAttribute("visibility", "hidden");
  hideTip();
});

// ----- drawer actions -----
$("#d-close").addEventListener("click", closeDrawer);
$("#scrim").addEventListener("click", closeDrawer);

$("#d-rename").addEventListener("click", () => {
  const form = $("#rename-form");
  form.hidden = !form.hidden;
  if (!form.hidden) {
    $("#rename-input").value = studentById(ui.selectedId).name;
    $("#rename-input").select();
  }
});
$("#rename-cancel").addEventListener("click", () => { $("#rename-form").hidden = true; });
$("#rename-form").addEventListener("submit", (e) => {
  e.preventDefault();
  if (act(`rename ${ui.selectedId} ${quote($("#rename-input").value)}`).ok) $("#rename-form").hidden = true;
});

$("#d-remove").addEventListener("click", () => {
  const id = ui.selectedId;
  closeDrawer();
  act(`remove ${id}`);   // the toast has an Undo button
});

$("#d-grades").addEventListener("click", (e) => {
  const edit = e.target.closest("[data-edit]");
  const del = e.target.closest("[data-del]");
  if (del) {
    act(`delgrade ${ui.selectedId} ${del.dataset.del}`);
  } else if (edit) {
    const n = Number(edit.dataset.edit);
    const g = studentById(ui.selectedId).grades[n - 1];
    $("#edit-title").textContent = `Edit grade #${n}`;
    $("#edit-label").value = g.label;
    $("#edit-type").value = typeArg(g.type);
    $("#edit-score").value = g.score;
    $("#form-edit").dataset.n = n;
    $("#dlg-edit").showModal();
  }
});

$("#form-edit").addEventListener("submit", (e) => {
  if (e.submitter?.value !== "ok") return;
  e.preventDefault();
  const n = $("#form-edit").dataset.n;
  const cmd = `edit ${ui.selectedId} ${n} ${$("#edit-score").value} ${quote($("#edit-label").value)} ${$("#edit-type").value}`;
  if (act(cmd).ok) $("#dlg-edit").close();
});

$("#add-grade-form").addEventListener("submit", (e) => {
  e.preventDefault();
  const label = $("#g-label").value;
  const cmd = `grade ${ui.selectedId} ${quote(label)} ${$("#g-type").value} ${$("#g-score").value}`;
  if (act(cmd).ok) {
    $("#g-label").value = "";
    $("#g-score").value = "";
    $("#g-label").focus();
  }
});

// "What do I need?" -> C++ need command. The answer is shown as-is.
$("#need-form").addEventListener("submit", (e) => {
  e.preventDefault();
  const out = cpp(`need ${ui.selectedId} ${$("#need-type").value} ${$("#need-target").value || 0}`).trim();
  const box = $("#need-result");
  box.hidden = false;
  box.textContent = out.replace(/^Error:\s*/, "");
  box.className = "need-result " + (out.startsWith("Error") ? "err"
    : out.includes("not possible") ? "hard"
    : out.includes("already safe") ? "ok" : "");
});

// ----- DIALOGS -----

// Opens a yes/no dialog and resolves to true/false.
function confirmDialog(title, text, okLabel = "Confirm") {
  const dlg = $("#dlg-confirm");
  $("#confirm-title").textContent = title;
  $("#confirm-text").textContent = text;
  $("#confirm-ok").textContent = okLabel;
  dlg.returnValue = "";
  dlg.showModal();
  return new Promise((resolve) => {
    dlg.addEventListener("close", () => resolve(dlg.returnValue === "ok"), { once: true });
  });
}

// ----- add student -----
function openAddStudent() {
  $("#form-student").reset();
  $("#dlg-student").showModal();
  $("#new-name").focus();
}
$("#form-student").addEventListener("submit", (e) => {
  if (e.submitter?.value !== "ok") return;
  e.preventDefault();
  const grade = $("#new-grade").value.trim();
  if (act(`add ${quote($("#new-name").value)}${grade ? " " + grade : ""}`).ok) $("#dlg-student").close();
});

// ----- class assignment -----
function openAssignment() {
  if (!state.students.length) {
    toast("Add some students first.", "error");
    return;
  }
  $("#form-assign").reset();
  $("#as-list").innerHTML = state.students.map((s) => `
    <label class="as-row"><span class="avatar" aria-hidden="true">${escapeHtml(initials(s.name))}</span>
      <span>${escapeHtml(s.name)}</span>
      <input type="number" min="0" max="120" step="0.1" data-id="${s.id}" placeholder="skip" aria-label="Score for ${escapeHtml(s.name)}">
    </label>`).join("");
  $("#dlg-assign").showModal();
  $("#as-title").focus();
}
$("#as-fill-btn").addEventListener("click", () => {
  const value = $("#as-fill").value;
  if (value === "") return;
  $$("#as-list input").forEach((input) => { if (input.value === "") input.value = value; });
});
$("#form-assign").addEventListener("submit", (e) => {
  if (e.submitter?.value !== "ok") return;
  e.preventDefault();
  // "1001=88 1002= 1003=91"  (blank after = means "skip this student")
  const pairs = $$("#as-list input").map((input) => `${input.dataset.id}=${input.value.trim()}`).join(" ");
  const cmd = `assign ${quote($("#as-title").value)} ${$("#as-type").value} ${pairs}`;
  if (act(cmd).ok) $("#dlg-assign").close();
});

// ----- curve with a live C++ preview -----
function openCurve() {
  if (!state.stats.graded) {
    toast("There are no grades to curve yet.", "error");
    return;
  }
  $("#curve-range").value = 5;
  previewCurve();
  $("#dlg-curve").showModal();
}
function previewCurve() {
  const points = Number($("#curve-range").value);
  $("#curve-points").textContent = `+${fmt(points)} pts`;
  $("#curve-apply").disabled = points === 0;
  const out = cpp(`curve ${points} preview`, "auto");
  const m = out.match(/class average ([\d.]+) -> ([\d.]+), pass rate ([\d.]+) -> ([\d.]+)/);
  if (!m) return;
  $("#cp-avg-before").textContent = fmt(m[1]);
  $("#cp-avg-after").textContent = fmt(m[2]);
  $("#cp-pass-before").textContent = `${fmt(m[3], 0)}%`;
  $("#cp-pass-after").textContent = `${fmt(m[4], 0)}%`;
}
$("#curve-range").addEventListener("input", previewCurve);
$("#form-curve").addEventListener("submit", (e) => {
  if (e.submitter?.value !== "ok") return;
  e.preventDefault();
  if (act(`curve ${$("#curve-range").value}`).ok) $("#dlg-curve").close();
});

// ----- settings: class name, cutoffs, theme -----
function openSettings() {
  $("#set-name").value = state.className;
  $("#set-pass").value = state.passCutoff;
  $("#set-needs").value = state.needsCutoff;
  const theme = currentTheme();
  $$('input[name="theme"]').forEach((r) => { r.checked = r.value === theme; });
  updateCutoffPreview();
  $("#dlg-settings").showModal();
}
function updateCutoffPreview() {
  const pass = Number($("#set-pass").value);
  const needs = Number($("#set-needs").value);
  const box = $("#cutoff-preview");
  if (!(needs >= 0 && needs < pass && pass <= 100)) {
    box.innerHTML = `<div style="flex-grow:1;background:var(--bad-soft);color:var(--bad-text)">Needs-improvement cutoff must be below the pass cutoff</div>`;
    return;
  }
  box.innerHTML =
    `<div style="flex-grow:${needs};background:var(--bad);color:#fff">${needs > 12 ? `Failed &lt; ${fmt(needs, 0)}` : ""}</div>` +
    `<div style="flex-grow:${pass - needs};background:var(--warn);color:#0b0b0b">${pass - needs > 12 ? "Needs impr." : ""}</div>` +
    `<div style="flex-grow:${100 - pass};background:var(--good);color:#fff">${100 - pass > 12 ? `Passed ≥ ${fmt(pass, 0)}` : ""}</div>`;
}
$("#set-pass").addEventListener("input", updateCutoffPreview);
$("#set-needs").addEventListener("input", updateCutoffPreview);
$$('input[name="theme"]').forEach((r) => r.addEventListener("change", () => setTheme(r.value)));
$("#form-settings").addEventListener("submit", (e) => {
  if (e.submitter?.value !== "ok") return;
  e.preventDefault();
  const commands = [];
  const name = $("#set-name").value.trim();
  const pass = Number($("#set-pass").value);
  const needs = Number($("#set-needs").value);
  if (name !== state.className) commands.push(`classname ${quote(name)}`);
  if (pass !== state.passCutoff || needs !== state.needsCutoff) commands.push(`thresholds ${pass} ${needs}`);
  if (!commands.length) { $("#dlg-settings").close(); return; }
  const replies = [];
  for (const command of commands) {
    const out = cpp(command).trim();
    if (out.startsWith("Error")) {
      toast(out.replace(/^Error:\s*/, ""), "error");
      if (replies.length) refresh();
      return;
    }
    replies.push(out);
  }
  toast(replies.join(" "), "success", { label: "Undo", onClick: doUndo });
  refresh();
  $("#dlg-settings").close();
});

// ----- CSV EXPORT / IMPORT (the C++ export/import commands do the work) -----
function exportCsv() {
  const csv = cpp("export");
  const blob = new Blob([csv], { type: "text/csv;charset=utf-8" });
  const link = document.createElement("a");
  link.href = URL.createObjectURL(blob);
  link.download = `${state.className.replace(/[^A-Za-z0-9]+/g, "-").replace(/^-+|-+$/g, "") || "class"}-grades.csv`;
  // Chrome skipped the download when started inside the menu's click (DEBUG_LOG #11).
  setTimeout(() => {
    document.body.appendChild(link);
    link.click();
    link.remove();
    setTimeout(() => URL.revokeObjectURL(link.href), 10000);
  }, 0);
  toast(`Exported ${state.students.length} students to ${link.download}`, "success");
}

$("#file-input").addEventListener("change", async (e) => {
  const file = e.target.files[0];
  e.target.value = "";   // so choosing the same file again still triggers "change"
  if (!file) return;
  if (file.size > 2_000_000) {
    toast("That file is too big for a class roster (2 MB max).", "error");
    return;
  }
  const text = await file.text();
  act("import\n" + text);
});

// ----- THEME -----
function currentTheme() {
  return document.documentElement.dataset.theme || "system";
}
function setTheme(theme) {
  if (theme === "system") delete document.documentElement.dataset.theme;
  else document.documentElement.dataset.theme = theme;
  try {
    if (theme === "system") localStorage.removeItem(THEME_KEY);
    else localStorage.setItem(THEME_KEY, theme);
  } catch (e) { /* storage blocked: the theme just won't be remembered */ }
  const btn = $("#theme-btn");
  btn.querySelector("use").setAttribute("href", { system: "#i-monitor", light: "#i-sun", dark: "#i-moon" }[theme]);
  btn.setAttribute("aria-label", `Theme: ${theme} (click to change)`);
  btn.title = `Theme: ${theme}`;
}
$("#theme-btn").addEventListener("click", () => {
  const order = ["system", "light", "dark"];
  setTheme(order[(order.indexOf(currentTheme()) + 1) % order.length]);
});

// ----- TOASTS + TOOLTIP -----
function toast(message, type = "success", action = null) {
  const el = document.createElement("div");
  el.className = `toast toast-${type}`;
  el.setAttribute("role", type === "error" ? "alert" : "status");
  el.innerHTML = icon(type === "error" ? "i-x-circle" : "i-check-circle") + `<span class="toast-msg"></span>`;
  el.querySelector(".toast-msg").textContent = message;   // C++ text, inserted safely
  if (action) {
    const btn = document.createElement("button");
    btn.type = "button";
    btn.textContent = action.label;
    btn.addEventListener("click", () => { dismiss(); action.onClick(); });
    el.appendChild(btn);
  }
  const box = $("#toasts");
  box.appendChild(el);
  while (box.children.length > 4) box.firstElementChild.remove();
  let timer = setTimeout(dismiss, type === "error" ? 6000 : 4200);
  el.addEventListener("mouseenter", () => clearTimeout(timer));
  el.addEventListener("mouseleave", () => { timer = setTimeout(dismiss, 2000); });
  function dismiss() {
    clearTimeout(timer);
    el.classList.add("leaving");
    setTimeout(() => el.remove(), 250);
  }
}

const tip = $("#tooltip");
function showTip(html, x, y) {
  tip.innerHTML = html;
  tip.hidden = false;
  const w = tip.offsetWidth;
  const h = tip.offsetHeight;
  let left = x + 14;
  let top = y - h - 12;
  if (left + w > window.innerWidth - 8) left = x - w - 14;
  if (top < 8) top = y + 16;
  tip.style.left = `${Math.max(8, left)}px`;
  tip.style.top = `${top}px`;
}
function hideTip() {
  tip.hidden = true;
}
// Any element with data-tip-value gets a tooltip on hover and keyboard focus.
function tipFor(el) {
  return `<span class="tt-value">${escapeHtml(el.dataset.tipValue)}</span><span class="tt-label">${escapeHtml(el.dataset.tipLabel || "")}</span>`;
}
document.addEventListener("pointermove", (e) => {
  const el = e.target.closest?.("[data-tip-value]");
  if (el) showTip(tipFor(el), e.clientX, e.clientY);
  else if (!e.target.closest?.("#d-chart")) hideTip();
});
document.addEventListener("focusin", (e) => {
  const el = e.target.closest?.("[data-tip-value]");
  if (!el) return;
  const r = el.getBoundingClientRect();
  showTip(tipFor(el), r.left + r.width / 2, r.top);
});
document.addEventListener("focusout", hideTip);
window.addEventListener("scroll", hideTip, { passive: true });

// ----- C++ CONSOLE -----
const consoleLog = $("#console-log");
let commandCount = 0;
const cmdHistory = [];
let historyIndex = 0;

function logToConsole(command, output, ms, source) {
  commandCount++;
  const verb = command.split(/\s/)[0];
  $("#console-meta").textContent = `${commandCount} commands sent to run() · last: ${verb} (${ms.toFixed(2)} ms)`;

  // Dragging the curve slider or typing a search sends many commands in a
  // row; keep only the newest one of each so the log stays readable.
  const key = /^curve .*preview$/.test(command) ? "curve-preview" : verb === "find" ? "find" : "";
  const last = consoleLog.lastElementChild;
  if (key && last && last.dataset.key === key) last.remove();

  const entry = document.createElement("div");
  entry.className = `log-entry ${source}`;
  entry.dataset.key = key;
  const lines = command.split("\n");
  const shown = lines.length > 1 ? `${lines[0]}   (+${lines.length - 1} lines of CSV)` : command;
  entry.innerHTML = `<div class="log-cmd"><span class="who-tag ${source === "you" ? "you" : ""}">${source === "you" ? "you" : source === "auto" ? "auto" : "button"}</span><span class="txt"></span><span class="ms">${ms.toFixed(2)} ms</span></div>`;
  entry.querySelector(".txt").textContent = shown;

  const text = output.replace(/\n$/, "") || "(no output)";
  const pre = document.createElement("pre");
  pre.className = "log-out" + (text.startsWith("Error") ? " err" : "");
  pre.textContent = text;
  // Long background output (json/export) starts collapsed; anything you
  // typed yourself is always shown in full.
  if (source !== "you" && (text.length > 360 || text.split("\n").length > 10)) {
    const details = document.createElement("details");
    const summary = document.createElement("summary");
    summary.textContent = `${verb === "json" ? "JSON snapshot" : verb === "export" ? "CSV" : "Output"} · ${(text.length / 1024).toFixed(1)} KB · ${text.split("\n").length} line(s)`;
    details.append(summary, pre);
    entry.appendChild(details);
  } else {
    entry.appendChild(pre);
  }

  const nearBottom = consoleLog.scrollHeight - consoleLog.scrollTop - consoleLog.clientHeight < 60;
  consoleLog.appendChild(entry);
  while (consoleLog.children.length > 250) consoleLog.firstElementChild.remove();
  if (nearBottom) consoleLog.scrollTop = consoleLog.scrollHeight;
}

function toggleConsole(open) {
  const panel = $("#console-panel");
  const show = open ?? panel.hidden;
  panel.hidden = !show;
  $("#console").classList.toggle("open", show);
  document.body.classList.toggle("console-open", show);
  $("#console-toggle").setAttribute("aria-expanded", String(show));
  if (show) {
    consoleLog.scrollTop = consoleLog.scrollHeight;
    $("#console-cmd").focus({ preventScroll: true });
  }
}
$("#console-toggle").addEventListener("click", () => toggleConsole());
$("#console-clear").addEventListener("click", () => { consoleLog.innerHTML = ""; });
$("#console-auto").addEventListener("change", (e) => {
  const style = $("#console-auto-style") || document.head.appendChild(Object.assign(document.createElement("style"), { id: "console-auto-style" }));
  style.textContent = e.target.checked ? "" : ".log-entry.auto { display: none; }";
});

$("#console-form").addEventListener("submit", (e) => {
  e.preventDefault();
  const input = $("#console-cmd");
  const command = input.value.trim();
  if (!command) return;
  cmdHistory.push(command);
  historyIndex = cmdHistory.length;
  input.value = "";
  consoleLog.scrollTop = consoleLog.scrollHeight;
  cpp(command, "you");
  refresh();   // the command may have changed the class
});
$("#console-cmd").addEventListener("keydown", (e) => {
  if (e.key === "ArrowUp" && historyIndex > 0) {
    historyIndex--;
    e.target.value = cmdHistory[historyIndex];
    e.preventDefault();
  } else if (e.key === "ArrowDown") {
    historyIndex = Math.min(cmdHistory.length, historyIndex + 1);
    e.target.value = cmdHistory[historyIndex] || "";
    e.preventDefault();
  }
});

// ----- WIRING: buttons, menu, keyboard -----
async function runAction(action) {
  closeMenu();
  if (action === "add") openAddStudent();
  else if (action === "export") exportCsv();
  else if (action === "import") $("#file-input").click();
  else if (action === "settings") openSettings();
  else if (action === "console") toggleConsole();
  else if (action === "clear-filters") {
    ui.status = "all";
    $("#search").value = "";
    runSearch("");
    renderRoster();
  } else if (action === "demo") {
    if (state.students.length && !(await confirmDialog("Load the demo class?",
      "This replaces the current roster with 8 sample students. You can undo it.", "Load demo"))) return;
    act("demo");
  } else if (action === "clear") {
    if (!(await confirmDialog("Remove every student?",
      `All ${state.students.length} students and their grades will be removed. You can undo it.`, "Remove all"))) return;
    closeDrawer();
    act("clear");
  }
}

document.addEventListener("click", (e) => {
  const actionEl = e.target.closest("[data-action]");
  if (actionEl) runAction(actionEl.dataset.action);
});

$("#undo-btn").addEventListener("click", doUndo);
$("#add-student-btn").addEventListener("click", openAddStudent);
$("#assign-btn").addEventListener("click", openAssignment);
$("#curve-btn").addEventListener("click", openCurve);
$("#class-name-btn").addEventListener("click", openSettings);

// ----- the "more" menu -----
function closeMenu() {
  $("#menu").hidden = true;
  $("#menu-btn").setAttribute("aria-expanded", "false");
}
$("#menu-btn").addEventListener("click", (e) => {
  e.stopPropagation();
  const menu = $("#menu");
  menu.hidden = !menu.hidden;
  $("#menu-btn").setAttribute("aria-expanded", String(!menu.hidden));
  if (!menu.hidden) menu.querySelector("button").focus();
});
document.addEventListener("click", (e) => {
  if (!e.target.closest(".menu-wrap")) closeMenu();
});

// ----- roster interactions -----
$("#roster-body").addEventListener("click", (e) => {
  const row = e.target.closest("tr[data-id]");
  if (row) openDrawer(Number(row.dataset.id));
});
$("#roster-body").addEventListener("keydown", (e) => {
  const row = e.target.closest("tr[data-id]");
  if (row && (e.key === "Enter" || e.key === " ")) {
    e.preventDefault();
    openDrawer(Number(row.dataset.id));
  }
});
$("#status-filter").addEventListener("click", (e) => {
  const btn = e.target.closest("[data-status]");
  if (!btn) return;
  ui.status = btn.dataset.status;
  renderRoster();
});
$$(".status-fig").forEach((fig) => {
  fig.setAttribute("role", "button");
  fig.tabIndex = 0;
  const pick = () => {
    ui.status = ui.status === fig.dataset.status ? "all" : fig.dataset.status;
    renderRoster();
    $(".roster-card").scrollIntoView({ behavior: reducedMotion ? "auto" : "smooth", block: "start" });
  };
  fig.addEventListener("click", pick);
  fig.addEventListener("keydown", (e) => { if (e.key === "Enter" || e.key === " ") { e.preventDefault(); pick(); } });
});

let searchTimer = null;
$("#search").addEventListener("input", (e) => {
  clearTimeout(searchTimer);
  searchTimer = setTimeout(() => {
    runSearch(e.target.value);
    renderRoster();
  }, 140);
});

function applySort(value) {
  ui.sort = value;
  $("#sort-select").value = value;
  savePrefs();
  refresh();   // refresh() sends "sort ..." to C++ first
}
$("#sort-select").addEventListener("change", (e) => applySort(e.target.value));
$$(".th-sort").forEach((btn) => btn.addEventListener("click", () => {
  const field = btn.dataset.sort;
  const [current, dir] = ui.sort.split(" ");
  if (current === field) applySort(dir === "desc" ? field : `${field} desc`);
  else applySort(field === "average" ? "average desc" : field);
}));

// ----- keyboard shortcuts -----
document.addEventListener("keydown", (e) => {
  const typing = e.target.closest?.("input, textarea, select, [contenteditable]");
  const dialogOpen = document.querySelector("dialog[open]");
  if ((e.ctrlKey || e.metaKey) && !e.shiftKey && e.key.toLowerCase() === "z" && !typing && !dialogOpen) {
    e.preventDefault();
    doUndo();
  } else if (e.key === "/" && !typing && !dialogOpen) {
    e.preventDefault();
    $("#search").focus();
  } else if (e.key === "Escape" && !dialogOpen) {
    if (!$("#menu").hidden) closeMenu();
    else if (ui.selectedId !== null) closeDrawer();
  }
});

// Redraw width-dependent pieces when the layout changes size.
let resizeFrame = 0;
new ResizeObserver(() => {
  cancelAnimationFrame(resizeFrame);
  resizeFrame = requestAnimationFrame(() => {
    if (!state) return;
    renderStatus();
    if (ui.selectedId !== null) drawLineChart();
  });
}).observe(document.body);

// ----- BOOT: download the .wasm, start C++, restore saved data -----
async function boot() {
  setTheme(currentTheme());
  const bootEl = $("#boot");
  try {
    const started = performance.now();
    // fetch() + arrayBuffer() + WebAssembly.instantiate works even if the
    // server sends the wrong MIME type for .wasm files.
    const response = await fetch("gradebook.wasm", { cache: "no-cache" });
    if (!response.ok) throw new Error(`the server answered ${response.status} for gradebook.wasm`);
    const bytes = await response.arrayBuffer();
    $("#boot-sub").textContent = "Starting C++…";
    engine = await loadEngine(bytes, (text) => console.info("[C++ stdout]", text));
    const ms = performance.now() - started;
    bootInfo = `${(bytes.byteLength / 1024).toFixed(0)} KB · ready in ${ms.toFixed(0)} ms`;

    const restored = restore();
    refresh();
    $("#engine-pill").classList.add("ready");
    $("#engine-pill-text").textContent = "C++ engine running";
    $("#engine-pill").title = `gradebook.wasm (${bootInfo}). All grade logic runs in C++.`;
    bootEl.classList.add("done");
    if (restored) toast(`Welcome back! Restored ${state.students.length} students from this browser.`, "success");
  } catch (err) {
    console.error(err);
    bootEl.classList.add("error");
    $(".boot-title").textContent = "The C++ engine couldn't start";
    $("#boot-sub").textContent = location.protocol === "file:"
      ? "Browsers block WebAssembly on file:// pages. Run  python -m http.server 8000 -d web  and open http://localhost:8000, or use the Vercel link."
      : `Problem: ${err.message}. Try reloading the page.`;
  }
}

boot();

// Runs the REAL web/gradebook.wasm in Node through the same engine.js the
// website uses, and checks that every command behaves correctly.
//   Usage:  node tests/engine_test.mjs
import { readFileSync } from "node:fs";
import { loadEngine, quote } from "../web/engine.js";

const wasm = readFileSync(new URL("../web/gradebook.wasm", import.meta.url));
const stray = [];
const engine = await loadEngine(wasm, (text) => stray.push(text));
const run = (cmd) => engine.run(cmd);
const state = () => JSON.parse(run("json"));

let passed = 0;
let failed = 0;
function check(name, condition, detail = "") {
  if (condition) passed++;
  else failed++;
  console.log(`${condition ? "PASS" : "FAIL"}  ${name}${detail ? "  ->  " + detail : ""}`);
}

// ---- demo + json ----
let out = run("demo");
check("demo loads 8 students", out.trim() === "Loaded the demo class (8 students).", out.trim());
let s = state();
check("json parses and has 8 students", s.students.length === 8);
const ava = s.students.find((x) => x.name === "Ava Chen");
// Ava by hand: (95*.5 + 92 + 94*2 + 98*1.5 + 96 + 100*.5 + 97*2) / 8.5 = 814.5 / 8.5 = 95.82
check("Ava's weighted average is 95.82", ava.average === 95.82, String(ava.average));
check("Ava is the top student", s.stats.topId === ava.id);
const ethan = s.students.find((x) => x.name === "Ethan Brooks");
check("Ethan is Failed and most at-risk", ethan.status === "Failed" && s.stats.riskId === ethan.id,
  `${ethan.status} ${ethan.average}`);
const sumLetters = Object.values(s.stats.letters).reduce((a, b) => a + b, 0);
check("letter counts add up to 8", sumLetters === 8, JSON.stringify(s.stats.letters));
check("status counts add up to 8",
  s.stats.passed + s.stats.needsImprovement + s.stats.failed + s.stats.noGrades === 8);
check("Hana trend is up, Liam trend is down",
  s.students.find((x) => x.name === "Hana Kim").trend === "up" &&
  s.students.find((x) => x.name === "Liam O'Connor").trend === "down");

// ---- add with tricky names ----
out = run(`add ${quote('Zoe "Z" Park')} 88`);
check("add name containing quotes", /Added Zoe "Z" Park \(ID 1009\)/.test(out), out.trim());
out = run(`add ${quote("José Núñez")} 91.5`);
check("add name with accents (UTF-8)", out.includes("José Núñez"), out.trim());
out = run(`add ${quote("<img src=x onerror=alert(1)>")}`);
s = state();
check("HTML-looking name stored as plain text", s.students.some((x) => x.name === "<img src=x onerror=alert(1)>"));
check("student added without starting grade has status 'No Grades'",
  s.students.find((x) => x.id === 1011).status === "No Grades");

// ---- bad input ----
check("score 150 is rejected", run('grade 1001 "Quiz 9" quiz 150').startsWith("Error"));
check("type 'essay' is rejected", run('grade 1001 "Essay" essay 90').startsWith("Error"));
check("score 'abc' is rejected", run('grade 1001 "Quiz 9" quiz abc').startsWith("Error"));
check("unknown student is rejected", run('grade 4242 "Quiz 9" quiz 90').startsWith("Error"));
check("unknown command is rejected", run("dance").startsWith("Error: unknown command"));
check("empty name is rejected", run('add "" 90').startsWith("Error"));

// ---- grade / edit / delete ----
out = run('grade 1004 "Unit 3 Test" test 90');
check("grade adds to Diego", out.startsWith('Added Test "Unit 3 Test" (90.0) for Diego Ramirez'), out.trim());
let diego = state().students.find((x) => x.id === 1004);
check("Diego now has 8 grades", diego.grades.length === 8);
out = run('edit 1004 8 95 "Unit 3 Test (retake)" test');
diego = state().students.find((x) => x.id === 1004);
check("edit changes score and label", diego.grades[7].score === 95 && diego.grades[7].label === "Unit 3 Test (retake)", out.trim());
check("edit of grade #99 is rejected", run("edit 1004 99 50").startsWith("Error"));
out = run("delgrade 1004 8");
check("delgrade removes it", state().students.find((x) => x.id === 1004).grades.length === 7, out.trim());

// ---- need ----
out = run("need 1006 test 60");
// By hand: (60 * (8.5 + 2) - 52.06 * 8.5) / 2 = 93.75
check("need gives a number for Ethan (60% target)", /Ethan Brooks needs 93\.[78]% on the next Test to reach 60\.0%/.test(out), out.trim());
check("need says impossible when > 100", run("need 1006 quiz 99").includes("not possible"));
check("need says already safe for Ava at 50%", run("need 1001 homework 50").includes("already safe"));

// ---- class assignment (blank = skip) ----
const before = state().students.find((x) => x.id === 1002).grades.length;
out = run('assign "Pop Quiz" quiz 1001=100 1002= 1003=80');
s = state();
check("assign grades 2 students", out.includes("for 2 student(s)"), out.trim());
check("blank score skips Marcus", s.students.find((x) => x.id === 1002).grades.length === before);
check("bad score in assign changes nothing",
  run('assign "Bad" quiz 1001=90 1003=500').startsWith("Error") &&
  state().students.find((x) => x.id === 1001).grades.length === s.students.find((x) => x.id === 1001).grades.length);

// ---- curve preview / apply / undo ----
const avgBefore = state().stats.average;
out = run("curve 5 preview");
check("curve preview leaves data alone", state().stats.average === avgBefore, out.trim());
const m = out.match(/class average ([\d.]+) -> ([\d.]+), pass rate ([\d.]+) -> ([\d.]+)/);
check("curve preview reports before and after", m && Number(m[1]) === avgBefore && Number(m[2]) > avgBefore);
run("curve 5");
const curvedState = state();
check("curve applies the previewed average and pass rate",
  curvedState.stats.average === Number(m[2]) && curvedState.stats.passRate === Number(m[4]),
  `${curvedState.stats.average} vs ${m[2]}, ${curvedState.stats.passRate} vs ${m[4]}`);
out = run("undo");
check("undo reverts the curve", out.trim() === "Undid: curve." && state().stats.average === avgBefore, out.trim());

// ---- thresholds ----
run("thresholds 80 70");
s = state();
check("thresholds change statuses", s.passCutoff === 80 && s.students.find((x) => x.id === 1002).status !== "Passed");
check("backwards thresholds rejected", run("thresholds 50 60").startsWith("Error"));
run("undo");
check("undo restores cutoffs", state().passCutoff === 70);

// ---- rename / classname / sort / find / remove ----
run("rename 1003 Priya P.");
check("rename without quotes", state().students.find((x) => x.id === 1003).name === "Priya P.");
run(`classname ${quote("AP Chem, Period 3")}`);
check("classname with a comma", state().className === "AP Chem, Period 3");
run("sort average desc");
s = state();
check("sort average desc puts top student first", s.students[0].id === s.stats.topId);
run("sort name");
s = state();
check("sort name is alphabetical",
  s.students.map((x) => x.name.toLowerCase()).join() === s.students.map((x) => x.name.toLowerCase()).sort().join());
out = run("find an");
check("find 'an' returns Ethan and Hana by ID", /\[1006\] Ethan Brooks/.test(out) && /\[1007\] Hana Kim/.test(out), out.split("\n")[0]);
out = run("remove 1011");
check("remove deletes the student", !state().students.some((x) => x.id === 1011), out.trim());

// ---- CSV round trip ----
const snapshot = state();
const csv = run("export");
check("export has a header row", csv.startsWith("id,name,assignment,type,score\n"));
run("clear");
check("clear empties the class", state().students.length === 0);
out = run("import\n" + csv);
const reloaded = state();
check("import restores every student", reloaded.students.length === snapshot.students.length, out.trim());
check("import restores every grade and name exactly",
  JSON.stringify(reloaded.students.map((x) => [x.id, x.name, x.grades])) ===
  JSON.stringify(snapshot.students.map((x) => [x.id, x.name, x.grades])));
out = run("import\nthis,is,not\nvalid");
check("garbage import is rejected and changes nothing",
  out.startsWith("Error") && state().students.length === snapshot.students.length, out.trim());
out = run("import\nid,name,assignment,type,score\n1,Test Kid,Quiz 1,quiz,90\n2,Bad Row,Quiz 1,quiz,999\n");
check("import skips bad rows but keeps good ones", out.includes("1 bad row(s) skipped"), out.trim());
run("undo");

// ---- running average + resetundo ----
const hana = state().students.find((x) => x.name === "Hana Kim");
check("running average of grade #1 equals the first score", hana.grades[0].running === hana.grades[0].score);
check("running average of the last grade equals the overall average",
  hana.grades[hana.grades.length - 1].running === hana.average);
run("resetundo");
check("resetundo empties the undo history", !state().canUndo && run("undo").startsWith("Error"));

// ---- the cin state is reset between calls ----
let ok = true;
for (let i = 0; i < 300; i++) {
  if (!run("list").includes("students")) ok = false;
}
check("300 run() calls in a row all work (cin is reset each time)", ok);
check("nothing leaked to the real stdout", stray.length === 0, stray.join("|"));

console.log(`\n${passed} passed, ${failed} failed`);
process.exit(failed ? 1 : 0);

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
// Ava by hand: Homework 100, Quiz 97.5, Test (94+96+97)/3 = 95.67, Class Activity 100, no Project
//   (100*10 + 97.5*10 + 95.67*20 + 100*10) / 50 = 97.77
check("Ava's weighted average is 97.77", ava.average === 97.77, String(ava.average));
check("weights are the class's 5 categories",
  JSON.stringify(s.weights) === '{"Homework":10,"Quiz":10,"Test":20,"Project":50,"Class Activity":10}', JSON.stringify(s.weights));
check("Ava's category averages (Project empty)",
  ava.categories.Test === 95.67 && ava.categories.Project === null && ava.categories["Class Activity"] === 100,
  JSON.stringify(ava.categories));
check("class category averages: Test 76.04, Project none",
  s.stats.categories.Test === 76.04 && s.stats.categories.Project === null, JSON.stringify(s.stats.categories));
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
check("Diego now has 10 grades", diego.grades.length === 10);
out = run('edit 1004 10 95 "Unit 3 Test (retake)" test');
diego = state().students.find((x) => x.id === 1004);
check("edit changes score and label", diego.grades[9].score === 95 && diego.grades[9].label === "Unit 3 Test (retake)", out.trim());
check("edit of grade #99 is rejected", run("edit 1004 99 50").startsWith("Error"));
out = run("delgrade 1004 10");
check("delgrade removes it", state().students.find((x) => x.id === 1004).grades.length === 9, out.trim());

// ---- need ----
out = run("need 1006 project 70");
// By hand: the other categories give 45*10 + 57.5*10 + 48.33*20 + 70*10 = 2691.67 over weight 50,
//   so the Project must average (70 * 100 - 2691.67) / 50 = 86.17
check("need for Ethan's first Project (70% target)", /Ethan Brooks needs 86\.2% on the next Project to reach 70\.0%/.test(out), out.trim());
check("one Test can't get Ethan to 60% (needs 110)", run("need 1006 test 60").includes("110.0% on the next Test"));
check("need says impossible when > 100", run("need 1006 quiz 99").includes("not possible"));
check("need says already safe for Ava at 50%", run("need 1001 homework 50").includes("already safe"));

// ---- categories ----
out = run('grade 1006 "Circuit Project" project 90');
// (2691.67 + 90 * 50) / 100 = 71.92: one Project is half the grade
check("a Project counts for 50% once it exists", state().students.find((x) => x.id === 1006).average === 71.92, out.trim());
run("undo");
out = run('grade 1006 "Lab Day" activity 100');
check("'activity' means Class Activity", out.startsWith('Added Class Activity "Lab Day"'), out.trim());
run("undo");
check('quoted "Class Activity" works as a type', run('grade 1006 "Lab Day" "Class Activity" 100').startsWith("Added Class Activity"));
run("undo");

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

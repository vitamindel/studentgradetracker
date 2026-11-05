// Tests login, teacher/student accounts, and the database rules against Firebase's
// local emulators (no real Firebase project needed). Needs Java 21+. From the project folder:
//
//   npx firebase-tools emulators:exec --project demo-gradebook "node tests/cloud_test.mjs"
//
// It reads the database directly through the emulator's REST API to check what was saved.
import { chromium } from "playwright-core";
import { createServer } from "node:http";
import { readFile, mkdir } from "node:fs/promises";
import { extname, join, normalize } from "node:path";
import { fileURLToPath } from "node:url";

const WEB = fileURLToPath(new URL("../web/", import.meta.url));
const SHOTS = fileURLToPath(new URL("../build/shots/", import.meta.url));
await mkdir(SHOTS, { recursive: true });

const PROJECT = "demo-gradebook";
const AUTH = "http://127.0.0.1:9099";
const DB = `http://127.0.0.1:8080/v1/projects/${PROJECT}/databases/(default)/documents`;
const CONFIG = `export const firebaseConfig = { apiKey: "demo-key", authDomain: "${PROJECT}.firebaseapp.com", projectId: "${PROJECT}", appId: "demo" };`;

// ---- a tiny static server, with a config that points at the emulators ----
const TYPES = { ".html": "text/html", ".js": "text/javascript", ".css": "text/css", ".wasm": "application/wasm" };
const server = createServer(async (req, res) => {
  const path = normalize(decodeURIComponent(new URL(req.url, "http://x").pathname)).replace(/^([/\\])+/, "");
  const file = path || "index.html";
  if (file === "firebase-config.js") {
    res.writeHead(200, { "Content-Type": "text/javascript" });
    res.end(CONFIG);
    return;
  }
  try {
    const body = await readFile(join(WEB, file));
    res.writeHead(200, { "Content-Type": TYPES[extname(file)] || "application/octet-stream" });
    res.end(body);
  } catch {
    res.writeHead(404);
    res.end("not found");
  }
});
await new Promise((resolve) => server.listen(0, "127.0.0.1", resolve));
const URL_ROOT = `http://127.0.0.1:${server.address().port}/`;

// ---- emulator helpers ----
await fetch(`${AUTH}/emulator/v1/projects/${PROJECT}/accounts`, { method: "DELETE" });
await fetch(`http://127.0.0.1:8080/emulator/v1/projects/${PROJECT}/databases/(default)/documents`, { method: "DELETE" });

// Signs in through the REST API to get a user's ID and token.
async function restSignIn(email, password) {
  const res = await fetch(`${AUTH}/identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key=demo-key`, {
    method: "POST", headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ email, password, returnSecureToken: true }),
  });
  const data = await res.json();
  return { uid: data.localId, token: data.idToken };
}

// Firestore REST values -> plain JS values
function decode(v) {
  if ("stringValue" in v) return v.stringValue;
  if ("integerValue" in v) return Number(v.integerValue);
  if ("doubleValue" in v) return v.doubleValue;
  if ("nullValue" in v) return null;
  if ("timestampValue" in v) return v.timestampValue;
  if ("arrayValue" in v) return (v.arrayValue.values || []).map(decode);
  if ("mapValue" in v) return fields(v.mapValue.fields || {});
  return v;
}
const fields = (f) => Object.fromEntries(Object.entries(f).map(([k, v]) => [k, decode(v)]));

// "owner" is the emulator's admin token: it skips the security rules.
async function dbGet(path, token = "owner") {
  const res = await fetch(`${DB}/${path}`, { headers: { Authorization: `Bearer ${token}` } });
  return { status: res.status, body: await res.json() };
}
async function students(uid) {
  const { body } = await dbGet(`users/${uid}/students?pageSize=300`);
  return (body.documents || []).map((d) => fields(d.fields));
}
async function userDoc(uid) {
  const { status, body } = await dbGet(`users/${uid}`);
  return status === 200 ? fields(body.fields) : null;
}
// Writes one string field with a user's token; returns the HTTP status (403 = refused by the rules).
async function dbWrite(path, data, token) {
  const body = { fields: Object.fromEntries(Object.entries(data).map(([k, v]) => [k, { stringValue: v }])) };
  const res = await fetch(`${DB}/${path}`, {
    method: "PATCH", headers: { Authorization: `Bearer ${token}`, "Content-Type": "application/json" }, body: JSON.stringify(body),
  });
  return res.status;
}
// Checks a condition every 200 ms for up to 10 s (saves happen after a short pause).
async function waitUntil(condition, ms = 10000) {
  for (const end = Date.now() + ms; Date.now() < end; await new Promise((r) => setTimeout(r, 200))) {
    if (await condition()) return true;
  }
  return false;
}
async function restSignUp(email, password) {
  await fetch(`${AUTH}/identitytoolkit.googleapis.com/v1/accounts:signUp?key=demo-key`, {
    method: "POST", headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ email, password, returnSecureToken: true }),
  });
}

// ---- launch the installed browser ----
let browser;
for (const channel of ["chrome", "msedge"]) {
  try { browser = await chromium.launch({ channel, headless: true }); break; } catch { /* try the next one */ }
}
if (!browser) throw new Error("Could not find Chrome or Edge to run the cloud test.");

const context = await browser.newContext({ viewport: { width: 1440, height: 900 } });
const page = await context.newPage();

// Wrong-password tests make the emulator answer 400, which Chrome logs; those are expected.
const problems = [];
const expected = (text) => text.includes("status of 400");
page.on("console", (msg) => { if (msg.type() === "error" && !expected(msg.text())) problems.push(`console.error: ${msg.text()}`); });
page.on("pageerror", (err) => problems.push(`page error: ${err.message}`));

let passed = 0;
let failed = 0;
function check(name, condition, detail = "") {
  if (condition) passed++;
  else failed++;
  console.log(`${condition ? "PASS" : "FAIL"}  ${name}${detail !== "" ? "  ->  " + detail : ""}`);
}
const rows = () => page.locator("#roster-body tr").count();
const loginShown = () => page.locator("#login").isVisible();
const loginMsg = () => page.locator("#login-msg").innerText();
const lastToast = async () => (await page.locator(".toast .toast-msg").last().innerText()).trim();
const waitSaved = () => page.waitForFunction(() => document.querySelector("#sync-text").textContent === "Saved to cloud"
  && !document.querySelector("#sync-pill").classList.contains("is-saving"), null, { timeout: 15000 });
const saveSoon = async () => { await page.waitForTimeout(900); await waitSaved(); };   // 700 ms pause, then the save
async function typeCommand(command) {
  if (!(await page.locator("#console-cmd").isVisible())) await page.click("#console-toggle");
  await page.fill("#console-cmd", command);
  await page.press("#console-cmd", "Enter");
}
async function submitLogin(email, password) {
  await page.fill("#login-email", email);
  await page.fill("#login-password", password);
  await page.click("#login-submit");
}
async function signOut() {
  await page.click("#menu-btn");
  await page.click("#menu-signout");
  await page.waitForSelector("#login", { state: "visible", timeout: 15000 });
}

// ---------------------------------------------------------------------
// 1. Login screen
// ---------------------------------------------------------------------
await page.goto(URL_ROOT);
await page.waitForSelector("#login", { state: "visible", timeout: 20000 });
check("login screen shows when Firebase is set up", await loginShown());
await page.screenshot({ path: SHOTS + "cloud-01-login.png" });

await page.click("#login-submit");
check("empty form asks for email and password", (await loginMsg()) === "Type your email and password.");
await submitLogin("nobody@example.com", "whatever1");
await page.waitForSelector("#login-msg:not([hidden])");
await page.waitForFunction(() => document.querySelector("#login-msg").textContent !== "");
check("unknown account -> friendly error", (await loginMsg()) === "Wrong email or password.", await loginMsg());

await page.click("#login-switch");
check("switch to create-account mode", (await page.locator("#login-submit").innerText()) === "Create account");
await submitLogin("teacher1@example.com", "123");
await page.waitForFunction(() => document.querySelector("#login-msg").textContent.includes("6 characters"));
check("short password -> friendly error", (await loginMsg()) === "Passwords need at least 6 characters.");

// ---------------------------------------------------------------------
// 2. New account: demo class goes into the database
// ---------------------------------------------------------------------
await submitLogin("teacher1@example.com", "secret123");
await page.waitForSelector("#boot.done", { state: "attached", timeout: 20000 });
await page.waitForFunction(() => document.querySelectorAll("#roster-body tr").length > 0);
check("new account starts with the demo class", (await rows()) === 8);
check("welcome toast", (await lastToast()).startsWith("Welcome, teacher1@example.com!"), await lastToast());
await saveSoon();
const t1 = await restSignIn("teacher1@example.com", "secret123");
let saved = await students(t1.uid);
check("database has 8 student documents", saved.length === 8, saved.length);
const ava = saved.find((s) => s.name === "Ava Chen");
check("Ava's document: 9 grades, average 97.77 from C++, letter A",
  ava && ava.grades.length === 9 && ava.average === 97.77 && ava.letter === "A", JSON.stringify(ava && { n: ava.grades.length, avg: ava.average }));
let settings = await userDoc(t1.uid);
check("class settings document", settings.className === "Electronics/Computer Engineering 11" && settings.studentCount === 8
  && settings.passCutoff === 70 && settings.needsCutoff === 60, JSON.stringify(settings));
check("header pill says Saved to cloud", (await page.locator("#sync-text").innerText()) === "Saved to cloud");
await page.screenshot({ path: SHOTS + "cloud-02-signed-in.png" });

// ---------------------------------------------------------------------
// 3. Changes are saved: add, rename, remove
// ---------------------------------------------------------------------
await page.click("#add-student-btn");
await page.fill("#new-name", 'Kai "K" Lopez, Jr.');
await page.fill("#new-grade", "91");
await page.click('#form-student button[value="ok"]');
await saveSoon();
saved = await students(t1.uid);
const kai = saved.find((s) => s.id === 1009);
check("added student saved with a starting grade",
  saved.length === 9 && kai && kai.name === 'Kai "K" Lopez, Jr.' && kai.grades[0].score === 91 && kai.grades[0].type === "Quiz", JSON.stringify(kai));

await typeCommand('classname "Period 2 Engineering"');
await typeCommand("remove 1001");
await saveSoon();
saved = await students(t1.uid);
check("removed student's document is deleted", saved.length === 8 && !saved.some((s) => s.id === 1001));
check("class rename saved", (await userDoc(t1.uid)).className === "Period 2 Engineering");

// ---------------------------------------------------------------------
// 4. Reload: still signed in, class comes back from the database
// ---------------------------------------------------------------------
await page.reload();
await page.waitForSelector("#boot.done", { state: "attached", timeout: 20000 });
await page.waitForFunction(() => document.querySelectorAll("#roster-body tr").length > 0);
check("reload skips the login screen (still signed in)", !(await loginShown()));
check("roster loaded from the database", (await rows()) === 8 && (await page.locator("#roster-body").innerText()).includes('Kai "K" Lopez, Jr.'));
check("class name loaded from the database", (await page.locator("#class-name").innerText()) === "Period 2 Engineering");
check("loaded toast", (await lastToast()) === "Signed in as teacher1@example.com. Loaded 8 students from the database.", await lastToast());
check("undo can't undo the page load", await page.locator("#undo-btn").isDisabled());
check("cloud class is not copied into browser storage", await page.evaluate(() => localStorage.getItem("gradebookpp:v2") === null));
await page.click("#menu-btn");
check("menu shows the signed-in email", (await page.locator("#account-name").innerText()) === "teacher1@example.com");
await page.screenshot({ path: SHOTS + "cloud-03-menu.png" });
await page.keyboard.press("Escape");

// ---------------------------------------------------------------------
// 4b. Student login codes (teacher view)
// ---------------------------------------------------------------------
await page.click('#roster-body tr[data-id="1002"]');
const marcusCode = (await page.locator("#d-code").innerText()).trim();
check("student panel shows a login code", /^[A-HJ-NP-Z2-9]{4}-[A-HJ-NP-Z2-9]{4}$/.test(marcusCode), marcusCode);
const marcusDoc = (await students(t1.uid)).find((s) => s.id === 1002);
check("code is saved on the student's document", marcusDoc.joinCode === marcusCode.replace("-", ""), marcusDoc.joinCode);
const codeDoc = fields((await dbGet(`joinCodes/${marcusDoc.joinCode}`)).body.fields);
check("joinCodes document points at this teacher + student", codeDoc.teacherUid === t1.uid && codeDoc.studentId === "1002", JSON.stringify(codeDoc));
await page.screenshot({ path: SHOTS + "cloud-04-teacher-code.png" });
await page.click("#d-close");
await page.click("#menu-btn");
await page.click("#menu-codes");
check("codes list has every student", (await page.locator("#codes-body tr").count()) === 8);
await page.keyboard.press("Escape");

// ---------------------------------------------------------------------
// 5. Sign out, wrong password, second account, security rules
// ---------------------------------------------------------------------
await signOut();
check("sign out returns to the login screen", await loginShown());
await submitLogin("teacher1@example.com", "wrongpass1");
await page.waitForFunction(() => document.querySelector("#login-msg").textContent !== "");
check("wrong password -> friendly error", (await loginMsg()) === "Wrong email or password.", await loginMsg());

await page.click("#login-switch");
await submitLogin("teacher2@example.com", "secret456");
await page.waitForSelector("#boot.done", { state: "attached", timeout: 20000 });
await page.waitForFunction(() => document.querySelectorAll("#roster-body tr").length > 0);
check("second account gets its own class", (await rows()) === 8 && (await page.locator("#roster-body").innerText()).includes("Ava Chen"));
await saveSoon();
const t2 = await restSignIn("teacher2@example.com", "secret456");
check("first account's data is unchanged", (await students(t1.uid)).length === 8 && (await userDoc(t1.uid)).className === "Period 2 Engineering");
check("security rules: a user can read their own class", (await dbGet(`users/${t1.uid}`, t1.token)).status === 200);
check("security rules: a user CAN'T read someone else's class", (await dbGet(`users/${t1.uid}`, t2.token)).status === 403);
check("security rules: signed-out requests are refused", (await fetch(`${DB}/users/${t1.uid}`)).status === 403);

// ---------------------------------------------------------------------
// 6. Student account: sees only their own grades
// ---------------------------------------------------------------------
await signOut();
await page.click("#login-switch");
await page.click('label.seg:has(input[value="student"])');
check("picking Student shows the code box", await page.locator("#code-field").isVisible());
await page.fill("#login-code", "ZZZZ-ZZZZ");
await submitLogin("marcus@example.com", "secret111");
await page.waitForFunction(() => document.querySelector("#login-msg").textContent !== "");
check("wrong code -> friendly error", (await loginMsg()) === "That code doesn't match any student. Check it with your teacher.", await loginMsg());
check("no account is made with a wrong code", !(await restSignIn("marcus@example.com", "secret111")).uid);

await page.fill("#login-code", marcusCode.toLowerCase());   // codes work in any case
await submitLogin("marcus@example.com", "secret111");
await page.waitForFunction(() => document.body.classList.contains("student-mode") && document.querySelector("#boot").classList.contains("done"), null, { timeout: 15000 });
await page.waitForFunction(() => document.querySelector("#d-name").textContent === "Marcus Johnson");
check("student sees their own report card", (await page.locator("#d-name").innerText()) === "Marcus Johnson");
check("student sees no class roster or teacher tools", !(await page.locator(".page").isVisible()) && !(await page.locator("#add-student-btn").isVisible())
  && !(await page.locator("#console").isVisible()) && !(await page.locator("#add-grade-form").isVisible()) && !(await page.locator("#d-remove").isVisible()));
check("student can't edit or delete grades", (await page.locator(".g-actions").first().isVisible()) === false);
const marcusNow = (await students(t1.uid)).find((s) => s.id === 1002);
check("student's average comes from C++ and matches the teacher's", (await page.locator("#d-average").innerText()) === marcusNow.average.toFixed(1),
  `${await page.locator("#d-average").innerText()} vs ${marcusNow.average}`);
check("grade count matches the database", (await page.locator("#d-grades li").count()) === marcusNow.grades.length);
check("class name shown", (await page.locator("#class-name").innerText()) === "Period 2 Engineering");
check("header says Live grades", (await page.locator("#sync-text").innerText()) === "Live grades");
await page.selectOption("#need-type", "project");
await page.fill("#need-target", "80");
await page.click('#need-form button[type="submit"]');
check("student can use What do I need? (C++)", (await page.locator("#need-result").innerText()).includes("Marcus Johnson"), await page.locator("#need-result").innerText());
await page.screenshot({ path: SHOTS + "cloud-05-student-view.png", fullPage: true });

const marcus = await restSignIn("marcus@example.com", "secret111");
check("rules: student can read their own record", (await dbGet(`users/${t1.uid}/students/1002`, marcus.token)).status === 200);
check("rules: student CAN'T read another student", (await dbGet(`users/${t1.uid}/students/1003`, marcus.token)).status === 403);
check("rules: student CAN'T list the class", (await dbGet(`users/${t1.uid}/students`, marcus.token)).status === 403);
check("rules: student CAN'T change their grades", (await dbWrite(`users/${t1.uid}/students/1002?updateMask.fieldPaths=name`, { name: "Hacked" }, marcus.token)) === 403);
check("rules: student CAN'T relink to another student with their code",
  (await dbWrite(`accounts/${marcus.uid}`, { role: "student", teacherUid: t1.uid, studentId: "1003", code: marcusDoc.joinCode }, marcus.token)) === 403);

// The teacher adds a grade on another computer; the student's page updates by itself.
const teacherContext = await browser.newContext({ viewport: { width: 1280, height: 800 } });
const teacherPage = await teacherContext.newPage();
teacherPage.on("pageerror", (err) => problems.push(`teacher page error: ${err.message}`));
await teacherPage.goto(URL_ROOT);
await teacherPage.waitForSelector("#login", { state: "visible", timeout: 20000 });
await teacherPage.fill("#login-email", "teacher1@example.com");
await teacherPage.fill("#login-password", "secret123");
await teacherPage.click("#login-submit");
await teacherPage.waitForFunction(() => document.querySelectorAll("#roster-body tr").length > 0, null, { timeout: 15000 });
const gradesBefore = await page.locator("#d-grades li").count();
await teacherPage.click("#console-toggle");
await teacherPage.fill("#console-cmd", 'grade 1002 "Live Quiz" quiz 100');
await teacherPage.press("#console-cmd", "Enter");
await page.waitForFunction((n) => document.querySelectorAll("#d-grades li").length === n + 1, gradesBefore, { timeout: 15000 });
check("teacher's new grade appears on the student's page live", (await page.locator("#d-grades").innerText()).includes("Live Quiz"));
check("student is told about the update", (await lastToast()) === "Your teacher just updated your grades.", await lastToast());

// Removing the student cuts off access; undo brings back the same code.
await teacherPage.fill("#console-cmd", "remove 1002");
await teacherPage.press("#console-cmd", "Enter");
await page.waitForSelector("#login", { state: "visible", timeout: 15000 });
check("removed student loses access", (await loginMsg()).startsWith("Your grades aren't available."), await loginMsg());
check("removed student's code is deleted", (await dbGet(`joinCodes/${marcusDoc.joinCode}`)).status === 404);
await teacherPage.fill("#console-cmd", "undo");
await teacherPage.press("#console-cmd", "Enter");
check("undo restores the same code", await waitUntil(async () => (await dbGet(`joinCodes/${marcusDoc.joinCode}`)).status === 200),
  await teacherPage.locator("#sync-text").innerText());
await page.fill("#login-code", marcusCode);
await page.click("#login-submit");
await page.waitForFunction(() => document.querySelector("#login").hidden && document.body.classList.contains("student-mode"), null, { timeout: 15000 });
check("student gets back in with the code", (await page.locator("#d-name").innerText()) === "Marcus Johnson");

await page.reload();
await page.waitForFunction(() => document.body.classList.contains("student-mode") && document.querySelector("#boot").classList.contains("done"), null, { timeout: 15000 });
check("reload keeps the student view", (await page.locator("#d-name").innerText()) === "Marcus Johnson" && !(await loginShown()));
await page.click("#menu-btn");
check("student menu: account + sign out only", (await page.locator("#account-sub").innerText()) === "Student account · live grades"
  && !(await page.locator("#menu-codes").isVisible()) && !(await page.locator('[data-action="export"]').isVisible()));
await page.keyboard.press("Escape");

// A new roster reuses IDs from 1001, so every old code must stop working.
await teacherPage.fill("#console-cmd", "demo");
await teacherPage.press("#console-cmd", "Enter");
await page.waitForSelector("#login", { state: "visible", timeout: 15000 });
check("new roster (demo) cuts off old student accounts", (await loginMsg()).startsWith("Your grades aren't available."), await loginMsg());
check("old code is deleted", (await dbGet(`joinCodes/${marcusDoc.joinCode}`)).status === 404);
const newMarcus = (await students(t1.uid)).find((s) => s.id === 1002);
check("student 1002 gets a different code", newMarcus.joinCode && newMarcus.joinCode !== marcusDoc.joinCode);
await teacherContext.close();

// ---------------------------------------------------------------------
// 7. An account with no role yet (like a first Google sign-in) is asked
// ---------------------------------------------------------------------
await page.click("#login-signout");
await page.waitForSelector("#login-links", { state: "visible", timeout: 15000 });
await restSignUp("newteacher@example.com", "secret222");
await submitLogin("newteacher@example.com", "secret222");
await page.waitForFunction(() => document.querySelector("#login-title").textContent === "One more step", null, { timeout: 15000 });
check("no role yet -> asks teacher or student", await page.locator("#role-pick").isVisible() && !(await page.locator("#login-creds").isVisible()));
await page.screenshot({ path: SHOTS + "cloud-06-setup.png" });
await page.click('label.seg:has(input[value="teacher"])');
await page.click("#login-submit");
await page.waitForFunction(() => document.querySelectorAll("#roster-body tr").length > 0, null, { timeout: 15000 });
check("choosing Teacher opens the gradebook", (await rows()) === 8
  && !(await page.evaluate(() => document.body.classList.contains("student-mode"))));

// ---------------------------------------------------------------------
// 8. Guest mode, then making an account keeps the guest's class
// ---------------------------------------------------------------------
await signOut();
await page.evaluate(() => localStorage.clear());
await page.click("#login-guest");
await page.waitForFunction(() => document.querySelectorAll("#roster-body tr").length > 0);
check("guest mode works without an account", !(await loginShown()) && (await rows()) === 8);
check("guest pill offers sign-in", (await page.locator("#sync-text").innerText()) === "Sign in to save online");
await typeCommand('add "Guest Kid" 77');
await page.waitForTimeout(900);
check("guest class saved in this browser", await page.evaluate(() => (localStorage.getItem("gradebookpp:v2") || "").includes("Guest Kid")));
await page.click("#sync-pill");
check("guest pill opens the login screen", await loginShown());
check("login offers a way back", (await page.locator("#login-guest").innerText()) === "Back to my class");
await page.click("#login-switch");
await submitLogin("teacher3@example.com", "secret789");
await page.waitForFunction(() => document.querySelector("#sync-text").textContent === "Saved to cloud", null, { timeout: 15000 });
check("new account keeps the guest's class", (await rows()) === 9 && (await page.locator("#roster-body").innerText()).includes("Guest Kid"));
await saveSoon();
const t3 = await restSignIn("teacher3@example.com", "secret789");
check("guest's class uploaded to the new account", (await students(t3.uid)).some((s) => s.name === "Guest Kid"));

// ---------------------------------------------------------------------
// 9. Phone size
// ---------------------------------------------------------------------
await signOut();
await page.setViewportSize({ width: 390, height: 844 });
await page.waitForTimeout(200);
check("phone: login card fits with no sideways scroll", await page.evaluate(() => document.documentElement.scrollWidth <= 390
  && document.querySelector(".login-card").getBoundingClientRect().right <= 390));
await page.screenshot({ path: SHOTS + "cloud-07-phone-login.png" });
await page.click("#login-switch");
await page.click('label.seg:has(input[value="student"])');
check("phone: create-account form with student code fits", await page.evaluate(() => document.documentElement.scrollWidth <= 390));
await page.screenshot({ path: SHOTS + "cloud-08-phone-student-signup.png", fullPage: true });

check("no console errors or page errors", problems.length === 0, problems.join(" | "));
console.log(`\n${passed} passed, ${failed} failed   (screenshots in build/shots/)`);
await browser.close();
server.close();
process.exit(failed ? 1 : 0);

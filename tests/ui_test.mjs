// Clicks through every feature of the website in a real (headless) Chrome
// and fails if anything errors in the console.
//
//   cd tests && npm install && node ui_test.mjs
//
// Uses playwright-core with the Chrome (or Edge) already installed on the
// computer, so no browser download is needed. Screenshots go to build/shots/.
import { chromium } from "playwright-core";
import { createServer } from "node:http";
import { readFile, mkdir } from "node:fs/promises";
import { extname, join, normalize } from "node:path";
import { fileURLToPath } from "node:url";

const WEB = fileURLToPath(new URL("../web/", import.meta.url));
const SHOTS = fileURLToPath(new URL("../build/shots/", import.meta.url));
await mkdir(SHOTS, { recursive: true });

// ---- a tiny static server. NOTE: .wasm is sent with the WRONG MIME type on
// purpose, to prove the site still loads (it uses fetch + arrayBuffer).
const TYPES = { ".html": "text/html", ".js": "text/javascript", ".css": "text/css", ".wasm": "application/octet-stream" };
const server = createServer(async (req, res) => {
  const path = normalize(decodeURIComponent(new URL(req.url, "http://x").pathname)).replace(/^([/\\])+/, "");
  const file = path || "index.html";
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

// ---- launch the installed browser ----
let browser;
for (const channel of ["chrome", "msedge"]) {
  try { browser = await chromium.launch({ channel, headless: true }); break; } catch { /* try the next one */ }
}
if (!browser) throw new Error("Could not find Chrome or Edge to run the UI test.");

const context = await browser.newContext({ viewport: { width: 1440, height: 900 }, acceptDownloads: true });
// This test covers the no-login version; tests/cloud_test.mjs covers login + the database.
await context.route("**/firebase-config.js", (route) => route.fulfill({
  contentType: "text/javascript", body: 'export const firebaseConfig = { apiKey: "" };',
}));
const page = await context.newPage();

const problems = [];
page.on("console", (msg) => { if (msg.type() === "error") problems.push(`console.error: ${msg.text()}`); });
page.on("pageerror", (err) => problems.push(`page error: ${err.message}`));
page.on("requestfailed", (req) => {
  if (!req.url().includes("fonts.g")) problems.push(`request failed: ${req.url()} ${req.failure()?.errorText}`);
});

let passed = 0;
let failed = 0;
function check(name, condition, detail = "") {
  if (condition) passed++;
  else failed++;
  console.log(`${condition ? "PASS" : "FAIL"}  ${name}${detail !== "" ? "  ->  " + detail : ""}`);
}
const rows = () => page.locator("#roster-body tr").count();
const text = (sel) => page.locator(sel).first().innerText();
const lastToast = async () => (await page.locator(".toast .toast-msg").last().innerText()).trim();
const settle = () => page.waitForTimeout(250);

// ---------------------------------------------------------------------
// 1. First visit: the demo class loads from C++
// ---------------------------------------------------------------------
// Wipe saved data before the page's own scripts run (only on the very first
// load in this tab), so this really is a first visit.
await page.addInitScript(() => {
  if (!sessionStorage.getItem("wiped")) { localStorage.clear(); sessionStorage.setItem("wiped", "1"); }
});
await page.goto(URL_ROOT);
await page.waitForSelector("#boot.done", { state: "attached", timeout: 20000 });
check("first visit shows no 'Welcome back' toast", (await page.locator(".toast").count()) === 0);
await page.waitForTimeout(900);   // let number animations finish
check("engine boots and pill says running", (await text("#engine-pill-text")) === "C++ engine running");
check("first visit loads the 8-student demo", (await rows()) === 8, await rows());
check("class average tile shows 80.7", (await text("#stat-average")) === "80.7", await text("#stat-average"));
check("hero letter is B", (await text("#hero-letter")) === "B");
check("pass rate 75%", (await text("#stat-passrate")) === "75");
check("status counts 6 / 1 / 1",
  `${await text("#count-passed")}/${await text("#count-needs")}/${await text("#count-failed")}` === "6/1/1");
check("8 bars in the averages chart", (await page.locator(".hbar-row").count()) === 8);
check("letter chart shows A=2", (await page.locator('.col[data-letter="A"] .col-value').innerText()) === "2");
check("categories card shows the 5 weights from C++",
  (await page.locator("#cat-grid .cat-weight").allInnerTexts()).join(" ").replace(/\s+/g, "") === "10%10%20%50%10%");
check("empty Project category says so", (await page.locator("#cat-grid .cat").nth(3).innerText()).includes("No grades yet"));
check("category dropdowns are filled from C++", (await page.locator("#g-type option").count()) === 5 &&
  (await page.locator("#g-type").inputValue()) === "test");
check("top student badge on Ava", (await page.locator("tr.is-top").innerText()).includes("Ava Chen"));
check("at-risk badge on Ethan", (await page.locator("tr.is-risk").innerText()).includes("Ethan Brooks"));
check("undo starts disabled (resetundo after load)", await page.locator("#undo-btn").isDisabled());
await page.screenshot({ path: SHOTS + "01-desktop-light.png", fullPage: true });

// ---------------------------------------------------------------------
// 2. Add a student with a name that LOOKS like HTML (must stay plain text)
// ---------------------------------------------------------------------
const trickyName = `<b>Zoe</b> "Z" O'Neil`;
await page.click("#add-student-btn");
await page.fill("#new-name", trickyName);
await page.fill("#new-grade", "105");
await page.click('#form-student button[value="ok"]');
check("add student: starting grade over 100 is blocked",
  (await page.locator("#dlg-student").evaluate((d) => d.open)) && (await rows()) === 8);
await page.fill("#new-grade", "88");
await page.click('#form-student button[value="ok"]');
await settle();
check("add student: toast is C++'s reply", (await lastToast()).startsWith(`Added ${trickyName} (ID 1009)`), await lastToast());
check("add student: 9 rows", (await rows()) === 9);
check("tricky name shown as text, no <b> injected",
  (await page.locator("#roster-body b").count()) === 0 && (await page.locator("#roster-body").innerText()).includes(trickyName));
check("undo is now enabled", !(await page.locator("#undo-btn").isDisabled()));

// ---------------------------------------------------------------------
// 3. Search (C++ find), status filter, sorting (C++ sort)
// ---------------------------------------------------------------------
await page.fill("#search", "an");
await page.waitForTimeout(400);
const searchNames = await page.locator("#roster-body .who-name").allInnerTexts();
check("search 'an' shows Ethan + Hana (from C++ find)", searchNames.length === 2 &&
  searchNames.some((n) => n.includes("Ethan")) && searchNames.some((n) => n.includes("Hana")), searchNames.join(" | "));
await page.fill("#search", "");
await page.waitForTimeout(400);
check("clearing search shows everyone", (await rows()) === 9);

await page.click('#status-filter [data-status="Failed"]');
check("filter Failed shows 1 row", (await rows()) === 1);
await page.click('#status-filter [data-status="all"]');

await page.selectOption("#sort-select", "average desc");
await settle();
check("sort average high->low puts Ava first", (await page.locator("#roster-body tr").first().innerText()).includes("Ava Chen"));
check("averages chart follows the sort", (await page.locator(".hbar-row .hbar-name").first().innerText()) === "Ava Chen");
await page.click('.th-sort[data-sort="name"]');
await settle();
const sortedNames = await page.$$eval("#roster-body tr", (trs) => trs.map((t) => t.getAttribute("aria-label").replace(/^Open /, "")));
const expected = [...sortedNames].sort((a, b) => (a.toLowerCase() < b.toLowerCase() ? -1 : 1));
check("click Student header sorts by name (C++ compares lowercase text)", JSON.stringify(sortedNames) === JSON.stringify(expected),
  sortedNames.slice(0, 3).join(" | "));

// ---------------------------------------------------------------------
// 4. Student detail drawer: chart, add/edit/delete grade, need, rename
// ---------------------------------------------------------------------
await page.locator("#roster-body tr", { hasText: "Ethan Brooks" }).click();
await page.waitForSelector("#drawer.open");
await settle();
check("drawer opens with Ethan", (await text("#d-name")) === "Ethan Brooks");
check("line chart drew score + running paths", (await page.locator("#d-chart path.lc-score").count()) === 1 &&
  (await page.locator("#d-chart path.lc-running").count()) === 1);
check("grade list has 9 grades", (await page.locator("#d-grades li").count()) === 9);
check("drawer category table shows Ethan's Test average", (await page.locator("#d-cats tr").nth(2).innerText()).includes("48.3%"),
  await page.locator("#d-cats tr").nth(2).innerText());
await page.locator("#d-chart").hover({ position: { x: 200, y: 100 } });
check("chart crosshair tooltip appears", await page.locator("#tooltip").isVisible());

await page.fill("#g-label", "Unit 3 Retest");
await page.selectOption("#g-type", "test");
await page.fill("#g-score", "95");
await page.click('#add-grade-form button[type="submit"]');
await settle();
check("add grade -> 10 grades", (await page.locator("#d-grades li").count()) === 10, await lastToast());

await page.locator('#d-grades [data-edit="10"]').click();
await page.fill("#edit-score", "90");
await page.click('#form-edit button[value="ok"]');
await settle();
check("edit grade #10 -> 90.0", (await page.locator("#d-grades li").nth(9).locator(".g-score").innerText()) === "90.0", await lastToast());

await page.locator('#d-grades [data-del="10"]').click();
await settle();
check("delete grade #10 -> 9 grades", (await page.locator("#d-grades li").count()) === 9, await lastToast());

await page.selectOption("#need-type", "project");
await page.fill("#need-target", "70");
await page.click('#need-form button[type="submit"]');
check("what-do-I-need answer comes from C++", (await text("#need-result")).startsWith("Ethan Brooks needs 86.2% on the next Project"),
  await text("#need-result"));
await page.fill("#need-target", "99");
await page.click('#need-form button[type="submit"]');
check("impossible target is explained", (await text("#need-result")).includes("not possible"));

await page.click("#d-rename");
await page.fill("#rename-input", "Ethan B.");
await page.click('#rename-form button[type="submit"]');
await settle();
check("rename updates the drawer", (await text("#d-name")) === "Ethan B.");
await page.keyboard.press("Escape");
await settle();
check("Escape closes the drawer", !(await page.locator("#drawer").evaluate((d) => d.classList.contains("open"))));

// ---------------------------------------------------------------------
// 5. Class assignment modal (blank = skip)
// ---------------------------------------------------------------------
await page.click("#assign-btn");
await page.fill("#as-title", "Pop Quiz");
await page.selectOption("#as-type", "quiz");
await page.fill("#as-fill", "85");
await page.click("#as-fill-btn");
await page.locator("#as-list input").first().fill("");   // first student skips
await page.click('#form-assign button[value="ok"]');
await settle();
check("assignment graded 8 of 9 students", (await lastToast()).includes("for 8 student(s)"), await lastToast());

// ---------------------------------------------------------------------
// 6. Curve with live preview, then Ctrl+Z and the Undo button
// ---------------------------------------------------------------------
const avgBeforeCurve = Number(await page.locator("#stat-average").getAttribute("data-value"));
await page.click("#curve-btn");
await page.locator("#curve-range").fill("10");
await settle();
const cpBefore = Number(await text("#cp-avg-before"));
const cpAfter = Number(await text("#cp-avg-after"));
check("curve preview shows a higher average", cpAfter > cpBefore, `${cpBefore} -> ${cpAfter}`);
check("preview did not change the class", Number(await page.locator("#stat-average").getAttribute("data-value")) === avgBeforeCurve);
await page.click('#form-curve button[value="ok"]');
await settle();
const avgAfterCurve = Number(await page.locator("#stat-average").getAttribute("data-value"));
check("applying the curve matches the preview", Math.abs(avgAfterCurve - cpAfter) < 0.06, `${avgAfterCurve} vs ${cpAfter}`);
await page.waitForTimeout(800);
await page.screenshot({ path: SHOTS + "02-after-curve.png" });

await page.locator("body").click({ position: { x: 5, y: 300 } });   // make sure no input has focus
await page.keyboard.press("Control+z");
await settle();
check("Ctrl+Z undoes the curve", Number(await page.locator("#stat-average").getAttribute("data-value")) === avgBeforeCurve,
  await lastToast());
await page.click("#undo-btn");
await settle();
check("Undo button undoes the assignment", (await lastToast()) === "Undid: class assignment.", await lastToast());

// ---------------------------------------------------------------------
// 7. Settings: class name + cutoffs
// ---------------------------------------------------------------------
const passedBefore = await text("#count-passed");
await page.click("#class-name-btn");
await page.fill("#set-name", "AP Chemistry, Period 3");
await page.fill("#set-pass", "80");
await page.fill("#set-needs", "70");
await page.click('#form-settings button[value="ok"]');
await settle();
check("class name updates in the header", (await text("#class-name")) === "AP Chemistry, Period 3");
check("stricter cutoffs change the status counts", (await text("#count-passed")) !== passedBefore,
  `${passedBefore} -> ${await text("#count-passed")}`);
check("cutoff lines moved", (await page.locator(".cut-pass span").innerText()) === "Pass 80");

// ---------------------------------------------------------------------
// 8. CSV export (download) and import (file upload)
// ---------------------------------------------------------------------
await page.click("#menu-btn");
const [download] = await Promise.all([page.waitForEvent("download"), page.click('[data-action="export"]')]);
const exported = await (await download.createReadStream()).toArray().then((chunks) => Buffer.concat(chunks).toString("utf8"));
check("export downloads the C++ CSV", exported.startsWith("id,name,assignment,type,score\n") && exported.includes("Ethan B."),
  download.suggestedFilename());

const importCsv = "id,name,assignment,type,score\n2001,Imported One,Quiz 1,quiz,91\n2002,Imported Two,Quiz 1,quiz,64\n2003,\"Three, Imported\",Quiz 1,quiz,77\n";
await page.setInputFiles("#file-input", { name: "roster.csv", mimeType: "text/csv", buffer: Buffer.from(importCsv) });
await settle();
check("import replaces the roster with 3 students", (await rows()) === 3, await lastToast());
check("quoted CSV name with a comma survives", (await page.locator("#roster-body").innerText()).includes("Three, Imported"));
await page.click("#undo-btn");
await settle();
check("undo brings the old roster back", (await rows()) === 9);

// ---------------------------------------------------------------------
// 9. The C++ console: typed commands go straight to run()
// ---------------------------------------------------------------------
await page.click("#console-toggle");
await page.fill("#console-cmd", "stats");
await page.press("#console-cmd", "Enter");
await settle();
const consoleText = await page.locator("#console-log").innerText();
check("typed 'stats' shows C++ output", consoleText.includes("=== Class Statistics: AP Chemistry, Period 3 ==="));
await page.fill("#console-cmd", 'add "Console Kid" 70');
await page.press("#console-cmd", "Enter");
await settle();
check("typed add command updates the roster", (await rows()) === 10);
check("console logs button commands too", consoleText.includes("thresholds 80 70") && consoleText.includes("json"));
await page.screenshot({ path: SHOTS + "03-console-open.png" });
await page.click("#console-toggle");

// ---------------------------------------------------------------------
// 10. Remove with the toast's Undo button
// ---------------------------------------------------------------------
await page.locator("#roster-body tr", { hasText: "Console Kid" }).click();
await page.waitForSelector("#drawer.open");
await page.click("#d-remove");
await settle();
check("remove student -> 9 rows", (await rows()) === 9, await lastToast());
await page.locator(".toast button", { hasText: "Undo" }).last().click();
await settle();
check("toast Undo restores the student", (await rows()) === 10);

// ---------------------------------------------------------------------
// 11. Dark mode
// ---------------------------------------------------------------------
await page.click("#theme-btn");   // system -> light
await page.click("#theme-btn");   // light -> dark
check("theme toggle sets data-theme=dark", (await page.evaluate(() => document.documentElement.dataset.theme)) === "dark");
await page.waitForTimeout(500);   // the background fades over 0.3s
check("dark surface color applied", (await page.evaluate(() => getComputedStyle(document.body).backgroundColor)) === "rgb(13, 13, 13)");
await page.screenshot({ path: SHOTS + "04-desktop-dark.png", fullPage: true });

// ---------------------------------------------------------------------
// 12. Saving: reload and everything is still there
// ---------------------------------------------------------------------
await page.waitForTimeout(900);   // the save is debounced
await page.reload();
await page.waitForSelector("#boot.done", { state: "attached", timeout: 20000 });
await settle();
check("reload restores all 10 students", (await rows()) === 10);
check("reload restores the class name", (await text("#class-name")) === "AP Chemistry, Period 3");
check("reload restores cutoffs", (await page.locator(".cut-pass span").innerText()) === "Pass 80");
check("reload restores the theme", (await page.evaluate(() => document.documentElement.dataset.theme)) === "dark");
check("undo is disabled right after reload", await page.locator("#undo-btn").isDisabled());

// ---------------------------------------------------------------------
// 13. Clear all -> empty state -> load demo from the empty state
// ---------------------------------------------------------------------
await page.click("#menu-btn");
await page.click('#menu [data-action="clear"]');
await page.click("#confirm-ok");
await settle();
check("clear all shows the empty state", (await rows()) === 0 && (await page.locator("#roster-empty").isVisible()));
await page.screenshot({ path: SHOTS + "05-empty-dark.png" });
await page.click('#empty-actions [data-action="demo"]');
await settle();
check("demo button in empty state reloads 8 students", (await rows()) === 8);

// ---------------------------------------------------------------------
// 14. Light theme back, then Chromebook + phone sizes
// ---------------------------------------------------------------------
await page.click("#theme-btn");   // dark -> system
await page.emulateMedia({ colorScheme: "light" });
for (const [name, width, height] of [["06-chromebook", 1366, 768], ["07-phone", 390, 844]]) {
  await page.setViewportSize({ width, height });
  await page.waitForTimeout(700);
  const overflow = await page.evaluate(() => document.documentElement.scrollWidth - window.innerWidth);
  check(`${name}: no horizontal page scroll`, overflow <= 0, `${overflow}px`);
  await page.screenshot({ path: SHOTS + `${name}.png`, fullPage: name === "07-phone" });
}
await page.locator("#roster-body tr").first().click();
await page.waitForSelector("#drawer.open");
await page.waitForTimeout(500);
await page.screenshot({ path: SHOTS + "08-phone-drawer.png" });
await page.keyboard.press("Escape");

// ---------------------------------------------------------------------
check("no console errors, page errors, or failed requests", problems.length === 0, problems.join(" || "));
console.log(`\n${passed} passed, ${failed} failed   (screenshots in build/shots/)`);
await browser.close();
server.close();
process.exit(failed ? 1 : 0);

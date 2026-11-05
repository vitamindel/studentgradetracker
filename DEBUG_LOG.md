# Debug Log

A running list of every real problem we hit while building the Student Grade Tracker, in the order it happened. Each entry says **what happened**, **why**, and **how we fixed it**. The ones marked ⭐ are the best stories for the report and video.

---

## Stage 1: C++ core + console version

### 1. The starter headers weren't in the project folder
- **What happened:** The plan said `src/Student.h`, `src/GradeBook.h`, and `src/CommandShell.h` already existed, but the project folder was empty. A search of Downloads, Desktop, Documents, and OneDrive found nothing either.
- **Cause:** The files were never copied to this computer (they may be on a partner's machine).
- **Fix:** We rewrote all three from the written design: same class names, same `Student(name, id, startingGrade)` constructor, the same weights (Test 2x, Project 1.5x, Quiz 1x, Homework 0.5x), and the same command list.

### 2. No C++ compiler on the laptop
- **What happened:** `g++ --version` gave `g++: command not found`. Neither `clang` nor Emscripten (`emcc`) was installed.
- **Cause:** Windows doesn't come with a C++ compiler.
- **Fix:** We installed two tools:
  - `pip install ziglang` provides `python -m ziglang c++`. This is a full clang compiler that can build Windows `.exe` files **and** WebAssembly, so one install covers both. It was much easier than installing Emscripten.
  - Inside WSL (the Ubuntu that runs inside Windows): `apt-get install g++`. That gives the real `g++ -std=c++17 -Wall -Wextra` the assignment asks for.

### 3. WSL said "The system cannot find the path specified"
- **What happened:** `wsl.exe -d Ubuntu --cd /mnt/c/Users/.../cppproject -- g++ ...` failed with `Error code: Wsl/ERROR_PATH_NOT_FOUND`, even though `ls /mnt/c/Users/.../cppproject` worked fine inside WSL.
- **Cause:** We ran the command from Git Bash. Git Bash "helpfully" rewrites any argument that looks like a Linux path (`/mnt/c/...`) into a Windows path (`C:/Program Files/Git/mnt/c/...`) before passing it on. WSL got a path that doesn't exist.
- **Fix:** Put the `cd` *inside* the Linux command string, where Git Bash doesn't touch it:
  `wsl.exe -d Ubuntu -- bash -c "cd /mnt/c/.../cppproject && g++ ..."`

### 4. ⭐ `warning: #pragma once in main file` in the OnlineGDB single file
- **What happened:** The normal build (`main.cpp` + headers) compiled with **zero** warnings. The generated one-file version (`onlinegdb/GradeTracker_SingleFile.cpp`) printed:
  ```
  GradeTracker_SingleFile.cpp:27:9: warning: #pragma once in main file
     27 | #pragma once   // "only include this file once" (stops duplicate definitions)
  ```
- **Cause:** Our Python script that glues the headers into one file was supposed to delete every `#pragma once` line. It used `if line.strip() == "#pragma once"`, which only matches the line when it is *exactly* that text. In `Student.h` we had added a comment on the same line, so the line didn't match, wasn't deleted, and landed in the middle of a `.cpp` file where `#pragma once` makes no sense.
- **Fix:** Match the start of the line with a regular expression instead: `^\s*#pragma\s+once\b`. That handles extra spaces and trailing comments. After regenerating, the single file compiles with 0 warnings, and it prints *byte-for-byte* the same output as the multi-file version on our test script (checked with `cmp`).
- **Lesson:** Adding a "harmless" comment broke an exact-match string comparison in a different program. Exact matching is fragile.

### 5. A `sed` edit split a string literal across two lines
- **What happened:** We used a `sed` command to fix the roster table's header. Afterwards the line ended with `<< "` and the next line was `";`. That's a broken string literal, and it would not have compiled.
- **Cause:** In the replacement text, `\\n` was meant to be the two characters `\n` inside the C++ string. `sed` turned it into a real line break.
- **Fix:** We caught it by reading the line back before compiling, then fixed it with a normal editor. Afterwards we only used `sed` for edits without backslashes.

### 6. Compiler warnings: `implicit conversion from 'size_type' to 'double' may lose precision`
- **What happened:** Building with clang plus extra-strict warnings (`-Wconversion`) flagged two lines in `GradeBook.h`: `sum / avgs.size()` and `sumSquares / avgs.size()`.
- **Cause:** `avgs.size()` is an unsigned integer type (`size_t`). Dividing a `double` by it makes C++ convert it silently, and clang wants that conversion written out.
- **Fix:** `sum / static_cast<double>(avgs.size())`. The answer didn't change, but the code now says exactly what it means. (Plain `g++ -Wall -Wextra` didn't warn about this. We only saw it because we tried a stricter compiler too.)

### 7. Thousands of `-Wnullability-completeness` warnings from the compiler's own files
- **What happened:** The first `zig c++` build printed pages of warnings like `pointer is missing a nullability type specifier` from files inside `ziglang\lib\libcxx\include\string`.
- **Cause:** On the first run, zig compiles its own copy of the C++ standard library (libc++), and those library files trigger warnings in this zig version. None of them were in *our* code.
- **Fix:** No fix needed. We filtered the output to our own files (`grep src/`) to make sure our code was clean. Later builds reuse zig's cache, so they're fast and quiet.

### Caught in review (before it caused a visible bug)
- **`std::left` leaking out of tables:** `cout << std::left` stays switched on after a table is printed. In command mode that would have left-aligned the main menu's numbers later. `CommandShell::execute()` now saves the stream's formatting flags before each command and restores them afterwards.
- **`tolower()` on accented letters:** `std::tolower(c)` is undefined behavior when `c` is a negative `char`, and accented letters like `é` are stored that way. Every call casts to `unsigned char` first.

---

## Stage 2: WebAssembly + the JavaScript bridge

### 8. ⭐ Two "failed" tests where the C++ was right and the TEST was wrong
- **What happened:** The first run of `tests/engine_test.mjs` (which runs the real `gradebook.wasm` in Node) gave **48 passed, 2 failed**:
  ```
  FAIL  need gives a number for Ethan  ->  Ethan Brooks would need 146.2% on the next Test to reach 70.0% - not possible with one Test.
  FAIL  find returns matching IDs      ->  Found 2 student(s) matching "an":
  ```
- **Cause:** Neither failure was a bug in the program. We checked both by hand:
  - Ethan's demo average is 52.06, and his grades add up to a total weight of 8.5. To reach 70% with one Test (weight 2): `(70 × (8.5 + 2) − 52.06 × 8.5) / 2 = (735 − 442.5) / 2 ≈ 146%`. One test can't fix that, so C++ was right to say "not possible". Our test assumed a normal number would come back.
  - The test expected `find an` to include "Ava Chen". Read it again: there's no "an" in "Ava Chen". The real matches are Eth**an** Brooks and H**an**a Kim, which is exactly what C++ returned.
- **Fix:** We changed the tests, not the C++. The `need` test now uses a 60% target (worked out by hand: 93.75, and C++ prints 93.8%). The `find` test now expects Ethan and Hana by ID. Result: **50 passed, 0 failed**. (More checks were added later, for 53 total.)
- **Lesson:** When a test fails, check the test's expected answer before "fixing" working code. Doing the math by hand settled both of these.

---

## Stage 3: The website

### 9. Playwright couldn't find its browser
- **What happened:** The browser-testing tool crashed with `Executable doesn't exist at ...\ms-playwright\chromium-1248\chrome.exe`.
- **Cause:** The computer had Chromium build **1243** downloaded, but that version of the tool wanted build **1248**.
- **Fix:** Instead of downloading another 150 MB browser, the project's UI test uses `playwright-core` (8 MB) with `channel: "chrome"`. That drives the Google Chrome already installed on the laptop, falling back to Microsoft Edge. It lives in `tests/package.json` so the project root stays a plain static site for Vercel.

### 10. The test server made Chrome *download* the web page instead of showing it
- **What happened:** The first `page.goto()` failed with `net::ERR_ABORTED`.
- **Cause:** Our tiny test web server picks the `Content-Type` from the file extension. For the URL `/`, the code did `extname(path || ".html")`. But `extname(".html")` is `""` in Node, because a name that *starts* with a dot counts as a hidden file with no extension. So the page went out as `application/octet-stream`, and Chrome treated it as a file download.
- **Fix:** Work out the real filename first (`index.html`), then take the extension of that.

### 11. ⭐ "Export CSV" showed a success message but no file downloaded
- **What happened:** Clicking **⋯ → Export CSV** showed the toast *"Exported 8 students to Period-3-Chemistry-grades.csv"*, but nothing downloaded. The UI test timed out waiting for the download. It also failed in a visible (non-headless) Chrome window, so it wasn't a headless-only glitch.
- **How we tracked it down** (one change at a time):
  1. A bare `<a download>` link on a blank page **worked**, so Chrome itself could download.
  2. The same blob-link code run inside our page, outside any click, **worked**, so nothing on the page was blocking downloads.
  3. The CSV's MIME type and the odd file name (`Period-3---Chemistry...`) **worked** when tried alone.
  4. `preventDefault()` was never called on the click. We checked by wrapping it with a logger.
  5. Calling `exportCsv()` directly **worked**. From a button's *own* click listener it **worked**.
  6. From our *document-level* click listener (the one that handles every `data-action` button) it **failed every time**, even with a do-nothing listener that only called `exportCsv()`.
- **Cause:** Starting a download with `link.click()` *while* the page-wide click handler was still running was silently ignored by Chrome on this page. We could not find the exact rule Chrome applies here. A minimal blank-page copy of the same setup *did* download, so there's an extra condition we never pinned down.
- **Fix:** Start the download one moment later, after the click has finished being handled: `setTimeout(() => link.click(), 0)`. We tested this alone first (download count went 0 → 1), then in the real app. The UI test now saves the downloaded CSV and checks that it starts with `id,name,assignment,type,score`. We also cleaned up the file name, which had become `Period-3---Chemistry`.
- **Lesson:** Make one change at a time. Each experiment above changed exactly one thing, and that's how we narrowed six possible causes down to one.

### 12. The console hid the output of commands we typed
- **What happened:** The UI test typed `stats` into the C++ console and couldn't find "Class Statistics" in the log.
- **Cause:** To keep the log tidy, any output longer than 10 lines is folded into a collapsed "▸ Output · 0.5 KB" row, which is right for the huge automatic `json` snapshots. `stats` is 12 lines, so it got folded too.
- **Fix:** Only *automatic* output gets folded. Anything you type yourself is always shown in full.

### 13. ⭐ The page scrolled sideways on a phone
- **What happened:** At phone width (390 px) the page was 450 px wide, so you could drag it sideways. The roster's status chips were cut off at the edge.
- **How we found it:** We hid one section at a time and re-measured `document.documentElement.scrollWidth`. Only hiding the **roster card** brought it back to 390.
- **Cause:** On small screens our CSS switched the layout to `grid-template-columns: 1fr`. In CSS Grid, `1fr` really means `minmax(auto, 1fr)`, and that `auto` minimum lets wide content (the table) push the column wider than the screen. The table even had its own scroll box, but the grid column grew anyway.
- **Fix:** Use `minmax(0, 1fr)` ("this column may shrink to zero") for the page and the main grid. On phones we also hide the avatar circles and let long chips like "Needs Improvement" wrap onto two lines. The test now checks `scrollWidth <= innerWidth` at both 1366 px (Chromebook) and 390 px (phone).

### 14. Smaller test hiccups (good reminders, not app bugs)
- **Waiting for something hidden:** `waitForSelector("#boot.done")` timed out even though the app had loaded. The loading screen *becomes hidden* when it's done, and Playwright waits for a *visible* element by default. Fixed with `{ state: "attached" }`.
- **Measuring during an animation:** the dark-mode check read the background color while its 0.3 s fade was still running. Fixed by waiting 0.5 s first.
- **`<` sorts before letters:** the name-sort test expected "Ava Chen" first, but C++ put `<b>Zoe</b> "Z" O'Neil` (our HTML-escaping test name) first. That's correct, because `<` comes before every letter in character order. The test now compares against a properly sorted copy of the list.
- **A "first visit" that wasn't:** the test cleared saved data and reloaded. But the page saves itself when you leave (`pagehide`), so the reload wrote the data straight back and showed "Welcome back!". The app was right. The test now clears storage *before* the page's scripts run.

### 15. A warning we decided to keep
- `g++ -Wshadow` (stricter than the required `-Wall -Wextra`) warns that the constructor parameters `name` and `id` "shadow" the fields with the same names. We kept it that way on purpose: `this->name = name;` is the exact pattern from Java (AP CSA), and it's easy to explain. With the required flags (`-Wall -Wextra`), both the multi-file and single-file builds have **0 warnings**.

## Stage 5: Matching our class's grading categories

### 16. A test passed for the wrong reason, then failed for the wrong reason
- **What happened:** After switching to our class's categories (Project 50%, Test 20%, the rest 10%), the browser test "stricter cutoffs change the status counts" failed: the Passed count was still 6.
- **Cause:** The test checked `Passed !== "6"`, assuming 6 students passed *before* the cutoffs changed. But earlier in the same test we add a student (Zoe, 88), so it was really **7** before. With the new demo scores, raising the pass cutoff to 80 happened to drop it to exactly 6. The cutoffs worked fine, and the old version of the test had only passed by luck.
- **Fix:** The test now saves the Passed count right before opening Settings and checks that it changed (it went 7 → 6).
- **Lesson:** Compare against a value you measured, not a number you remember.

### 17. The "Project" card was centered while the others weren't
- **What happened:** In the new *Grading categories* card, the Project tile (no grades yet) had all its text centered, unlike the other four.
- **Cause:** We marked it with the CSS class `empty`, but `.empty` already existed in our stylesheet for the roster's "No students yet" box, which centers everything.
- **Fix:** Renamed our class to `no-grades`. Found by screenshotting the page instead of assuming the CSS was fine.

## Stage 6: Teacher/student accounts and the cloud database

### 18. The test couldn't click "Student" on the sign-up form
- **What happened:** The cloud test froze for 30 seconds on `page.check('input[value="student"]')`, then failed with *"`<label class="seg">` intercepts pointer events"*.
- **Cause:** The Teacher/Student picker is a pair of radio buttons styled as two big buttons. The real radio circles are invisible on purpose, and the label sits on top. The test tried to click the invisible circle, which a real person can't do either.
- **Fix:** The test clicks the label, which is what a person does. The app was fine.
- **A second test hiccup:** "undo restores the same code" failed once because the test waited a fixed 1.5 seconds. A save waits 0.7 s for you to stop clicking, then talks to the database. The test now checks every 0.2 s until the code appears (up to 10 s), and it passed while the page still said "Saving…". This is the same lesson as #16: wait for the thing you're checking, not for a number you guessed.

### 19. ⭐ A new student could have inherited an old student's login code
- **What happened:** We found this one by thinking it through, before it caused a visible bug. A student's code was linked to their **ID** (like 1002). But **Load demo**, **Clear all students**, and **Import CSV** start the IDs over at 1001. So if a teacher cleared the class and added a new student, the new student 1002 would get the *old* 1002's code, and the old student's account could suddenly read the new student's grades.
- **Fix:** Those three commands now give every student a brand-new code, and each save deletes any code that no current student uses. (Undoing a single removal still brings back the same code, because it's the same student.) The cloud test proves it: after the teacher loads the demo, the old student account is cut off and the old code document is gone.
- **Lesson:** Security bugs often hide in "this can't happen" assumptions. Here the assumption was "IDs are never reused."

### Caught in review (before it caused a visible bug)
- **Deleting a code that was never saved:** If a student was added and removed before their first save, the save would have tried to delete a `joinCodes` document that didn't exist. Our security rule allows a delete only when `resource.data.teacherUid` is the teacher, and a missing document has no data, so Firestore would have refused the **whole batch** and nothing would save. Now a save only deletes codes it knows are in the database.

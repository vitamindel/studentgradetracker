# Video Script: Student Grade Tracker (≈3:00)

**Speakers:** **A** = [Your Name], **B** = [Partner Name]
**Speaking pace:** about 450 words total, so speak at a normal, relaxed speed. Times are approximate.

### Before you hit record
- Open the Vercel link in Chrome at full screen. Use **light mode** (the theme button in the top bar).
- Open **⋯ → Load demo class** so you start from the same 8 students every take.
- In a second tab, have `src/web_main.cpp` open (GitHub or VS Code), scrolled to `run()`.
- In a third tab, have `DEBUG_LOG.md` open at entry **#11 (Export CSV)**.
- Close other tabs and turn off notifications.

---

## 0:00 – 0:20 · Goal & purpose

| Time | Speaker | Says | On screen |
|---|---|---|---|
| 0:00 | **A** | "Hi, I'm [Your Name]…" | Website dashboard, whole page visible |
| 0:03 | **B** | "…and I'm [Partner Name]. This is Gradebook++, a student grade tracker we built in C++." | Slowly move the mouse across the stat cards |
| 0:08 | **A** | "Teachers deal with tons of grades, and a test should count more than homework. Our app does the weighted math instantly, shows who's passing and who needs help, and tells a student exactly what they need on their next test." | Point at *Class average*, then the *Status breakdown*, then the red **At risk** badge on Ethan |

## 0:20 – 1:20 · Live demo

| Time | Speaker | Says | On screen |
|---|---|---|---|
| 0:20 | **B** | "Let's add a student. This calls our C++ constructor with a name and a starting grade." | Click **Student** → type `Jordan Lee`, grade `84` → **Add student**. Green toast appears. |
| 0:28 | **B** | "Every number updates: the average, the charts, the roster." | Point at the new row and the new bar in *Student averages* |
| 0:33 | **A** | "Click a student to see their history. Here's Ethan. He's failing, and the chart shows every grade plus his running average." | Click **Ethan Brooks** → the panel slides in. Hover the line chart so the tooltip shows. |
| 0:42 | **A** | "Ethan wants to know what he needs on the next test to reach 60 percent…" | In *What do I need?* choose **Test**, type `60`, click **Calculate** |
| 0:47 | **A** | "…93.8. That answer came straight from C++." | Result box: *"Ethan Brooks needs 93.8%…"* |
| 0:51 | **B** | "Now a whole-class assignment: one score per student." | Press Esc. Click **Assignment** → title `Pop Quiz`, type in `85` + **Fill blanks** → **Add assignment** |
| 0:59 | **B** | "And a curve. The slider previews the new average before anything changes. C++ works that out on a copy of the class." | Click **Curve**, drag to **+6**, pause on the before → after numbers, click **Apply curve** |
| 1:07 | **A** | "Watch the bars slide. And if we change our minds…" | Bars animate. Then press **Ctrl+Z**. |
| 1:11 | **A** | "…Ctrl+Z. Undo works because C++ saves a snapshot of the whole class before every change." | Bars slide back, toast says *"Undid: curve."* |
| 1:15 | **B** | "It saves automatically, works on a phone, and has dark mode." | Click the theme button twice to show dark, then once more to go back |

## 1:20 – 2:15 · How we made it

| Time | Speaker | Says | On screen |
|---|---|---|---|
| 1:20 | **A** | "Here's the cool part. There's no grade math in the JavaScript at all. Open the C++ Console." | Click **C++ Console** at the bottom. The log is visible. |
| 1:26 | **A** | "Every button sends a text command, like this one. Watch." | With the console still open, click **Student** → `Maya Ortiz`, `91` → **Add student**. Point at the new `add "Maya Ortiz" 91` line in the console and C++'s reply under it: *"Added Maya Ortiz (ID …) with a starting grade of 91.0."* (Don't open a student panel here: its dark backdrop dims the console.) |
| 1:34 | **B** | "That command goes into one C++ function called `run`. Inside, we point `cin` at the command and `cout` at an empty string. Our `CommandShell` class reads it with `cin`, updates the `Student` objects, and answers with `cout`." | Switch to the `web_main.cpp` tab. Highlight the two `rdbuf` lines, then `shell.execute(std::cin, std::cout)`. |
| 1:48 | **A** | "We have three classes. `Student` has private name, ID, and grades, plus methods like `average` and `status`. `GradeBook` holds everyone and does class stats, sorting, and undo. `CommandShell` turns text into method calls." | Quickly show `Student.h` (the constructor at line 25), then `GradeBook.h` |
| 2:00 | **B** | "To run C++ in a browser, we compiled it to WebAssembly with zig. And the same classes also run as a normal terminal program, with a `cin`/`cout` menu, as a backup." | Back in the browser console, type `stats` + Enter, so C++ prints ASCII bar charts. (Or show OnlineGDB running for 2 seconds.) |

## 2:15 – 2:40 · Debugging story

| Time | Speaker | Says | On screen |
|---|---|---|---|
| 2:15 | **B** | "Our favorite bug: Export CSV showed 'Exported 8 students', but no file downloaded." | `DEBUG_LOG.md` entry #11 |
| 2:21 | **A** | "So we changed one thing at a time. A plain download link worked. Our export code on its own worked. It only failed when it started from our page-wide click handler. Starting the download a split second later fixed it." | Scroll slowly through the numbered experiment list in entry #11 |
| 2:33 | **B** | "And twice our tests said the C++ was wrong, but when we did the math by hand, the test was wrong. Lesson: check the test too." | Entry #8 in the debug log |

## 2:40 – 3:00 · Reflection

| Time | Speaker | Says | On screen |
|---|---|---|---|
| 2:40 | **A** | "Coming from Java, pointers scared me, but `findById` returning a `Student*` is basically a Java reference. [One honest sentence about what you learned.]" | Back to the dashboard |
| 2:48 | **B** | "For me it was seeing that C++ isn't just black-and-white terminal programs. The same classes can run a real website. [One honest sentence.]" | Scroll down to the *Under the hood* section |
| 2:56 | **A + B** | "Thanks for watching!" | The pipeline diagram (Click → run() → CommandShell → …) |

---

### If something goes wrong during recording
- **Page says "The C++ engine couldn't start":** you opened the file directly. Use the Vercel link, or run `python -m http.server 8000 -d web` and open `http://localhost:8000`.
- **The demo data looks different:** **⋯ → Load demo class** resets it.
- **Running long?** Cut the dark-mode moment (1:15) and the `stats` command (2:00) first.

# Student Grade Tracker: Project Report

**Names:** [Your Name] and [Partner Name]
**Course:** Electronics/Computer Engineering 11
**Date:** [Date]

> *Note to us before we hand this in: read it out loud and change anything that doesn't sound like us. Check the class policy on AI tools, and if we need to say we had help from one, add a sentence in Section 5.*

---

## 1. Project Goal

Our goal was to build a student grade tracker in C++ that does more than print a list. It had to show every concept from the unit (variables, `cin`/`cout`, if/else, loops, classes and objects, a parameterized constructor, and calculations). We also wanted it to feel like a real app someone would actually use. So we wrote all the logic in C++, then compiled that same C++ to run inside a website with a proper interface, charts, and a dark mode.

## 2. Purpose

**What problem does it solve?** Teachers juggle a lot of grades, and not all assignments count the same. In our Electronics/Computer Engineering class, the Project category is 50% of the grade, Tests are 20%, and Homework, Quizzes, and Class Activity are 10% each. Working out weighted averages by hand, deciding who is passing, and figuring out who needs help is slow and easy to get wrong. Our tracker does that math instantly and shows it at a glance.

**Who could use it?** Mainly teachers, but students could use it too. The "What do I need?" calculator answers the question every student asks before a test: *"What do I need on the next test to get a 90?"*

**Why we chose it:** We both know what it's like to stress about a grade without knowing where we stand, so the project felt real to us. It was also a good fit for the assignment, because grades naturally need classes (each student is an object), loops (averaging lots of scores), and if/else (Passed / Needs Improvement / Failed).

## 3. Program Description (Input → Processing → Output)

**Input.** The user adds students (name and an optional starting grade) and grades (assignment name, type, and score). They can also give one assignment to the whole class at once, change the pass cutoffs, curve the class, or import a CSV file. On the website these are buttons and forms. In the terminal version it's a numbered menu read with `cin`.

**Processing.** Everything is calculated in C++:
- Each `Student` works out its own **weighted average** the same way our school portal does: it averages each category first, then weights those averages (Project 50%, Test 20%, Homework / Quiz / Class Activity 10% each). A category with no grades yet is skipped. It also works out the letter grade, GPA, **status**, and trend.
- The `GradeBook` calculates the **class average, median, standard deviation, and pass rate**, finds the top student and the most at-risk student, sorts the roster with a selection sort, and keeps undo "snapshots".
- The `CommandShell` reads text commands like `grade 1001 "Quiz 3" quiz 88` and calls the right method.

**Output.** In the terminal, the program prints a colored roster, report cards, and ASCII bar charts with `cout`. On the website, C++ answers every command with `cout`. JavaScript then shows that answer as a message and redraws the dashboard: stat cards, a letter-grade chart, a status breakdown, a bar for every student's average with the cutoff lines drawn on it, and a line chart of each student's grade history.

**The part we're proudest of:** the website doesn't calculate anything itself. When you click a button, JavaScript sends a text command into a C++ function called `run()`. That function temporarily points `cin` at the command and `cout` at an empty string, lets our C++ classes do the work, and hands back whatever was printed. So every button really is a `cin`/`cout` conversation with C++. There's a "C++ Console" panel on the site that shows every command and C++'s exact reply, so anyone can watch it happen.

## 4. C++ Concepts

| Concept | Where it is in our code | What it does |
|---|---|---|
| **Variables / Data Types** | `Student.h` lines 9–19: `std::string name`, `int id`, `double score`, `std::vector<GradeEntry> grades`. `bool` is used everywhere for "did it work?" | Stores each student's information and grades. We used `double` for scores so 87.5 works. |
| **cin / cout** | `main.cpp` line 90 (`readInt`) reads menu choices with `cin` and fixes bad input with `cin.fail()`, `cin.clear()`, and `cin.ignore()`. `web_main.cpp` line 40 (`run()`) redirects `cin`/`cout` for the website. | All input and output. If someone types "abc" for a number, the program says so and asks again instead of crashing. |
| **if / else** | `Student.h` line 193 (`status()`): Passed / Needs Improvement / Failed. `Student.h` line 171 (`letterFor()`): A–F. | Decides each student's status and letter grade. |
| **Loops** | `Student.h` line 149 (`averageOfFirst()`) loops over the 5 categories, and `categoryTotals()` (line 125) loops through every grade. `GradeBook.h` line 426 (`sortBy()`) is a selection sort with nested loops. `main.cpp` line 521 is the menu `while` loop. | Adds up grades, sorts the roster, and keeps the menu running until the user exits. |
| **Classes and Objects** | `Student.h` line 15, `GradeBook.h` line 12, `CommandShell.h` line 14. `main.cpp` line 491 creates three `Student` objects. | Each student is an object with private data (`name`, `id`, `grades`) and public methods. |
| **Methods / Functions** | `average()`, `categoryAverage()`, `letterGrade()`, `gpa()`, `status()`, `trend()`, `scoreNeeded()` in `Student.h`. `classAverage()`, `median()`, `standardDeviation()` in `GradeBook.h`. | Each method does one job, which made it easy to test pieces one at a time. |
| **Constructor** | `Student.h` line 26: `Student(const std::string& name, int id, double startingGrade)`. Called in `main.cpp` line 491: `Student student1("Ava Chen", 1001, 92.5);` | Sets each student's name, ID, and starting grade the moment the object is created. |
| **Other** | **Vectors** (`std::vector<Student>`), **pointers** (`findById()` returns `Student*`, `GradeBook.h` line 219), **references** (`categoryTotals(type, count, double& sum, int& n)`), **structs** (`GradeEntry`, `Snapshot`), **static** methods and constants, **string streams** for parsing commands and CSV, and copying whole objects for **undo** (`saveSnapshot()`, `GradeBook.h` line 33). | Things we learned beyond the basics because the project needed them. |

The terminal version prints the rubric items right at the start: it creates three `Student` objects with the constructor, displays each one with its status, and then calculates their class average (`main.cpp` line 507).

## 5. How We Built It

We worked in stages and made sure each one worked before starting the next.

1. **The C++ core.** We started with the three classes: `Student`, `GradeBook`, and `CommandShell`. We put the method bodies inside the class like in Java, because that's what we're used to from AP CSA. Then we wrote `main.cpp`, the terminal version with a menu, and tested it by feeding it a file of typed-in answers. That file included wrong ones on purpose, like "abc" and "eighty". We compiled with `g++ -std=c++17 -Wall -Wextra` and got it to zero warnings.
2. **WebAssembly.** WebAssembly lets C++ run inside a web browser. We compiled the exact same classes with `zig c++ -target wasm32-wasi`, which was much easier to install than the usual tool (Emscripten). We wrote `web_main.cpp`, which has the `run()` function that redirects `cin` and `cout`. A browser isn't a real operating system, so we also wrote a small JavaScript file that pretends to be the eight system functions the C++ expects. We tested the compiled file with automatic checks before building any of the website.
3. **The website.** We built the interface in plain HTML, CSS, and JavaScript, with no frameworks. Every button builds a command and sends it to C++. After every change, the page asks C++ for a `json` snapshot of the whole class and redraws from it. We added light and dark mode, made it work on a Chromebook screen and a phone, and wrote a browser test that clicks through every feature and checks for errors. It runs 67 checks.
4. **Deploying.** We commit the compiled `.wasm` file to GitHub, and Vercel serves the `web` folder as a normal website, so nothing has to be compiled on their end.
5. **Making it match our class.** Our first version gave each assignment its own weight (a Test counted twice as much as a Quiz). Our school portal works differently: it averages each category, then weights the categories (Project 50%, Test 20%, the rest 10%), and skips categories with no grades yet. We rewrote `average()` and `scoreNeeded()` in `Student.h` to match, and changed the demo class to our real assignment names (with made-up scores). The "What do I need?" answer got more interesting too: because a Project is half the grade, one good project can save a student when no single test can.

[If the class AI policy requires it, add a sentence here about which tools helped us and how.]

## 6. Troubleshooting

| Problem | Cause | Fix |
|---|---|---|
| The one-file version for OnlineGDB gave `warning: #pragma once in main file`, even though the normal version had **zero** warnings. | Our script that glues all the files into one was supposed to delete every `#pragma once` line, but it only matched lines that were *exactly* that text. We had added a comment on the same line in `Student.h`, so that line slipped through. | We changed the check to match the start of the line (a regular expression), so comments don't matter. Afterwards, the single file compiled with 0 warnings and printed exactly the same output as the normal version. |
| Two automatic tests "failed" on the first run: Ethan's "What do I need?" answer and a name search for "an". | **Our tests were wrong, not the program.** Ethan's average is 52, so reaching 70% would take about 146% on one test, and C++ correctly said "not possible". And "Ava Chen" doesn't even contain "an". | We did the math by hand to prove the C++ was right, then fixed the tests' expected answers. 50/50 passed after that. |
| "Export CSV" showed a success message but no file downloaded. | Starting a download *while* the page was still handling the menu click was silently ignored by Chrome. We found this by changing one thing at a time: a blank page worked, the export code on its own worked, and only the page-wide click handler failed. | We start the download a split second later, after the click is done (`setTimeout(..., 0)`). The automatic test now checks the downloaded file's contents. |
| On a phone, the whole page could be dragged sideways (450 px wide on a 390 px screen). | In CSS Grid, `1fr` secretly means "at least as wide as the content", and our grade table was wide. We found it by hiding one section at a time until the width went back to normal. | Changed the layout to `minmax(0, 1fr)`, so the column can shrink, and let long status labels wrap on small screens. |

**What troubleshooting taught us:** The biggest lesson was **change one thing at a time.** With the export bug there were six possible causes, and we only found the real one by testing them one by one. The second lesson surprised us: **sometimes the code is right and the test is wrong.** When our tests failed, we almost changed working C++ code. Doing the math by hand on paper saved us. We also learned to read compiler warnings instead of ignoring them. The `#pragma once` warning was tiny, but it pointed at a real bug in our build script.

## 7. Testing

We tested the terminal version by feeding it a file of answers (`tests/console_input.txt`), and the C++ engine and website with automatic test scripts. These are real results that we copied from the program's output.

| # | Input | Expected | Actual |
|---|---|---|---|
| 1 | Terminal menu: typed `abc`, then `99` | Doesn't crash; asks again both times | `Please type a whole number.` then `Please choose from 0 to 12.` ✅ |
| 2 | Terminal startup: Ava (starting 92.5 + Test 95), Marcus (72 + Test 62), Ethan (55 + Test 50) | Ava (92.5 × 10 + 95 × 20) ÷ 30 = 94.2 Passed. Marcus 65.3 Needs Improvement. Ethan 51.7 Failed. Class average 70.4 | `94.2% … Passed`, `65.3% … Needs Improvement`, `51.7% … Failed`, `Class average of these 3: 70.4%` ✅ |
| 3 | Demo class: `show 1001` (Ava's 9 grades) | By hand: (100 × 10 + 97.5 × 10 + 95.67 × 20 + 100 × 10) ÷ 50 = **97.77** | `Weighted average: 97.8%   Letter: A   GPA: 4.0` ✅ |
| 4 | `need 1006 project 70` (Ethan, no Project yet) | By hand: (70 × 100 − 2691.7) ÷ 50 = **86.17** | `Ethan Brooks needs 86.2% on the next Project to reach 70.0%.` ✅ |
| 5 | `need 1006 test 60` (Ethan) | About 110%, so impossible | `Ethan Brooks would need 110.0% on the next Test to reach 60.0% - not possible with one Test.` ✅ |
| 6 | `grade 1001 "Quiz 9" quiz 150` (score too high) | Rejected, nothing changes | `Error: the score must be from 0 to 120.` ✅ |
| 7 | `thresholds 50 60` (pass cutoff below the needs-improvement cutoff) | Rejected | `Error: cutoffs must follow  0 <= needs-improvement < pass <= 100.` ✅ |
| 8 | Website: add a student named `<b>Zoe</b> "Z" O'Neil` | Name shows as plain text; it must NOT turn bold | Shown exactly as typed; no bold tag created on the page ✅ |
| 9 | Website: curve slider at +10, then Apply | Preview average = average after applying; Ctrl+Z undoes it | Preview 81.8 → 89.7, applied 89.73; Ctrl+Z → `Undid: curve.` ✅ |
| 10 | Demo class: export CSV → clear the class → import the same CSV | Every student, grade, and name comes back exactly | `Imported 8 student(s) and 72 grade(s).` All identical ✅ |

In total the automatic tests run **60 checks on the C++ engine** and **67 checks on the website** (in a real Chrome browser, at desktop, Chromebook, and phone sizes), and all of them pass.

## 8. Final Reflection

[Your Name]: The coolest moment was seeing the "C++ Console" panel fill up while clicking around the website. It proved that the C++ we wrote was actually running in the browser, not some JavaScript copy of it. Coming from Java, pointers and references scared me at first. But once I thought of a pointer as "a Java reference that you have to be honest about", `findById()` made sense. [Add something personal: what was hard for you, what you'd tell yourself at the start.]

[Partner Name]: [Your reflection, e.g. which part you built, what you'd do differently, what clicked for you.]

**If we had more time,** we'd add a login so a teacher's data follows them between computers. Right now it's saved in the browser, and you move it with CSV export/import. We'd also add attendance tracking. Overall, this project showed us that C++ isn't just for black-and-white terminal programs. The same classes we'd write for a school assignment can power a real website.

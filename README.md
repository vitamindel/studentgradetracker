# Gradebook++ (Student Grade Tracker)

A student grade tracker whose **logic is 100% C++**. The C++ is compiled to WebAssembly and runs inside a modern web page. Every button on the site sends a text command to C++, C++ reads it with `cin` and answers with `cout`, and the page draws whatever C++ said.

The same C++ classes also run as a normal terminal program (for OnlineGDB).

- **Website:** `web/` (open the Vercel link, or run it locally; see below)
- **Terminal version:** `src/main.cpp`, or the one-file copy `onlinegdb/GradeTracker_SingleFile.cpp`
- **How it works, in plain English:** [EXPLAIN.md](EXPLAIN.md)
- **Every bug we hit and how we fixed it:** [DEBUG_LOG.md](DEBUG_LOG.md)

---

## Project layout

```
src/
  Student.h          one student: private name, id, grades + weighted average, letter, GPA, status, trend, scoreNeeded
  GradeBook.h        the whole class: stats, sorting, curve, undo snapshots, CSV import/export
  CommandShell.h     reads text commands from any istream, answers on any ostream
  web_main.cpp       the bridge: run(line) points cin/cout at strings and calls CommandShell
  main.cpp           terminal version: 3 Student objects + a cin/cout menu
web/
  index.html  style.css  app.js     the website (plain JS, no framework)
  engine.js                         loads gradebook.wasm and provides the tiny WASI "shim"
  gradebook.wasm                    the compiled C++ (committed, so Vercel needs no build)
onlinegdb/GradeTracker_SingleFile.cpp   all headers + main.cpp in one file (generated)
tests/
  console_input.txt   scripted input for the terminal version
  engine_test.mjs     53 checks that run the real .wasm in Node
  ui_test.mjs         63 checks that click through the website in headless Chrome
tools/make_single_file.py   builds the OnlineGDB file
build.sh / build.ps1        rebuild everything
vercel.json                 tells Vercel to serve the web/ folder
```

---

## 1. Put it on GitHub

You need a free GitHub account and Git installed (`git --version` to check).

1. Go to <https://github.com/new>, name the repository `gradebook-plus-plus` (any name works), leave **"Add a README" unchecked**, and click **Create repository**.
2. In a terminal inside this project folder, run (replace `YOUR-USERNAME`):
   ```bash
   git init -b main          # skip if this folder is already a git repo
   git add .
   git commit -m "Student Grade Tracker"
   git remote add origin https://github.com/YOUR-USERNAME/gradebook-plus-plus.git
   git push -u origin main
   ```
3. Refresh the GitHub page. You should see `src/`, `web/`, and **`web/gradebook.wasm`** in the file list. If the `.wasm` is missing, Vercel will show "The C++ engine couldn't start".

## 2. Put it on Vercel

1. Go to <https://vercel.com/new> and sign in **with GitHub**.
2. Find `gradebook-plus-plus` in the list and click **Import**.
3. Leave every setting as it is. `vercel.json` already says "serve the `web` folder, nothing to build". (If Vercel asks for a Framework Preset, choose **Other**.)
4. Click **Deploy**. After about 20 seconds you get a link like `https://gradebook-plus-plus.vercel.app`. That's the link to open at school.
5. Every `git push` after this redeploys automatically.

> **School Chromebook tip:** the site works offline after it loads, and your class is saved in that browser (localStorage). A different computer starts fresh with the demo class. To move your data between computers, use **⋯ → Export CSV**, then **⋯ → Import CSV** on the other one.

## 3. Run it on your own computer

The page must be opened through a web server, because browsers block WebAssembly on `file://` pages:

```bash
python -m http.server 8000 -d web
```

Then open <http://localhost:8000>.

## 4. Run the terminal version (OnlineGDB backup)

1. Open <https://www.onlinegdb.com>, set the language (top right) to **C++17**.
2. Paste the whole of `onlinegdb/GradeTracker_SingleFile.cpp` into `main.cpp` and press **Run**.
3. If you see odd codes like `[32m` instead of colors, change `const bool FANCY_UI = true;` to `false`.

Or with g++ on any computer:

```bash
g++ -std=c++17 -Wall -Wextra -o tracker src/main.cpp
./tracker
```

## 5. Rebuild after changing the C++

You only need this if you edit anything in `src/`. Install the compiler once:

```bash
python -m pip install ziglang
```

Then:

```bash
./build.sh                                              # Mac / Linux / Git Bash
powershell -ExecutionPolicy Bypass -File build.ps1      # Windows
```

This recompiles `web/gradebook.wasm`, regenerates the OnlineGDB file, builds the console version, and runs the engine tests. **Commit the new `web/gradebook.wasm`** afterwards so Vercel gets it.

The WebAssembly command inside the scripts is:

```bash
python -m ziglang c++ -target wasm32-wasi -Os -s -fno-exceptions -mexec-model=reactor -std=c++17 -o web/gradebook.wasm src/web_main.cpp
```

## 6. Tests

```bash
node tests/engine_test.mjs               # 53 checks on the real .wasm (needs Node 18+)
cd tests && npm install && node ui_test.mjs   # 63 browser checks; uses your installed Chrome or Edge
```

The UI test saves screenshots to `build/shots/`.

---

## Command reference (what the buttons send)

Type any of these into the **C++ Console** at the bottom of the website, or in **Command mode** (option 12) in the terminal version. Text with spaces goes in quotes.

| Command | Example | What it does |
|---|---|---|
| `add "<name>" [grade]` | `add "Ava Chen" 92` | new student (calls the `Student` constructor) |
| `grade <id> "<label>" <type> <score>` | `grade 1001 "Quiz 3" quiz 88` | add one grade |
| `assign "<title>" <type> <id>=<score> …` | `assign "Unit 2 Test" test 1001=90 1002=` | grade the whole class (blank = skip) |
| `edit <id> <#> <score> ["<label>"] [type]` | `edit 1001 2 95` | change grade #2 |
| `delgrade <id> <#>` | `delgrade 1001 2` | delete grade #2 |
| `rename <id> <name>` / `remove <id>` | `rename 1001 Ava C.` | rename / remove a student |
| `show <id>` | `show 1001` | report card |
| `need <id> <type> <target>` | `need 1001 test 95` | score needed on the next assignment |
| `list` / `find <text>` / `sort <name\|average\|id> [desc]` | `sort average desc` | roster |
| `stats` | `stats` | class statistics with text bar charts |
| `curve <points> [preview]` | `curve 5 preview` | add points to every grade (max 100) |
| `thresholds <pass> <needs>` | `thresholds 70 60` | change status cutoffs |
| `classname <name>` | `classname AP Chemistry` | rename the class |
| `undo` / `demo` / `clear` | | undo, load the sample class, remove everyone |
| `export` / `import` | | CSV out / CSV in (`import` reads the following lines) |
| `json` | | the whole class as JSON (what the website draws from) |

Grade weights: **Test ×2, Project ×1.5, Quiz ×1, Homework ×0.5.** Default status cutoffs: **Passed ≥ 70, Needs Improvement ≥ 60, below that Failed.**

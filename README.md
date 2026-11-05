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
  cloud.js                          login + cloud database (Firebase Authentication + Cloud Firestore)
  firebase-config.js                your Firebase project's settings (blank = no login, saves in the browser)
onlinegdb/GradeTracker_SingleFile.cpp   all headers + main.cpp in one file (generated)
tests/
  console_input.txt   scripted input for the terminal version
  engine_test.mjs     62 checks that run the real .wasm in Node
  ui_test.mjs         68 checks that click through the website in headless Chrome
  cloud_test.mjs      71 checks of login, teacher/student accounts, and the database rules (Firebase emulators)
tools/make_single_file.py   builds the OnlineGDB file
build.sh / build.ps1        rebuild everything
vercel.json                 tells Vercel to serve the web/ folder
firestore.rules             who can read/write what in the database (checked by Google's servers)
firebase.json               points Firebase at firestore.rules (and sets up the test emulators)
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

> **Without login** (until you do step 3), your class is saved in that browser only. To move it between computers, use **⋯ → Export CSV**, then **⋯ → Import CSV** on the other one.

## 3. Turn on login and the cloud database (Firebase)

This adds **teacher and student accounts** and saves every class in **Cloud Firestore**, Google's cloud database. It's free (the Spark plan, no credit card) and takes about 10 minutes.

1. Go to <https://console.firebase.google.com>, click **Create a project**, and name it `gradebook-plus-plus`. You can turn Google Analytics off.
2. On the project's home page, click the **Web** icon (`</>`), give the app a nickname, and click **Register app**. Firebase shows a block of code with `const firebaseConfig = { apiKey: ..., ... }`. Copy those six values into [web/firebase-config.js](web/firebase-config.js).
3. In the left menu, open **Authentication** → **Get started** → **Sign-in method**. Turn on **Email/Password**, then **Google** (pick your email as the support email).
4. Still in Authentication, open **Settings** → **Authorized domains** → **Add domain**, and add your Vercel address (like `gradebook-plus-plus.vercel.app`). `localhost` is already there.
5. In the left menu, open **Firestore Database** → **Create database**. Pick a location near you and start in **production mode**.
6. Open the **Rules** tab, delete what's there, paste in everything from [firestore.rules](firestore.rules), and click **Publish**.
7. Commit and push. Vercel redeploys, and the site now opens on a sign-in screen.

> The `apiKey` in `firebase-config.js` is **meant to be public**. Every Firebase website ships it to the browser. What protects the data is `firestore.rules`. If GitHub emails you about a "Google API key" in the repo, that's why it's safe to leave.

**How the accounts work**

- **Teacher:** click **Create an account**, pick **Teacher**. You get the full gradebook (a new account starts with the demo class), and it's saved to the database after every change. Sign in on any computer and it's there.
- **Student:** the teacher opens **⋯ → Student login codes** (or a student's panel) and gives each student their code, like `K7P2-QX9M`. The student clicks **Create an account**, picks **Student**, and types the code. They see **only their own** report card: average, categories, grade history, and the "What do I need?" calculator. They can't change anything, and it updates by itself when the teacher adds a grade.
- **Use without an account** still works, saving in that browser like before.

The rule that a student can only read their own record is enforced by **Google's servers** (`firestore.rules`), not just by hiding buttons. So even someone who edits the page's JavaScript can't read another student's grades. Removing a student deletes their code, so they lose access too. **Load demo class**, **Clear all students**, and **Import CSV** start a new roster, so every student gets a new code and the old ones stop working.

## 4. Run it on your own computer

The page must be opened through a web server, because browsers block WebAssembly on `file://` pages:

```bash
python -m http.server 8000 -d web
```

Then open <http://localhost:8000>.

## 5. Run the terminal version (OnlineGDB backup)

1. Open <https://www.onlinegdb.com>, set the language (top right) to **C++17**.
2. Paste the whole of `onlinegdb/GradeTracker_SingleFile.cpp` into `main.cpp` and press **Run**.
3. If you see odd codes like `[32m` instead of colors, change `const bool FANCY_UI = true;` to `false`.

Or with g++ on any computer:

```bash
g++ -std=c++17 -Wall -Wextra -o tracker src/main.cpp
./tracker
```

## 6. Rebuild after changing the C++

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

## 7. Tests

```bash
node tests/engine_test.mjs               # 62 checks on the real .wasm (needs Node 18+)
cd tests && npm install && node ui_test.mjs   # 68 browser checks; uses your installed Chrome or Edge
```

The cloud test runs on Firebase's **emulators**: a fake login service and database on your own computer, so it never touches the real project. It needs Java 21 or newer:

```bash
npx firebase-tools emulators:exec --project demo-gradebook "node tests/cloud_test.mjs"
```

The tests save screenshots to `build/shots/`.

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
| `classname <name>` | `classname Electronics/Computer Engineering 11` | rename the class |
| `undo` / `demo` / `clear` | | undo, load the sample class, remove everyone |
| `export` / `import` | | CSV out / CSV in (`import` reads the following lines) |
| `json` | | the whole class as JSON (what the website draws from) |

Grading categories (same as the school portal): **Homework 10%, Quiz 10%, Test 20%, Project 50%, Class Activity 10%.** Each category is averaged first, then the categories are weighted; categories with no grades yet are skipped. Types can be typed as `homework`/`hw`, `quiz`, `test`, `project`, or `activity`/`ca`. Default status cutoffs: **Passed ≥ 70, Needs Improvement ≥ 60, below that Failed.**

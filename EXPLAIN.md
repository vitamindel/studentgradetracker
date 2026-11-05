# How Gradebook++ Works (in plain English)

This guide walks through every C++ file, then follows **one button click** all the way from the website into C++ and back. Analogies first, code second.

---

## The big picture: a restaurant

Imagine the whole program is a small restaurant.

| Restaurant | Our program | File |
|---|---|---|
| A **recipe card** for one dish, with its own notes on the back | One `Student` object: name, ID, list of grades, and math about itself | `src/Student.h` |
| The **recipe box** holding every card, plus photocopies in case you spill coffee | The `GradeBook`: every student, class-wide stats, undo snapshots | `src/GradeBook.h` |
| The **waiter**, who reads your order and brings back the answer | The `CommandShell`: reads a text command with `>>`, answers with `<<` | `src/CommandShell.h` |
| The **drive-through window** | `run()`: JavaScript passes text in, C++ hands text out | `src/web_main.cpp` |
| The **dining room** with a paper menu | The terminal version with a `cin`/`cout` menu | `src/main.cpp` |
| The **fancy front-of-house** with screens and buttons | The website | `web/` |

The website is **only decoration**. It doesn't know what a weighted average is. It knows how to *ask C++*, and how to draw what C++ answers.

---

## `src/Student.h`: one recipe card

A `Student` is like one index card. The front has the **name** and **ID**, and the back has a **list of grades**. These three things are `private`: only the card's own methods can change them. It's the same idea as private fields in Java, so nobody can write "-50" on the card by accident.

```cpp
class Student {
private:
    std::string name;
    int id;
    std::vector<GradeEntry> grades;   // like ArrayList<GradeEntry>
```

**The constructor** ([Student.h:26](src/Student.h#L26)) runs automatically when you write `Student s("Ava Chen", 1001, 92);`. It fills in the card:

```cpp
Student(const std::string& name, int id, double startingGrade) {
    this->name = name;     // exactly like Java
    this->id = id;
    if (startingGrade != NO_GRADE && isValidScore(startingGrade)) {
        grades.push_back({"Starting Grade", "Quiz", startingGrade});
    }
}
```

**The weighted average** ([Student.h:149](src/Student.h#L149)) works the same way as our school's grade portal. First, each **category** gets its own average. Then those averages are combined by weight: **Project 50%, Test 20%, and Homework, Quiz, and Class Activity 10% each.** Think of a pizza cut into slices of different sizes. The Project is half the pizza, so it matters most. If a category has no grades yet, its slice is left out and the other slices are stretched to fill the whole pizza:

> average = (each category's average × its weight, added up) ÷ (the weights of the categories that have grades)
>
> Example (Ava, no Project yet): (100 × 10 + 97.5 × 10 + 95.67 × 20 + 100 × 10) ÷ 50 = **97.77**

`categoryTotals()` is a good example of **pass by reference**: it fills in the caller's `sum` and `n` variables through `double& sum, int& n`, something Java can't do with plain numbers.

**The status decision** ([Student.h:193](src/Student.h#L193)) is a plain if / else-if / else chain, like a bouncer checking heights at a ride:

```cpp
if (avg >= passCutoff)                  return "Passed";
else if (avg >= needsImprovementCutoff) return "Needs Improvement";
else                                    return "Failed";
```

The cutoffs are *parameters*, so the teacher can move them (Settings on the website, or `thresholds 80 70`).

**"What do I need?"** ([Student.h:226](src/Student.h#L226)) is algebra. The other categories don't change, so only one category's average moves. If that category has `n` grades adding up to `sum`, one more score `x` makes its average `(sum + x) ÷ (n + 1)`. So we work out the category average we need, then solve for `x`:

> needed category average = (target × (other weights + w) − other weights × their averages) ÷ w
> → x = needed category average × (n + 1) − sum

Example: Ethan has no Project yet, and a Project is worth 50%, so one good project can rescue him. He needs **86.2%** on it to reach 70%. One Test can't do that: he would need 110%.

If `x` comes out above 100, it's impossible. If it's 0 or less, the student is already safe.

**Other methods:** `letterGrade()` (90/80/70/60), `gpa()`, `trend()` (is the newest grade above or below their earlier average?), `highestScore()`, `lowestScore()`.

---

## `src/GradeBook.h`: the recipe box

A `GradeBook` holds a `std::vector<Student>`, which is a growable list of Student objects (Java: `ArrayList<Student>`). It does everything that needs *more than one* student:

- **Class average, median, standard deviation, pass rate** ([GradeBook.h:316](src/GradeBook.h#L316) and below). The median sorts the averages and picks the middle one. The standard deviation measures "how spread out are the averages?"
- **Category averages:** `categoryClassAverage()` ([GradeBook.h:368](src/GradeBook.h#L368)) averages one category across the class. That's what the website's *Grading categories* card shows.
- **Finding a student:** `findById()` ([GradeBook.h:219](src/GradeBook.h#L219)) returns a **pointer** (`Student*`). A pointer is like a Java object reference: it *points at* the real card in the box, so changing it through the pointer changes the original. If nobody has that ID, it returns `nullptr` (C++'s `null`).
- **Sorting:** `sortBy()` ([GradeBook.h:426](src/GradeBook.h#L426)) is a **selection sort**, the same algorithm from AP CSA Unit 7. Find the student who should come first, swap them to the front, repeat.
- **Undo:** before *every* change, `saveSnapshot()` ([GradeBook.h:33](src/GradeBook.h#L33)) makes a **photocopy** of the whole box (copying a vector copies every Student inside it). `undo()` ([GradeBook.h:481](src/GradeBook.h#L481)) throws away the current box and puts the newest photocopy back. We keep the last 50 photocopies.
- **Curve preview:** `curvedCopy()` ([GradeBook.h:454](src/GradeBook.h#L454)) photocopies the class, curves the *copy*, and lets you look at the copy's average. The real class is never touched. That's how the slider shows "80.7 → 84.7" before you press Apply.
- **CSV import/export** ([GradeBook.h:541](src/GradeBook.h#L541)). Export writes one line per grade (`1001,Ava Chen,Cybersecurity Analysis,Test,94`). Import reads them back, handling `"quoted, names"` with commas in them.

**Why no `stoi` / `stod`?** Our WebAssembly build has exceptions switched off, to keep the file small. `std::stoi("abc")` would *throw*, and with no exceptions the program would just crash. So we convert text to numbers with a string stream, which only raises a "fail" flag:

```cpp
std::istringstream stream(text);
stream >> result;
if (stream.fail()) return false;   // "abc" lands here: no crash
```

---

## `src/CommandShell.h`: the waiter

The waiter takes an order written as text, like

```
grade 1001 "Unit 3 Quiz" quiz 88
```

and works out what to do. The key method is `execute(istream& in, ostream& out)` ([CommandShell.h:633](src/CommandShell.h#L633)):

1. `getline(in, line)` reads one whole line (one order).
2. `args >> command` reads the first word (`grade`).
3. A big if / else-if chain ([CommandShell.h:653](src/CommandShell.h#L653)) picks the matching method, `cmdGrade()`.
4. `cmdGrade()` ([CommandShell.h:146](src/CommandShell.h#L146)) reads the rest with `>>`: the ID, the label, the type, and the score.
5. It asks the GradeBook to do the work, then writes a reply with `out << "Added Quiz ..."`.

The clever part: `execute()` takes **any** input stream and **any** output stream. In the terminal those are the real `cin` and `cout`. On the website they're `cin` and `cout` *redirected to strings*. Same waiter, same code, two different restaurants.

**Quoted text:** names have spaces, so `"Ava Chen"` goes in quotes. `std::quoted` ([CommandShell.h:55](src/CommandShell.h#L55)) reads a quoted phrase as one piece and strips off the quotes.

**Errors always start with `Error:`**, which is how the website knows to show a red toast instead of a green one.

**The `json` command** ([CommandShell.h:540](src/CommandShell.h#L540)) loops over every student and writes the whole class as JSON. JSON is a text format JavaScript can read instantly. After every change, the website asks for `json` and redraws everything from it.

---

## `src/web_main.cpp`: the drive-through window

The website can't call C++ classes directly. It can only call plain functions that the WebAssembly file *exports*. So we export exactly one important function, `run()` ([web_main.cpp:40](src/web_main.cpp#L40)):

```cpp
const char* run(const char* line) {
    std::istringstream input(line);    // the order, as a stream
    std::ostringstream output;         // an empty bag for the answer

    std::streambuf* oldCin  = std::cin.rdbuf(input.rdbuf());    // cin now reads the order
    std::streambuf* oldCout = std::cout.rdbuf(output.rdbuf());  // cout now fills the bag

    shell.execute(std::cin, std::cout);   // the waiter does the real work

    std::cin.rdbuf(oldCin);               // put the real cin/cout back
    std::cout.rdbuf(oldCout);
    std::cin.clear();                     // forget "end of input" for next time
    result = output.str();
    return result.c_str();               // hand the bag out the window
}
```

**Analogy:** `rdbuf()` is like swapping the pipe a faucet is connected to. `cin` is still `cin` and `cout` is still `cout`, but for a moment `cin`'s pipe comes from our string and `cout`'s pipe drains into our bag.

That's why we can honestly say: **every button on the website sends a command that C++ reads with `cin` and answers with `cout`.**

`alloc()` and `dealloc()` exist because JavaScript can't create C++ strings. JS asks C++ for a chunk of memory, copies the command's bytes into it, and gives it back afterwards.

---

## `src/main.cpp`: the terminal version

This is the OnlineGDB backup, built from the **same three classes**.

- **On startup** ([main.cpp:491](src/main.cpp#L491)) it builds three Student objects with the constructor, prints each one with its status, and calculates their class average. That covers the rubric in the first 20 lines of output.
- **Then a menu loop** asks for a number 0–12 and calls one function per option.
- **Safe input** ([main.cpp:90](src/main.cpp#L90)). If you type `abc` where a number is expected, `cin` jams (it enters a "fail" state) and ignores everything after. The fix is three steps:
  1. `cin.fail()`: did that go wrong?
  2. `cin.clear()`: un-jam it.
  3. `cin.ignore(...)`: throw away the rest of the bad line.
- **Command mode** (option 12, [main.cpp:459](src/main.cpp#L459)) lets you type the exact same commands the website sends, using the same `CommandShell`.
- **Colors** use ANSI codes like `\033[32m` (green). Set `FANCY_UI = false` to turn them off.

---

## Following one click, start to finish

You open Ethan's panel, type **"Unit 3 Retest"**, choose **Test**, enter **95**, and click **Add**.

```
 YOU click "Add"
   │
   ▼
 1. app.js builds a text command                        web/app.js  (add-grade form)
      grade 1006 "Unit 3 Retest" test 95
   │
   ▼
 2. cpp() logs it to the console panel, calls engine.run()      web/app.js:70
   │
   ▼
 3. engine.js copies the text into WebAssembly memory           web/engine.js:97
      ptr = alloc(n)  →  copy bytes  →  run(ptr)
   │
   ═════════════════  now we're inside C++  ═════════════════
   ▼
 4. run() points cin at the text, cout at an empty string       src/web_main.cpp:40
   │
   ▼
 5. CommandShell::execute(cin, cout)                            src/CommandShell.h:633
      getline(cin, line)   →  "grade 1006 "Unit 3 Retest" test 95"
      args >> command      →  "grade"   →  cmdGrade()
      readInt → 1006, std::quoted → Unit 3 Retest, "test", 95
   │
   ▼
 6. GradeBook::addGrade(1006, ...)                              src/GradeBook.h:268
      findById(1006) → pointer to Ethan's Student object
      saveSnapshot("add grade")   ← photocopy for Undo
   │
   ▼
 7. Student::addGrade(...)                                      src/Student.h
      normalizeType("test") → "Test";  isValidScore(95) → true
      grades.push_back(...)
   │
   ▼
 8. cout << "Added Test "Unit 3 Retest" (95.0) for Ethan Brooks. New average: 58.5%."
   │
   ═════════════════  back out to JavaScript  ═════════════════
   ▼
 9. run() returns the bag; engine.js reads bytes until the 0 at the end
   │
   ▼
10. app.js shows that exact text in a green toast (with an Undo button)
   │
   ▼
11. app.js sends "json"; C++ loops over every student and prints the class as JSON
   │
   ▼
12. JSON.parse → redraw the tiles, charts, roster, and Ethan's panel
```

Every step from 4 to 8 is C++. JavaScript never calculates a grade.

**Want to see it live?** Open the **C++ Console** at the bottom of the page. Every command from steps 1 and 11 appears there, along with C++'s exact output.

---

## WebAssembly in one paragraph

**WebAssembly (`.wasm`)** is a compact program format that every modern browser can run almost as fast as a normal app. We compile our C++ into it with `zig c++ -target wasm32-wasi`. **WASI** is a standard list of "operating system" services a program might need, like "print this text" (`fd_write`) or "quit" (`proc_exit`). A browser isn't an operating system, so `web/engine.js` provides tiny stand-ins for the 8 services our program asks for. Think of it as a **travel adapter**: the plug (our C++) expects a certain socket, so we hand it one that fits.

One last step: before using `run()`, JavaScript calls **`_initialize()`** ([engine.js:94](web/engine.js#L94)). That's like turning on the lights before opening the shop. It runs the C++ setup code that builds `cout` itself and our global `GradeBook` and `CommandShell`.

---

## Java → C++ cheat sheet

| Java | C++ (in this project) | Notes |
|---|---|---|
| `String` | `std::string` | `std::` is like a package prefix (`java.util.`) |
| `ArrayList<Student>` | `std::vector<Student>` | `push_back()` = `add()`, `size()` = `size()` |
| `null` | `nullptr` | |
| object reference | pointer `Student*` | use `->` to call a method: `s->getName()` |
| (no equivalent) | reference `double&` | lets a function write into the caller's variable |
| (no direct equivalent) | `const` after the `()` | a promise the method won't change the object: `double average() const` |
| `public static final int` | `static const int` | `Student::NO_GRADE` |
| `System.out.println(x)` | `std::cout << x << "\n";` | |
| `Scanner.nextInt()` | `std::cin >> x;` | check `cin.fail()` afterwards |
| one class per `.java` file | class in a `.h` "header", `#include`d where needed | `#pragma once` stops double-including |
| `import` | `#include` | |
| `try { } catch` | (turned off in our web build) | we check return values instead |

---

## The 30-second version (for the video)

> "Our grade tracker has three C++ classes. `Student` is one student with private grades and methods like `average()` and `status()`. `GradeBook` holds all the students and does class-wide math, sorting, and undo. `CommandShell` reads text commands with `cin` and answers with `cout`. We compiled that C++ to WebAssembly so it runs in the browser. When you click a button, JavaScript sends a command like `grade 1001 quiz 88` into a C++ function called `run()`. `run()` points `cin` at that text, C++ reads it, updates the objects, and writes the answer with `cout`. The website just draws whatever C++ said."

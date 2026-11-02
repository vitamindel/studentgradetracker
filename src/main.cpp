// =====================================================================
//  main.cpp  -  TERMINAL version of the Student Grade Tracker
// ---------------------------------------------------------------------
//  This is the backup version that runs in OnlineGDB (or any terminal).
//  It uses the SAME Student / GradeBook / CommandShell classes as the
//  website, but talks to the user with a cin/cout menu instead of buttons.
//
//  Compile:  g++ -std=c++17 -Wall -Wextra main.cpp -o tracker
// =====================================================================
#include "Student.h"
#include "GradeBook.h"
#include "CommandShell.h"

#include <iostream>   // cin, cout
#include <iomanip>    // setw, setprecision, fixed
#include <limits>     // numeric_limits (used to clear bad input)
#include <sstream>    // istringstream (used to read "blank = skip" answers)
#include <string>
#include <vector>
#include <cstdlib>    // exit()

// "using namespace std" lets us write cout instead of std::cout.
// It's fine in a .cpp file like this one, but we avoid it in the .h files
// so we don't force it on every file that includes them.
using namespace std;

// =====================================================================
//  COLORS  (ANSI escape codes). Set FANCY_UI to false if your terminal
//  shows weird symbols like "[32m" instead of colors.
// =====================================================================
const bool FANCY_UI = true;

const string BOLD = "1";
const string DIM = "2";
const string RED = "31";
const string GREEN = "32";
const string YELLOW = "33";
const string BLUE = "34";
const string MAGENTA = "35";
const string CYAN = "36";

// Wraps text in a color code, e.g. paint("Passed", GREEN)
string paint(const string& text, const string& code) {
    if (!FANCY_UI) {
        return text;   // plain text when colors are turned off
    }
    return "\033[" + code + "m" + text + "\033[0m";
}

// Which color goes with which status?
string statusColor(const string& status) {
    if (status == "Passed") return GREEN;
    if (status == "Needs Improvement") return YELLOW;
    if (status == "Failed") return RED;
    return DIM;   // "No Grades"
}

// Trend as a short symbol: ^ up, v down, = steady
string trendSymbol(const string& trend) {
    if (trend == "up") return paint("^ up", GREEN);
    if (trend == "down") return paint("v down", RED);
    if (trend == "steady") return paint("= steady", DIM);
    return paint("-", DIM);
}

// A number as text with 1 decimal place, e.g. oneDecimal(92.456) -> "92.5"
string oneDecimal(double value) {
    ostringstream out;
    out << fixed << setprecision(1) << value;
    return out.str();
}

// A row of the same character, e.g. repeatChar('#', 5) -> "#####"
string repeatChar(char c, int count) {
    string result = "";
    for (int i = 0; i < count; i++) {
        result += c;
    }
    return result;
}

void printLine() {
    cout << paint(repeatChar('-', 62), DIM) << "\n";
}

void printHeader(const string& title) {
    cout << "\n";
    printLine();
    cout << "  " << paint(title, BOLD) << "\n";
    printLine();
}

// =====================================================================
//  SAFE INPUT HELPERS
//  If someone types "abc" when we asked for a number, cin goes into a
//  "fail" state and every later cin >> is ignored. These helpers fix that:
//    1. cin.fail()   -> did the last read go wrong?
//    2. cin.clear()  -> reset the error flag so cin works again
//    3. cin.ignore() -> throw away the rest of the bad line
// =====================================================================

// If the input ran out completely (e.g. Ctrl+D), quit cleanly instead of
// looping forever. (eof + fail together means "tried to read, nothing left".)
void exitIfInputEnded() {
    if (cin.eof() && cin.fail()) {
        cout << "\n\nInput ended. Goodbye!\n";
        exit(0);
    }
}

// Asks until the user types a whole number from low to high.
int readInt(const string& prompt, int low, int high) {
    while (true) {
        cout << prompt;
        int value;
        cin >> value;
        exitIfInputEnded();
        if (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << paint("  Please type a whole number.\n", RED);
            continue;   // ask again
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');   // eat the leftover Enter key
        if (value < low || value > high) {
            cout << paint("  Please choose from " + to_string(low) + " to " + to_string(high) + ".\n", RED);
            continue;
        }
        return value;
    }
}

// Asks until the user types a decimal number from low to high.
double readDouble(const string& prompt, double low, double high) {
    while (true) {
        cout << prompt;
        double value;
        cin >> value;
        exitIfInputEnded();
        if (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << paint("  Please type a number.\n", RED);
            continue;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        if (value < low || value > high) {
            cout << paint("  Please type a number from ", RED) << low << paint(" to ", RED) << high << "\n";
            continue;
        }
        return value;
    }
}

// Reads a whole line of text (names can have spaces, so we use getline).
string readLine(const string& prompt) {
    cout << prompt;
    string text;
    getline(cin, text);
    exitIfInputEnded();
    // trim spaces off both ends
    while (!text.empty() && (text.back() == ' ' || text.back() == '\r')) text.pop_back();
    while (!text.empty() && text[0] == ' ') text.erase(0, 1);
    return text;
}

// Asks until the user types a valid grade type.
string readType(const string& prompt) {
    while (true) {
        string type = Student::normalizeType(readLine(prompt));
        if (type != "") {
            return type;
        }
        cout << paint("  Type must be test, quiz, project, or homework.\n", RED);
    }
}

// Asks for a student ID until it matches someone in the gradebook.
// Returns 0 if the class is empty.
int readStudentId(GradeBook& book) {
    if (book.size() == 0) {
        cout << paint("  There are no students yet.\n", YELLOW);
        return 0;
    }
    while (true) {
        int id = readInt("  Student ID: ", 0, 99999999);
        if (book.findById(id) != nullptr) {
            return id;
        }
        cout << paint("  No student has that ID. (IDs are shown in the roster.)\n", RED);
    }
}

bool askYesNo(const string& prompt) {
    string answer = readLine(prompt + " (y/n): ");
    return answer == "y" || answer == "Y" || answer == "yes";
}

// =====================================================================
//  DISPLAY HELPERS
// =====================================================================

// Shows one Student object's info (used for the three rubric objects).
void displayStudent(const Student& s, double passCutoff, double needsCutoff) {
    string status = s.status(passCutoff, needsCutoff);
    cout << "  " << paint(s.getName(), BOLD) << paint("  (ID " + to_string(s.getId()) + ")", DIM) << "\n";
    cout << "     Grades: " << s.gradeCount()
         << "   Average: " << s.average() << "%"
         << "   Letter: " << s.letterGrade()
         << "   GPA: " << s.gpa() << "\n";
    cout << "     Status: " << paint(status, statusColor(status)) << "\n";
}

// =====================================================================
//  MENU OPTIONS  (one function per menu choice)
// =====================================================================

// 1) View roster
void viewRoster(const GradeBook& book) {
    printHeader(book.getClassName() + "  -  " + to_string(book.size()) + " students");
    if (book.size() == 0) {
        cout << "  No students yet. Choose 2 to add one.\n";
        return;
    }
    int topId = book.topStudentId();
    int riskId = book.atRiskStudentId();

    cout << paint("  ID    Name                     Avg  Ltr GPA  Trend     Status", DIM) << "\n";
    for (const Student& s : book.getStudents()) {
        string status = book.statusOf(s);
        cout << "  " << left << setw(6) << s.getId() << setw(22) << s.getName() << right;
        if (s.hasGrades()) {
            cout << setw(6) << s.average() << "  " << left << setw(4) << s.letterGrade() << right
                 << setw(3) << s.gpa() << "  ";
        } else {
            cout << setw(6) << "--" << "  " << left << setw(4) << "-" << right << setw(3) << "-" << "  ";
        }
        // trendSymbol() contains invisible color codes, so we pad by hand
        string trend = s.trend();
        int trendWidth = (trend == "up") ? 4 : (trend == "down") ? 6 : (trend == "steady") ? 8 : 1;
        cout << trendSymbol(trend) << repeatChar(' ', 10 - trendWidth);
        cout << paint(status, statusColor(status));
        if (s.getId() == topId) cout << paint("  * top", MAGENTA);
        if (s.getId() == riskId) cout << paint("  ! at risk", RED);
        cout << "\n";
    }
    cout << "\n  Class average: " << paint(oneDecimal(book.classAverage()) + "%", BOLD)
         << "   (cutoffs: pass " << book.getPassCutoff() << ", needs improvement "
         << book.getNeedsCutoff() << ")\n";
}

// 2) Add a student (uses the parameterized constructor inside addStudent)
void addStudentMenu(GradeBook& book) {
    printHeader("Add a Student");
    string name = readLine("  Full name: ");
    if (!GradeBook::isValidName(name)) {
        cout << paint("  Names must be 1-40 characters.\n", RED);
        return;
    }
    double grade = readDouble("  Starting grade (0-120): ", 0, 120);
    int id = book.addStudent(name, grade);
    cout << paint("  Added " + name + " with ID " + to_string(id) + ".\n", GREEN);
}

// 3) Add one grade to one student
void addGradeMenu(GradeBook& book) {
    printHeader("Add a Grade");
    int id = readStudentId(book);
    if (id == 0) return;
    string label = readLine("  Assignment name: ");
    if (label.empty()) label = "Untitled";
    string type = readType("  Type (test/quiz/project/homework): ");
    double score = readDouble("  Score (0-120): ", 0, 120);
    book.addGrade(id, label, type, score);
    const Student* s = book.findById(id);
    cout << paint("  Saved! " + s->getName() + "'s new average: ", GREEN) << s->average() << "%\n";
}

// 4) One assignment for the whole class
void classAssignmentMenu(GradeBook& book) {
    printHeader("New Class Assignment");
    if (book.size() == 0) {
        cout << "  Add some students first.\n";
        return;
    }
    string title = readLine("  Assignment title: ");
    if (title.empty()) title = "Untitled";
    string type = readType("  Type (test/quiz/project/homework): ");
    cout << paint("  Enter a score for each student (press Enter to skip someone).\n", DIM);

    vector<int> ids;
    vector<double> scores;
    for (const Student& s : book.getStudents()) {
        while (true) {
            string answer = readLine("    " + s.getName() + ": ");
            if (answer.empty()) {
                break;   // skipped
            }
            istringstream reader(answer);   // read a number out of the text
            double score;
            reader >> score;
            if (!reader.fail() && Student::isValidScore(score)) {
                ids.push_back(s.getId());
                scores.push_back(score);
                break;
            }
            cout << paint("    Type a score from 0 to 120, or press Enter to skip.\n", RED);
        }
    }
    if (ids.empty()) {
        cout << paint("  No scores entered, nothing changed.\n", YELLOW);
        return;
    }
    book.addAssignment(title, type, ids, scores);
    cout << paint("  Graded " + to_string(ids.size()) + " student(s). ", GREEN)
         << "Class average is now " << book.classAverage() << "%\n";
}

// 5) Report card for one student
void reportCardMenu(GradeBook& book) {
    printHeader("Report Card");
    int id = readStudentId(book);
    if (id == 0) return;
    const Student* s = book.findById(id);

    cout << "\n  " << paint(s->getName(), BOLD) << paint("  (ID " + to_string(id) + ")", DIM) << "\n\n";
    if (!s->hasGrades()) {
        cout << "  No grades yet.\n";
        return;
    }
    cout << paint("  #   Assignment              Type       Score  Weight", DIM) << "\n";
    int number = 1;
    for (const GradeEntry& g : s->getGrades()) {
        cout << "  " << left << setw(4) << number << setw(24) << g.label << setw(9) << g.type << right
             << setw(7) << g.score << "  x" << Student::typeWeight(g.type) << "\n";
        number++;
    }
    string status = book.statusOf(*s);
    cout << "\n  Weighted average: " << paint(oneDecimal(s->average()) + "%", BOLD)
         << "   Letter: " << paint(s->letterGrade(), BOLD) << "   GPA: " << s->gpa() << "\n";
    cout << "  Status: " << paint(status, statusColor(status)) << "   Trend: " << trendSymbol(s->trend())
         << "\n  Best score: " << s->highestScore() << "   Lowest score: " << s->lowestScore() << "\n";
}

// 6) Stats with ASCII bar charts
void statsMenu(const GradeBook& book) {
    printHeader("Class Statistics  -  " + book.getClassName());
    if (book.gradedCount() == 0) {
        cout << "  No grades yet.\n";
        return;
    }
    cout << "  Average " << paint(oneDecimal(book.classAverage()), BOLD)
         << "    Median " << book.median()
         << "    Std dev " << book.standardDeviation()
         << "    Pass rate " << book.passRate() << "%\n";
    cout << "  " << paint("Passed " + to_string(book.countStatus("Passed")), GREEN)
         << "    " << paint("Needs Improvement " + to_string(book.countStatus("Needs Improvement")), YELLOW)
         << "    " << paint("Failed " + to_string(book.countStatus("Failed")), RED) << "\n";

    // --- Chart 1: each student's average. 1 '#' = 2.5 percentage points ---
    cout << "\n  " << paint("Student averages", BOLD)
         << paint("   ('|' marks the pass cutoff)", DIM) << "\n";
    int passColumn = static_cast<int>(book.getPassCutoff() / 2.5);
    for (const Student& s : book.getStudents()) {
        if (!s.hasGrades()) continue;
        int length = static_cast<int>(s.average() / 2.5 + 0.5);
        if (length > 40) length = 40;
        // Draw the bar, then spaces, with a '|' at the pass cutoff position
        string chart = "";
        for (int i = 0; i < 40; i++) {
            if (i < length) chart += '#';
            else if (i == passColumn) chart += '|';
            else chart += ' ';
        }
        string name = s.getName();
        if (name.size() > 16) name = name.substr(0, 15) + ".";
        cout << "  " << left << setw(17) << name << right
             << paint(chart, statusColor(book.statusOf(s))) << " " << s.average() << "\n";
    }

    // --- Chart 2: letter grade distribution ---
    cout << "\n  " << paint("Letter grades", BOLD) << "\n";
    string letters[5] = {"A", "B", "C", "D", "F"};
    string letterColors[5] = {GREEN, CYAN, BLUE, YELLOW, RED};
    for (int i = 0; i < 5; i++) {
        int count = book.countLetter(letters[i]);
        cout << "  " << letters[i] << " | " << paint(repeatChar('#', count * 4), letterColors[i]) << " "
             << count << "\n";
    }
}

// 7) "What do I need on my next ... ?"
void whatDoINeedMenu(GradeBook& book) {
    printHeader("What Do I Need?");
    int id = readStudentId(book);
    if (id == 0) return;
    string type = readType("  Next assignment type (test/quiz/project/homework): ");
    double target = readDouble("  Target average (%): ", 1, 100);
    const Student* s = book.findById(id);
    double needed = s->scoreNeeded(type, target);

    if (needed > 100) {
        cout << paint("  Even 100% on one " + type + " won't reach that target (would need ", YELLOW)
             << needed << "%).\n";
    } else if (needed <= 0) {
        cout << paint("  Already safe! Even a 0 keeps " + s->getName() + " at or above the target.\n", GREEN);
    } else {
        cout << "  " << s->getName() << " needs " << paint(oneDecimal(needed) + "%", BOLD)
             << " on the next " << type << " to reach " << target << "%.\n";
    }
}

// 8) Curve
void curveMenu(GradeBook& book) {
    printHeader("Curve the Class");
    if (book.gradedCount() == 0) {
        cout << "  No grades to curve yet.\n";
        return;
    }
    double points = readDouble("  Points to add to every grade (1-50): ", 1, 50);
    cout << "  Class average would go from " << book.classAverage() << " to "
         << paint(oneDecimal(book.previewCurve(points)), BOLD) << "\n";
    if (askYesNo("  Apply this curve?")) {
        book.applyCurve(points);
        cout << paint("  Curve applied. (Undo is option 11.)\n", GREEN);
    } else {
        cout << "  Cancelled.\n";
    }
}

// 9) Sort or search
void sortSearchMenu(GradeBook& book) {
    printHeader("Sort / Search");
    cout << "  1) Sort by name (A-Z)\n  2) Sort by average (highest first)\n"
         << "  3) Sort by ID\n  4) Search by name\n";
    int choice = readInt("  Choose: ", 1, 4);
    if (choice == 1) book.sortBy("name", false);
    else if (choice == 2) book.sortBy("average", true);
    else if (choice == 3) book.sortBy("id", false);
    else {
        string query = readLine("  Part of a name: ");
        vector<int> ids = book.search(query);
        if (ids.empty()) {
            cout << paint("  No matches.\n", YELLOW);
            return;
        }
        for (int id : ids) {
            displayStudent(*book.findById(id), book.getPassCutoff(), book.getNeedsCutoff());
        }
        return;
    }
    viewRoster(book);
}

// 10) Remove a student
void removeMenu(GradeBook& book) {
    printHeader("Remove a Student");
    int id = readStudentId(book);
    if (id == 0) return;
    string name = book.findById(id)->getName();
    if (askYesNo("  Really remove " + name + "?")) {
        book.removeStudent(id);
        cout << paint("  Removed " + name + ". (Undo is option 11.)\n", GREEN);
    }
}

// 11) Undo
void undoMenu(GradeBook& book) {
    string what = book.undo();
    if (what == "") cout << paint("\n  Nothing to undo.\n", YELLOW);
    else cout << paint("\n  Undid: " + what + "\n", GREEN);
}

// 12) Command mode: type the SAME text commands the website sends
void commandMode(CommandShell& shell) {
    printHeader("Command Mode  (type 'help' for commands, 'back' to return)");
    cout << paint("  These are the exact commands the website's buttons send to C++.\n", DIM);
    while (true) {
        cout << paint("cmd> ", CYAN);
        if (!shell.execute(cin, cout)) {   // false = "back" (or input ended)
            break;
        }
    }
    cin.clear();   // in case input ended while in command mode
}

void printMenu() {
    cout << "\n";
    printLine();
    cout << "  " << paint("MAIN MENU", BOLD) << "\n";
    cout << "   1) View roster            7) What do I need?\n"
         << "   2) Add a student          8) Curve the class\n"
         << "   3) Add a grade            9) Sort / search\n"
         << "   4) Class assignment      10) Remove a student\n"
         << "   5) Report card           11) Undo last change\n"
         << "   6) Stats + charts        12) Command mode\n"
         << "   0) Exit\n";
}

// =====================================================================
//  main(): the program starts here
// =====================================================================
int main() {
    // Show every decimal number with exactly 1 digit after the point.
    cout << fixed << setprecision(1);

    cout << "\n" << paint("  ===  STUDENT GRADE TRACKER  ===", BOLD + ";" + CYAN) << "\n";
    cout << paint("  Weights: Test x2, Project x1.5, Quiz x1, Homework x0.5", DIM) << "\n";

    // -----------------------------------------------------------------
    //  Create 3 Student OBJECTS with the PARAMETERIZED CONSTRUCTOR.
    //  Each call sets the student's name, ID, and starting grade.
    // -----------------------------------------------------------------
    Student student1("Ava Chen", 1001, 92.5);
    Student student2("Marcus Johnson", 1002, 72.0);
    Student student3("Ethan Brooks", 1003, 55.0);

    // Give each one a test score too, so the weighted average matters.
    student1.addGrade("Unit 1 Test", "Test", 95);
    student2.addGrade("Unit 1 Test", "Test", 62);
    student3.addGrade("Unit 1 Test", "Test", 50);

    // Status cutoffs (the teacher can change these later in the gradebook).
    double passCutoff = 70;
    double needsCutoff = 60;

    // Display each object's info, including its status.
    printHeader("Our 3 starting Student objects");
    displayStudent(student1, passCutoff, needsCutoff);
    displayStudent(student2, passCutoff, needsCutoff);
    displayStudent(student3, passCutoff, needsCutoff);

    // A basic calculation across the objects: the class average.
    double classAverage = (student1.average() + student2.average() + student3.average()) / 3.0;
    cout << "\n  Class average of these 3: " << paint(oneDecimal(classAverage) + "%", BOLD) << "\n";

    // Put the 3 objects into a GradeBook so the menu can work with them.
    GradeBook book;
    book.setClassName("Period 3 - Chemistry");
    book.addExistingStudent(student1);
    book.addExistingStudent(student2);
    book.addExistingStudent(student3);
    book.clearUndoHistory();   // start with a clean undo history

    CommandShell shell(book);  // the same command interpreter the website uses

    // -----------------------------------------------------------------
    //  MENU LOOP: keep going until the user picks 0
    // -----------------------------------------------------------------
    bool running = true;
    while (running) {
        printMenu();
        int choice = readInt("  Choose an option: ", 0, 12);

        if (choice == 1) viewRoster(book);
        else if (choice == 2) addStudentMenu(book);
        else if (choice == 3) addGradeMenu(book);
        else if (choice == 4) classAssignmentMenu(book);
        else if (choice == 5) reportCardMenu(book);
        else if (choice == 6) statsMenu(book);
        else if (choice == 7) whatDoINeedMenu(book);
        else if (choice == 8) curveMenu(book);
        else if (choice == 9) sortSearchMenu(book);
        else if (choice == 10) removeMenu(book);
        else if (choice == 11) undoMenu(book);
        else if (choice == 12) commandMode(shell);
        else running = false;   // 0 = exit
    }

    cout << paint("\n  Goodbye! Keep those grades up.\n\n", CYAN);
    return 0;
}

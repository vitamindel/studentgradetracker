// main.cpp - terminal version of the Student Grade Tracker (a cin/cout menu).
// Uses the same classes as the website.
// Compile:  g++ -std=c++17 -Wall -Wextra main.cpp -o tracker
#include "Student.h"
#include "GradeBook.h"
#include "CommandShell.h"

#include <iostream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>

using namespace std;

// Set to false if your terminal shows codes like "[32m" instead of colors.
const bool FANCY_UI = true;

const string BOLD = "1";
const string DIM = "2";
const string RED = "31";
const string GREEN = "32";
const string YELLOW = "33";
const string BLUE = "34";
const string MAGENTA = "35";
const string CYAN = "36";

// paint("Passed", GREEN) -> green text
string paint(const string& text, const string& code) {
    if (!FANCY_UI) {
        return text;
    }
    return "\033[" + code + "m" + text + "\033[0m";
}

string statusColor(const string& status) {
    if (status == "Passed") return GREEN;
    if (status == "Needs Improvement") return YELLOW;
    if (status == "Failed") return RED;
    return DIM;   // "No Grades"
}

string trendSymbol(const string& trend) {
    if (trend == "up") return paint("^ up", GREEN);
    if (trend == "down") return paint("v down", RED);
    if (trend == "steady") return paint("= steady", DIM);
    return paint("-", DIM);
}

// oneDecimal(92.456) -> "92.5"
string oneDecimal(double value) {
    ostringstream out;
    out << fixed << setprecision(1) << value;
    return out.str();
}

// repeatChar('#', 5) -> "#####"
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

// ----- Safe input: on bad input, cin.clear() resets cin and cin.ignore() drops the line -----

// Quit cleanly if the input runs out (e.g. Ctrl+D) instead of looping forever.
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
            continue;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');   // drop the leftover Enter
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

// getline reads a whole line, so names can have spaces.
string readLine(const string& prompt) {
    cout << prompt;
    string text;
    getline(cin, text);
    exitIfInputEnded();
    while (!text.empty() && (text.back() == ' ' || text.back() == '\r')) text.pop_back();
    while (!text.empty() && text[0] == ' ') text.erase(0, 1);
    return text;
}

string readType(const string& prompt) {
    while (true) {
        string type = Student::normalizeType(readLine(prompt));
        if (type != "") {
            return type;
        }
        cout << paint("  Type must be homework, quiz, test, project, or activity.\n", RED);
    }
}

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

void displayStudent(const Student& s, double passCutoff, double needsCutoff) {
    string status = s.status(passCutoff, needsCutoff);
    cout << "  " << paint(s.getName(), BOLD) << paint("  (ID " + to_string(s.getId()) + ")", DIM) << "\n";
    cout << "     Grades: " << s.gradeCount()
         << "   Average: " << s.average() << "%"
         << "   Letter: " << s.letterGrade()
         << "   GPA: " << s.gpa() << "\n";
    cout << "     Status: " << paint(status, statusColor(status)) << "\n";
}

// ----- One function per menu option -----

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
        // Color codes are invisible but count for setw, so pad by hand
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

// 2) Add a student
void addStudentMenu(GradeBook& book) {
    printHeader("Add a Student");
    string name = readLine("  Full name: ");
    if (!GradeBook::isValidName(name)) {
        cout << paint("  Names must be 1-40 characters.\n", RED);
        return;
    }
    double grade = readDouble("  Starting grade (0-100): ", 0, 100);
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
    string type = readType("  Category (homework/quiz/test/project/activity): ");
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
    string type = readType("  Category (homework/quiz/test/project/activity): ");
    cout << paint("  Enter a score for each student (press Enter to skip someone).\n", DIM);

    vector<int> ids;
    vector<double> scores;
    for (const Student& s : book.getStudents()) {
        while (true) {
            string answer = readLine("    " + s.getName() + ": ");
            if (answer.empty()) {
                break;   // skipped
            }
            istringstream reader(answer);
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
    cout << paint("  #   Assignment              Category         Score", DIM) << "\n";
    int number = 1;
    for (const GradeEntry& g : s->getGrades()) {
        cout << "  " << left << setw(4) << number << setw(24) << g.label << setw(15) << g.type << right
             << setw(7) << g.score << "\n";
        number++;
    }

    // Like the "Grading Information" table on the school portal
    cout << "\n" << paint("  Category          Weight   Average", DIM) << "\n";
    for (int t = 0; t < Student::NUM_TYPES; t++) {
        string type = Student::typeName(t);
        double avg = s->categoryAverage(type);
        cout << "  " << left << setw(18) << type << right << setw(5) << static_cast<int>(Student::typeWeight(type))
             << "%" << setw(10) << (avg >= 0 ? oneDecimal(avg) : "--") << "\n";
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

    cout << "\n  " << paint("Category averages", BOLD) << "\n";
    for (int t = 0; t < Student::NUM_TYPES; t++) {
        string type = Student::typeName(t);
        double avg = book.categoryClassAverage(type);
        cout << "  " << left << setw(16) << type << right << setw(3) << static_cast<int>(Student::typeWeight(type))
             << "%   " << (avg >= 0 ? oneDecimal(avg) : paint("no grades yet", DIM)) << "\n";
    }

    // Chart 1: each student's average (1 '#' = 2.5 points)
    cout << "\n  " << paint("Student averages", BOLD)
         << paint("   ('|' marks the pass cutoff)", DIM) << "\n";
    int passColumn = static_cast<int>(book.getPassCutoff() / 2.5);
    for (const Student& s : book.getStudents()) {
        if (!s.hasGrades()) continue;
        int length = static_cast<int>(s.average() / 2.5 + 0.5);
        if (length > 40) length = 40;
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

    // Chart 2: letter grades
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
    string type = readType("  Next assignment category (homework/quiz/test/project/activity): ");
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

// 12) Command mode: the same text commands the website sends
void commandMode(CommandShell& shell) {
    printHeader("Command Mode  (type 'help' for commands, 'back' to return)");
    cout << paint("  These are the exact commands the website's buttons send to C++.\n", DIM);
    while (true) {
        cout << paint("cmd> ", CYAN);
        if (!shell.execute(cin, cout)) {
            break;
        }
    }
    cin.clear();
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

int main() {
    cout << fixed << setprecision(1);   // 1 decimal place everywhere

    cout << "\n" << paint("  ===  STUDENT GRADE TRACKER  ===", BOLD + ";" + CYAN) << "\n";
    cout << paint("  Weights: Homework 10%, Quiz 10%, Test 20%, Project 50%, Class Activity 10%", DIM) << "\n";

    // 3 Student objects made with the parameterized constructor (name, ID, starting grade)
    Student student1("Ava Chen", 1001, 92.5);
    Student student2("Marcus Johnson", 1002, 72.0);
    Student student3("Ethan Brooks", 1003, 55.0);

    student1.addGrade("C++ Unit 1", "Test", 95);
    student2.addGrade("C++ Unit 1", "Test", 62);
    student3.addGrade("C++ Unit 1", "Test", 50);

    double passCutoff = 70;
    double needsCutoff = 60;

    printHeader("Our 3 starting Student objects");
    displayStudent(student1, passCutoff, needsCutoff);
    displayStudent(student2, passCutoff, needsCutoff);
    displayStudent(student3, passCutoff, needsCutoff);

    double classAverage = (student1.average() + student2.average() + student3.average()) / 3.0;
    cout << "\n  Class average of these 3: " << paint(oneDecimal(classAverage) + "%", BOLD) << "\n";

    // Move the 3 objects into a GradeBook for the menu.
    GradeBook book;
    book.setClassName("Electronics/Computer Engineering 11");
    book.addExistingStudent(student1);
    book.addExistingStudent(student2);
    book.addExistingStudent(student3);
    book.clearUndoHistory();

    CommandShell shell(book);

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
        else running = false;
    }

    cout << paint("\n  Goodbye! Keep those grades up.\n\n", CYAN);
    return 0;
}

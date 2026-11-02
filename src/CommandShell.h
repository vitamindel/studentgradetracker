// CommandShell.h - turns text commands like  grade 1001 "Quiz 3" quiz 88
// into GradeBook calls. It reads from any istream and writes to any ostream, so
// the terminal and the website (cin/cout redirected to strings) share this code.
// Errors always start with "Error:". Text with spaces goes in "quotes".
#pragma once

#include "GradeBook.h"
#include <iostream>
#include <sstream>
#include <iomanip>   // setw, setprecision, quoted
#include <string>
#include <vector>

class CommandShell {
private:
    GradeBook& book;   // a reference: the real gradebook, not a copy

    // fmt(92.456, 1) -> "92.5"
    static std::string fmt(double value, int decimals) {
        std::ostringstream out;
        out << std::fixed << std::setprecision(decimals) << value;
        return out.str();
    }

    static std::string toLower(std::string text) {
        for (int i = 0; i < static_cast<int>(text.size()); i++) {
            text[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(text[i])));
        }
        return text;
    }

    // Text -> number with no exceptions (see GradeBook::parseNumber).
    static bool parseNumber(const std::string& text, double& result) {
        std::istringstream stream(text);
        stream >> result;
        if (stream.fail()) return false;
        std::string leftover;
        stream >> leftover;
        return leftover.empty();
    }

    static bool parseInt(const std::string& text, int& result) {
        double value;
        if (!parseNumber(text, value)) return false;
        if (value < -1000000000 || value > 1000000000) return false;   // too big for an int
        if (value != static_cast<int>(value)) return false;            // reject 3.5
        result = static_cast<int>(value);
        return true;
    }

    // Reads one word or a whole "quoted phrase" (std::quoted strips the quotes).
    static bool readText(std::istream& args, std::string& result) {
        result = "";
        args >> std::quoted(result);
        return !args.fail();
    }

    static bool readInt(std::istream& args, int& result) {
        std::string word;
        return readText(args, word) && parseInt(word, result);
    }

    static bool readNumber(std::istream& args, double& result) {
        std::string word;
        return readText(args, word) && parseNumber(word, result);
    }

    // Reads the rest of the line (quotes optional), e.g.  rename 1001 Ava Chen
    static std::string readRest(std::istream& args) {
        std::string rest;
        std::getline(args, rest);
        int start = 0;
        while (start < static_cast<int>(rest.size()) && rest[start] == ' ') start++;
        rest = rest.substr(start);
        while (!rest.empty() && (rest.back() == ' ' || rest.back() == '\r')) rest.pop_back();
        if (!rest.empty() && rest[0] == '"') {
            std::istringstream quoted(rest);
            std::string inside;
            quoted >> std::quoted(inside);
            return inside;
        }
        return rest;
    }

    // Makes text safe to put inside JSON "quotes".
    static std::string jsonString(const std::string& text) {
        std::string out = "\"";
        for (char c : text) {
            if (c == '"') out += "\\\"";
            else if (c == '\\') out += "\\\\";
            else if (c == '\n') out += "\\n";
            else if (c == '\t') out += "\\t";
            else if (static_cast<unsigned char>(c) < 0x20) out += ' ';   // other control chars
            else out += c;
        }
        return out + "\"";
    }

    // Looks up a student and prints a friendly error if the ID is unknown.
    Student* requireStudent(int id, std::ostream& out) {
        Student* s = book.findById(id);
        if (s == nullptr) {
            out << "Error: no student has ID " << id << ". Type 'list' to see IDs.\n";
        }
        return s;
    }

    // bar(3) -> "###"
    static std::string bar(int length) {
        std::string result = "";
        for (int i = 0; i < length; i++) result += "#";
        return result;
    }

    // ----- One method per command -----

    // add "<name>" [startingGrade]
    void cmdAdd(std::istream& args, std::ostream& out) {
        std::string name;
        if (!readText(args, name) || !GradeBook::isValidName(name)) {
            out << "Error: usage is  add \"<name>\" [starting grade]  (name 1-40 characters).\n";
            return;
        }
        double starting = Student::NO_GRADE;
        std::string gradeWord;
        if (readText(args, gradeWord)) {
            if (!parseNumber(gradeWord, starting) || !Student::isValidScore(starting)) {
                out << "Error: the starting grade must be a number from 0 to 120.\n";
                return;
            }
        }
        int id = book.addStudent(name, starting);
        if (id == -1) {
            out << "Error: could not add that student.\n";
            return;
        }
        out << "Added " << book.findById(id)->getName() << " (ID " << id << ")";
        if (starting != Student::NO_GRADE) out << " with a starting grade of " << fmt(starting, 1);
        out << ".\n";
    }

    // grade <id> "<label>" <type> <score>
    void cmdGrade(std::istream& args, std::ostream& out) {
        int id;
        std::string label, type;
        double score;
        if (!readInt(args, id) || !readText(args, label) || !readText(args, type) ||
            !readNumber(args, score)) {
            out << "Error: usage is  grade <id> \"<label>\" <type> <score>\n";
            return;
        }
        Student* s = requireStudent(id, out);
        if (s == nullptr) return;
        if (Student::normalizeType(type) == "") {
            out << "Error: type must be test, quiz, project, or homework.\n";
            return;
        }
        if (!Student::isValidScore(score)) {
            out << "Error: the score must be from 0 to 120.\n";
            return;
        }
        if (!book.addGrade(id, label, type, score)) {
            out << "Error: the assignment needs a name.\n";
            return;
        }
        out << "Added " << Student::normalizeType(type) << " \"" << label << "\" (" << fmt(score, 1)
            << ") for " << s->getName() << ". New average: " << fmt(s->average(), 1) << "%.\n";
    }

    // assign "<title>" <type> <id>=<score> <id>=<score> ...   (blank score = skip)
    void cmdAssign(std::istream& args, std::ostream& out) {
        std::string title, type;
        if (!readText(args, title) || !readText(args, type)) {
            out << "Error: usage is  assign \"<title>\" <type> <id>=<score> ...\n";
            return;
        }
        if (Student::normalizeType(type) == "") {
            out << "Error: type must be test, quiz, project, or homework.\n";
            return;
        }
        std::vector<int> ids;
        std::vector<double> scores;
        std::string pair;
        while (args >> pair) {                       // read "1001=88", one at a time
            int equals = static_cast<int>(pair.find('='));
            if (equals < 0) {
                out << "Error: '" << pair << "' should look like 1001=88\n";
                return;
            }
            std::string idText = pair.substr(0, equals);
            std::string scoreText = pair.substr(equals + 1);
            int id;
            if (!parseInt(idText, id) || book.findById(id) == nullptr) {
                out << "Error: '" << idText << "' is not a student ID.\n";
                return;
            }
            if (scoreText.empty()) continue;         // blank = this student skips it
            double score;
            if (!parseNumber(scoreText, score) || !Student::isValidScore(score)) {
                out << "Error: score for ID " << id << " must be from 0 to 120.\n";
                return;
            }
            ids.push_back(id);
            scores.push_back(score);
        }
        if (ids.empty()) {
            out << "Error: no scores were entered.\n";
            return;
        }
        int count = book.addAssignment(title, type, ids, scores);
        if (count < 0) {
            out << "Error: could not add that assignment.\n";
            return;
        }
        out << "Added " << Student::normalizeType(type) << " \"" << title << "\" for " << count
            << " student(s). Class average is now " << fmt(book.classAverage(), 1) << "%.\n";
    }

    // edit <id> <grade#> <score> ["<label>"] [type]
    void cmdEdit(std::istream& args, std::ostream& out) {
        int id, number;
        double score;
        if (!readInt(args, id) || !readInt(args, number) || !readNumber(args, score)) {
            out << "Error: usage is  edit <id> <grade #> <new score> [\"<label>\"] [type]\n";
            return;
        }
        Student* s = requireStudent(id, out);
        if (s == nullptr) return;
        if (number < 1 || number > s->gradeCount()) {
            out << "Error: " << s->getName() << " has grades #1 to #" << s->gradeCount() << ".\n";
            return;
        }
        // Keep the old label/type unless new ones were typed.
        GradeEntry old = s->getGrades()[number - 1];
        std::string label = old.label, type = old.type, word;
        if (readText(args, word)) label = word;
        if (readText(args, word)) type = word;
        if (Student::normalizeType(type) == "") {
            out << "Error: type must be test, quiz, project, or homework.\n";
            return;
        }
        if (!book.editGrade(id, number - 1, score, label, type)) {
            out << "Error: the score must be from 0 to 120 and the label can't be blank.\n";
            return;
        }
        out << "Updated grade #" << number << " for " << s->getName() << ": \"" << label << "\" = "
            << fmt(score, 1) << ". New average: " << fmt(s->average(), 1) << "%.\n";
    }

    // delgrade <id> <grade#>
    void cmdDelGrade(std::istream& args, std::ostream& out) {
        int id, number;
        if (!readInt(args, id) || !readInt(args, number)) {
            out << "Error: usage is  delgrade <id> <grade #>\n";
            return;
        }
        Student* s = requireStudent(id, out);
        if (s == nullptr) return;
        if (number < 1 || number > s->gradeCount()) {
            out << "Error: " << s->getName() << " has no grade #" << number << ".\n";
            return;
        }
        std::string label = s->getGrades()[number - 1].label;
        book.deleteGrade(id, number - 1);
        out << "Deleted grade #" << number << " (\"" << label << "\") from " << s->getName() << ".\n";
    }

    // rename <id> <new name>
    void cmdRename(std::istream& args, std::ostream& out) {
        int id;
        if (!readInt(args, id)) {
            out << "Error: usage is  rename <id> <new name>\n";
            return;
        }
        std::string newName = readRest(args);
        Student* s = requireStudent(id, out);
        if (s == nullptr) return;
        std::string oldName = s->getName();
        if (!book.renameStudent(id, newName)) {
            out << "Error: names must be 1-40 characters.\n";
            return;
        }
        out << "Renamed " << oldName << " to " << s->getName() << ".\n";
    }

    // remove <id>
    void cmdRemove(std::istream& args, std::ostream& out) {
        int id;
        if (!readInt(args, id)) {
            out << "Error: usage is  remove <id>\n";
            return;
        }
        Student* s = requireStudent(id, out);
        if (s == nullptr) return;
        std::string name = s->getName();   // save it now: the object is about to disappear
        book.removeStudent(id);
        out << "Removed " << name << " (ID " << id << ").\n";
    }

    // show <id>   -> a full report card
    void cmdShow(std::istream& args, std::ostream& out) {
        int id;
        if (!readInt(args, id)) {
            out << "Error: usage is  show <id>\n";
            return;
        }
        const Student* s = requireStudent(id, out);
        if (s == nullptr) return;

        out << "=== Report Card: " << s->getName() << " (ID " << s->getId() << ") ===\n";
        if (!s->hasGrades()) {
            out << "No grades yet.\n";
            return;
        }
        out << std::left << "  " << std::setw(4) << "#" << std::setw(24) << "Assignment"
            << std::setw(10) << "Type" << std::right << std::setw(6) << "Score" << "\n";
        int number = 1;
        for (const GradeEntry& g : s->getGrades()) {
            out << std::left << "  " << std::setw(4) << number << std::setw(24) << g.label
                << std::setw(10) << g.type << std::right << std::setw(6) << fmt(g.score, 1) << "\n";
            number++;
        }
        out << "Weighted average: " << fmt(s->average(), 1) << "%   Letter: " << s->letterGrade()
            << "   GPA: " << fmt(s->gpa(), 1) << "\n";
        out << "Status: " << book.statusOf(*s) << "   Trend: " << s->trend()
            << "   Best: " << fmt(s->highestScore(), 1) << "   Lowest: " << fmt(s->lowestScore(), 1)
            << "\n";
    }

    // need <id> <type> <target>
    void cmdNeed(std::istream& args, std::ostream& out) {
        int id;
        std::string type;
        double target;
        if (!readInt(args, id) || !readText(args, type) || !readNumber(args, target)) {
            out << "Error: usage is  need <id> <type> <target %>\n";
            return;
        }
        const Student* s = requireStudent(id, out);
        if (s == nullptr) return;
        std::string niceType = Student::normalizeType(type);
        if (niceType == "") {
            out << "Error: type must be test, quiz, project, or homework.\n";
            return;
        }
        if (target <= 0 || target > 100) {
            out << "Error: the target must be between 0 and 100.\n";
            return;
        }
        double needed = s->scoreNeeded(niceType, target);
        if (needed > 100) {
            out << s->getName() << " would need " << fmt(needed, 1) << "% on the next " << niceType
                << " to reach " << fmt(target, 1) << "% - not possible with one " << niceType << ".\n";
        } else if (needed <= 0) {
            out << s->getName() << " is already safe: even a 0 on the next " << niceType
                << " keeps the average at or above " << fmt(target, 1) << "%.\n";
        } else {
            out << s->getName() << " needs " << fmt(needed, 1) << "% on the next " << niceType
                << " to reach " << fmt(target, 1) << "%.\n";
        }
    }

    // list  -> the roster as a table
    void cmdList(std::ostream& out) {
        out << book.getClassName() << " (" << book.size() << " students)\n";
        if (book.size() == 0) {
            out << "No students yet. Try: add \"Ava Chen\" 92   or   demo\n";
            return;
        }
        out << std::left << "  " << std::setw(6) << "ID" << std::setw(22) << "Name" << std::right
            << std::setw(6) << "Avg" << "  " << std::left << std::setw(4) << "Ltr" << "Status\n";
        for (const Student& s : book.getStudents()) {
            out << std::left << "  " << std::setw(6) << s.getId() << std::setw(22) << s.getName()
                << std::right << std::setw(6) << (s.hasGrades() ? fmt(s.average(), 1) : "--")
                << "  " << std::left << std::setw(4) << (s.hasGrades() ? s.letterGrade() : "-")
                << book.statusOf(s) << "\n";
        }
    }

    // find <text>
    void cmdFind(std::istream& args, std::ostream& out) {
        std::string query = readRest(args);
        if (query.empty()) {
            out << "Error: usage is  find <part of a name>\n";
            return;
        }
        std::vector<int> ids = book.search(query);
        if (ids.empty()) {
            out << "No students match \"" << query << "\".\n";
            return;
        }
        out << "Found " << ids.size() << " student(s) matching \"" << query << "\":\n";
        for (int id : ids) {
            const Student* s = book.findById(id);
            out << "  [" << id << "] " << s->getName();
            if (s->hasGrades()) out << " - " << fmt(s->average(), 1) << "% (" << s->letterGrade() << ")";
            out << "\n";
        }
    }

    // sort <name|average|id> [desc]
    void cmdSort(std::istream& args, std::ostream& out) {
        std::string field, direction;
        args >> field >> direction;
        field = toLower(field);
        if (field != "name" && field != "average" && field != "id") {
            out << "Error: usage is  sort <name|average|id> [desc]\n";
            return;
        }
        bool descending = toLower(direction) == "desc";
        book.sortBy(field, descending);
        out << "Sorted by " << field << (descending ? " (descending)" : " (ascending)") << ".\n";
    }

    // stats -> text statistics with ASCII bar charts
    void cmdStats(std::ostream& out) {
        out << "=== Class Statistics: " << book.getClassName() << " ===\n";
        out << "Students: " << book.size() << " (" << book.gradedCount() << " with grades)\n";
        if (book.gradedCount() == 0) {
            out << "No grades yet.\n";
            return;
        }
        out << "Average: " << fmt(book.classAverage(), 1) << "   Median: " << fmt(book.median(), 1)
            << "   Std dev: " << fmt(book.standardDeviation(), 1) << "\n";
        out << "Highest: " << fmt(book.highestAverage(), 1) << "   Lowest: "
            << fmt(book.lowestAverage(), 1) << "   Pass rate: " << fmt(book.passRate(), 1) << "%\n";
        out << "Passed: " << book.countStatus("Passed")
            << "   Needs Improvement: " << book.countStatus("Needs Improvement")
            << "   Failed: " << book.countStatus("Failed") << "\n";
        out << "Letter grades:\n";
        std::string letters[5] = {"A", "B", "C", "D", "F"};
        for (int i = 0; i < 5; i++) {
            int count = book.countLetter(letters[i]);
            out << "  " << letters[i] << " | " << bar(count * 3) << " " << count << "\n";
        }
    }

    // curve <points> [preview]
    void cmdCurve(std::istream& args, std::ostream& out) {
        double points;
        if (!readNumber(args, points)) {
            out << "Error: usage is  curve <points> [preview]\n";
            return;
        }
        std::string mode;
        args >> mode;
        double before = book.classAverage();

        if (toLower(mode) == "preview") {
            if (points < 0 || points > 50) {
                out << "Error: curve points must be from 0 to 50.\n";
                return;
            }
            GradeBook curved = book.curvedCopy(points);   // a "what if" copy
            out << "Curve preview (+" << fmt(points, 1) << "): class average " << fmt(before, 2)
                << " -> " << fmt(curved.classAverage(), 2) << ", pass rate "
                << fmt(book.passRate(), 2) << " -> " << fmt(curved.passRate(), 2) << "\n";
            return;
        }
        if (!book.applyCurve(points)) {
            out << "Error: curve must be more than 0 and at most 50 points, and someone needs grades.\n";
            return;
        }
        out << "Curved every grade by +" << fmt(points, 1) << " (capped at 100). Class average: "
            << fmt(before, 2) << " -> " << fmt(book.classAverage(), 2) << "\n";
    }

    // thresholds <pass> <needs-improvement>
    void cmdThresholds(std::istream& args, std::ostream& out) {
        double pass, needs;
        if (!readNumber(args, pass) || !readNumber(args, needs)) {
            out << "Error: usage is  thresholds <pass cutoff> <needs-improvement cutoff>\n";
            return;
        }
        if (!book.setThresholds(pass, needs)) {
            out << "Error: cutoffs must follow  0 <= needs-improvement < pass <= 100.\n";
            return;
        }
        out << "Cutoffs updated: Passed >= " << fmt(pass, 1) << ", Needs Improvement >= "
            << fmt(needs, 1) << ", below that = Failed.\n";
    }

    // classname <name>
    void cmdClassName(std::istream& args, std::ostream& out) {
        std::string name = readRest(args);
        if (!book.setClassName(name)) {
            out << "Error: class name must be 1-60 characters.\n";
            return;
        }
        out << "Class renamed to \"" << book.getClassName() << "\".\n";
    }

    // import  -> reads CSV lines until the input ends or a line says "end"
    void cmdImport(std::istream& in, std::ostream& out) {
        std::string text, line;
        while (std::getline(in, line)) {
            if (line == "end" || line == "end\r") break;
            text += line + "\n";
        }
        int studentsLoaded, gradesLoaded, skipped;
        if (!book.importCsv(text, studentsLoaded, gradesLoaded, skipped)) {
            out << "Error: no valid rows found. Expected columns: id,name,assignment,type,score\n";
            return;
        }
        out << "Imported " << studentsLoaded << " student(s) and " << gradesLoaded << " grade(s)";
        if (skipped > 0) out << " (" << skipped << " bad row(s) skipped)";
        out << ".\n";
    }

    // json -> the WHOLE gradebook as JSON, so the website can draw it
    void cmdJson(std::ostream& out) {
        out << "{";
        out << "\"className\":" << jsonString(book.getClassName());
        out << ",\"passCutoff\":" << fmt(book.getPassCutoff(), 2);
        out << ",\"needsCutoff\":" << fmt(book.getNeedsCutoff(), 2);
        out << ",\"canUndo\":" << (book.canUndo() ? "true" : "false");
        out << ",\"undoLabel\":" << jsonString(book.undoDescription());
        out << ",\"weights\":{\"Test\":2,\"Project\":1.5,\"Quiz\":1,\"Homework\":0.5}";

        out << ",\"stats\":{";
        out << "\"count\":" << book.size();
        out << ",\"graded\":" << book.gradedCount();
        out << ",\"average\":" << fmt(book.classAverage(), 2);
        out << ",\"classLetter\":"
            << jsonString(book.gradedCount() > 0 ? Student::letterFor(book.classAverage()) : "-");
        out << ",\"median\":" << fmt(book.median(), 2);
        out << ",\"stdDev\":" << fmt(book.standardDeviation(), 2);
        out << ",\"passRate\":" << fmt(book.passRate(), 2);
        out << ",\"highest\":" << fmt(book.highestAverage(), 2);
        out << ",\"lowest\":" << fmt(book.lowestAverage(), 2);
        out << ",\"passed\":" << book.countStatus("Passed");
        out << ",\"needsImprovement\":" << book.countStatus("Needs Improvement");
        out << ",\"failed\":" << book.countStatus("Failed");
        out << ",\"noGrades\":" << book.countStatus("No Grades");
        out << ",\"letters\":{\"A\":" << book.countLetter("A") << ",\"B\":" << book.countLetter("B")
            << ",\"C\":" << book.countLetter("C") << ",\"D\":" << book.countLetter("D")
            << ",\"F\":" << book.countLetter("F") << "}";
        out << ",\"topId\":" << book.topStudentId();
        out << ",\"riskId\":" << book.atRiskStudentId();
        out << "}";

        out << ",\"students\":[";
        bool firstStudent = true;
        for (const Student& s : book.getStudents()) {
            if (!firstStudent) out << ",";
            firstStudent = false;
            out << "{\"id\":" << s.getId();
            out << ",\"name\":" << jsonString(s.getName());
            out << ",\"hasGrades\":" << (s.hasGrades() ? "true" : "false");
            out << ",\"average\":" << fmt(s.average(), 2);
            out << ",\"letter\":" << jsonString(s.hasGrades() ? s.letterGrade() : "-");
            out << ",\"gpa\":" << fmt(s.gpa(), 1);
            out << ",\"status\":" << jsonString(book.statusOf(s));
            out << ",\"trend\":" << jsonString(s.trend());
            out << ",\"highest\":" << fmt(s.highestScore(), 2);
            out << ",\"lowest\":" << fmt(s.lowestScore(), 2);
            out << ",\"grades\":[";
            for (int i = 0; i < s.gradeCount(); i++) {
                const GradeEntry& g = s.getGrades()[i];
                if (i > 0) out << ",";
                out << "{\"label\":" << jsonString(g.label) << ",\"type\":" << jsonString(g.type)
                    << ",\"score\":" << fmt(g.score, 2)
                    << ",\"running\":" << fmt(s.averageOfFirst(i + 1), 2) << "}";
            }
            out << "]}";
        }
        out << "]}\n";
    }

    void cmdHelp(std::ostream& out) {
        out << "Commands (text with spaces goes in \"quotes\"):\n"
            << "  add \"<name>\" [grade]                 add a student\n"
            << "  grade <id> \"<label>\" <type> <score>  add a grade (test/quiz/project/homework)\n"
            << "  assign \"<title>\" <type> <id>=<score> ...  grade the whole class at once\n"
            << "  edit <id> <#> <score> [\"<label>\"] [type]  change grade number #\n"
            << "  delgrade <id> <#>                    delete grade number #\n"
            << "  rename <id> <new name>               rename a student\n"
            << "  remove <id>                          remove a student\n"
            << "  show <id>                            report card\n"
            << "  need <id> <type> <target>            score needed on the next assignment\n"
            << "  list | find <text> | sort <name|average|id> [desc]\n"
            << "  stats                                class statistics + charts\n"
            << "  curve <points> [preview]             add points to every grade\n"
            << "  thresholds <pass> <needs>            change the status cutoffs\n"
            << "  classname <name>                     rename the class\n"
            << "  undo | resetundo | demo | clear | export | import | json | help | exit\n";
    }

public:
    // A reference member must be set in the initializer list (after the colon).
    CommandShell(GradeBook& gradeBook) : book(gradeBook) {}

    // Runs one command from "in", writing the answer to "out".
    // Returns false at the end of input or on exit/quit/back.
    bool execute(std::istream& in, std::ostream& out) {
        std::string line;
        if (!std::getline(in, line)) {
            return false;
        }
        if (!line.empty() && line.back() == '\r') line.pop_back();

        std::istringstream args(line);
        std::string command;
        args >> command;
        command = toLower(command);

        // Tables change the stream's formatting, so restore it afterwards.
        std::ios::fmtflags savedFlags = out.flags();
        bool keepGoing = runCommand(command, args, in, out);
        out.flags(savedFlags);
        return keepGoing;
    }

private:
    bool runCommand(const std::string& command, std::istringstream& args,
                    std::istream& in, std::ostream& out) {
        if (command.empty()) return true;
        else if (command == "add") cmdAdd(args, out);
        else if (command == "grade") cmdGrade(args, out);
        else if (command == "assign") cmdAssign(args, out);
        else if (command == "edit") cmdEdit(args, out);
        else if (command == "delgrade") cmdDelGrade(args, out);
        else if (command == "rename") cmdRename(args, out);
        else if (command == "remove") cmdRemove(args, out);
        else if (command == "show") cmdShow(args, out);
        else if (command == "need") cmdNeed(args, out);
        else if (command == "list") cmdList(out);
        else if (command == "find") cmdFind(args, out);
        else if (command == "sort") cmdSort(args, out);
        else if (command == "stats") cmdStats(out);
        else if (command == "curve") cmdCurve(args, out);
        else if (command == "thresholds") cmdThresholds(args, out);
        else if (command == "classname") cmdClassName(args, out);
        else if (command == "import") cmdImport(in, out);   // reads MORE lines from "in"
        else if (command == "export") out << book.exportCsv();
        else if (command == "json") cmdJson(out);
        else if (command == "help") cmdHelp(out);
        else if (command == "undo") {
            std::string what = book.undo();
            if (what == "") out << "Error: nothing to undo.\n";
            else out << "Undid: " << what << ".\n";
        } else if (command == "demo") {
            book.loadDemo();
            out << "Loaded the demo class (" << book.size() << " students).\n";
        } else if (command == "clear") {
            book.clearAll();
            out << "Removed all students.\n";
        } else if (command == "resetundo") {
            // Sent by the website after restoring saved data, so Ctrl+Z can't undo the page load.
            book.clearUndoHistory();
            out << "Undo history cleared.\n";
        } else if (command == "exit" || command == "quit" || command == "back") {
            return false;
        } else {
            out << "Error: unknown command '" << command << "'. Type 'help' to see all commands.\n";
        }
        return true;
    }
};

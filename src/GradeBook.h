// GradeBook.h - the whole class: a list of Student objects plus class stats,
// sorting, curving, undo, and CSV import/export.
#pragma once

#include "Student.h"
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cmath>

class GradeBook {
private:
    // A full copy of everything that can change, saved before each edit (for undo).
    struct Snapshot {
        std::vector<Student> students;
        int nextId;
        std::string className;
        double passCutoff;
        double needsCutoff;
        std::string description;   // e.g. "add grade"
    };

    std::string className;
    std::vector<Student> students;
    int nextId;                      // ID for the next new student
    double passCutoff;
    double needsCutoff;
    std::vector<Snapshot> undoStack; // newest at the back

    static const int MAX_UNDO = 50;

    void saveSnapshot(const std::string& description) {
        Snapshot snap;
        snap.students = students;
        snap.nextId = nextId;
        snap.className = className;
        snap.passCutoff = passCutoff;
        snap.needsCutoff = needsCutoff;
        snap.description = description;
        undoStack.push_back(snap);
        if (static_cast<int>(undoStack.size()) > MAX_UNDO) {
            undoStack.erase(undoStack.begin());
        }
    }

    // Used when an edit turns out to be invalid.
    void discardLastSnapshot() {
        if (!undoStack.empty()) {
            undoStack.pop_back();
        }
    }

    // Averages of students who have grades (others are left out of class stats).
    std::vector<double> gradedAverages() const {
        std::vector<double> result;
        for (const Student& s : students) {
            if (s.hasGrades()) {
                result.push_back(s.average());
            }
        }
        return result;
    }

    static std::string toLower(const std::string& text) {
        std::string result = text;
        for (int i = 0; i < static_cast<int>(result.size()); i++) {
            result[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(result[i])));
        }
        return result;
    }

    static std::string trim(const std::string& text) {
        int start = 0;
        int end = static_cast<int>(text.size()) - 1;
        while (start <= end && std::isspace(static_cast<unsigned char>(text[start]))) start++;
        while (end >= start && std::isspace(static_cast<unsigned char>(text[end]))) end--;
        return text.substr(start, end - start + 1);
    }

    // Should a come before b when sorting by this field?
    bool comesBefore(const Student& a, const Student& b,
                     const std::string& field, bool descending) const {
        if (field == "average") {
            if (a.hasGrades() != b.hasGrades()) return a.hasGrades();   // no grades go last
            if (descending) return a.average() > b.average();
            return a.average() < b.average();
        }
        if (field == "id") {
            if (descending) return a.getId() > b.getId();
            return a.getId() < b.getId();
        }
        if (descending) return toLower(a.getName()) > toLower(b.getName());
        return toLower(a.getName()) < toLower(b.getName());
    }

    // Quotes a CSV field if it contains a comma or quote.
    static std::string csvField(const std::string& text) {
        bool needsQuotes = text.find(',') != std::string::npos ||
                           text.find('"') != std::string::npos;
        if (!needsQuotes) return text;
        std::string result = "\"";
        for (char c : text) {
            if (c == '"') result += "\"\"";
            else result += c;
        }
        return result + "\"";
    }

    // Splits one CSV line into fields, handling "quoted, fields".
    static std::vector<std::string> splitCsvLine(const std::string& line) {
        std::vector<std::string> fields;
        std::string current = "";
        bool inQuotes = false;
        for (int i = 0; i < static_cast<int>(line.size()); i++) {
            char c = line[i];
            if (inQuotes) {
                if (c == '"' && i + 1 < static_cast<int>(line.size()) && line[i + 1] == '"') {
                    current += '"';
                    i++;
                } else if (c == '"') {
                    inQuotes = false;
                } else {
                    current += c;
                }
            } else if (c == '"') {
                inQuotes = true;
            } else if (c == ',') {
                fields.push_back(current);
                current = "";
            } else {
                current += c;
            }
        }
        fields.push_back(current);
        return fields;
    }

    // Text to number without exceptions (stod would throw; the web build has exceptions off).
    static bool parseNumber(const std::string& text, double& result) {
        std::istringstream stream(text);
        stream >> result;
        if (stream.fail()) return false;
        std::string leftover;
        stream >> leftover;            // rejects things like "88abc"
        return leftover.empty();
    }

public:
    GradeBook() {
        className = "My Class";
        nextId = 1001;
        passCutoff = 70;
        needsCutoff = 60;
    }

    // ----- Settings -----
    std::string getClassName() const { return className; }
    double getPassCutoff() const { return passCutoff; }
    double getNeedsCutoff() const { return needsCutoff; }

    bool setClassName(const std::string& newName) {
        std::string cleaned = trim(newName);
        if (cleaned.empty() || cleaned.size() > 60) return false;
        saveSnapshot("rename class");
        className = cleaned;
        return true;
    }

    // Must be 0 <= needs-improvement < pass <= 100
    bool setThresholds(double newPass, double newNeeds) {
        if (!(newNeeds >= 0 && newNeeds < newPass && newPass <= 100)) {
            return false;
        }
        saveSnapshot("change cutoffs");
        passCutoff = newPass;
        needsCutoff = newNeeds;
        return true;
    }

    std::string statusOf(const Student& s) const {
        return s.status(passCutoff, needsCutoff);
    }

    // ----- Students -----
    const std::vector<Student>& getStudents() const { return students; }
    int size() const { return static_cast<int>(students.size()); }

    static bool isValidName(const std::string& name) {
        std::string cleaned = trim(name);
        return !cleaned.empty() && cleaned.size() <= 40;
    }

    // Returns the new student's ID, or -1 if the input was invalid.
    int addStudent(const std::string& name, double startingGrade) {
        if (!isValidName(name)) return -1;
        if (startingGrade != Student::NO_GRADE && !Student::isValidScore(startingGrade)) return -1;

        saveSnapshot("add student");
        int newId = nextId;
        nextId++;
        Student s(trim(name), newId, startingGrade);   // constructor call
        students.push_back(s);
        return newId;
    }

    // Adds a Student built elsewhere (main() builds three). Refuses duplicate IDs.
    bool addExistingStudent(const Student& s) {
        if (findById(s.getId()) != nullptr) return false;
        saveSnapshot("add student");
        students.push_back(s);
        if (s.getId() >= nextId) {
            nextId = s.getId() + 1;
        }
        return true;
    }

    // Returns a pointer to the real student (like a Java reference), or nullptr.
    Student* findById(int id) {
        for (int i = 0; i < size(); i++) {
            if (students[i].getId() == id) {
                return &students[i];
            }
        }
        return nullptr;
    }

    // Read-only version for const methods.
    const Student* findById(int id) const {
        for (int i = 0; i < size(); i++) {
            if (students[i].getId() == id) return &students[i];
        }
        return nullptr;
    }

    bool removeStudent(int id) {
        for (int i = 0; i < size(); i++) {
            if (students[i].getId() == id) {
                saveSnapshot("remove student");
                students.erase(students.begin() + i);
                return true;
            }
        }
        return false;
    }

    bool renameStudent(int id, const std::string& newName) {
        Student* s = findById(id);
        if (s == nullptr || !isValidName(newName)) return false;
        saveSnapshot("rename student");
        s->setName(trim(newName));   // -> calls a method through a pointer
        return true;
    }

    // Case-insensitive name search. Returns matching IDs.
    std::vector<int> search(const std::string& query) const {
        std::vector<int> matches;
        std::string q = toLower(trim(query));
        for (const Student& s : students) {
            if (toLower(s.getName()).find(q) != std::string::npos) {
                matches.push_back(s.getId());
            }
        }
        return matches;
    }

    // ----- Grades -----
    bool addGrade(int id, const std::string& label, const std::string& type, double score) {
        Student* s = findById(id);
        if (s == nullptr || trim(label).empty()) return false;
        saveSnapshot("add grade");
        bool ok = s->addGrade(trim(label), type, score);
        if (!ok) discardLastSnapshot();
        return ok;
    }

    // index is 0-based
    bool editGrade(int id, int index, double newScore, const std::string& newLabel,
                   const std::string& newType) {
        Student* s = findById(id);
        if (s == nullptr || trim(newLabel).empty()) return false;
        saveSnapshot("edit grade");
        bool ok = s->editGrade(index, newScore, trim(newLabel), newType);
        if (!ok) discardLastSnapshot();
        return ok;
    }

    bool deleteGrade(int id, int index) {
        Student* s = findById(id);
        if (s == nullptr) return false;
        saveSnapshot("delete grade");
        bool ok = s->removeGrade(index);
        if (!ok) discardLastSnapshot();
        return ok;
    }

    // One grade for many students: ids[i] gets scores[i]. Everything is checked
    // first so a bad score can't leave the class half-graded. Returns the count, or -1.
    int addAssignment(const std::string& label, const std::string& type,
                      const std::vector<int>& ids, const std::vector<double>& scores) {
        if (trim(label).empty() || Student::normalizeType(type) == "") return -1;
        if (ids.empty() || ids.size() != scores.size()) return -1;
        for (int i = 0; i < static_cast<int>(ids.size()); i++) {
            if (findById(ids[i]) == nullptr || !Student::isValidScore(scores[i])) return -1;
        }
        saveSnapshot("class assignment");
        for (int i = 0; i < static_cast<int>(ids.size()); i++) {
            findById(ids[i])->addGrade(trim(label), type, scores[i]);
        }
        return static_cast<int>(ids.size());
    }

    // ----- Class statistics -----
    int gradedCount() const { return static_cast<int>(gradedAverages().size()); }

    double classAverage() const {
        std::vector<double> avgs = gradedAverages();
        if (avgs.empty()) return 0;
        double sum = 0;
        for (double a : avgs) sum += a;
        return sum / static_cast<double>(avgs.size());
    }

    double median() const {
        std::vector<double> avgs = gradedAverages();
        if (avgs.empty()) return 0;
        std::sort(avgs.begin(), avgs.end());
        int n = static_cast<int>(avgs.size());
        if (n % 2 == 1) return avgs[n / 2];
        return (avgs[n / 2 - 1] + avgs[n / 2]) / 2.0;
    }

    // sqrt( average of (value - mean)^2 )
    double standardDeviation() const {
        std::vector<double> avgs = gradedAverages();
        if (avgs.empty()) return 0;
        double mean = classAverage();
        double sumSquares = 0;
        for (double a : avgs) {
            sumSquares += (a - mean) * (a - mean);
        }
        return std::sqrt(sumSquares / static_cast<double>(avgs.size()));
    }

    double passRate() const {
        int graded = gradedCount();
        if (graded == 0) return 0;
        return 100.0 * countStatus("Passed") / graded;
    }

    double highestAverage() const {
        std::vector<double> avgs = gradedAverages();
        if (avgs.empty()) return 0;
        double best = avgs[0];
        for (double a : avgs) if (a > best) best = a;
        return best;
    }

    double lowestAverage() const {
        std::vector<double> avgs = gradedAverages();
        if (avgs.empty()) return 0;
        double worst = avgs[0];
        for (double a : avgs) if (a < worst) worst = a;
        return worst;
    }

    // Class average for one category (students with no grades in it are skipped), or -1.
    double categoryClassAverage(const std::string& type) const {
        double sum = 0;
        int count = 0;
        for (const Student& s : students) {
            double avg = s.categoryAverage(type);
            if (avg >= 0) {
                sum += avg;
                count++;
            }
        }
        if (count == 0) return -1;
        return sum / count;
    }

    int countLetter(const std::string& letter) const {
        int count = 0;
        for (const Student& s : students) {
            if (s.hasGrades() && s.letterGrade() == letter) count++;
        }
        return count;
    }

    int countStatus(const std::string& status) const {
        int count = 0;
        for (const Student& s : students) {
            if (statusOf(s) == status) count++;
        }
        return count;
    }

    // -1 if nobody has grades
    int topStudentId() const {
        int bestId = -1;
        double best = -1;
        for (const Student& s : students) {
            if (s.hasGrades() && s.average() > best) {
                best = s.average();
                bestId = s.getId();
            }
        }
        return bestId;
    }

    // The lowest average, but only if that student isn't passing.
    int atRiskStudentId() const {
        int worstId = -1;
        double worst = 1000;
        for (const Student& s : students) {
            if (s.hasGrades() && s.average() < worst) {
                worst = s.average();
                worstId = s.getId();
            }
        }
        if (worstId != -1 && worst >= passCutoff) return -1;
        return worstId;
    }

    // ----- Sorting (selection sort, like AP CSA). field: name, average, or id -----
    void sortBy(const std::string& field, bool descending) {
        int n = size();
        for (int i = 0; i < n - 1; i++) {
            int bestIndex = i;
            for (int j = i + 1; j < n; j++) {
                if (comesBefore(students[j], students[bestIndex], field, descending)) {
                    bestIndex = j;
                }
            }
            if (bestIndex != i) {
                Student temp = students[i];
                students[i] = students[bestIndex];
                students[bestIndex] = temp;
            }
        }
    }

    // ----- Curve -----
    bool applyCurve(double points) {
        if (!(points > 0 && points <= 50) || gradedCount() == 0) return false;
        saveSnapshot("curve");
        for (int i = 0; i < size(); i++) {
            students[i].curveGrades(points);
        }
        return true;
    }

    // A curved copy for previews; the real gradebook isn't touched.
    GradeBook curvedCopy(double points) const {
        GradeBook copy;
        copy.students = students;    // undo history is not copied
        copy.className = className;
        copy.passCutoff = passCutoff;
        copy.needsCutoff = needsCutoff;
        for (int i = 0; i < copy.size(); i++) {
            copy.students[i].curveGrades(points);
        }
        return copy;
    }

    double previewCurve(double points) const {
        return curvedCopy(points).classAverage();
    }

    // ----- Undo -----
    bool canUndo() const { return !undoStack.empty(); }

    void clearUndoHistory() { undoStack.clear(); }

    std::string undoDescription() const {
        if (undoStack.empty()) return "";
        return undoStack.back().description;
    }

    // Restores the newest snapshot. Returns what was undone ("" if nothing).
    std::string undo() {
        if (undoStack.empty()) return "";
        Snapshot snap = undoStack.back();
        undoStack.pop_back();
        students = snap.students;
        nextId = snap.nextId;
        className = snap.className;
        passCutoff = snap.passCutoff;
        needsCutoff = snap.needsCutoff;
        return snap.description;
    }

    // ----- Clear and demo data -----
    void clearAll() {
        saveSnapshot("clear class");
        students.clear();
        nextId = 1001;
    }

    void loadDemo() {
        saveSnapshot("load demo");
        students.clear();
        nextId = 1001;
        className = "Electronics/Computer Engineering 11";

        // Marking Period 1 so far (no Project yet, so that category is skipped).
        const int NUM_ASSIGNMENTS = 9;
        std::string labels[NUM_ASSIGNMENTS] = {
            "Security Findings", "Next Level Cyber", "Intro to Cybersecurity",
            "Cryptography", "Cybersecurity Analysis", "Cybersecurity - AWS",
            "Elec Symbols & Design", "C++ Beginners", "C++ Unit 1"};
        std::string types[NUM_ASSIGNMENTS] = {
            "Class Activity", "Homework", "Quiz", "Homework", "Test", "Test",
            "Quiz", "Class Activity", "Test"};

        const int NUM_STUDENTS = 8;
        std::string names[NUM_STUDENTS] = {
            "Ava Chen", "Marcus Johnson", "Priya Patel", "Diego Ramirez",
            "Sofia Rossi", "Ethan Brooks", "Hana Kim", "Liam O'Connor"};
        double scores[NUM_STUDENTS][NUM_ASSIGNMENTS] = {   // 2D array, like double[][] in Java
            {100, 100, 95, 100, 94, 96, 100, 100, 97},
            {100, 80, 75, 70, 72, 68, 80, 100, 66},
            {100, 100, 90, 100, 88, 91, 90, 100, 95},
            {80, 60, 65, 70, 62, 58, 70, 80, 70},
            {100, 90, 85, 90, 84, 82, 90, 100, 90},
            {60, 50, 55, 40, 52, 48, 60, 80, 45},
            {80, 70, 70, 80, 72, 78, 85, 100, 88},
            {100, 100, 90, 90, 85, 78, 75, 80, 66}};

        for (int s = 0; s < NUM_STUDENTS; s++) {
            Student student(names[s], nextId, Student::NO_GRADE);
            nextId++;
            for (int a = 0; a < NUM_ASSIGNMENTS; a++) {
                student.addGrade(labels[a], types[a], scores[s][a]);
            }
            students.push_back(student);
        }
    }

    // ----- CSV: one row per grade -> id,name,assignment,type,score -----
    std::string exportCsv() const {
        std::ostringstream out;
        out << "id,name,assignment,type,score\n";
        for (const Student& s : students) {
            if (!s.hasGrades()) {
                out << s.getId() << "," << csvField(s.getName()) << ",,,\n";
            }
            for (const GradeEntry& g : s.getGrades()) {
                out << s.getId() << "," << csvField(s.getName()) << ","
                    << csvField(g.label) << "," << g.type << "," << g.score << "\n";
            }
        }
        return out.str();
    }

    // Replaces the roster with the CSV's students. The int& parameters send counts
    // back to the caller. Returns false (and changes nothing) if no rows were valid.
    bool importCsv(const std::string& text, int& studentsLoaded, int& gradesLoaded,
                   int& rowsSkipped) {
        studentsLoaded = 0;
        gradesLoaded = 0;
        rowsSkipped = 0;

        std::vector<Student> loaded;
        int maxId = 1000;
        std::istringstream lines(text);
        std::string line;

        while (std::getline(lines, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (trim(line).empty()) continue;

            std::vector<std::string> f = splitCsvLine(line);
            if (toLower(trim(f[0])) == "id") continue;   // header row

            double idNumber;
            if (f.size() < 2 || !parseNumber(trim(f[0]), idNumber) || idNumber < 1 ||
                idNumber > 9999999 || !isValidName(f[1])) {
                rowsSkipped++;
                continue;
            }
            int id = static_cast<int>(idNumber);

            int index = -1;
            for (int i = 0; i < static_cast<int>(loaded.size()); i++) {
                if (loaded[i].getId() == id) index = i;
            }
            if (index == -1) {
                loaded.push_back(Student(trim(f[1]), id, Student::NO_GRADE));
                index = static_cast<int>(loaded.size()) - 1;
                if (id > maxId) maxId = id;
            }

            // A row with no grade columns just means "this student exists".
            bool hasGradeColumns = f.size() >= 5 && !trim(f[4]).empty();
            if (!hasGradeColumns) continue;

            double score;
            std::string label = trim(f[2]);
            if (label.empty() || !parseNumber(trim(f[4]), score) ||
                !loaded[index].addGrade(label, trim(f[3]), score)) {
                rowsSkipped++;
                continue;
            }
            gradesLoaded++;
        }

        if (loaded.empty()) return false;

        saveSnapshot("import CSV");
        students = loaded;
        nextId = maxId + 1;
        studentsLoaded = static_cast<int>(loaded.size());
        return true;
    }
};

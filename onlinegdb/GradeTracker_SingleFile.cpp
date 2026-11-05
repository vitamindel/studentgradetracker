// =====================================================================
//  GradeTracker_SingleFile.cpp  (GENERATED - edit the files in src/ instead)
//  Student.h + GradeBook.h + CommandShell.h + main.cpp in one file, so it
//  can be pasted straight into OnlineGDB (Language: C++17) and run.
// =====================================================================
// main.cpp - terminal version of the Student Grade Tracker (a cin/cout menu).
// Uses the same classes as the website.
// Compile:  g++ -std=c++17 -Wall -Wextra main.cpp -o tracker
// ----- begin Student.h -----
// Student.h - one student: their name, ID, and grade history.

#include <string>
#include <vector>
#include <cctype>

// One grade in a student's history (a struct is a class with public members).
struct GradeEntry {
    std::string label;  // e.g. "C++ Unit 1"
    std::string type;   // a category: Homework, Quiz, Test, Project, Class Activity
    double score;       // 0 to 120 (over 100 = extra credit)
};

class Student {
private:
    std::string name;
    int id;
    std::vector<GradeEntry> grades;   // oldest first

public:
    static const int NO_GRADE = -1;   // "start with no grades"
    static const int NUM_TYPES = 5;

    // Parameterized constructor: Student s("Ava Chen", 1001, 92);
    Student(const std::string& name, int id, double startingGrade) {
        this->name = name;
        this->id = id;

        // The starting grade is recorded as a Quiz.
        if (startingGrade != NO_GRADE && isValidStartingGrade(startingGrade)) {
            grades.push_back({"Starting Grade", "Quiz", startingGrade});
        }
    }

    // The grading categories, in the same order as the school portal.
    static std::string typeName(int index) {
        std::string names[NUM_TYPES] = {"Homework", "Quiz", "Test", "Project", "Class Activity"};
        return names[index];
    }

    // Turns "test", "T", "hw", "activity", ... into a category name, or "" if invalid.
    static std::string normalizeType(const std::string& raw) {
        std::string t = "";
        for (char c : raw) {
            t += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        if (t == "homework" || t == "h" || t == "hw")                    return "Homework";
        if (t == "quiz" || t == "q" || t == "quizzes")                   return "Quiz";
        if (t == "test" || t == "t" || t == "tests" || t == "exam")      return "Test";
        if (t == "project" || t == "p" || t == "proj" || t == "lab")     return "Project";
        if (t == "class activity" || t == "activity" || t == "ca" || t == "classwork") return "Class Activity";
        return "";
    }

    // Each category's share of the final grade, in percent (they add up to 100).
    static double typeWeight(const std::string& type) {
        if (type == "Homework")       return 10;
        if (type == "Quiz")           return 10;
        if (type == "Test")           return 20;
        if (type == "Project")        return 50;
        if (type == "Class Activity") return 10;
        return 0;
    }

    // Scores can go up to 120 (extra credit), but a starting grade can't.
    static bool isValidScore(double score) { return score >= 0 && score <= 120; }
    static bool isValidStartingGrade(double grade) { return grade >= 0 && grade <= 100; }

    // Getters and setters
    std::string getName() const { return name; }
    int getId() const { return id; }
    const std::vector<GradeEntry>& getGrades() const { return grades; }
    int gradeCount() const { return static_cast<int>(grades.size()); }
    bool hasGrades() const { return !grades.empty(); }
    void setName(const std::string& newName) { name = newName; }

    bool addGrade(const std::string& label, const std::string& rawType, double score) {
        std::string type = normalizeType(rawType);
        if (type == "" || !isValidScore(score)) {
            return false;
        }
        grades.push_back({label, type, score});
        return true;
    }

    // index is 0-based
    bool editGrade(int index, double newScore, const std::string& newLabel,
                   const std::string& newRawType) {
        if (index < 0 || index >= gradeCount() || !isValidScore(newScore)) {
            return false;
        }
        std::string type = normalizeType(newRawType);
        if (type == "") {
            return false;
        }
        grades[index].score = newScore;
        grades[index].label = newLabel;
        grades[index].type = type;
        return true;
    }

    bool removeGrade(int index) {
        if (index < 0 || index >= gradeCount()) {
            return false;
        }
        grades.erase(grades.begin() + index);
        return true;
    }

    // Adds points to every grade, capped at 100.
    void curveGrades(double points) {
        for (int i = 0; i < gradeCount(); i++) {
            if (grades[i].score < 100) {
                grades[i].score += points;
                if (grades[i].score > 100) {
                    grades[i].score = 100;
                }
            }
        }
    }

    // Sum and count of one category's scores among the first "count" grades.
    // Pass by reference (&): the answers go straight into the caller's variables.
    void categoryTotals(const std::string& type, int count, double& sum, int& n) const {
        sum = 0;
        n = 0;
        for (int i = 0; i < count && i < gradeCount(); i++) {
            if (grades[i].type == type) {
                sum += grades[i].score;
                n++;
            }
        }
    }

    // Average of one category, or -1 if it has no grades yet.
    double categoryAverage(const std::string& type) const {
        double sum;
        int n;
        categoryTotals(type, gradeCount(), sum, n);
        if (n == 0) {
            return -1;
        }
        return sum / n;
    }

    // Weighted average of the first "count" grades: each category's average times its
    // weight, divided by the weights used. Empty categories are skipped.
    double averageOfFirst(int count) const {
        double weightedSum = 0;
        double totalWeight = 0;
        for (int t = 0; t < NUM_TYPES; t++) {
            double sum;
            int n;
            categoryTotals(typeName(t), count, sum, n);
            if (n > 0) {
                weightedSum += typeWeight(typeName(t)) * (sum / n);
                totalWeight += typeWeight(typeName(t));
            }
        }
        if (totalWeight == 0) {
            return 0;
        }
        return weightedSum / totalWeight;
    }

    double average() const {
        return averageOfFirst(gradeCount());
    }

    static std::string letterFor(double percent) {
        if (percent >= 90) return "A";
        else if (percent >= 80) return "B";
        else if (percent >= 70) return "C";
        else if (percent >= 60) return "D";
        else return "F";
    }

    std::string letterGrade() const {
        return letterFor(average());
    }

    double gpa() const {
        std::string letter = letterGrade();
        if (letter == "A") return 4.0;
        if (letter == "B") return 3.0;
        if (letter == "C") return 2.0;
        if (letter == "D") return 1.0;
        return 0.0;
    }

    // Passed / Needs Improvement / Failed, using the class's cutoffs.
    std::string status(double passCutoff, double needsImprovementCutoff) const {
        if (!hasGrades()) {
            return "No Grades";
        }
        double avg = average();
        if (avg >= passCutoff) {
            return "Passed";
        } else if (avg >= needsImprovementCutoff) {
            return "Needs Improvement";
        } else {
            return "Failed";
        }
    }

    // Newest grade vs. the average of the earlier ones: up, down, steady, or none.
    std::string trend() const {
        if (gradeCount() < 2) {
            return "none";
        }
        double sum = 0;
        for (int i = 0; i < gradeCount() - 1; i++) {
            sum += grades[i].score;
        }
        double earlierAverage = sum / (gradeCount() - 1);
        double latest = grades.back().score;

        if (latest > earlierAverage + 2) return "up";
        if (latest < earlierAverage - 2) return "down";
        return "steady";
    }

    // Score needed on the next assignment of this type to reach "target".
    // The other categories stay the same; this one's average becomes (sum + x) / (n + 1).
    double scoreNeeded(const std::string& rawType, double target) const {
        std::string type = normalizeType(rawType);
        if (type == "") {
            return -1;
        }
        double otherSum = 0;      // weight x average of the other graded categories
        double otherWeight = 0;
        for (int t = 0; t < NUM_TYPES; t++) {
            double avg = categoryAverage(typeName(t));
            if (typeName(t) != type && avg >= 0) {
                otherSum += typeWeight(typeName(t)) * avg;
                otherWeight += typeWeight(typeName(t));
            }
        }
        double w = typeWeight(type);
        double sum;
        int n;
        categoryTotals(type, gradeCount(), sum, n);

        // Solve (otherSum + w * newCategoryAverage) / (otherWeight + w) = target
        double newCategoryAverage = (target * (otherWeight + w) - otherSum) / w;
        return newCategoryAverage * (n + 1) - sum;
    }

    double highestScore() const {
        if (!hasGrades()) return 0;
        double best = grades[0].score;
        for (const GradeEntry& g : grades) {
            if (g.score > best) best = g.score;
        }
        return best;
    }

    double lowestScore() const {
        if (!hasGrades()) return 0;
        double worst = grades[0].score;
        for (const GradeEntry& g : grades) {
            if (g.score < worst) worst = g.score;
        }
        return worst;
    }
};
// ----- end Student.h -----
// ----- begin GradeBook.h -----
// GradeBook.h - the whole class: a list of Student objects plus class stats,
// sorting, curving, undo, and CSV import/export.

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
        if (startingGrade != Student::NO_GRADE && !Student::isValidStartingGrade(startingGrade)) return -1;

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
// ----- end GradeBook.h -----
// ----- begin CommandShell.h -----
// CommandShell.h - turns text commands like  grade 1001 "Quiz 3" quiz 88
// into GradeBook calls. It reads from any istream and writes to any ostream, so
// the terminal and the website (cin/cout redirected to strings) share this code.
// Errors always start with "Error:". Text with spaces goes in "quotes".

#include <iostream>
#include <sstream>
#include <iomanip>   // setw, setprecision, quoted
#include <string>
#include <vector>

class CommandShell {
private:
    GradeBook& book;   // a reference: the real gradebook, not a copy

    static constexpr const char* TYPE_ERROR =
        "Error: type must be homework, quiz, test, project, or activity.\n";

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
            if (!parseNumber(gradeWord, starting) || !Student::isValidStartingGrade(starting)) {
                out << "Error: the starting grade must be a number from 0 to 100.\n";
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
            out << TYPE_ERROR;
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
            out << TYPE_ERROR;
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
            out << TYPE_ERROR;
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
            << std::setw(16) << "Category" << std::right << std::setw(6) << "Score" << "\n";
        int number = 1;
        for (const GradeEntry& g : s->getGrades()) {
            out << std::left << "  " << std::setw(4) << number << std::setw(24) << g.label
                << std::setw(16) << g.type << std::right << std::setw(6) << fmt(g.score, 1) << "\n";
            number++;
        }
        out << std::left << "  " << std::setw(18) << "Category" << std::setw(8) << "Weight" << "Average\n";
        for (int t = 0; t < Student::NUM_TYPES; t++) {
            std::string type = Student::typeName(t);
            double avg = s->categoryAverage(type);
            out << "  " << std::setw(18) << type << std::setw(8) << (fmt(Student::typeWeight(type), 0) + "%")
                << (avg >= 0 ? fmt(avg, 1) : "--") << "\n";
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
            out << TYPE_ERROR;
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
        out << "Category averages:\n";
        for (int t = 0; t < Student::NUM_TYPES; t++) {
            std::string type = Student::typeName(t);
            double avg = book.categoryClassAverage(type);
            out << "  " << std::left << std::setw(16) << type << std::right << std::setw(4)
                << fmt(Student::typeWeight(type), 0) << "%   " << (avg >= 0 ? fmt(avg, 1) : "no grades") << "\n";
        }
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

    // {"Homework":95.00,...,"Project":null} for one student, or the class if s is nullptr
    void jsonCategories(std::ostream& out, const Student* s) {
        out << "{";
        for (int t = 0; t < Student::NUM_TYPES; t++) {
            std::string type = Student::typeName(t);
            double avg = (s == nullptr) ? book.categoryClassAverage(type) : s->categoryAverage(type);
            if (t > 0) out << ",";
            out << jsonString(type) << ":" << (avg >= 0 ? fmt(avg, 2) : "null");
        }
        out << "}";
    }

    // json -> the WHOLE gradebook as JSON, so the website can draw it
    void cmdJson(std::ostream& out) {
        out << "{";
        out << "\"className\":" << jsonString(book.getClassName());
        out << ",\"passCutoff\":" << fmt(book.getPassCutoff(), 2);
        out << ",\"needsCutoff\":" << fmt(book.getNeedsCutoff(), 2);
        out << ",\"canUndo\":" << (book.canUndo() ? "true" : "false");
        out << ",\"undoLabel\":" << jsonString(book.undoDescription());
        out << ",\"weights\":{";
        for (int t = 0; t < Student::NUM_TYPES; t++) {
            if (t > 0) out << ",";
            out << jsonString(Student::typeName(t)) << ":" << fmt(Student::typeWeight(Student::typeName(t)), 0);
        }
        out << "}";

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
        out << ",\"categories\":";
        jsonCategories(out, nullptr);
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
            out << ",\"categories\":";
            jsonCategories(out, &s);
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
            << "  grade <id> \"<label>\" <type> <score>  add a grade (homework/quiz/test/project/activity)\n"
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
// ----- end CommandShell.h -----

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

// =====================================================================
//  GradeBook.h  -  the WHOLE CLASS (a list of Student objects)
// ---------------------------------------------------------------------
//  If a Student is one index card, the GradeBook is the recipe box that
//  holds all the cards. It can add/remove cards, find a card by ID, sort
//  the cards, and do math across ALL cards (class average, median, ...).
//
//  It also keeps "photocopies" of itself (undo snapshots), so if you make
//  a mistake you can go back to the last photocopy.
// =====================================================================
#pragma once

#include "Student.h"
#include <string>
#include <vector>
#include <sstream>    // std::istringstream / std::ostringstream (string streams)
#include <algorithm>  // std::sort
#include <cmath>      // std::sqrt

class GradeBook {
private:
    // ----- An undo snapshot = a full copy of everything that can change -----
    struct Snapshot {
        std::vector<Student> students;
        int nextId;
        std::string className;
        double passCutoff;
        double needsCutoff;
        std::string description;   // what the change was, e.g. "add grade"
    };

    std::string className;
    std::vector<Student> students;   // every Student object in the class
    int nextId;                      // the ID the next new student will get
    double passCutoff;               // average needed to "Pass"
    double needsCutoff;              // average needed for "Needs Improvement"
    std::vector<Snapshot> undoStack; // newest snapshot is at the back

    static const int MAX_UNDO = 50;  // keep at most 50 snapshots

    // Take a "photocopy" of the gradebook BEFORE changing it.
    void saveSnapshot(const std::string& description) {
        Snapshot snap;
        snap.students = students;    // copying a vector copies every Student
        snap.nextId = nextId;
        snap.className = className;
        snap.passCutoff = passCutoff;
        snap.needsCutoff = needsCutoff;
        snap.description = description;
        undoStack.push_back(snap);
        if (static_cast<int>(undoStack.size()) > MAX_UNDO) {
            undoStack.erase(undoStack.begin());   // forget the oldest one
        }
    }

    // If a change turned out to be invalid, throw away the photocopy we took.
    void discardLastSnapshot() {
        if (!undoStack.empty()) {
            undoStack.pop_back();
        }
    }

    // The averages of every student who has at least one grade.
    // (Students with no grades are left out of class statistics.)
    std::vector<double> gradedAverages() const {
        std::vector<double> result;
        for (const Student& s : students) {
            if (s.hasGrades()) {
                result.push_back(s.average());
            }
        }
        return result;
    }

    // Lowercase copy of a string (for case-insensitive search and sort).
    static std::string toLower(const std::string& text) {
        std::string result = text;
        for (int i = 0; i < static_cast<int>(result.size()); i++) {
            // The unsigned char cast matters: letters like "e with an accent"
            // are stored as negative chars, and tolower() crashes on those.
            result[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(result[i])));
        }
        return result;
    }

    // Removes spaces from the start and end of a string.
    static std::string trim(const std::string& text) {
        int start = 0;
        int end = static_cast<int>(text.size()) - 1;
        while (start <= end && std::isspace(static_cast<unsigned char>(text[start]))) start++;
        while (end >= start && std::isspace(static_cast<unsigned char>(text[end]))) end--;
        return text.substr(start, end - start + 1);
    }

    // Should student a come before student b when sorting by "field"?
    bool comesBefore(const Student& a, const Student& b,
                     const std::string& field, bool descending) const {
        if (field == "average") {
            // Students with no grades always go to the bottom.
            if (a.hasGrades() != b.hasGrades()) return a.hasGrades();
            if (descending) return a.average() > b.average();
            return a.average() < b.average();
        }
        if (field == "id") {
            if (descending) return a.getId() > b.getId();
            return a.getId() < b.getId();
        }
        // default: sort by name, ignoring upper/lower case
        if (descending) return toLower(a.getName()) > toLower(b.getName());
        return toLower(a.getName()) < toLower(b.getName());
    }

    // ---- tiny CSV helpers ----
    // Wraps a field in quotes if it contains a comma or a quote.
    static std::string csvField(const std::string& text) {
        bool needsQuotes = text.find(',') != std::string::npos ||
                           text.find('"') != std::string::npos;
        if (!needsQuotes) return text;
        std::string result = "\"";
        for (char c : text) {
            if (c == '"') result += "\"\"";   // a quote inside becomes two quotes
            else result += c;
        }
        return result + "\"";
    }

    // Splits one CSV line into fields, respecting "quoted, fields".
    static std::vector<std::string> splitCsvLine(const std::string& line) {
        std::vector<std::string> fields;
        std::string current = "";
        bool inQuotes = false;
        for (int i = 0; i < static_cast<int>(line.size()); i++) {
            char c = line[i];
            if (inQuotes) {
                if (c == '"' && i + 1 < static_cast<int>(line.size()) && line[i + 1] == '"') {
                    current += '"';   // "" inside quotes means one real quote
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

    // Converts text to a number WITHOUT exceptions (std::stoi/stod would
    // throw on bad input, and our WebAssembly build has exceptions turned
    // off). A string stream just sets a "fail" flag instead.
    static bool parseNumber(const std::string& text, double& result) {
        std::istringstream stream(text);
        stream >> result;
        if (stream.fail()) return false;
        std::string leftover;
        stream >> leftover;            // anything after the number? ("88abc")
        return leftover.empty();
    }

public:
    // =================================================================
    //  CONSTRUCTOR: an empty gradebook with default settings
    // =================================================================
    GradeBook() {
        className = "My Class";
        nextId = 1001;
        passCutoff = 70;
        needsCutoff = 60;
    }

    // =================================================================
    //  SETTINGS
    // =================================================================
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

    // Cutoffs must make sense: 0 <= needs-improvement < pass <= 100
    bool setThresholds(double newPass, double newNeeds) {
        if (!(newNeeds >= 0 && newNeeds < newPass && newPass <= 100)) {
            return false;
        }
        saveSnapshot("change cutoffs");
        passCutoff = newPass;
        needsCutoff = newNeeds;
        return true;
    }

    // The status of one student using THIS class's cutoffs.
    std::string statusOf(const Student& s) const {
        return s.status(passCutoff, needsCutoff);
    }

    // =================================================================
    //  ADDING / FINDING / REMOVING STUDENTS
    // =================================================================
    const std::vector<Student>& getStudents() const { return students; }
    int size() const { return static_cast<int>(students.size()); }

    static bool isValidName(const std::string& name) {
        std::string cleaned = trim(name);
        return !cleaned.empty() && cleaned.size() <= 40;
    }

    // Creates a new Student with the parameterized constructor and gives it
    // the next free ID. Returns the new ID, or -1 if the input was invalid.
    int addStudent(const std::string& name, double startingGrade) {
        if (!isValidName(name)) return -1;
        if (startingGrade != Student::NO_GRADE && !Student::isValidScore(startingGrade)) return -1;

        saveSnapshot("add student");
        int newId = nextId;
        nextId++;
        Student s(trim(name), newId, startingGrade);   // <-- constructor call
        students.push_back(s);
        return newId;
    }

    // Adds a Student object that was already built somewhere else (main()
    // builds three by hand). Refuses duplicate IDs.
    bool addExistingStudent(const Student& s) {
        if (findById(s.getId()) != nullptr) return false;
        saveSnapshot("add student");
        students.push_back(s);
        if (s.getId() >= nextId) {
            nextId = s.getId() + 1;   // keep future IDs unique
        }
        return true;
    }

    // Returns a POINTER to the student with this ID, or nullptr if missing.
    // A pointer is like a Java object reference: it "points at" the real
    // Student inside the vector, so changes through it change the original.
    Student* findById(int id) {
        for (int i = 0; i < size(); i++) {
            if (students[i].getId() == id) {
                return &students[i];   // "&" = "the address of"
            }
        }
        return nullptr;   // C++'s version of Java's null
    }

    // Same thing, but read-only (used inside const methods).
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
        s->setName(trim(newName));   // "->" calls a method through a pointer
        return true;
    }

    // Case-insensitive "contains" search on names. Returns matching IDs.
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

    // =================================================================
    //  CHANGING GRADES (each one takes an undo snapshot first)
    // =================================================================
    bool addGrade(int id, const std::string& label, const std::string& type, double score) {
        Student* s = findById(id);
        if (s == nullptr || trim(label).empty()) return false;
        saveSnapshot("add grade");
        bool ok = s->addGrade(trim(label), type, score);
        if (!ok) discardLastSnapshot();   // nothing changed, so no undo needed
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

    // A CLASS ASSIGNMENT gives one grade to many students at once.
    // ids[i] gets scores[i]. Everything is checked BEFORE anything changes,
    // so a single bad score can't leave the class half-graded.
    // Returns how many students were graded, or -1 if the input was bad.
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

    // =================================================================
    //  CLASS STATISTICS
    // =================================================================
    int gradedCount() const { return static_cast<int>(gradedAverages().size()); }

    // CLASS AVERAGE = (sum of every student's average) / (number of students)
    double classAverage() const {
        std::vector<double> avgs = gradedAverages();
        if (avgs.empty()) return 0;
        double sum = 0;
        for (double a : avgs) sum += a;
        return sum / static_cast<double>(avgs.size());
    }

    // MEDIAN = the middle value after sorting (or the mean of the two
    // middle values when there is an even count).
    double median() const {
        std::vector<double> avgs = gradedAverages();
        if (avgs.empty()) return 0;
        std::sort(avgs.begin(), avgs.end());   // C++'s version of Arrays.sort
        int n = static_cast<int>(avgs.size());
        if (n % 2 == 1) return avgs[n / 2];
        return (avgs[n / 2 - 1] + avgs[n / 2]) / 2.0;
    }

    // STANDARD DEVIATION = how spread out the averages are.
    // sqrt( average of (each value - mean)^2 )
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

    // PASS RATE = percent of graded students whose status is "Passed".
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

    // How many graded students have this letter ("A", "B", ...)?
    int countLetter(const std::string& letter) const {
        int count = 0;
        for (const Student& s : students) {
            if (s.hasGrades() && s.letterGrade() == letter) count++;
        }
        return count;
    }

    // How many students have this status ("Passed", "Failed", ...)?
    int countStatus(const std::string& status) const {
        int count = 0;
        for (const Student& s : students) {
            if (statusOf(s) == status) count++;
        }
        return count;
    }

    // ID of the student with the highest average (-1 if nobody has grades).
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

    // ID of the most "at-risk" student: the lowest average, but only if that
    // student is NOT passing. (If everyone passes, nobody is at risk.)
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

    // =================================================================
    //  SORTING: selection sort (the same algorithm from AP CSA!)
    //  field is "name", "average", or "id".
    // =================================================================
    void sortBy(const std::string& field, bool descending) {
        int n = size();
        for (int i = 0; i < n - 1; i++) {
            // find the student that should come first among i..n-1
            int bestIndex = i;
            for (int j = i + 1; j < n; j++) {
                if (comesBefore(students[j], students[bestIndex], field, descending)) {
                    bestIndex = j;
                }
            }
            // swap it into position i
            if (bestIndex != i) {
                Student temp = students[i];
                students[i] = students[bestIndex];
                students[bestIndex] = temp;
            }
        }
    }

    // =================================================================
    //  CURVE: add points to every grade (capped at 100)
    // =================================================================
    bool applyCurve(double points) {
        if (!(points > 0 && points <= 50) || gradedCount() == 0) return false;
        saveSnapshot("curve");
        for (int i = 0; i < size(); i++) {
            students[i].curveGrades(points);
        }
        return true;
    }

    // A COPY of this gradebook with the curve applied. The real gradebook is
    // never touched, so this is perfect for "what if?" previews.
    GradeBook curvedCopy(double points) const {
        GradeBook copy;              // a fresh, empty gradebook...
        copy.students = students;    // ...with copies of our students and settings
        copy.className = className;  // (but NOT our undo history: that would be
        copy.passCutoff = passCutoff;   //  50 extra copies of the class for nothing)
        copy.needsCutoff = needsCutoff;
        for (int i = 0; i < copy.size(); i++) {
            copy.students[i].curveGrades(points);
        }
        return copy;
    }

    // What WOULD the class average be after a curve?
    double previewCurve(double points) const {
        return curvedCopy(points).classAverage();
    }

    // =================================================================
    //  UNDO
    // =================================================================
    bool canUndo() const { return !undoStack.empty(); }

    // Forget every snapshot (used once at startup so "undo" can't remove
    // the starter students).
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

    // =================================================================
    //  CLEAR and DEMO DATA
    // =================================================================
    void clearAll() {
        saveSnapshot("clear class");
        students.clear();
        nextId = 1001;
    }

    // Replaces the roster with a realistic sample class for demos.
    void loadDemo() {
        saveSnapshot("load demo");
        students.clear();
        nextId = 1001;
        className = "Period 3 - Chemistry";

        // The same 7 assignments for everyone (label + type).
        const int NUM_ASSIGNMENTS = 7;
        std::string labels[NUM_ASSIGNMENTS] = {
            "Homework 1", "Quiz 1: Atoms", "Unit 1 Test", "Lab Project",
            "Quiz 2: Bonding", "Homework 2", "Unit 2 Test"};
        std::string types[NUM_ASSIGNMENTS] = {
            "Homework", "Quiz", "Test", "Project", "Quiz", "Homework", "Test"};

        // One row of scores per student (a 2D array, like int[][] in Java).
        const int NUM_STUDENTS = 8;
        std::string names[NUM_STUDENTS] = {
            "Ava Chen", "Marcus Johnson", "Priya Patel", "Diego Ramirez",
            "Sofia Rossi", "Ethan Brooks", "Hana Kim", "Liam O'Connor"};
        double scores[NUM_STUDENTS][NUM_ASSIGNMENTS] = {
            {95, 92, 94, 98, 96, 100, 97},   // Ava: consistent top student
            {78, 72, 70, 80, 75, 85, 74},    // Marcus: solid C
            {88, 85, 90, 92, 89, 95, 91},    // Priya: strong
            {60, 55, 58, 65, 62, 70, 66},    // Diego: on the edge
            {82, 79, 84, 88, 86, 90, 85},    // Sofia: steady B
            {50, 48, 52, 60, 45, 65, 49},    // Ethan: struggling
            {70, 74, 76, 80, 82, 88, 85},    // Hana: improving every week
            {92, 85, 80, 78, 75, 70, 68}};   // Liam: sliding downward

        for (int s = 0; s < NUM_STUDENTS; s++) {
            Student student(names[s], nextId, Student::NO_GRADE);
            nextId++;
            for (int a = 0; a < NUM_ASSIGNMENTS; a++) {
                student.addGrade(labels[a], types[a], scores[s][a]);
            }
            students.push_back(student);
        }
    }

    // =================================================================
    //  CSV EXPORT / IMPORT
    //  Format (one row per grade; a student with no grades gets one row
    //  with the last three columns empty):
    //      id,name,assignment,type,score
    //      1001,Ava Chen,Unit 1 Test,Test,94
    // =================================================================
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

    // Replaces the roster with the students in the CSV text.
    // The three "int&" parameters are extra answers sent back to the caller.
    // Returns false (and changes nothing) if no valid rows were found.
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
            if (!line.empty() && line.back() == '\r') line.pop_back();   // Windows line endings
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

            // Find this student in what we've loaded so far, or create them.
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

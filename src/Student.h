// =====================================================================
//  Student.h  -  ONE student and all of their grades
// ---------------------------------------------------------------------
//  Think of a Student object like a single index card in a teacher's
//  recipe box: the front has the name and ID, the back has a list of
//  every grade they ever got. The card can also do math about itself
//  (its own average, letter grade, status, and so on).
//
//  Java comparison: this whole file is basically "public class Student"
//  with the method bodies written right inside the class, just like Java.
// =====================================================================
#pragma once   // "only include this file once" (stops duplicate definitions)

#include <string>   // std::string  (like Java's String)
#include <vector>   // std::vector  (like Java's ArrayList)
#include <cctype>   // std::tolower, std::isspace

// ---------------------------------------------------------------------
//  GradeEntry: one row in a student's grade history.
//  A "struct" is just a class whose members are public by default.
//  Java analogy: a tiny class with three public fields and no methods.
// ---------------------------------------------------------------------
struct GradeEntry {
    std::string label;  // what the assignment was called, e.g. "Unit 2 Test"
    std::string type;   // one of: Test, Project, Quiz, Homework
    double score;       // a percentage from 0 to 120 (over 100 = extra credit)
};

class Student {
private:
    // ----- PRIVATE data: only Student's own methods can touch these -----
    // (Same idea as "private" fields in Java: other code must go through
    //  the public methods, so nobody can put a student in a broken state.)
    std::string name;
    int id;
    std::vector<GradeEntry> grades;   // the full grade history, oldest first

public:
    // Passing NO_GRADE as the starting grade means "start with no grades".
    // "static const" = one shared constant for the whole class (like Java's
    // "public static final").
    static const int NO_GRADE = -1;

    // =================================================================
    //  PARAMETERIZED CONSTRUCTOR
    //  Runs automatically when we write:  Student s("Ava Chen", 1001, 92);
    //  It sets the student's name, ID, and their first (starting) grade.
    // =================================================================
    Student(const std::string& name, int id, double startingGrade) {
        // "this->name" is the field, "name" is the parameter (same as Java).
        this->name = name;
        this->id = id;

        // Record the starting grade as the first entry in their history.
        // It is stored as a Quiz because a Quiz has the "normal" 1x weight.
        if (startingGrade != NO_GRADE && isValidScore(startingGrade)) {
            grades.push_back({"Starting Grade", "Quiz", startingGrade});
        }
    }

    // =================================================================
    //  STATIC HELPERS about grade types (they don't need a specific
    //  student, so they are "static", exactly like Java static methods)
    // =================================================================

    // Turns whatever the user typed ("test", "T", "hw", "QUIZ") into one of
    // the four official type names. Returns "" (empty) if it isn't valid.
    static std::string normalizeType(const std::string& raw) {
        std::string t = "";
        for (char c : raw) {   // "for each char c in raw" (like Java's for-each)
            t += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        if (t == "test" || t == "t" || t == "exam")              return "Test";
        if (t == "project" || t == "p" || t == "proj" || t == "lab") return "Project";
        if (t == "quiz" || t == "q")                             return "Quiz";
        if (t == "homework" || t == "h" || t == "hw")            return "Homework";
        return "";
    }

    // How much each type counts toward the average.
    // A Test counts twice as much as a Quiz; Homework counts half as much.
    static double typeWeight(const std::string& type) {
        if (type == "Test")     return 2.0;
        if (type == "Project")  return 1.5;
        if (type == "Quiz")     return 1.0;
        if (type == "Homework") return 0.5;
        return 0.0;   // unknown type counts for nothing
    }

    // A score is valid if it is between 0 and 120 (120 allows extra credit).
    // Note: this also rejects "not a number" values, because NaN fails
    // every comparison.
    static bool isValidScore(double score) {
        return score >= 0 && score <= 120;
    }

    // =================================================================
    //  GETTERS and SETTERS  (the "front door" to the private data)
    // =================================================================
    std::string getName() const { return name; }
    int getId() const { return id; }

    // "const" after the parentheses promises this method won't change the
    // student. The "&" returns the real vector (no copy) but read-only.
    const std::vector<GradeEntry>& getGrades() const { return grades; }

    int gradeCount() const { return static_cast<int>(grades.size()); }
    bool hasGrades() const { return !grades.empty(); }

    void setName(const std::string& newName) { name = newName; }

    // =================================================================
    //  CHANGING GRADES  (each returns true if it worked, false if not)
    // =================================================================
    bool addGrade(const std::string& label, const std::string& rawType, double score) {
        std::string type = normalizeType(rawType);
        if (type == "" || !isValidScore(score)) {
            return false;   // refuse bad data instead of storing it
        }
        grades.push_back({label, type, score});
        return true;
    }

    // index is 0-based here (0 = first grade), just like Java arrays.
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
        grades.erase(grades.begin() + index);   // like ArrayList.remove(index)
        return true;
    }

    // Adds "points" to every grade, but never pushes a score above 100.
    // (Scores already above 100 from extra credit are left alone.)
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

    // =================================================================
    //  CALCULATIONS
    // =================================================================

    // Adds up (score x weight) and the total weight.
    // The "&" means these two variables are passed BY REFERENCE: the
    // function writes its answers straight into the caller's variables.
    // (Java can't do this with doubles; C++ can.)
    void weightedTotals(double& weightedSum, double& totalWeight) const {
        weightedSum = 0;
        totalWeight = 0;
        for (const GradeEntry& g : grades) {
            double w = typeWeight(g.type);
            weightedSum += g.score * w;
            totalWeight += w;
        }
    }

    // WEIGHTED AVERAGE:  sum(score x weight) / sum(weights)
    // Example: Test 80 (x2) and Quiz 100 (x1) -> (160 + 100) / 3 = 86.7
    double average() const {
        double weightedSum, totalWeight;
        weightedTotals(weightedSum, totalWeight);
        if (totalWeight == 0) {
            return 0;   // no grades yet: avoid dividing by zero
        }
        return weightedSum / totalWeight;
    }

    // The weighted average using only the FIRST "count" grades.
    // averageOfFirst(3) = "what was the average right after grade #3?"
    // (The website uses this to draw the running-average line.)
    double averageOfFirst(int count) const {
        double weightedSum = 0;
        double totalWeight = 0;
        for (int i = 0; i < count && i < gradeCount(); i++) {
            double w = typeWeight(grades[i].type);
            weightedSum += grades[i].score * w;
            totalWeight += w;
        }
        if (totalWeight == 0) {
            return 0;
        }
        return weightedSum / totalWeight;
    }

    // Letter for ANY percentage on the standard 90/80/70/60 scale.
    // (static, so the GradeBook can also use it for the class average)
    static std::string letterFor(double percent) {
        if (percent >= 90) return "A";
        else if (percent >= 80) return "B";
        else if (percent >= 70) return "C";
        else if (percent >= 60) return "D";
        else return "F";
    }

    // This student's letter grade.
    std::string letterGrade() const {
        return letterFor(average());
    }

    // GPA on a 4.0 scale, based on the letter grade.
    double gpa() const {
        std::string letter = letterGrade();
        if (letter == "A") return 4.0;
        if (letter == "B") return 3.0;
        if (letter == "C") return 2.0;
        if (letter == "D") return 1.0;
        return 0.0;
    }

    // STATUS decision with adjustable cutoffs (the teacher can change them).
    // Example with cutoffs 70 / 60:   75 -> Passed,  65 -> Needs Improvement,
    //                                 50 -> Failed
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

    // TREND: compares the newest grade with the average of all earlier ones.
    // Returns "up", "down", "steady", or "none" (fewer than 2 grades).
    std::string trend() const {
        if (gradeCount() < 2) {
            return "none";
        }
        double sum = 0;
        for (int i = 0; i < gradeCount() - 1; i++) {   // every grade except the last
            sum += grades[i].score;
        }
        double earlierAverage = sum / (gradeCount() - 1);
        double latest = grades.back().score;

        if (latest > earlierAverage + 2) return "up";
        if (latest < earlierAverage - 2) return "down";
        return "steady";
    }

    // "WHAT DO I NEED?"  What score on the next assignment of this type
    // would bring the weighted average up (or down) to exactly "target"?
    //
    //   (weightedSum + w * x) / (totalWeight + w) = target
    //   ->  x = (target * (totalWeight + w) - weightedSum) / w
    //
    // The answer can be above 100 (impossible) or below 0 (already safe);
    // the caller decides how to explain that.
    double scoreNeeded(const std::string& rawType, double target) const {
        double w = typeWeight(normalizeType(rawType));
        if (w == 0) {
            return -1;   // caller should have checked the type first
        }
        double weightedSum, totalWeight;
        weightedTotals(weightedSum, totalWeight);
        return (target * (totalWeight + w) - weightedSum) / w;
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

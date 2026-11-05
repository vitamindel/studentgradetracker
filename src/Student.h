// Student.h - one student: their name, ID, and grade history.
#pragma once

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
        if (startingGrade != NO_GRADE && isValidScore(startingGrade)) {
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

    static bool isValidScore(double score) {
        return score >= 0 && score <= 120;
    }

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

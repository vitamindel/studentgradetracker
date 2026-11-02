// Student.h - one student: their name, ID, and grade history.
#pragma once

#include <string>
#include <vector>
#include <cctype>

// One grade in a student's history (a struct is a class with public members).
struct GradeEntry {
    std::string label;  // e.g. "Unit 2 Test"
    std::string type;   // Test, Project, Quiz, or Homework
    double score;       // 0 to 120 (over 100 = extra credit)
};

class Student {
private:
    std::string name;
    int id;
    std::vector<GradeEntry> grades;   // oldest first

public:
    static const int NO_GRADE = -1;   // "start with no grades"

    // Parameterized constructor: Student s("Ava Chen", 1001, 92);
    Student(const std::string& name, int id, double startingGrade) {
        this->name = name;
        this->id = id;

        // The starting grade counts as a Quiz (normal 1x weight).
        if (startingGrade != NO_GRADE && isValidScore(startingGrade)) {
            grades.push_back({"Starting Grade", "Quiz", startingGrade});
        }
    }

    // Turns "test", "T", "hw", ... into an official type name, or "" if invalid.
    static std::string normalizeType(const std::string& raw) {
        std::string t = "";
        for (char c : raw) {
            t += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        if (t == "test" || t == "t" || t == "exam")              return "Test";
        if (t == "project" || t == "p" || t == "proj" || t == "lab") return "Project";
        if (t == "quiz" || t == "q")                             return "Quiz";
        if (t == "homework" || t == "h" || t == "hw")            return "Homework";
        return "";
    }

    // How much each type counts toward the average.
    static double typeWeight(const std::string& type) {
        if (type == "Test")     return 2.0;
        if (type == "Project")  return 1.5;
        if (type == "Quiz")     return 1.0;
        if (type == "Homework") return 0.5;
        return 0.0;
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

    // Pass by reference (&): the answers go straight into the caller's variables.
    void weightedTotals(double& weightedSum, double& totalWeight) const {
        weightedSum = 0;
        totalWeight = 0;
        for (const GradeEntry& g : grades) {
            double w = typeWeight(g.type);
            weightedSum += g.score * w;
            totalWeight += w;
        }
    }

    // Weighted average = sum(score x weight) / sum(weights)
    double average() const {
        double weightedSum, totalWeight;
        weightedTotals(weightedSum, totalWeight);
        if (totalWeight == 0) {
            return 0;
        }
        return weightedSum / totalWeight;
    }

    // Average using only the first "count" grades (for the running-average line).
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

    // Score needed on the next assignment to reach "target":
    //   x = (target * (totalWeight + w) - weightedSum) / w
    double scoreNeeded(const std::string& rawType, double target) const {
        double w = typeWeight(normalizeType(rawType));
        if (w == 0) {
            return -1;
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

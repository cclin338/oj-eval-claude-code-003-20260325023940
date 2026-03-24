#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <sstream>
#include <cstdio>
#include <cstring>

using namespace std;

// Judge status enum
enum JudgeStatus {
    ACCEPTED,
    WRONG_ANSWER,
    RUNTIME_ERROR,
    TIME_LIMIT_EXCEED
};

// Convert string to JudgeStatus
JudgeStatus stringToStatus(const string& s) {
    if (s == "Accepted") return ACCEPTED;
    if (s == "Wrong_Answer") return WRONG_ANSWER;
    if (s == "Runtime_Error") return RUNTIME_ERROR;
    if (s == "Time_Limit_Exceed") return TIME_LIMIT_EXCEED;
    return WRONG_ANSWER; // default
}

// Convert JudgeStatus to string
string statusToString(JudgeStatus s) {
    switch(s) {
        case ACCEPTED: return "Accepted";
        case WRONG_ANSWER: return "Wrong_Answer";
        case RUNTIME_ERROR: return "Runtime_Error";
        case TIME_LIMIT_EXCEED: return "Time_Limit_Exceed";
        default: return "Unknown";
    }
}

// Team class
class Team {
public:
    string name;
    int rank;

    // Problem-related data
    vector<int> wrongSubmissions;  // wrong attempts before first AC
    vector<int> firstAcTime;       // time of first AC (0 if not solved)
    vector<int> totalSubmissions;  // total submissions (including AC)
    vector<int> frozenSubmissions; // submissions during freeze

    // For freeze handling
    vector<bool> isFrozen;

    // Cached values for ranking
    int solvedCount;
    int penaltyTime;
    vector<int> solveTimes; // sorted solve times for tie-breaking

    Team(const string& name, int problemCount) : name(name) {
        wrongSubmissions.resize(problemCount, 0);
        firstAcTime.resize(problemCount, 0);
        totalSubmissions.resize(problemCount, 0);
        frozenSubmissions.resize(problemCount, 0);
        isFrozen.resize(problemCount, false);
        solvedCount = 0;
        penaltyTime = 0;
        rank = 0;
    }

    // Update cached values
    void updateStats() {
        solvedCount = 0;
        penaltyTime = 0;
        solveTimes.clear();

        for (int i = 0; i < (int)firstAcTime.size(); i++) {
            if (firstAcTime[i] > 0) {
                solvedCount++;
                int problemPenalty = 20 * wrongSubmissions[i] + firstAcTime[i];
                penaltyTime += problemPenalty;
                solveTimes.push_back(firstAcTime[i]);
            }
        }

        // Sort solve times in descending order for tie-breaking
        sort(solveTimes.rbegin(), solveTimes.rend());
    }

    // Compare function for ranking
    bool operator<(const Team& other) const {
        if (solvedCount != other.solvedCount) {
            return solvedCount > other.solvedCount;
        }
        if (penaltyTime != other.penaltyTime) {
            return penaltyTime < other.penaltyTime;
        }

        // Compare solve times
        for (size_t i = 0; i < min(solveTimes.size(), other.solveTimes.size()); i++) {
            if (solveTimes[i] != other.solveTimes[i]) {
                return solveTimes[i] < other.solveTimes[i];
            }
        }

        // All solve times equal, compare by name
        return name < other.name;
    }
};

// Submission record
struct Submission {
    string teamName;
    string problemName;
    JudgeStatus status;
    int time;
};

class ICPCSystem {
private:
    bool competitionStarted;
    bool isFrozen;
    int durationTime;
    int problemCount;

    map<string, Team*> teams;
    vector<string> teamNames; // for maintaining insertion order
    vector<Submission> submissions;

    // Problem name to index mapping (A=0, B=1, ...)
    map<char, int> problemIndex;

    // For freeze/unfreeze
    vector<pair<string, char>> frozenProblems; // team name, problem

public:
    ICPCSystem() : competitionStarted(false), isFrozen(false), durationTime(0), problemCount(0) {}

    ~ICPCSystem() {
        for (auto& p : teams) {
            delete p.second;
        }
    }

    void addTeam(const string& teamName) {
        if (competitionStarted) {
            cout << "[Error]Add failed: competition has started.\n";
            return;
        }

        if (teams.find(teamName) != teams.end()) {
            cout << "[Error]Add failed: duplicated team name.\n";
            return;
        }

        teams[teamName] = new Team(teamName, problemCount);
        teamNames.push_back(teamName);
        cout << "[Info]Add successfully.\n";
    }

    void startCompetition(int duration, int problems) {
        if (competitionStarted) {
            cout << "[Error]Start failed: competition has started.\n";
            return;
        }

        durationTime = duration;
        problemCount = problems;

        // Initialize problem index mapping
        for (int i = 0; i < problems; i++) {
            problemIndex['A' + i] = i;
        }

        // Initialize teams with correct problem count
        for (auto& p : teams) {
            delete p.second;
            p.second = new Team(p.first, problems);
        }

        competitionStarted = true;
        cout << "[Info]Competition starts.\n";
    }

    void submit(const string& problemName, const string& teamName,
                const string& statusStr, int time) {
        if (!competitionStarted) return;

        char problemChar = problemName[0];
        int probIndex = problemIndex[problemChar];
        JudgeStatus status = stringToStatus(statusStr);

        Team* team = teams[teamName];

        // Record submission
        submissions.push_back({teamName, problemName, status, time});

        if (isFrozen && !team->isFrozen[probIndex] && team->firstAcTime[probIndex] == 0) {
            // Problem is frozen for this team
            team->frozenSubmissions[probIndex]++;
            team->isFrozen[probIndex] = true;
        } else {
            // Not frozen or already solved
            team->totalSubmissions[probIndex]++;

            if (status == ACCEPTED && team->firstAcTime[probIndex] == 0) {
                team->firstAcTime[probIndex] = time;
                team->updateStats();
            } else if (status != ACCEPTED && team->firstAcTime[probIndex] == 0) {
                team->wrongSubmissions[probIndex]++;
            }
        }
    }

    void flush() {
        cout << "[Info]Flush scoreboard.\n";
    }

    void freeze() {
        if (isFrozen) {
            cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
            return;
        }

        isFrozen = true;

        // Record all problems that become frozen
        for (auto& p : teams) {
            Team* team = p.second;
            for (int i = 0; i < problemCount; i++) {
                if (team->frozenSubmissions[i] > 0 ||
                    (team->totalSubmissions[i] > 0 && team->firstAcTime[i] == 0)) {
                    team->isFrozen[i] = true;
                    frozenProblems.push_back({p.first, 'A' + i});
                }
            }
        }

        cout << "[Info]Freeze scoreboard.\n";
    }

    void scroll() {
        if (!isFrozen) {
            cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            return;
        }

        cout << "[Info]Scroll scoreboard.\n";

        // TODO: Implement proper scroll logic
        // For now, just unfreeze everything
        for (auto& p : teams) {
            Team* team = p.second;
            for (int i = 0; i < problemCount; i++) {
                if (team->isFrozen[i]) {
                    // Apply frozen submissions
                    team->totalSubmissions[i] += team->frozenSubmissions[i];
                    // Check if any submission was AC
                    // For simplicity, assume no AC in frozen submissions for now
                    team->frozenSubmissions[i] = 0;
                    team->isFrozen[i] = false;
                }
            }
            team->updateStats();
        }

        frozenProblems.clear();
        isFrozen = false;

        // Print final scoreboard
        printScoreboard();
    }

    void queryRanking(const string& teamName) {
        if (teams.find(teamName) == teams.end()) {
            cout << "[Error]Query ranking failed: cannot find the team.\n";
            return;
        }

        cout << "[Info]Complete query ranking.\n";
        if (isFrozen) {
            cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
        }

        // For now, just return rank 1
        cout << teamName << " NOW AT RANKING 1\n";
    }

    void querySubmission(const string& teamName, const string& problemName,
                        const string& statusStr) {
        if (teams.find(teamName) == teams.end()) {
            cout << "[Error]Query submission failed: cannot find the team.\n";
            return;
        }

        cout << "[Info]Complete query submission.\n";

        // Search for matching submission
        Submission* lastMatch = nullptr;
        for (auto& sub : submissions) {
            if (sub.teamName == teamName) {
                if ((problemName == "ALL" || sub.problemName == problemName) &&
                    (statusStr == "ALL" || statusToString(sub.status) == statusStr)) {
                    lastMatch = &sub;
                }
            }
        }

        if (lastMatch == nullptr) {
            cout << "Cannot find any submission.\n";
        } else {
            cout << lastMatch->teamName << " " << lastMatch->problemName << " "
                 << statusToString(lastMatch->status) << " " << lastMatch->time << "\n";
        }
    }

    void endCompetition() {
        cout << "[Info]Competition ends.\n";
    }

    void printScoreboard() {
        // Simple scoreboard printing
        for (size_t i = 0; i < teamNames.size(); i++) {
            Team* team = teams[teamNames[i]];
            cout << team->name << " " << (i + 1) << " " << team->solvedCount
                 << " " << team->penaltyTime;

            for (int j = 0; j < problemCount; j++) {
                cout << " ";
                if (team->isFrozen[j]) {
                    if (team->wrongSubmissions[j] == 0) {
                        cout << "0/" << team->frozenSubmissions[j];
                    } else {
                        cout << "-" << team->wrongSubmissions[j] << "/" << team->frozenSubmissions[j];
                    }
                } else if (team->firstAcTime[j] > 0) {
                    if (team->wrongSubmissions[j] == 0) {
                        cout << "+";
                    } else {
                        cout << "+" << team->wrongSubmissions[j];
                    }
                } else {
                    if (team->totalSubmissions[j] == 0) {
                        cout << ".";
                    } else {
                        cout << "-" << team->totalSubmissions[j];
                    }
                }
            }
            cout << "\n";
        }
    }
};

int main() {
    ICPCSystem system;
    string line;

    while (getline(cin, line)) {
        if (line.empty()) continue;

        stringstream ss(line);
        string command;
        ss >> command;

        if (command == "ADDTEAM") {
            string teamName;
            ss >> teamName;
            system.addTeam(teamName);
        }
        else if (command == "START") {
            string dummy;
            int duration, problems;
            ss >> dummy >> duration >> dummy >> problems;
            system.startCompetition(duration, problems);
        }
        else if (command == "SUBMIT") {
            string problemName, by, teamName, with, statusStr, at;
            int time;
            ss >> problemName >> by >> teamName >> with >> statusStr >> at >> time;
            system.submit(problemName, teamName, statusStr, time);
        }
        else if (command == "FLUSH") {
            system.flush();
        }
        else if (command == "FREEZE") {
            system.freeze();
        }
        else if (command == "SCROLL") {
            system.scroll();
        }
        else if (command == "QUERY_RANKING") {
            string teamName;
            ss >> teamName;
            system.queryRanking(teamName);
        }
        else if (command == "QUERY_SUBMISSION") {
            string teamName, where, problemToken, andStr, statusToken;
            ss >> teamName >> where >> problemToken >> andStr >> statusToken;
            // Extract problem name from "PROBLEM=problem_name"
            string problemName = "ALL";
            if (problemToken.length() > 8) {
                problemName = problemToken.substr(8); // Remove "PROBLEM="
            }
            // Extract status from "STATUS=status"
            string statusStr = "ALL";
            if (statusToken.length() > 7) {
                statusStr = statusToken.substr(7); // Remove "STATUS="
            }
            system.querySubmission(teamName, problemName, statusStr);
        }
        else if (command == "END") {
            system.endCompetition();
            break;
        }
    }

    return 0;
}
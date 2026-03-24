#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <set>

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

        // Compare solve times from largest to smallest
        for (size_t i = 0; i < min(solveTimes.size(), other.solveTimes.size()); i++) {
            if (solveTimes[i] != other.solveTimes[i]) {
                return solveTimes[i] < other.solveTimes[i];
            }
        }

        // All solve times equal or one has more solved problems
        if (solveTimes.size() != other.solveTimes.size()) {
            return solveTimes.size() > other.solveTimes.size();
        }

        // All equal, compare by name
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
    bool hasFlushed; // Whether scoreboard has been flushed at least once

    map<string, Team*> teams;
    vector<string> teamNames; // for maintaining insertion order
    vector<Submission> submissions;

    // Problem name to index mapping (A=0, B=1, ...)
    map<char, int> problemIndex;

    // For ranking
    vector<string> rankedTeams; // teams in ranked order

public:
    ICPCSystem() : competitionStarted(false), isFrozen(false),
                   durationTime(0), problemCount(0), hasFlushed(false) {}

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

        if (isFrozen && team->firstAcTime[probIndex] == 0) {
            // Problem is frozen for this team (unsolved before freeze)
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
        updateRankings();
        printScoreboard();
        hasFlushed = true;
    }

    void freeze() {
        if (isFrozen) {
            cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
            return;
        }

        isFrozen = true;
        cout << "[Info]Freeze scoreboard.\n";
    }

    void scroll() {
        if (!isFrozen) {
            cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            return;
        }

        cout << "[Info]Scroll scoreboard.\n";

        // First flush to show current scoreboard
        updateRankings();
        printScoreboard();

        // TODO: Implement proper scroll logic with unfreezing
        // For now, just unfreeze everything
        for (auto& p : teams) {
            Team* team = p.second;
            for (int i = 0; i < problemCount; i++) {
                if (team->isFrozen[i]) {
                    // Apply frozen submissions
                    // For now, assume no AC in frozen submissions
                    team->totalSubmissions[i] += team->frozenSubmissions[i];
                    team->frozenSubmissions[i] = 0;
                    team->isFrozen[i] = false;
                }
            }
            team->updateStats();
        }

        isFrozen = false;

        // Update rankings and print final scoreboard
        updateRankings();
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

        // Get team's ranking
        if (!hasFlushed) {
            // Before first flush, ranking is based on lexicographic order
            int rank = 1;
            for (const auto& name : teamNames) {
                if (name == teamName) {
                    cout << teamName << " NOW AT RANKING " << rank << "\n";
                    return;
                }
                rank++;
            }
        } else {
            // Find team in ranked list
            for (size_t i = 0; i < rankedTeams.size(); i++) {
                if (rankedTeams[i] == teamName) {
                    cout << teamName << " NOW AT RANKING " << (i + 1) << "\n";
                    return;
                }
            }
        }
    }

    void querySubmission(const string& teamName, const string& problemName,
                        const string& statusStr) {
        if (teams.find(teamName) == teams.end()) {
            cout << "[Error]Query submission failed: cannot find the team.\n";
            return;
        }

        cout << "[Info]Complete query submission.\n";

        // Search for matching submission (from end to beginning)
        for (int i = submissions.size() - 1; i >= 0; i--) {
            const Submission& sub = submissions[i];
            if (sub.teamName == teamName) {
                if ((problemName == "ALL" || sub.problemName == problemName) &&
                    (statusStr == "ALL" || statusToString(sub.status) == statusStr)) {
                    cout << sub.teamName << " " << sub.problemName << " "
                         << statusToString(sub.status) << " " << sub.time << "\n";
                    return;
                }
            }
        }

        cout << "Cannot find any submission.\n";
    }

    void endCompetition() {
        cout << "[Info]Competition ends.\n";
    }

private:
    void updateRankings() {
        // Create vector of team pointers
        vector<Team*> teamList;
        for (auto& p : teams) {
            teamList.push_back(p.second);
        }

        // Sort teams according to ranking rules
        sort(teamList.begin(), teamList.end(), [](Team* a, Team* b) {
            return *a < *b;
        });

        // Update rankedTeams list
        rankedTeams.clear();
        for (auto team : teamList) {
            rankedTeams.push_back(team->name);
        }
    }

    void printScoreboard() {
        updateRankings(); // Ensure rankings are up to date

        for (size_t i = 0; i < rankedTeams.size(); i++) {
            Team* team = teams[rankedTeams[i]];
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
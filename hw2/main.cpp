#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

std::random_device rndd;
std::default_random_engine rnde(rndd());

int randint(int i, int j) {
    std::uniform_int_distribution<int> dist(i, j - 1);
    return dist(rnde);
}

int getRandomEl(const std::vector<int>& arr) {
    return arr[randint(0, static_cast<int>(arr.size()))];
}

std::vector<int> findExtremaIndices(const std::vector<int>& arr, int dir) {
    std::vector<int> indices;
    if (arr.empty()) return indices;

    int extrema = arr[0];
    indices.push_back(0);

    for (int i = 1; i < arr.size(); ++i) {
        bool compare =
            (dir < 0 && arr[i] < extrema) ||
            (dir > 0 && arr[i] > extrema);

        if (compare) {
            extrema = arr[i];
            indices.clear();
            indices.push_back(i);
        } else if (arr[i] == extrema) {
            indices.push_back(i);
        }
    }

    return indices;
}

inline std::vector<int> findMaxIndices(const std::vector<int>& arr) {
    return findExtremaIndices(arr, +1);
}

inline std::vector<int> findMinIndices(const std::vector<int>& arr) {
    return findExtremaIndices(arr, -1);
}

class Solver {
private:
    const int N;
    const int max_iter;
    int iter;

    std::vector<int> queens;
    std::vector<int> queensPerRow;
    std::vector<int> queensPerDL;
    std::vector<int> queensPerDR;

public:
    Solver(int N) :
    N(N),
    max_iter(N * 2),
    iter(0),
    queens(N),
    queensPerRow(N),
    queensPerDL(2 * N - 1),
    queensPerDR(2 * N - 1)
    {}

    std::vector<int> solve() {
        if (N == 1) return {0};
        if (N == 2 || N == 3) return {};

        do {
            if (iter == 0) {
                init();
            }

            int col = getColWithQueenWithMaxConf();
            int row = getRowWithMinConflict(col);
            moveQueen(col, row);

            if (++iter >= max_iter) {
                reset();
            }
        } while (hasConflicts());

        return queens;
    }

private:
    void init() {
        init_horseWalk();
    }

    void init_minConflict() {
        for (int col = 0; col < N; col++) {
            int row = getRowWithMinConflict(col);
            placeQueen(col, row);
        }
    }

    void init_horseWalk() {
        for (int col = 0, row = randint(0, N); col < N; col += 1, row += 2) {
            if (row >= N) row = 0;
            placeQueen(col, row);
        }
    }

    void reset() {
        iter = 0;
        resetConflicts();
    }

    int getColWithQueenWithMaxConf() const {
        std::vector<int> queensConflicts(N);
        for (int col = 0; col < N; col++) {
            int row = queens[col];
            queensConflicts[col] = countConflicts(row, col);
        }
        std::vector<int> queensCols = findMaxIndices(queensConflicts);
        return getRandomEl(queensCols);
    }

    int getRowWithMinConflict(int col) const {
        std::vector<int> conflictsOnRow(N);
        for (int row = 0; row < N; row++) {
            conflictsOnRow[row] = countConflicts(row, col);
        }
        std::vector<int> rows = findMinIndices(conflictsOnRow);
        return getRandomEl(rows);
    }

    int hasConflicts() const {
        for (int col = 0; col < N; col++) {
            int row = queens[col];
            if (countConflicts(row, col) != 0) return true;
        }
        return false;
    }

    void moveQueen(int col, int newRow) {
        int row = queens[col];
        updateConflicts(row, col, -1);
        placeQueen(col, newRow);
    }

    void placeQueen(int col, int row) {
        queens[col] = row;
        updateConflicts(row, col, +1);
    }

    int countConflicts(int row, int col) const {
        int conflicts = queensPerRow[row] + queensPerDL[DLI(row, col)] + queensPerDR[DRI(row, col)];
        if (row == queens[col]) conflicts -= 3;
        return conflicts;
    }
    
    void resetConflicts() {
        std::fill(queensPerRow.begin(), queensPerRow.end(), 0);
        std::fill(queensPerDL.begin(), queensPerDL.end(), 0);
        std::fill(queensPerDR.begin(), queensPerDR.end(), 0);
    }

    void updateConflicts(int row, int col, int val) {
        queensPerRow[row] += val;
        queensPerDL[DLI(row, col)] += val;
        queensPerDR[DRI(row, col)] += val;
    }

    inline int DLI(int row, int col) const {
        return row - col + N - 1;
    }

    inline int DRI(int row, int col) const {
        return row + col;
    }
};

std::vector<int> solve(int N) {
    return Solver(N).solve();
}

int main(int argc, const char* argv[]) {
    using namespace std;
    using namespace std::chrono;

    bool isAutomatedTest = !(argc >= 2 && argv[1] == string("-t"));

    int N ;
    cin >> N;

    auto start = high_resolution_clock::now();
    auto output = solve(N);
    auto stop = high_resolution_clock::now();
    double time = duration<double>(stop - start).count();

    if (N <= 100) {
        if (output.size() == 0) {
            cout << -1 << endl;
        } else {
            if (isAutomatedTest) {
                cout << "[";
                for (size_t i = 0; i < output.size(); i++) {
                    cout << output[i];
                    if (i != output.size() - 1) {
                        cout << ", ";
                    }
                }
                cout << "]" << endl;
            } else {
                for (size_t row = 0; row < output.size(); row++) {
                    for (size_t col = 0; col < output.size(); col++) {
                        if (output[col] == row) {
                            cout << "*";
                        } else {
                            cout << "_";
                        }
                        cout << " ";
                    }
                    cout << endl;
                }
                cout << fixed << "Exection time: " << time << "s" << endl;
            }
        }
    } else {
        cout << fixed << setprecision(2) << time << endl;
    }

    return 0;
}

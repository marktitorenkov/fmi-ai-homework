#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <optional>
#include <vector>

constexpr int INF = std::numeric_limits<int>::max();
constexpr int FOUND = -1;

class State {
private:
    struct Move {
        const int row;
        const int col;
        const std::string move;

        Move(int row, int col, const std::string& move) :
        row(row), col(col), move(move)
        {}
    };

    const std::vector<Move> moves = {
        {+1, 0, "up"},
        {-1, 0, "down"},
        {0, +1, "left"},
        {0, -1, "right"}
    };

    const int n;
    const std::vector<int> board;
    const std::vector<int> board_swp;
    const State* goal;

public:
    const std::string move;

    State(const std::vector<int>& board,
          const State* goal = nullptr,
          const std::string& move = "") :
    n(sqrt(board.size() + 1)),
    board(board),
    board_swp(swap_indices(board)),
    goal(goal),
    move(move)
    {}

    int h() const {
        int distance = 0;
        for (int i = 1; i < n * n; i++) {
            distance +=
                abs(row(i) - goal->row(i)) +
                abs(col(i) - goal->col(i));
        }

        return distance;
    }

    int cost() const {
        return 1;
    }

    bool is_goal() const {
        return board == goal->board;
    }

    // https://stackoverflow.com/a/72173322/5958676
    bool is_solvable() {
        if (n != goal->n) return false;

        int si = inversions();
        int gi = goal->inversions();
        if (n % 2 == 1) {
            return gi % 2 == si % 2;
        } else {
            int sz = row(0);
            int gz = goal->row(0);
            return gi % 2 == (si + gz + sz) % 2;
        }
    }

    std::vector<State> successors() {
        std::vector<State> successors;

        for (const Move& move : moves) {
            int rn = row(0) + move.row;
            int cn = col(0) + move.col;

            if (rn >= 0 && rn < n && cn >= 0 && cn < n) {
                std::vector<int> boardn = board;
                std::swap(boardn[idx(0)], boardn[idx(rn, cn)]);
                successors.emplace_back(boardn, goal, move.move);
            }
        }

        return successors;
    }

    bool operator==(const State& other) const {
        return board == other.board;
    }

private:
    int row(int val) const {
        return board_swp[val] / n;
    }

    int col(int val) const {
        return board_swp[val] % n;
    }
    
    int idx(int val) const {
        return board_swp[val];
    }

    int idx(int row, int col) const {
        return row * n + col;
    }

    int inversions() const {
        int inversions = 0;
        for (int i = 0; i < board.size() - 1; i++) {
            if (board[i] == 0) continue;
            for (int j = i; j < board.size(); j++) {
                if (board[j] == 0) continue;
                if (board[i] > board[j]) {
                    inversions++;
                }
            }
        }
        return inversions;
    }

    static std::vector<int> swap_indices(const std::vector<int> v) {
        std::vector<int> vnew(v.size());
        for (int i = 0; i < v.size(); i++) {
            vnew[v[i]] = i;
        }
        return vnew;
    }
};

int search(std::vector<State>& path, int g, int bound) {
    State state = path.back();
    int f = g + state.h();
    if (f > bound) return f;
    if (state.is_goal()) return FOUND;

    int min = INF;
    for (const State& succ : state.successors()) {
        if (std::find(path.begin(), path.end(), succ) != path.end()) {
            continue;
        }

        path.push_back(succ);
        int t = search(path, g + succ.cost(), bound);
        if (t == FOUND) return FOUND;
        if (t < min) min = t;
        path.pop_back();
    }

    return min;
}

std::vector<State> ida_star(const State& root) {
    int bound = root.h();
    std::vector<State> path{ root };

    while (true) {
        int t = search(path, 0, bound);
        if (t == FOUND) return path;
        if (t == INF) return {};
        bound = t;
    }
}

std::optional<std::vector<std::string>> solve(int N, int I,
                                              const std::vector<int>& board) {
    I = I == -1 ? N : I;
    std::vector<int> goalBoard(board.size());
    for (int i = 0, v = 1; i < board.size(); i++) {
        if (i == I) {
            goalBoard[i] = 0;
        } else {
            goalBoard[i] = v++;
        }
    }

    State goal(goalBoard);
    State root(board, &goal);
    if (!root.is_solvable()) {
        return {};
    }

    std::vector<State> path = ida_star(root);
    std::vector<std::string> moves;
    for (const State& s : path) {
        if (s.move == "") continue;
        moves.push_back(s.move);
    }

    return moves;
}

int main(int argc, const char* argv[]) {
    using namespace std;
    using namespace std::chrono;

    int N, I;
    cin >> N >> I;
    vector<int> board(N+1);
    for (int i = 0; i < N+1; i++) {
        cin >> board[i];
    }

    auto start = high_resolution_clock::now();
    auto steps = solve(N, I, board);
    auto stop = high_resolution_clock::now();

    if (!steps.has_value()) {
        cout << -1 << endl;
    } else {
        cout << steps.value().size() << endl;
        for (const string& s : steps.value()) {
            cout << s << endl;
        }
    }

    if (argc >= 2 && argv[1] == string("-t")) {
        double duration = duration_cast<microseconds>(stop - start).count() / 1.0e6;
        cout << "Exection time: " << duration << "s" << endl;
    }

    return 0;
}

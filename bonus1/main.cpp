#include <iostream>
#include <vector>
#include <stack>
#include <chrono>

class State {
private:
    size_t N;
    size_t iEmpty;
    std::vector<char> board;

    void swapEmpty(size_t iEmptyNew) {
        std::swap(board[iEmpty], board[iEmptyNew]);
        iEmpty = iEmptyNew;
    }

public:
    State(size_t N): N(N), board(2*N+1) {
        int j = 0;
        for (int i = 0; i < N; i++) {
            board[j++] = '>';
        }
        iEmpty = j;
        board[j++] = '_';
        for (int i = 0; i < N; i++) {
            board[j++] = '<';
        }
    }

    bool isGoalState() const {
        if (iEmpty != N) {
            return false;
        }
        for (int i = 0; i < N; i++) {
            if (board[i] != '<' || board[N + 1 + i] != '>') {
                return false;
            }
        }
        return true;
    }

    std::vector<int> moves() const {
        std::vector<int> moves;
        moves.reserve(4);

        // Left moves
        if (iEmpty > 0) {
            if (board[iEmpty - 1] == '>') {
                // Jump
                moves.push_back(-1);
            }
            if (iEmpty > 1 && board[iEmpty - 2] == '>') {
                // Double jump
                moves.push_back(-2);
            }
        }

        // Right moves
        if (iEmpty < board.size() - 1) {
            if (board[iEmpty + 1] == '<') {
                // Jump
                moves.push_back(+1);
            }
            if (iEmpty < board.size() - 2 && board[iEmpty + 2] == '<') {
                // Double jump
                moves.push_back(+2);
            }
        }

        return moves;
    }

    void applyMove(int move) {
        swapEmpty(iEmpty + move);
    }

    void undoMove(int move) {
        swapEmpty(iEmpty - move);
    }

    const std::string toString() const {
        return std::string(board.data(), board.size());
    }
};

bool solve_dfs(State& state, std::stack<std::string>& path) {
    if (state.isGoalState()) {
        path.push(state.toString());
        return true;
    }

    for (const int move : state.moves()) {
        state.applyMove(move);
        bool solved = solve_dfs(state, path);
        state.undoMove(move);
        if (solved) {
            path.push(state.toString());
            return true;
        }
    }

    return false;
}

std::stack<std::string> solve(size_t N) {
    State state(N);
    std::stack<std::string> path;
    solve_dfs(state, path);
    return path;
}

int main(int argc, const char* argv[]) {
    using namespace std;
    using namespace std::chrono;

    cout << "N: ";
    size_t N;
    cin >> N;

    auto start = high_resolution_clock::now();
    auto path = solve(N);
    auto stop = high_resolution_clock::now();

    while (!path.empty()) {
        cout << path.top() << endl;
        path.pop();
    }

    double duration = duration_cast<microseconds>(stop - start).count() / 1.0e6;
    cout << "Exection time: " << duration << "s" << endl;

    return 0;
}

#include <algorithm>
#include <iostream>
#include <limits>
#include <string>
#include <sstream>
#include <vector>

constexpr int MIN = std::numeric_limits<int>::min();
constexpr int MAX = std::numeric_limits<int>::max();

constexpr char X = 'X';
constexpr char O = 'O';
constexpr char EMPTY = '_';
constexpr const char* LINE_SEP = "=====";

class Board {
private:
  static constexpr size_t SIZE = 3;
  static int winnings[8][3][2];

  char board[SIZE][SIZE];
  size_t emptyCount;

public:
  Board() : emptyCount(SIZE * SIZE) {
    for (auto& arr : board)
      std::fill(std::begin(arr), std::end(arr), EMPTY);
  }

public:
  size_t getEmptyCount() const {
      return emptyCount;
  }

  std::vector<Board> successors(char ch) const {
    std::vector<Board> successors;
    for (size_t r = 0; r < SIZE; ++r) {
      for (size_t c = 0; c < SIZE; ++c) {
        Board succ = *this;
        if (succ.move(ch, r, c)) {
            successors.push_back(succ);
        }
      }
    }

    return successors;
  }

  bool isWinner(char ch) const {
    return std::any_of(std::begin(winnings), std::end(winnings), [&](auto& w) {
      return std::all_of(std::begin(w), std::end(w), [&](auto& p) {
        return board[p[0]][p[1]] == ch;
      });
    });
  }

  bool isFilled() const {
    return emptyCount == 0;
  }

  bool isFinal() const {
    return isWinner(X) || isWinner(O) || isFilled();
  }

  bool move(char ch, size_t r, size_t c) {
    if (r >= SIZE || c >= SIZE || board[r][c] != EMPTY) {
      return false;
    }
    board[r][c] = ch;
    --emptyCount;
    return true;
  }

  void print() const {
    for (size_t r = 0; r < SIZE; ++r) {
      for (size_t c = 0; c < SIZE; ++c) {
        std::cout << board[r][c] << " ";
      }
      std::cout << std::endl;
    }
    std::cout << LINE_SEP << std::endl;
  }

};


class Game {
private:
  Board board;
  bool isComputerFirst;
  char computerCh;
  char playerCh;

public:
  Game(bool isComputerFirst) :
    isComputerFirst(isComputerFirst),
    computerCh(isComputerFirst ? X : O),
    playerCh(isComputerFirst ? O : X)
  {}

  void play() {
    board.print();

    bool isComputersTurn = isComputerFirst;
    while (!board.isFinal()) {
      if (isComputersTurn) {
        computersTurn();
        board.print();
      } else {
        playersTurn();
      }
      isComputersTurn = !isComputersTurn;
    }

    if (isComputersTurn) {
      board.print();
    }

    if (board.isWinner(playerCh)) {
      std::cout << "Player wins!" << std::endl;
    } else if (board.isWinner(computerCh)) {
      std::cout << "Computer wins!" << std::endl;
    } else if (board.isFilled()) {
      std::cout << "Draw!" << std::endl;
    }
  }

private:
  void playersTurn() {
    while (true) {
      std::cout << "Your [" << playerCh << "] move ― row[1-3] col[1-3]: ";

      std::string input;
      std::getline(std::cin, input);
      std::stringstream ss(input);

      size_t row, col;
      ss >> row >> col;
      if (!ss) {
        std::cout << "Invalid input!" << std::endl;
        continue;
      }

      if (!board.move(playerCh, row - 1, col - 1)) {
        std::cout << "Invalid move!" << std::endl;
        continue;
      }

      break;
    }
    std::cout << LINE_SEP << std::endl;
  }

  void computersTurn() {
    board = minimaxAlphaBeta(board);
  }

  Board minimaxAlphaBeta(const Board& board) const {
    int bestScore = MIN;
    Board best;

    for (const Board& succ : board.successors(computerCh)) {
      int score = min(succ, MIN, MAX);
      if (score > bestScore) {
        best = succ;
        bestScore = score;
      }
    }

    return best;
  }

  int min(const Board& board, int alpha, int beta) const {
    if (board.isFinal()) {
      return finalScore(board);
    }

    int bestScore = MAX;
    for (const Board& succ : board.successors(playerCh)) {
      bestScore = std::min(bestScore, max(succ, alpha, beta));

      if (bestScore <= alpha) {
        return bestScore;
      }

      beta = std::min(beta, bestScore);
    }

    return bestScore;
  }

  int max(const Board& board, int alpha, int beta) const {
    if (board.isFinal()) {
      return finalScore(board);
    }

    int bestScore = MIN;
    for (const Board& succ : board.successors(computerCh)) {
      bestScore = std::max(bestScore, min(succ, alpha, beta));

      if (bestScore >= beta) {
        return bestScore;
      }

      alpha = std::max(alpha, bestScore);
    }

    return bestScore;
  }

  int finalScore(const Board& board) const {
    if (board.isWinner(computerCh))
      return 1 + board.getEmptyCount();
    if (board.isWinner(playerCh))
      return -1 - board.getEmptyCount();
    return 0;
  }

};

int Board::winnings[8][3][2] {
  {{0, 0}, {0, 1}, {0, 2}},
  {{1, 0}, {1, 1}, {1, 2}},
  {{2, 0}, {2, 1}, {2, 2}},

  {{0, 0}, {1, 0}, {2, 0}},
  {{0, 1}, {1, 1}, {2, 1}},
  {{0, 2}, {1, 2}, {2, 2}},

  {{0, 0}, {1, 1}, {2, 2}},
  {{0, 2}, {1, 1}, {2, 0}},
};

int main() {
  while (true) {
    std::string input;

    std::cout << "Do you want to go first? [Y/n]: ";
    std::getline(std::cin, input);

    Game(input == "n").play();

    std::cout << "Do you want to play again? [Y/n]: ";
    std::getline(std::cin, input);

    if (input == "n") {
      break;
    }
  }
}

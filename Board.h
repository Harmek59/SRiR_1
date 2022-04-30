#pragma once

#include <array>
#include <vector>
#include <algorithm>
#include <numeric>
#include <ctime>
#include <cstdlib>
#include <exception>
#include <string>
#include <iostream>


enum class BoardMove : char {
    TOP, RIGHT, DOWN, LEFT
};

const char* boardMoveToString(BoardMove move) {
    switch (move) {
    case BoardMove::TOP:
        return "TOP";
    case BoardMove::RIGHT:
        return "RIGHT";
    case BoardMove::DOWN:
        return "DOWN";
    case BoardMove::LEFT:
        return "LEFT";
    }
    return "";
}

class Board {
public:
    constexpr static std::pair<int, int> dimension = std::make_pair(3, 3);
    using BoardArr = std::array<char, dimension.first* dimension.second>;

    Board() {
        reset();
    }

    bool isBoardSolved() const {
        return board.back() == 0 && std::is_sorted(board.begin(), std::prev(board.end()));

    }

    void shuffle(int numberOfRandomMoves) {
        for (int i = 0; i < numberOfRandomMoves; i++) {
            auto validMoves = getAllValidMoves();
            int randMoveIdx = std::rand() % validMoves.size();
            move(validMoves[randMoveIdx]);
        }
    }

    void reset() {
        std::iota(board.begin(), std::prev(board.end()), 1);
        board.back() = 0;
        emptyField = { dimension.first - 1, dimension.second - 1 };
    }

    void move(BoardMove move) {
        auto newEmptyField = getEmptyFieldAfterMove(move);

        if (areIndicesInRange(newEmptyField)) {
            std::swap(get(newEmptyField), get(emptyField));
            emptyField = newEmptyField;
        }
        else {
            throw std::runtime_error("Move not valid");
        }
    }

    std::vector<BoardMove> getAllValidMoves() const {
        std::vector<BoardMove> validMoves;
        validMoves.reserve(4);

        for (auto move : { BoardMove::TOP, BoardMove::RIGHT, BoardMove::DOWN, BoardMove::LEFT }) {
            if (areIndicesInRange(getEmptyFieldAfterMove(move))) {
                validMoves.push_back(move);
            }
        }
        return validMoves;
    }

    void draw() const {
        for (int i = 0; i < dimension.first; i++) {
            std::cout << "----";
        }
        std::cout << "-\n";
        for (int y = 0; y < dimension.second; y++) {
            for (int x = 0; x < dimension.first; x++) {
                if (x == 0) {
                    std::cout << "|";
                }
                auto c = get({ x, y });
                if (c == 0) {
                    std::cout << "  ";
                }
                else if (c > 9) {
                    std::cout << std::to_string(int(c));
                }
                else {
                    std::cout << " " << std::to_string(int(c));
                }
                std::cout << " |";
            }
            std::cout << "\n";
            for (int i = 0; i < dimension.first; i++) {
                std::cout << "----";
            }
            std::cout << "-\n";
        }
    }

    int size() const {
        return dimension.first * dimension.second;
    }

    void setData(const char* newData) {
        std::memcpy(board.data(), newData, size() * sizeof(char));
        findEmptyField();
    }

    const char* data() const {
        return board.data();
    }

    const BoardArr& getBoardArray() const {
        return board;
    }

private:
    void findEmptyField() {
        for (int y = 0; y < dimension.second; y++) {
            for (int x = 0; x < dimension.first; x++) {
                if (get({ x, y }) == 0) {
                    emptyField = { x, y };
                }
            }
        }
    }

    const char& get(const std::pair<int, int>& idx) const {
        return board[idx.second * dimension.first + idx.first];
    }

    char& get(const std::pair<int, int>& idx) {
        return board[idx.second * dimension.first + idx.first];
    }

    std::pair<int, int> getEmptyFieldAfterMove(BoardMove move) const {
        auto newEmptyField = emptyField;

        switch (move) {
        case BoardMove::TOP:
            newEmptyField.second--;
            break;
        case BoardMove::RIGHT:
            newEmptyField.first++;
            break;
        case BoardMove::DOWN:
            newEmptyField.second++;
            break;
        case BoardMove::LEFT:
            newEmptyField.first--;
            break;
        }

        return newEmptyField;
    }

    bool areIndicesInRange(const std::pair<int, int>& idxs) const {
        return idxs.first >= 0 && idxs.first < dimension.first&&
            idxs.second >= 0 && idxs.second < dimension.second;
    };

    std::pair<int, int> emptyField;

    // row wise
    BoardArr board;
};
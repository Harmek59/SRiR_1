#pragma once

#include "Board.h"
#include "KnownPositions.h"

#include <optional>


class TaskManager {
public:
    struct Task {
        char board[Board::dimension.first * Board::dimension.second];
        int depth;
    };

    static Task createTask(const Board& board, int depth) {
        Task task;
        task.depth = depth;
        std::memcpy(task.board, board.data(), board.size() * sizeof(char));
        return task;
    }

    TaskManager(const Board& startingBoard, int minimalTasksNumber) {
        taskMinimalSize = minimalTasksNumber;

        // reserve +4 because it might be slightly higher than taskMinimalSize
        taskBuffer.reserve(taskMinimalSize + 4);
        taskBuffer.push_back(createTask(startingBoard, 0));
        taskMoveSequences.reserve(taskMinimalSize + 4);
        taskMoveSequences.push_back({});
    }

    std::optional<std::vector<BoardMove>> expandUntilTaskBufferIsFull(KnownPositionsType& knownPositions) {
        while (int(taskBuffer.size()) < taskMinimalSize) {
            if (auto solvedBoardIdx = expand(knownPositions)) {
                std::cout << "Found solution while expanding\n";
                return taskMoveSequences[*solvedBoardIdx];
            }
        }
        return std::nullopt;
    }

    size_t size() const {
        return taskBuffer.size();
    }

    const Task* getTaskBuffer() const {
        return taskBuffer.data();
    }

    const Task& getTask(int idx) const {
        return taskBuffer[idx];
    }

    const std::vector<BoardMove>& getMoveSequence(int idx) const {
        return taskMoveSequences[idx];
    }

private:

    std::optional<size_t>
        expand(KnownPositionsType& knownPositions) {
        if (taskBuffer.empty()) {
            throw std::runtime_error("Task buffer cannot be empty");
        }
        int minDepth = taskBuffer.front().depth;
        auto nodeToExpand = std::prev(
            std::find_if_not(taskBuffer.begin(), taskBuffer.end(), [minDepth](const Task& task) {
                return task.depth == minDepth;
                }));

        auto nodeIdx = std::distance(taskBuffer.begin(), nodeToExpand);

        Board boardToExpand{};
        boardToExpand.setData(nodeToExpand->board);
        for (auto move : boardToExpand.getAllValidMoves()) {
            Board expandedBoard = boardToExpand;
            expandedBoard.move(move);

            // check is it known position
            if (knownPositions.count(expandedBoard.getBoardArray())) { // C++ 20 .contains(str_view)
                continue;
            }
            else {
                if (knownPositions.size() < knownPositionsSizeLimit) {
                    knownPositions[expandedBoard.getBoardArray()] = 0;
                }
            }


            auto moveSequence = taskMoveSequences[nodeIdx];
            moveSequence.push_back(move);

            taskMoveSequences.push_back(std::move(moveSequence));
            taskBuffer.push_back(createTask(expandedBoard, minDepth + 1));

            if (expandedBoard.isBoardSolved()) {
                return taskBuffer.size() - 1;
            }
        }

        // remove expanded board/task
        std::swap(taskMoveSequences[nodeIdx], taskMoveSequences.back());
        taskMoveSequences.pop_back();
        std::swap(taskBuffer[nodeIdx], taskBuffer.back());
        taskBuffer.pop_back();

        return std::nullopt;
    }

    int taskMinimalSize;
    std::vector<Task> taskBuffer;
    // moves to get to n-th task
    std::vector<std::vector<BoardMove>> taskMoveSequences;
};

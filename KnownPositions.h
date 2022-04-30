#pragma once

#include <unordered_map>

constexpr size_t knownPositionsSizeLimit = 1'000'000;
auto boardHasher = [](const Board::BoardArr& board) {
    return std::hash<std::string_view>()(std::string_view(board.data(), board.size()));
};
using KnownPositionsType = std::unordered_map<Board::BoardArr, int, decltype(boardHasher)>;
// int is remaining depth after getting to this position (it is possible to get to known position, but with better depth)
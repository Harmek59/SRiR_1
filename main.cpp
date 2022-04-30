#include "Board.h"
#include "TaskManager.h"

#include <mpi.h>

#include <chrono>


bool DFS(const Board& board, std::vector<BoardMove>& moveSequence, int depth, KnownPositionsType& knownPositions) {
    if (board.isBoardSolved()) {
        return true;
    }
    if (depth == 0) {
        return false;
    }
    for (auto move : board.getAllValidMoves()) {
        auto boardCpy = board;
        boardCpy.move(move);

        // check is it known position
        if (knownPositions.count(boardCpy.getBoardArray())) { // C++ 20 .contains(str_view)
            if (knownPositions[boardCpy.getBoardArray()] > depth) {
                continue;
            }
            else {
                knownPositions.at(boardCpy.getBoardArray()) = depth;
            }
        }
        else {
            if (knownPositions.size() < knownPositionsSizeLimit) {
                knownPositions[boardCpy.getBoardArray()] = depth;
            }
        }

        moveSequence.push_back(move);

        if (DFS(boardCpy, moveSequence, depth - 1, knownPositions)) {
            return true;
        }
        else {
            moveSequence.pop_back();
        }
    }
    return false;
}


int main(int argc, char* argv[]) {

    std::srand(unsigned(std::time(nullptr)));

    int DFSmaxDepth = 40;


    Board board;
    std::unique_ptr<TaskManager> taskManager;

    int procsId, numProcs;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
    MPI_Comm_rank(MPI_COMM_WORLD, &procsId);

    MPI_Status status;

    constexpr int soulutionTaskIdTransfer = 1;
    constexpr int solutionSizeTransfer = 2;
    constexpr int solutionTransfer = 2;

    int masterProcs = numProcs - 1;
    auto isMaster = [&]() -> bool {
        return procsId == masterProcs;
    };

    std::vector<BoardMove> moveSequence;

    KnownPositionsType knownPositions(0, boardHasher);
    knownPositions.reserve(knownPositionsSizeLimit);


    MPI_Barrier(MPI_COMM_WORLD);
    auto start = std::chrono::high_resolution_clock::now();


    int taskBufferSize = -1;
    std::vector<TaskManager::Task> taskBuffer;

    int taskIdToGetToSolution = -1;
    auto isSolutionFound = [&]() -> bool {
        return taskIdToGetToSolution != taskBufferSize;
    };

    if (isMaster()) {
        std::cout << "NumProcs: " << numProcs << std::endl;

        board.shuffle(50000);
        //char b[] = { 4, 1, 8, 2, 0, 7, 3, 6, 5 };
        //board.setData(b);
        board.draw();

        bool solutionFound = board.isBoardSolved();

        taskManager = std::make_unique<TaskManager>(board, 1000);
        if (!solutionFound) {
            if (auto res = taskManager->expandUntilTaskBufferIsFull(knownPositions)) {
                solutionFound = true;
                moveSequence = res.value();
            }
            knownPositions.clear();
        }

        taskBufferSize = taskManager->size();

        taskIdToGetToSolution = taskBufferSize;

        if (solutionFound) {
            taskIdToGetToSolution = taskBufferSize - 1;
        }

        taskBuffer.resize(taskBufferSize);
        std::memcpy(taskBuffer.data(), taskManager->getTaskBuffer(), sizeof(TaskManager::Task) * taskBufferSize);
    }

    MPI_Bcast(&taskBufferSize, 1, MPI_INT, masterProcs, MPI_COMM_WORLD);
    MPI_Bcast(&taskIdToGetToSolution, 1, MPI_INT, masterProcs, MPI_COMM_WORLD);
    if (!isMaster()) {
        taskBuffer.resize(taskBufferSize);
    }
    MPI_Bcast(taskBuffer.data(), taskBufferSize * sizeof(TaskManager::Task), MPI_CHAR, masterProcs, MPI_COMM_WORLD);

    for (const auto& task : taskBuffer) {
        Board b;
        b.setData(task.board);
        knownPositions[b.getBoardArray()] = DFSmaxDepth;
    }


    if (!isSolutionFound()) {
        int startingDepth = taskBuffer[0].depth + 1;
        moveSequence.reserve(DFSmaxDepth);

        auto threadExec = [&]() {
            for (int currDepth = startingDepth; currDepth <= DFSmaxDepth; currDepth++) {
                if (isMaster()) {
                    std::cout << "CurrDepth: " << currDepth << std::endl;
                }

                for (int taskIdx = procsId; taskIdx < taskBufferSize; taskIdx += numProcs) {
                    moveSequence.clear();
                    const auto& task = taskBuffer[taskIdx];
                    Board board;
                    board.setData(task.board);
                    if (DFS(board, moveSequence, currDepth - task.depth, knownPositions)) {
                        taskIdToGetToSolution = taskIdx;
                        break;
                    }
                }


                if (!isMaster()) {
                    MPI_Send(&taskIdToGetToSolution, 1, MPI_INT, masterProcs, soulutionTaskIdTransfer, MPI_COMM_WORLD);
                }

                if (isMaster()) {
                    for (int i = 0; i < masterProcs; i++) {
                        int tempSolutionFinder;
                        MPI_Recv(&tempSolutionFinder, 1, MPI_INT, MPI_ANY_SOURCE, soulutionTaskIdTransfer,
                            MPI_COMM_WORLD, &status);
                        if (tempSolutionFinder != taskBufferSize) {
                            taskIdToGetToSolution = tempSolutionFinder;
                        }
                    }
                }

                MPI_Bcast(&taskIdToGetToSolution, 1, MPI_INT, masterProcs, MPI_COMM_WORLD);

                if (isSolutionFound()) {
                    break;
                }
            }
        };

        threadExec();

        if (isSolutionFound()) {
            //get solution
            int threadThatFoundSolution =
                taskIdToGetToSolution < numProcs ? taskIdToGetToSolution : taskIdToGetToSolution % numProcs;
            if (isMaster() && threadThatFoundSolution == masterProcs) {
                moveSequence.insert(moveSequence.begin(), taskManager->getMoveSequence(taskIdToGetToSolution).begin(),
                    taskManager->getMoveSequence(taskIdToGetToSolution).end());
            }
            else if (procsId == threadThatFoundSolution) {
                int solutionSize = moveSequence.size();
                MPI_Send(&solutionSize, 1, MPI_INT, masterProcs, solutionSizeTransfer, MPI_COMM_WORLD);
                MPI_Send(moveSequence.data(), solutionSize, MPI_CHAR, masterProcs, solutionTransfer, MPI_COMM_WORLD);
            }
            else if (isMaster()) {
                int solutionSize;
                MPI_Recv(&solutionSize, 1, MPI_INT, MPI_ANY_SOURCE, solutionSizeTransfer, MPI_COMM_WORLD, &status);
                moveSequence.resize(taskManager->getMoveSequence(taskIdToGetToSolution).size() + solutionSize);
                std::copy(taskManager->getMoveSequence(taskIdToGetToSolution).begin(),
                    taskManager->getMoveSequence(taskIdToGetToSolution).end(), moveSequence.begin());
                MPI_Recv(moveSequence.data() + taskManager->getMoveSequence(taskIdToGetToSolution).size(), solutionSize,
                    MPI_LONG, MPI_ANY_SOURCE, solutionSizeTransfer, MPI_COMM_WORLD, &status);
            }
        }
    }


    MPI_Barrier(MPI_COMM_WORLD);
    auto stop = std::chrono::high_resolution_clock::now();

    if (isMaster()) {
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
        std::cout << "Time: " << duration.count() << "ms\n";
    }


    if (isMaster()) {
        if (isSolutionFound()) {
            std::cout << "Found solution. Number of moves: " << moveSequence.size() << "\nTaskId: "
                << taskIdToGetToSolution <<
                "\nMove sequence: ";
            for (auto move : moveSequence) {
                std::cout << boardMoveToString(move) << ", ";
            }

            std::cout << "Solving:\n";
            board.draw();
            for (auto move : moveSequence) {
                board.move(move);
                board.draw();
            }

        }
        else {
            std::cout << "Solution not found :(\n";
        }
    }

    MPI_Finalize();
    return 0;
}
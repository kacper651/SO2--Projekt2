#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <queue>
#include <random>
#include <ncurses.h>
#include <algorithm>
#include <unordered_map>

std::mutex dockMutex;
std::mutex printMutex;
std::mutex cargoMutex;

std::queue <std::pair<std::string, int>> warehouse;

const int NUM_WORKERS = 20;
const int NUM_TRUCKS = 5;
const int MAX_CARGO_SIZE = 5;
const double FORKLIFT_CARGO_RATIO = 0.75;

struct Cargo {
    std::string id;
    std::string state;
};

std::unordered_map <std::string, Cargo> cargoStates;

void printMessage(const std::string &message, int line) {
    std::unique_lock <std::mutex> lock(printMutex);
    move(line, 0);
    clrtoeol();
    printw("%s", message.c_str());
    refresh();
}

int getRandomCargoSize() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> distribution(1, MAX_CARGO_SIZE);
    return distribution(gen);
}

void printWarehouseStatus(int line) {
    std::unique_lock <std::mutex> lock(printMutex);
    move(line, 0);
    clrtoeol();
    printw("Warehouse Items: %zu", warehouse.size());
    refresh();
}

void printWorkerHeader(int line) {
    std::unique_lock <std::mutex> lock(printMutex);
    move(line, 0);
    clrtoeol();
    printw("Worker ID\tState\n");
    refresh();
}

void printCargoTable(int line) {
    std::unique_lock <std::mutex> lock(printMutex);
    move(line, 0);
    clrtoeol();
    printw("Cargo ID\tState\n");
    for (const auto &cargo: cargoStates) {
        printw("%s\t%s\n", cargo.first.c_str(), cargo.second.state.c_str());
    }
    refresh();
}

void worker(int id) {
    int line = id;

    while (true) {
        std::pair<std::string, int> cargo;

        {
            std::lock_guard <std::mutex> lock(dockMutex);
            if (!warehouse.empty()) {
                cargo = warehouse.front();
                warehouse.pop();
            }
        }

        if (!cargo.first.empty()) {
            std::string cargoId = cargo.first;

            {
                std::lock_guard <std::mutex> lock(cargoMutex);
                cargoStates[cargoId] = {cargoId, "New"};
            }

            printMessage("Worker " + std::to_string(id) + "\t is processing cargo: " + cargo.first, line);
            std::this_thread::sleep_for(std::chrono::milliseconds(800 * cargo.second));

            if (cargo.second > FORKLIFT_CARGO_RATIO * MAX_CARGO_SIZE) {
                std::lock_guard <std::mutex> lock(cargoMutex);
                cargoStates[cargoId].state = "Moving";

                printMessage("Worker " + std::to_string(id) + "\t is using the forklift to move cargo: " + cargo.first,
                             line);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }

            {
                std::lock_guard <std::mutex> lock(cargoMutex);
                cargoStates[cargoId].state = "Processing";
            }

            printMessage("Worker " + std::to_string(id) + "\t is further processing cargo: " + cargo.first, line);
            std::this_thread::sleep_for(std::chrono::milliseconds(500 * cargo.second));

            {
                std::lock_guard <std::mutex> lock(cargoMutex);
                cargoStates.erase(cargoId);
            }
        } else {
            printMessage("Worker " + std::to_string(id) + "\t is taking a break.", line);
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        }
        printWorkerHeader(0);
        printWarehouseStatus(NUM_WORKERS + NUM_TRUCKS + 5);
        printCargoTable(NUM_WORKERS + NUM_TRUCKS + 7);
    }
}

void truck(int id) {
    int line = NUM_WORKERS + 2 + id;

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        printMessage("Delivery Truck " + std::to_string(id) + " has arrived.", line);

        int cargoSize = getRandomCargoSize();

        {
            std::lock_guard <std::mutex> lock(dockMutex);
            printMessage("Delivery Truck " + std::to_string(id) + " has accessed a dock.", line);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            for (int i = 1; i <= cargoSize; ++i) {
                std::string cargoId = "Cargo " + std::to_string(id) + "-" + std::to_string(i);
                Cargo cargo;
                cargo.id = cargoId;
                cargo.state = "Unloaded";

                {
                    std::lock_guard <std::mutex> lock(cargoMutex);
                    warehouse.emplace(cargoId, getRandomCargoSize());
                    cargoStates[cargoId] = cargo;
                }

                printMessage("Delivery Truck " + std::to_string(id) + " unloaded cargo: " + cargoId, line);
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
            }
        }

        printMessage("Delivery Truck " + std::to_string(id) + " has released the dock.", line);
        printWorkerHeader(0);
        printWarehouseStatus(NUM_WORKERS + NUM_TRUCKS + 5);
        printCargoTable(NUM_WORKERS + NUM_TRUCKS + 7);
    }
}

int main() {
    initscr();
    clear();

    std::vector <std::thread> workers;
    std::vector <std::thread> trucks;

    int line = 0;
    for (int i = 1; i <= NUM_WORKERS; ++i) {
        line++;
        workers.emplace_back(worker, i);
    }

    for (int i = 1; i <= NUM_TRUCKS; ++i) {
        line++;
        trucks.emplace_back(truck, i);
    }

    printWorkerHeader(0);
    printCargoTable(NUM_WORKERS + NUM_TRUCKS + 7);

    for (auto &workerThread: workers) {
        workerThread.join();
    }

    for (auto &truckThread: trucks) {
        truckThread.join();
    }

    endwin();
    return 0;
}

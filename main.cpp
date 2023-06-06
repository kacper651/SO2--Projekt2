#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <queue>
#include <random>
#include <ncurses.h>

std::mutex dockMutex;
std::mutex forkliftMutex;
std::mutex printMutex;

std::queue<std::pair<std::string, int>> warehouse;

const int NUM_WORKERS = 10;
const int NUM_TRUCKS = 1;
const int MAX_CARGO_SIZE = 5;
const double FORKLIFT_CARGO_RATIO = 0.75;

void printMessage(const std::string& message, int line) {
    std::lock_guard<std::mutex> lock(printMutex);
    move(line, 0);
    clrtoeol();
    printw("%s", message.c_str());
    refresh();
}

int getRandomCargoSize() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distribution(1, MAX_CARGO_SIZE);
    return distribution(gen);
}

void printWarehouseStatus(int line) {
    std::lock_guard<std::mutex> lock(printMutex);
    move(line, 0);
    clrtoeol();
    printw("Warehouse Items: %zu", warehouse.size());
    refresh();
}

void worker(int id) {
    int line = id;

    while (true) {
        std::pair<std::string, int> cargo;

        dockMutex.lock();
        if (!warehouse.empty()) {
            cargo = warehouse.front();
            warehouse.pop();
        }
        dockMutex.unlock();

        if (!cargo.first.empty()) {
            printMessage("Worker " + std::to_string(id) + " is processing cargo: " + cargo.first, line);
            std::this_thread::sleep_for(std::chrono::milliseconds(800 * cargo.second));

            if (cargo.second > FORKLIFT_CARGO_RATIO * MAX_CARGO_SIZE) {
                forkliftMutex.lock();
                printMessage("Worker " + std::to_string(id) + " is using the forklift to move cargo: " + cargo.first, line);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                forkliftMutex.unlock();
            }

            printMessage("Worker " + std::to_string(id) + " is further processing cargo: " + cargo.first, line);
            std::this_thread::sleep_for(std::chrono::milliseconds(500 * cargo.second));
        }
        else {
            printMessage("Worker " + std::to_string(id) + " is taking a break.", line);
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        }
        printWarehouseStatus(NUM_WORKERS + NUM_TRUCKS + 5);
    }
}

void truck(int id) {
    int line = NUM_WORKERS + 2 + id;

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        printMessage("Delivery Truck " + std::to_string(id) + " has arrived.", line);

        int cargoSize = getRandomCargoSize();

        dockMutex.lock();
        printMessage("Delivery Truck " + std::to_string(id) + " has accessed a dock.", line);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        for (int i = 1; i <= cargoSize; ++i) {
            std::string cargo = "Cargo " + std::to_string(id) + "-" + std::to_string(i);
            int cargoWeight = getRandomCargoSize();
            warehouse.emplace(cargo, cargoWeight);
            printMessage("Delivery Truck " + std::to_string(id) + " unloaded cargo: " + cargo, line);
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }

        dockMutex.unlock();
        printMessage("Delivery Truck " + std::to_string(id) + " has released the dock.", line);

        printWarehouseStatus(NUM_WORKERS + NUM_TRUCKS + 5);
    }
}

int main() {
    initscr();
    clear();

    std::vector<std::thread> workers;
    std::vector<std::thread> trucks;

    int line = 0;
    for (int i = 1; i <= NUM_WORKERS; ++i) {
        line++;
        workers.emplace_back(worker, i);
    }

    for (int i = 1; i <= NUM_TRUCKS; ++i) {
        line++;
        trucks.emplace_back(truck, i);
    }

    for (auto& workerThread : workers) {
        workerThread.join();
    }

    for (auto& truckThread : trucks) {
        truckThread.join();
    }

    endwin();
    return 0;
}

#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <iomanip>
#include <ctime>
#include <string>
#include <Windows.h>
#include <conio.h>
#include <fstream>
#include <nlohmann/json.hpp>
#pragma comment(lib, "winmm.lib")

using namespace std;
using json = nlohmann::json;

#define PLAY_SOUND(file) PlaySound(TEXT(file), NULL, SND_FILENAME | SND_ASYNC)
#define CLEAR_SCREEN() system("cls")
#define GETCH() _getch()
#define KEY_UP 72
#define KEY_DOWN 80
#define KEY_LEFT 75
#define KEY_RIGHT 77
#define KEY_ENTER 13
#define KEY_ESC 27

#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_CYAN "\033[36m"

int getArrowKey() {
    int c = GETCH();
    return (c == 0 || c == 224) ? GETCH() : c;
}

struct Player {
    string name;
    int score = 0;
    int flips = 0;
    Player(string n = "") : name(n) {}
};

class MemoryGame {
    int rows, cols, cursorRow = 0, cursorCol = 0, currentPlayerIndex = 0;
    vector<char> cards;
    vector<bool> revealed;
    vector<int> cardStatus;
    vector<Player> players;
    chrono::time_point<chrono::system_clock> startTime;
    bool isTwoPlayerMode, gameOver = false;

    void generateCards() {
        int pairs = (rows * cols) / 2;
        cards.clear();
        for (int i = 0; i < pairs; ++i) {
            char symbol = i < 26 ? 'A' + i : '0' + (i - 26) % 10;
            cards.insert(cards.end(), { symbol, symbol });
        }
        shuffle(cards.begin(), cards.end(), mt19937(random_device{}()));
        revealed.assign(rows * cols, false);
        cardStatus.assign(rows * cols, 0);
    }

    void switchPlayer() {
        if (isTwoPlayerMode) currentPlayerIndex = 1 - currentPlayerIndex;
    }

    void playMatchSound() {
        PLAY_SOUND("match.wav");
    }

    void saveStatsToJson(double elapsedSeconds) {
        json data;
        data["mode"] = isTwoPlayerMode ? "2 Player" : "1 Player";
        data["duration_seconds"] = elapsedSeconds;

        if (isTwoPlayerMode) {
            data["players"] = json::array();
            for (auto& p : players) {
                data["players"].push_back({
                    {"name", p.name},
                    {"score", p.score},
                    {"flips", p.flips}
                    });
            }
            data["winner"] = players[0].score == players[1].score ? "Tie" :
                (players[0].score > players[1].score ? players[0].name : players[1].name);
        }
        else {
            data["player"] = {
                {"name", players[0].name},
                {"flips", players[0].flips}
            };
        }

        ofstream file("game_stats.json", ios::app);
        if (file.is_open()) {
            file << data.dump(4) << "\n";
            file.close();
        }
    }

public:
    MemoryGame(int r, int c, bool twoPlayer)
        : rows(r + (r * c % 2)), cols(c), isTwoPlayerMode(twoPlayer) {
        if (isTwoPlayerMode) players = { Player("Player 1"), Player("Player 2") };
        else players = { Player("Player") };
        generateCards();
        startTime = chrono::system_clock::now();
    }

    void displayBoard() {
        CLEAR_SCREEN();
        cout << COLOR_CYAN << "Memory Card Game\n----------------" << COLOR_RESET << endl;
        if (isTwoPlayerMode) {
            cout << "Current Turn: " << COLOR_YELLOW << players[currentPlayerIndex].name << COLOR_RESET << endl;
            for (auto& p : players) cout << p.name << " Score: " << p.score << " | ";
            cout << endl;
        }
        else cout << "Flips: " << players[0].flips << endl;

        cout << "Use arrow keys to navigate, Enter to select, ESC to exit\n\n  ";
        for (int j = 0; j < cols; ++j) cout << " " << j + 1 << " ";
        cout << endl;

        for (int i = 0; i < rows; ++i) {
            cout << i + 1 << " ";
            for (int j = 0; j < cols; ++j) {
                int idx = i * cols + j;
                string color = (i == cursorRow && j == cursorCol) ? COLOR_YELLOW :
                    (cardStatus[idx] == 2) ? COLOR_GREEN :
                    (cardStatus[idx] == 3 || cardStatus[idx] == 1) ? COLOR_RED : COLOR_RESET;
                cout << color << "[" << (revealed[idx] ? cards[idx] : '?') << "]" << COLOR_RESET;
            }
            cout << endl;
        }
    }

    void processInput() {
        static int first = -1;
        int key = getArrowKey();
        switch (key) {
        case KEY_UP: if (cursorRow > 0) cursorRow--; break;
        case KEY_DOWN: if (cursorRow < rows - 1) cursorRow++; break;
        case KEY_LEFT: if (cursorCol > 0) cursorCol--; break;
        case KEY_RIGHT: if (cursorCol < cols - 1) cursorCol++; break;
        case KEY_ENTER: {
            int idx = cursorRow * cols + cursorCol;
            if (revealed[idx] || idx == first) break;
            revealed[idx] = true; cardStatus[idx] = 1; players[currentPlayerIndex].flips++;
            if (first == -1) { first = idx; break; }
            displayBoard();
            if (cards[first] == cards[idx]) {
                cout << COLOR_GREEN << "Match found!" << COLOR_RESET << endl; playMatchSound();
                cardStatus[first] = cardStatus[idx] = 2;
                players[currentPlayerIndex].score++;
            }
            else {
                cout << COLOR_RED << "No match. Flipping back..." << COLOR_RESET << endl;
                cardStatus[first] = cardStatus[idx] = 3;
                displayBoard(); this_thread::sleep_for(chrono::milliseconds(1000));
                revealed[first] = revealed[idx] = false;
                cardStatus[first] = cardStatus[idx] = 0;
                switchPlayer();
            }
            first = -1;
            gameOver = all_of(revealed.begin(), revealed.end(), [](bool v) { return v; });
            break;
        }
        case KEY_ESC: gameOver = true; break;
        }
    }

    bool isGameOver() const { return gameOver; }

    void showStats() {
        auto elapsed = chrono::duration<double>(chrono::system_clock::now() - startTime).count();
        cout << COLOR_CYAN << "\nGame Statistics:\n---------------" << COLOR_RESET << endl;
        cout << "Total time: " << fixed << setprecision(2) << elapsed << " seconds" << endl;
        if (isTwoPlayerMode) {
            string winner = players[0].score == players[1].score ? "It's a tie!" :
                (players[0].score > players[1].score ? players[0].name : players[1].name) + " wins!";
            cout << COLOR_GREEN << winner << COLOR_RESET << endl;
            for (auto& p : players) cout << p.name << " - Score: " << p.score << ", Flips: " << p.flips << endl;
        }
        else {
            cout << "Total flips: " << players[0].flips << endl;
        }

        saveStatsToJson(elapsed);
    }

};

int main() {
    int rows = 0, cols = 0, choice = 0;
    bool isTwoPlayer = false;

    while (true) {
        CLEAR_SCREEN();
        cout << COLOR_CYAN << "MEMORY CARD GAME\n=====================" << COLOR_RESET << endl;
        cout << "Press any key to continue..."; GETCH();

        while (true) {
            CLEAR_SCREEN();
            cout << COLOR_CYAN << "Select Mode:" << COLOR_RESET << endl;
            cout << (choice == 0 ? COLOR_YELLOW : COLOR_RESET) << "1 Player" << COLOR_RESET << endl;
            cout << (choice == 1 ? COLOR_YELLOW : COLOR_RESET) << "2 Player" << COLOR_RESET << endl;
            cout << (choice == 2 ? COLOR_YELLOW : COLOR_RESET) << "Exit" << COLOR_RESET << endl;
            int k = getArrowKey();
            if (k == KEY_UP) choice = (choice + 2) % 3;
            if (k == KEY_DOWN) choice = (choice + 1) % 3;
            if (k == KEY_ENTER || k == KEY_ESC) break;
        }

        if (choice == 2 || getArrowKey() == KEY_ESC) return 0;
        isTwoPlayer = (choice == 1);

        choice = 0;
        while (true) {
            CLEAR_SCREEN();
            cout << COLOR_CYAN << "Choose Difficulty:" << COLOR_RESET << endl;
            string opts[] = { "2x2", "4x4", "6x6", "Custom", "Back" };
            for (int i = 0; i < 5; ++i)
                cout << (choice == i ? COLOR_YELLOW : COLOR_RESET) << opts[i] << COLOR_RESET << endl;
            int k = getArrowKey();
            if (k == KEY_UP) choice = (choice + 4) % 5;
            if (k == KEY_DOWN) choice = (choice + 1) % 5;
            if (k == KEY_ENTER) break;
        }

        if (choice == 4) continue;
        if (choice < 3) {
            int size = (choice + 1) * 2;
            tie(rows, cols) = tie(size, size);
        }
        else {
            cout << "Custom size:\nRows: "; cin >> rows;
            cout << "Columns: "; cin >> cols;
            rows = max(2, rows); cols = max(2, cols);
            while (rows * cols > 52) --(cols > rows ? cols : rows);
        }

        MemoryGame game(rows, cols, isTwoPlayer);
        while (!game.isGameOver()) { game.displayBoard(); game.processInput(); }
        CLEAR_SCREEN();
        game.displayBoard();
        cout << COLOR_GREEN << "\n🎉 All pairs found! 🎉" << COLOR_RESET << endl;
        game.showStats();
        cout << "\nPress any key to exit..."; GETCH();
        return 0;
    }
}

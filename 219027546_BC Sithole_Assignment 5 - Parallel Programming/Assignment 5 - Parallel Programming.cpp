// Assignment 5 - Parallel Programming.cpp
// This file contains the 'main' function. Program execution begins and ends there.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <numeric>
#include <iomanip>
#include <filesystem>
#include <iomanip>

using namespace std;
namespace fs = filesystem;

// Console colors for Windows
#ifdef _WIN32
#include <windows.h>
void setColor(int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}
#else
void setColor(int color) {
    switch (color) {
    case 10: std::cout << "\033[32m"; break; // Green
    case 12: std::cout << "\033[31m"; break; // Red
    case 15: std::cout << "\033[37m"; break; // White
    default: std::cout << "\033[0m"; break;
    }
}
#endif

// Shared data
double globalAverage = 0.0;
mutex mtx;
condition_variable cv;
bool readyA = false, readyB = false;

// Global vectors to store data for display
vector<int> regionAData;
vector<int> regionBData;

// Function to auto-create files if they don't exist
void createFileIfNotExists(const string& filename, const vector<int>& defaultData) {
    if (!fs::exists(filename)) {
        setColor(12);
        cerr << "[WARNING] File not found: " << filename << ". Creating automatically..." << endl;

        ofstream file(filename);
        if (file.is_open()) {
            for (int value : defaultData) {
                file << value << endl;
            }
            file.close();
            setColor(10);
            cout << "[SUCCESS] Successfully created: " << filename << endl;
            setColor(15);
        }
        else {
            setColor(12);
            cerr << "[ERROR] Could not create " << filename << endl;
            setColor(15);
        }
    }
    else {
        setColor(10);
        cout << "[SUCCESS] File found: " << filename << endl;
        setColor(15);
    }
}

// Function to read integers from a file into a vector
vector<int> readFile(const string& filename) {
    vector<int> data;
    ifstream file(filename);

    if (!file.is_open()) {
        setColor(12);
        cerr << "[ERROR] Could not open " << filename << endl;
        setColor(15);
        return data;
    }

    int value;
    while (file >> value) {
        data.push_back(value);
    }
    file.close();

    if (data.empty()) {
        setColor(12);
        cerr << "[WARNING] " << filename << " is empty!" << endl;
        setColor(15);
    }
    else {
        setColor(10);
        cout << "[INFO] Read " << data.size() << " values from " << filename << endl;
        setColor(15);
    }

    return data;
}

// Function to display all numbers in a table format
void displayNumbersTable() {
    setColor(10);
    cout << "\n" << string(70, '=') << endl;
    cout << "                    DELIVERY TIMES - ALL REGIONS" << endl;
    cout << string(70, '=') << endl;

    // Find the maximum size for table rows
    size_t maxSize = max(regionAData.size(), regionBData.size());

    // Table header
    setColor(15);
    cout << left << setw(10) << "Row"
        << setw(20) << "Region A (minutes)"
        << setw(20) << "Region B (minutes)"
        << setw(15) << "Difference" << endl;
    cout << string(70, '-') << endl;

    // Display data row by row
    for (size_t i = 0; i < maxSize; i++) {
        cout << left << setw(10) << (i + 1);

        // Region A value
        if (i < regionAData.size()) {
            setColor(10);
            cout << setw(20) << regionAData[i];
            setColor(15);
        }
        else {
            cout << setw(20) << "N/A";
        }

        // Region B value
        if (i < regionBData.size()) {
            setColor(10);
            cout << setw(20) << regionBData[i];
            setColor(15);
        }
        else {
            cout << setw(20) << "N/A";
        }

        // Difference (if both exist)
        if (i < regionAData.size() && i < regionBData.size()) {
            int diff = regionAData[i] - regionBData[i];
            if (diff > 0) {
                setColor(12);
                cout << setw(15) << "+" + to_string(diff);
            }
            else if (diff < 0) {
                setColor(10);
                cout << setw(15) << to_string(diff);
            }
            else {
                setColor(15);
                cout << setw(15) << "0";
            }
            setColor(15);
        }
        else {
            cout << setw(15) << "N/A";
        }
        cout << endl;
    }

    cout << string(70, '-') << endl;

    // Summary statistics
    setColor(10);
    cout << "\nSUMMARY STATISTICS:" << endl;
    setColor(15);
    cout << "Region A - Total numbers: " << regionAData.size() << endl;
    cout << "Region B - Total numbers: " << regionBData.size() << endl;

    // Calculate sums
    double sumA = accumulate(regionAData.begin(), regionAData.end(), 0.0);
    double sumB = accumulate(regionBData.begin(), regionBData.end(), 0.0);

    cout << "Region A - Sum: " << fixed << setprecision(2) << sumA << endl;
    cout << "Region B - Sum: " << fixed << setprecision(2) << sumB << endl;

    cout << string(70, '=') << endl;
    setColor(15);
}

// Function to calculate average of a vector
double calculateAverage(const vector<int>& data, const string& regionName) {
    if (data.empty()) {
        setColor(12);
        cerr << "[ERROR] " << regionName << " has no data to calculate average!" << endl;
        setColor(15);
        return 0.0;
    }
    double sum = accumulate(data.begin(), data.end(), 0.0);
    return sum / data.size();
}

// Thread function for Region A
void processRegionA() {
    setColor(10);
    cout << "\n[Thread A] Starting Region A processing..." << endl;
    setColor(15);

    regionAData = readFile("RegionA.txt");
    double avg = calculateAverage(regionAData, "Region A");

    setColor(10);
    cout << "[Thread A] Region A Average: ";
    setColor(15);
    cout << fixed << setprecision(2) << avg << endl;

    lock_guard<mutex> lock(mtx);
    globalAverage += avg;
    readyA = true;

    setColor(10);
    cout << "[Thread A] Region A complete. Signaled condition variable." << endl;
    setColor(15);

    cv.notify_all();
}

// Thread function for Region B
void processRegionB() {
    setColor(10);
    cout << "\n[Thread B] Starting Region B processing..." << endl;
    setColor(15);

    regionBData = readFile("RegionB.txt");
    double avg = calculateAverage(regionBData, "Region B");

    setColor(10);
    cout << "[Thread B] Region B Average: ";
    setColor(15);
    cout << fixed << setprecision(2) << avg << endl;

    lock_guard<mutex> lock(mtx);
    globalAverage += avg;
    readyB = true;

    setColor(10);
    cout << "[Thread B] Region B complete. Signaled condition variable." << endl;
    setColor(15);

    cv.notify_all();
}

// Thread function to wait for both regions and print combined average
void waitAndPrintCombined() {
    setColor(10);
    cout << "\n[Thread Combined] Waiting for both regions to complete..." << endl;
    setColor(15);

    unique_lock<mutex> lock(mtx);
    cv.wait(lock, [] {
        return readyA && readyB;
        });

    // Display the table with all numbers
    displayNumbersTable();

    double combined = globalAverage / 2.0;

    setColor(10);
    cout << "\n==================================================" << endl;
    cout << "FINAL RESULTS" << endl;
    cout << "==================================================" << endl;
    cout << "Region A Average: ";
    setColor(15);
    cout << fixed << setprecision(2) << (globalAverage / 2.0) << endl;
    setColor(10);
    cout << "Region B Average: ";
    setColor(15);
    cout << fixed << setprecision(2) << (globalAverage / 2.0) << endl;
    setColor(10);
    cout << "Combined Average: ";
    setColor(15);
    cout << fixed << setprecision(2) << combined << endl;
    setColor(10);
    cout << "==================================================" << endl;
    setColor(15);
}

int main() {
    setColor(10);
    cout << "============================================================" << endl;
    cout << "   ASSIGNMENT 5: PARALLEL PROGRAMMING" << endl;
    cout << "============================================================" << endl;
    setColor(15);

    // Default data for auto-creation
    vector<int> defaultDataA = { 82, 15, 9, 20, 64, 73 };
    vector<int> defaultDataB = { 8, 14, 23, 50, 69, 71 };

    // Auto-create files if they don't exist
    cout << "\n[CHECK] Checking files..." << endl;
    createFileIfNotExists("RegionA.txt", defaultDataA);
    createFileIfNotExists("RegionB.txt", defaultDataB);

    cout << "\n[START] Starting parallel processing..." << endl;

    try {
        // Create threads
        thread tA(processRegionA);
        thread tB(processRegionB);
        thread tCombined(waitAndPrintCombined);

        // Join threads
        tA.join();
        tB.join();
        tCombined.join();

        setColor(10);
        cout << "\n[SUCCESS] Program completed successfully!" << endl;
        setColor(15);

    }
    catch (const exception& e) {
        setColor(12);
        cerr << "\n[EXCEPTION] Caught exception: " << e.what() << endl;
        setColor(15);
        return 1;
    }

    cout << "\nPress Enter to exit...";
    cin.get();

    return 0;
}
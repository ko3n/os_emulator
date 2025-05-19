#include <iostream>
#include <string>
#include <map>
#include <ctime>
#include <iomanip>
#include <sstream>

struct ScreenSession {
    std::string name;
    int currentLine;
    int totalLines;
    std::string timestamp;
};

std::map<std::string, ScreenSession> screens;

std::string getCurrentTimestamp() {
    std::time_t now = std::time(nullptr);
    std::tm* local = std::localtime(&now);

    std::ostringstream oss;
    oss << std::put_time(local, "%m/%d/%Y, %I:%M:%S %p");
    return oss.str();
}

void printHeader(){
    std::cout << R"(
      OO O o o o...      ______________________ _________________
  O     ____          |                    | |               |
 ][_n_i_| (   ooo___  |                    | |               |
(__________|_[______]_|____________________|_|_______________|
  0--0--0      0  0      0       0     0        0        0      Choo-Choo OS Emulator
)" <<"\n";
    std::cout << "\033[32mHello, Welcome to CSOPESY commandline!\033[0m\n";
    std::cout << "\033[33mType 'exit' to quit, 'clear' to clear the screen\033[0m\n\n";
    std::cout << "\033[34mUse: screen -s <name> to start a screen\033[0m\n";
    std::cout << "\033[34mUse: screen -r <name> to resume a screen\033[0m\n";
}

void clearScreen(){
    std::system("cls");
    printHeader();
}

void initialize(){
    std::cout <<"initialize command recognized. Doing something...\n";
}

void screen(){
    std::cout <<"screen command recognized. Doing something...\n";
}

void schedulerTest(){
    std::cout <<"scheduler-test command recognized. Doing something...\n";
}

void schedulerStop(){
    std::cout <<"scheduler-stop command recognized. Doing something...\n";
}

void reportUtil(){
    std::cout <<"report-util command recognized. Doing something...\n";
}

void screenSessionInterface(ScreenSession& session) {
    auto display = [&]() {
        std::cout << "\n";
        std::cout << "\033[31m=========== SCREEN : " << session.name << " ===========\033[0m\n";
        std::cout << "Process Name          : " << session.name << "\n";
        std::cout << "Instruction Progress  : " << session.currentLine << " / " << session.totalLines << "\n";
        std::cout << "Created At            : " << session.timestamp << "\n";
        std::cout << "\n\033[33mType 'exit' to return to main menu.\033[0m\n";
    };

    display();

    std::string input;
    while (true) {
        std::cout << "\n(" << session.name << ")> ";
        std::getline(std::cin, input);
        if (input == "exit") {
            break;
        } else if (input == "clear") {
            clearScreen();
            display();
        } else {
            session.currentLine = std::min(session.totalLines, session.currentLine + 1);
            display();
        }
    }

    clearScreen();
}

void handleScreenCommand(const std::string& command) {
    std::istringstream iss(command);
    std::string cmd, flag, name;
    iss >> cmd >> flag >> name;
    // Start screen
    if (flag == "-s") {
        if (screens.count(name)) {
            std::cout << "Screen session '" << name << "' already exists.\n";
        } else {
            ScreenSession newSession = {
                name,
                1, // current line
                100, // total lines
                getCurrentTimestamp()
            };
            screens[name] = newSession;
            screenSessionInterface(screens[name]);
        }
    // Resume screen
    } else if (flag == "-r") {
        if (!screens.count(name)) {
            std::cout << "No session named '" << name << "' found.\n";
        } else {
            screenSessionInterface(screens[name]);
        }
    } else {
        std::cout << "Invalid screen usage. Try: screen -s <name> or screen -r <name>\n";
    }
}

int main(){
    std::string userInput;
    printHeader();

    while(true){
        std::cout << "\n>";
        std::getline(std::cin, userInput);
        if(userInput == "initialize"){
            initialize();
        }else if(userInput == "screen"){
            screen();
        }else if(userInput == "scheduler-test"){
            schedulerTest();
        }else if(userInput == "scheduler-stop"){
            schedulerStop();
        }else if(userInput == "report-util"){
            reportUtil();
        }else if(userInput == "clear"){
            clearScreen();
        } else if (userInput.rfind("screen", 0) == 0) {
            handleScreenCommand(userInput);
        }else if(userInput == "exit"){
            break;
        }else{
            std::cout <<"Unknown command. Please try again\n";
        }
    }

    return 0;
}
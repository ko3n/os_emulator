#include <iostream>
#include <string>
#include <stdlib.h>
using namespace std;

void header(){
    cout << "  ___  ____   __  ____  ____  ____  _  _ \n";
    cout << " / __)/ ___) /  \\(  _ \\(  __)/ ___)( \\/ )\n";
    cout << "( (__ \\___ \\(  O )) __/ ) _) \\___ \\ )  / \n";
    cout << "\\___)(____/ \\__/(__)  (____)(____/(__/  \n";
    cout << "\033[32mHello, Welcome to CSOPESY commandline!\033[0m\n";
    cout << "\033[33mType 'exit' to quit, 'clear' to clear the screen\033[0m\n";
}
int main() {
    string command;
    header();

    do {
        cout << "\nEnter a command: ";
        cin >> command;

        if (command == "initialize" ||
            command == "screen" ||
            command == "scheduler-test" ||
            command == "scheduler-stop" ||
            command == "report-util") {
            cout << command << " command recognized. Doing something...\n";
        } else if (command == "clear") {
            system("CLS");
            header();
        } else {
            cout << "Unknown command. Try again.\n";
        }
        
    } while (command != "exit");

    return 0;
}
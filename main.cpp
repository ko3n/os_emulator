#include <iostream>
#include <string>
#include <cstdlib>

void printHeader(){
    std::cout << R"(
      OO O o o o...      ______________________ _________________
  O     ____          |                    | |               |
 ][_n_i_| (   ooo___  |                    | |               |
(__________|_[______]_|____________________|_|_______________|
  0--0--0      0  0      0       0     0        0        0      Choo-Choo OS Emulator
)" <<"\n";
    std::cout << "\033[32mHello, Welcome to CSOPESY commandline!\033[0m\n";
    std::cout << "\033[33mType 'exit' to quit, 'clear' to clear the screen\033[0m\n";
}

void clearScreen(){
    std::system("cls");
    printHeader();
}

void initialize(){
    std::cout <<"initialize command recognized. Doing something...""\n";
}

void screen(){
    std::cout <<"screen command recognized. Doing something...""\n";
}

void schedulerTest(){
    std::cout <<"scheduler-test command recognized. Doing something...""\n";
}

void schedulerStop(){
    std::cout <<"scheduler-stop command recognized. Doing something...""\n";
}

void reportUtil(){
    std::cout <<"report-util command recognized. Doing something...""\n";
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
        }else if(userInput == "exit"){
            break;
        }
    }

    return 0;
}
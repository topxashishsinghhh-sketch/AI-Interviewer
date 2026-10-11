// OOP: Application Entry Point
#include "ui/ConsoleUI.h"
#include <iostream>

int main(int argc, char* argv[]) {
    ui::ConsoleUI app(argc, argv);
    return app.run(argc, argv);
}
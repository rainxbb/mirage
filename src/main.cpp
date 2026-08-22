#include "Core/Application.h"

#include <iostream>
#include <string>

int main(int argc, char** argv)
{
    bool forceDesktop = false;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--desktop" || arg == "-d")
        {
            forceDesktop = true;
            std::cout << "[Mirage] Launching in Desktop-only mode (OpenXR disabled).\n";
        }
    }

    try
    {
        Mirage::Application app(forceDesktop);
        app.Run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

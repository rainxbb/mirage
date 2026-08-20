#include "Core/Application.h"

#include <iostream>
#include <stdexcept>

int main()
{
    try
    {
        Mirage::Application app;
        app.Run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

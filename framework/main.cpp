#include <sstream>
#include "application.h"
#include "magma/magma.h"

void onError(
    const std::string& msg,
    const char *caption)
{
    std::cerr << msg << std::endl;
#ifdef _WIN32
    MessageBoxA(NULL,
        msg.c_str(), caption,
        MB_ICONHAND);
#endif
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
#else
int main(int argc, char *argv[])
#endif
{
    AppEntry entry;
#ifdef _WIN32
    entry.hInstance = hInstance;
    entry.hPrevInstance = hPrevInstance;
    entry.lpCmdLine = pCmdLine;
    entry.nCmdShow = nCmdShow;
#else
    entry.argc = argc;
    entry.argv = argv;
#endif
    try
    {
        std::unique_ptr<IApplication> app = appFactory(entry);
        app->show();
        app->run();
    }
    catch (const magma::BadResult& exc)
    {
        std::ostringstream msg;
        msg << exc.file() << "(" << exc.line() << "):" << std::endl
            << std::endl
            << magma::helpers::stringize(exc.getResult()) << std::endl
            << exc.what();
        onError(msg.str(), "Vulkan");
    }
    catch (const magma::NotImplemented& exc)
    {
        std::ostringstream msg;
        msg << exc.file() << "(" << exc.line() << "):" << std::endl
            << "Error: " << exc.what();
        onError(msg.str(), "Not implemented");
    }
    catch (const magma::Exception& exc)
    {
        std::ostringstream msg;
        msg << exc.file() << "(" << exc.line() << "):" << std::endl
            << "Error: " << exc.what();
        onError(msg.str(), "Magma");
    }
    catch (const std::exception& exc)
    {
        std::ostringstream msg;
        msg << "Error: " << exc.what() << std::endl;
        onError(msg.str(), "Error");
    }
    return 0;
}
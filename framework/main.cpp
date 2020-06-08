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
    catch (const magma::exception::ErrorResult& exc)
    {
        std::ostringstream msg;
        msg << exc.location().file_name() << "(" << exc.location().line() << "):" << std::endl
            << std::endl
            << magma::helpers::stringize(exc.error()) << std::endl
            << exc.what();
        onError(msg.str(), "Vulkan");
    }
    catch (const magma::exception::ReflectionErrorResult& exc)
    {
        std::ostringstream msg;
        msg << exc.location().file_name() << "(" << exc.location().line() << "):" << std::endl
            << std::endl
            << magma::helpers::stringize(exc.error()) << std::endl
            << exc.what();
        onError(msg.str(), "SPIRV-Reflect");
    }
    catch (const magma::exception::NotImplemented& exc)
    {
        std::ostringstream msg;
        msg << exc.location().file_name() << "(" << exc.location().line() << "):" << std::endl
            << "Error: " << exc.what();
        onError(msg.str(), "Not implemented");
    }
    catch (const magma::exception::Exception& exc)
    {
        std::ostringstream msg;
        msg << exc.location().file_name() << "(" << exc.location().line() << "):" << std::endl
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
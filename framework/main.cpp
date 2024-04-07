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

#ifdef MAGMA_NO_EXCEPTIONS
void runApp(const AppEntry& entry)
{
    magma::exception::setExceptionHandler(
        [](const char *message, const magma::exception::source_location& location)
        {
            std::ostringstream msg;
            if (!location.file_name())
                msg << "Error: " << message;
            else
            {
                msg << location.file_name() << "(" << location.line() << "):" << std::endl
                    << "Error: " << message;
            }
            onError(msg.str(), "Magma");
            abort();
        });
    magma::exception::setErrorHandler(
        [](VkResult result, const char *message, const magma::exception::source_location& location)
        {
            std::ostringstream msg;
            if (!location.file_name())
                msg << magma::helpers::stringize(result) << std::endl << message;
            else
            {
                msg << location.file_name() << "(" << location.line() << "):" << std::endl
                    << std::endl
                    << magma::helpers::stringize(result) << std::endl << message;
            }
            onError(msg.str(), "Vulkan");
            abort();
        });
    std::unique_ptr<IApplication> app = appFactory(entry);
    app->show();
    app->run();
}
#else
void runAppWithExceptionHandling(const AppEntry& entry) try
{
    std::unique_ptr<IApplication> app = appFactory(entry);
    app->show();
    app->run();
}
catch (const magma::exception::ErrorResult& exc)
{
    std::ostringstream msg;
    if (!exc.location().file_name())
        msg << exc.error() << std::endl << exc.what();
    else
    {
        msg << exc.location().file_name() << "(" << exc.location().line() << "):" << std::endl
            << std::endl
            << exc.error() << std::endl
            << exc.what();
    }
    onError(msg.str(), "Vulkan");
}
catch (const magma::exception::ReflectionErrorResult& exc)
{
    std::ostringstream msg;
    if (!exc.location().file_name())
        msg << exc.error() << std::endl << exc.what();
    else
    {
        msg << exc.location().file_name() << "(" << exc.location().line() << "):" << std::endl
            << std::endl
            << exc.error() << std::endl
            << exc.what();
    }
    onError(msg.str(), "SPIRV-Reflect");
}
catch (const magma::exception::NotImplemented& exc)
{
    std::ostringstream msg;
    if (!exc.location().file_name())
        msg << "Error: " << exc.what();
    else
    {
        msg << exc.location().file_name() << "(" << exc.location().line() << "):" << std::endl
            << "Error: " << exc.what();
    }
    onError(msg.str(), "Not implemented");
}
catch (const magma::exception::Exception& exc)
{
    std::ostringstream msg;
    if (!exc.location().file_name())
        msg << "Error: " << exc.what();
    else
    {
        msg << exc.location().file_name() << "(" << exc.location().line() << "):" << std::endl
            << "Error: " << exc.what();
    }
    onError(msg.str(), "Magma");
}
catch (const std::exception& exc)
{
    std::ostringstream msg;
    msg << "Error: " << exc.what() << std::endl;
    onError(msg.str(), "Error");
}
#endif // MAGMA_NO_EXCEPTIONS

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
#ifdef MAGMA_NO_EXCEPTIONS
    runApp(entry);
#else
    runAppWithExceptionHandling(entry);
#endif
    return 0;
}

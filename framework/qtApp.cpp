#include <QApplication>
#include <QWindow>
#include <QMouseEvent>
#include <QTimer>

#include "qtApp.h"
#include "debugOutputStream.h"

class QtWindow : public QWindow
{
public:
    QtWindow(QtApp *app):
        app(app)
    {
        connect(&timer, &QTimer::timeout, this, &QtWindow::onIdle);
        timer.start();
    }

    void paintEvent(QPaintEvent *ev) override
    {
        QWindow::paintEvent(ev);
        app->onPaint();
    }

    void keyPressEvent(QKeyEvent *ev) override
    {
        QWindow::keyPressEvent(ev);
        const char key = app->translateKey(ev->key());
        app->onKeyDown(key, ev->isAutoRepeat(), ev->nativeModifiers());
    }

    void keyReleaseEvent(QKeyEvent *ev) override
    {
        QWindow::keyReleaseEvent(ev);
        const char key = app->translateKey(ev->key());
        app->onKeyUp(key, ev->isAutoRepeat(), ev->nativeModifiers());
    }

    void mouseMoveEvent(QMouseEvent *ev) override
    {
        QWindow::mouseMoveEvent(ev);
        int x = (int)ev->position().x();
        int y = (int)ev->position().y();
        app->onMouseMove(x, y);
    }

    void mousePressEvent(QMouseEvent *ev) override
    {
        QWindow::mousePressEvent(ev);
        int x = (int)ev->position().x();
        int y = (int)ev->position().y();
        switch (ev->button())
        {
        case Qt::MouseButton::LeftButton:
            app->onMouseLButton(true, x, y);
            break;
        case Qt::MouseButton::RightButton:
            app->onMouseRButton(true, x, y);
            break;
        case Qt::MouseButton::MiddleButton:
            app->onMouseMButton(true, x, y);
            break;
        default:
            break;
        }
    }

    void mouseReleaseEvent(QMouseEvent *ev) override
    {
        QWindow::mousePressEvent(ev);
        int x = (int)ev->position().x();
        int y = (int)ev->position().y();
        switch (ev->button())
        {
        case Qt::MouseButton::LeftButton:
            app->onMouseLButton(false, x, y);
            break;
        case Qt::MouseButton::RightButton:
            app->onMouseRButton(false, x, y);
            break;
        case Qt::MouseButton::MiddleButton:
            app->onMouseMButton(false, x, y);
            break;
        default:
            break;
        }
    }

    void wheelEvent(QWheelEvent *ev) override
    {
        QWindow::wheelEvent(ev);
        // Most mouse types work in steps of 15 degrees,
        // in which case the delta value is a multiple of 120
        app->onMouseWheel(ev->angleDelta().y() / 120.f);
    }

    void closeEvent(QCloseEvent *ev) override
    {
        timer.stop();
        QWindow::closeEvent(ev);
    }

private slots:
    void onIdle()
    {
        app->onIdle();
    }

private:
    QtApp *app;
    QTimer timer;

    static DebugOutputStream dostream;
};

QtApp::QtApp(AppEntry entry, const std::tstring& caption, uint32_t width, uint32_t height):
    BaseApp(caption, width, height)
{
    std::cout << "Platform: Qt" << std::endl;
    app = std::make_unique<QApplication>(entry.argc, entry.argv);
    window = std::make_unique<QtWindow>(this);
    window->resize(width, height);
    setWindowCaption(caption);
}

QtApp::~QtApp() {}

void QtApp::setWindowCaption(const std::tstring& caption)
{
#ifdef UNICODE
    window->setTitle(QString::fromStdWString(caption));
#else
    window->setTitle(QString::fromStdString(caption));
#endif
}

void QtApp::show()
{
    window->show();
}

void QtApp::run()
{
    QApplication::exec();
}

void QtApp::close()
{
    BaseApp::close();
    window->close();
}

void QtApp::onKeyDown(char key, int repeat, uint32_t flags)
{
    BaseApp::onKeyDown(key, repeat, flags);
}

void QtApp::onMouseMove(int x, int y)
{
    currPos.x = x;
    currPos.y = y;
    if (mousing)
    {
        spinX += (currPos.x - lastPos.x);
        spinY += (currPos.y - lastPos.y);
    }
    lastPos = currPos;
}

void QtApp::onMouseLButton(bool down, int x, int y)
{
    if (down)
    {
        lastPos.x = currPos.x = x;
        lastPos.y = currPos.y = y;
        mousing = true;
    }
    else
    {
        mousing = false;
    }
}

char QtApp::translateKey(int code) const
{
    switch (code)
    {
    case Qt::Key_Tab: return AppKey::Tab;
    case Qt::Key_Return: return AppKey::Enter;
    case Qt::Key_Escape: return AppKey::Escape;
    case Qt::Key_Left: return AppKey::Left;
    case Qt::Key_Right: return AppKey::Right;
    case Qt::Key_Up: return AppKey::Up;
    case Qt::Key_Down: return AppKey::Down;
    case Qt::Key_Home: return AppKey::Home;
    case Qt::Key_End: return AppKey::End;
    case Qt::Key_PageUp: return AppKey::PgUp;
    case Qt::Key_PageDown: return AppKey::PgDn;
    }
    return (char)code;
}

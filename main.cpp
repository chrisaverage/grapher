#include "inputdialog.h"
#include "graph.h"

#include <windows.h>

#include <QAbstractNativeEventFilter>
#include <QSystemTrayIcon>
#include <QStandardPaths>
#include <QElapsedTimer>
#include <QApplication>
#include <QProcess>
#include <QTimer>
#include <QMenu>
#include <QDir>

constexpr bool ENABLE_LOG = true;

void logActivity()
{
    if (!ENABLE_LOG)
        return;

    static bool recursive_call = false;
    if (recursive_call)
        return;

    recursive_call = true;

    auto date_time = QDateTime::currentDateTime();

    QString result = InputDialog::exec();
    if (!result.isEmpty())
    {
        QString entry = date_time.toString(Qt::ISODate) + "|" + result + "\n";
        QString location = getLogPath();
        QDir(QFileInfo(location).absolutePath()).mkpath(".");

        QFile file(location);
        if (file.open(QIODevice::Append | QIODevice::Text))
        {
            file.write(entry.toUtf8());
        }
    }

    recursive_call = false;
}

void logStartup()
{
    if (!ENABLE_LOG)
        return;

    QString entry = QDateTime::currentDateTime().toString(Qt::ISODate) + "|Startup\n";
    QString location = getLogPath();
    QDir(QFileInfo(location).absolutePath()).mkpath(".");

    QFile file(location);
    if (file.open(QIODevice::Append | QIODevice::Text))
    {
        file.write(entry.toUtf8());
    }
}

void logShutdown()
{
    if (!ENABLE_LOG)
        return;

    QString entry = QDateTime::currentDateTime().toString(Qt::ISODate) + "|Shutdown\n";
    QString location = getLogPath();
    QDir(QFileInfo(location).absolutePath()).mkpath(".");

    QFile file(location);
    if (file.open(QIODevice::Append | QIODevice::Text))
    {
        file.write(entry.toUtf8());
    }
}

void showGraph()
{
    static bool recursive_call = false;
    if (recursive_call)
        return;

    recursive_call = true;

    Graph dlg;
    dlg.exec();

    recursive_call = false;
}

class HotkeyFilter : public QAbstractNativeEventFilter
{
public:
    HotkeyFilter()
    {
        timer.start();
    }

    bool nativeEventFilter(const QByteArray&, void *message, long*) override
    {
        MSG* msg = (MSG*)message;
        if (msg->message == WM_HOTKEY && msg->wParam == 1)
        {
            auto elapsed = timer.restart();
            if(elapsed < 400)
            {
                logActivity();
            }
        }

        return false;
    }

private:
    QElapsedTimer timer;
};

void registerHotkey()
{
    RegisterHotKey(NULL, 1, 0, VK_PAUSE);
}

void unregisterHotkey()
{
    UnregisterHotKey(NULL, 1);
}

void openLogLocation()
{
    QString location = getLogDir();
    QDir(location).mkpath(".");

    QProcess::execute("explorer " + location);
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(!ENABLE_LOG);

    QSystemTrayIcon tray_icon;
    tray_icon.setToolTip(QObject::tr("Life Tracker"));
    tray_icon.setIcon(QIcon(":/res/recording.png"));

    QMenu tray_menu;
    tray_icon.setContextMenu(&tray_menu);
    tray_icon.show();

    tray_menu.addAction(QObject::tr("Show graph"), &a, &showGraph);
    tray_menu.addAction(QObject::tr("Open log location"), &a, &openLogLocation);
    tray_menu.addSeparator();
    tray_menu.addAction(QObject::tr("Exit"), &a, &QApplication::quit);

    HotkeyFilter filter;

    qApp->installNativeEventFilter(&filter);

    registerHotkey();
    logStartup();

    QObject::connect(&a, &QApplication::aboutToQuit, &unregisterHotkey);
    QObject::connect(&a, &QApplication::aboutToQuit, &logShutdown);


    if (!ENABLE_LOG)
        QTimer::singleShot(100, &showGraph);

    return a.exec();
}

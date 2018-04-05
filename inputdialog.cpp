#include "inputdialog.h"

#include <QEventLoop>
#include <QKeyEvent>
#include <QIcon>

InputDialog::InputDialog(QEventLoop* el) : event_loop(el)
{
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowStaysOnTopHint);
    setFixedSize(250,20);
    setWindowIcon(QIcon(":/res/recording.png"));
    setWindowTitle(tr("Enter activity:"));

    timer_id = startTimer(3000);
}


void InputDialog::keyPressEvent(QKeyEvent* evt)
{
    if (timer_id >= 0)
    {
        killTimer(timer_id);
        timer_id = -1;
    }

    if (evt->key() == Qt::Key_Escape && evt->modifiers() == Qt::NoModifier)
    {
        clear();
        close();

        if (event_loop)
            event_loop->quit();
    }

    if ((evt->key() == Qt::Key_Enter || evt->key() == Qt::Key_Return) && evt->modifiers() == Qt::NoModifier)
    {
        if (text().isEmpty())
            setText("Work");

        close();

        if (event_loop)
            event_loop->quit();
    }

    return QLineEdit::keyPressEvent(evt);
}


void InputDialog::timerEvent(QTimerEvent*)
{
    killTimer(timer_id);
    timer_id = -1;

    setText("Work");
    close();

    if (event_loop)
        event_loop->quit();
}


QString InputDialog::exec()
{
    QEventLoop el;

    InputDialog dialog(&el);
    dialog.show();
    dialog.activateWindow();
    dialog.raise();

    el.exec();

    return dialog.text();
}



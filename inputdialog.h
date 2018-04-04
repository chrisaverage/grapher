#pragma once
#include <QLineEdit>

class QEventLoop;

class InputDialog : QLineEdit
{
public:
    static QString exec();

protected:
    void keyPressEvent(QKeyEvent* evt) override;
    void timerEvent(QTimerEvent*) override;

private:
    InputDialog(QEventLoop* el);

    QEventLoop* event_loop = nullptr;
    int timer_id = -1;
};

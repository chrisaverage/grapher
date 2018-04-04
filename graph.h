#pragma once
#include <QDateTime>
#include <QDialog>
#include <QMap>

class QTreeWidget;

QString getLogPath();
QString getLogDir();

class Graph : public QDialog
{
    Q_OBJECT

public:
    Graph();

protected:
    void paintEvent(QPaintEvent* evt) override;

private:
    void readEntries();
    void processEntries();

    struct Entry
    {
        QDateTime date_time;
        QString name;
    };

    struct DailyStat
    {
        QTime total_time     = QTime(0, 0, 0, 0);
        QTime work_time      = QTime(0, 0, 0, 0);
        QTime longest_streak = QTime(0, 0, 0, 0);
    };

    QTime min_time = QTime(23, 59, 59, 999);
    QTime max_time = QTime(0, 0, 0, 0);
    QVector<QVector<Entry>> entries;
    QVector<DailyStat> daily_stats;
    QMap<QString, int> total_stats;
    int total_time = 0;

    QWidget* side_panel = nullptr;
    QTreeWidget* entries_tree = nullptr;
};

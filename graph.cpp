#include "graph.h"
#include <QFile>
#include <QIcon>
#include <QPainter>
#include <QStandardPaths>
#include <QTextOption>
#include <QBoxLayout>
#include <QLabel>
#include <QTreeWidget>
#include <QHeaderView>
#include <QMapIterator>

#include <QDebug>
#include <QItemSelectionModel>

QString getLogPath()
{
    return getLogDir() + "/log.txt";
}

QString getLogDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

QString fixupName(const QString& name)
{
    QString result = name.toLower();

    if (result == "shutdown" || result == "work")
        result = "planned";

    return result;
}

QString secToStr(int secs)
{
    int s = secs % 60;
    int m = (secs / 60) % 60;
    int h = secs / 3600;

    return QString("%1:%2:%3")
            .arg(QString::number(h).rightJustified(2, '0'))
            .arg(QString::number(m).rightJustified(2, '0'))
            .arg(QString::number(s).rightJustified(2, '0'));
}

Graph::Graph()
{
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setWindowIcon(QIcon(":/res/recording.png"));
    setWindowTitle(tr("Activity graph"));
    setMinimumSize(800, 600);

    auto side_label = new QLabel(tr("Activities:"));

    entries_tree = new QTreeWidget();
    entries_tree->header()->hide();
    entries_tree->setColumnCount(2);
    entries_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    entries_tree->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    entries_tree->header()->setStretchLastSection(false);
    entries_tree->setColumnWidth(1, 50);
    entries_tree->setIndentation(0);
    entries_tree->setFrameStyle(QFrame::NoFrame);

    auto side_layout = new QVBoxLayout();
    side_layout->setContentsMargins(2, 24, 2, 2);
    side_layout->addWidget(side_label);
    side_layout->addWidget(entries_tree);

    side_panel = new QWidget();
    side_panel->setFixedWidth(250);
    side_panel->setLayout(side_layout);

    auto main_layout = new QHBoxLayout();
    main_layout->setContentsMargins(0,0,0,0);
    main_layout->addStretch();
    main_layout->addWidget(side_panel);
    setLayout(main_layout);

    readEntries();
    processEntries();

    setStyleSheet("color: gray");

    connect(entries_tree->selectionModel(), &QItemSelectionModel::selectionChanged, this, [&]{ update(); });
}

void Graph::paintEvent(QPaintEvent* evt)
{
    QDialog::paintEvent(evt);

    QMargins marg(60, 40, 10 + side_panel->width(), 90);

    QPainter p(this);
    p.fillRect(rect(), Qt::white);

    QRect g_rect = rect().adjusted(marg.left(), marg.top(), -marg.right(), -marg.bottom());

    int font_height = fontMetrics().height();
    int font_height_2 = font_height / 2;
    int min_h = min_time.hour();
    int max_h = (max_time.minute() != 0) ? 24 : max_time.hour();
    int h_diff = max_h - min_h;


    const auto selected_items = entries_tree->selectedItems();
    QString selected_text = selected_items.isEmpty() ? QString() : fixupName(selected_items.first()->text(0));

    // Stat labels *************************************************************
    p.setPen(Qt::gray);
    p.drawText(25, height() - marg.bottom() + 30, "Total:");
    p.drawText(25, height() - marg.bottom() + 50, "Planned:");
    p.drawText(25, height() - marg.bottom() + 70, "Streak:");

    // Hour texts *************************************************************
    int hour_step = ((g_rect.height() / h_diff) > (font_height_2 * 3)) ? 1 : 2;

    p.setPen(Qt::gray);
    p.drawText(25, g_rect.bottom() + font_height_2, QString::number(max_h).rightJustified(2, '0') + ":00");

    for (int i = 0; i < h_diff; i += hour_step)
    {
        int h = i * g_rect.height() / h_diff;
        p.drawText(25, marg.top() + font_height_2 + h, QString::number(min_h + i).rightJustified(2, '0') + ":00");
    }

    if (entries.isEmpty())
        return;

    // Hour lines *************************************************************
    p.setPen(QColor(Qt::lightGray).lighter(120));
    for (int i = 1; i < h_diff; ++i)
    {
        int h = i * g_rect.height() / h_diff;
        p.drawLine(g_rect.left(), g_rect.top() + h,  g_rect.right(), g_rect.top() + h);
    }

    // Activities *************************************************************
    int day_w = qBound(30, g_rect.width() / entries.size(), 180);
    QTextOption opt;
    opt.setAlignment(Qt::AlignCenter);
    opt.setWrapMode(QTextOption::NoWrap);

    for (int i = 0; i < entries.size(); ++ i)
    {
        p.setPen(Qt::gray);
        auto& day = entries.at(i);
        QString day_text = day.front().date_time.toString("dd.MM");
        QRect day_text_rect(marg.left() + 2 + day_w * i, g_rect.top() - 20, day_w, 20);
        p.drawText(day_text_rect, day_text, opt);

        p.setPen(Qt::gray);
        QRect day_rect(marg.left() + 2 + day_w * i, marg.top(), day_w, g_rect.height());

        int sec_min_max = min_time.secsTo(max_time);
        int prev = min_time.secsTo(day.front().date_time.time());
        for (int j = 1; j < day.size(); ++j)
        {
            int curr = min_time.secsTo(day.at(j).date_time.time());

            if (day.at(j-1).name == "Shutdown")
            {
                prev= curr;
                continue;
            }

            int start_off = day_rect.top() + (day_rect.height() * prev / sec_min_max);
            int end_off   = day_rect.top() + (day_rect.height() * curr / sec_min_max);

            QRect activity_rect(day_rect.left() + 5,  start_off, day_rect.width() - 7, end_off - start_off);

            QString name = fixupName(day.at(j).name);

            QColor fill_color = QColor::fromRgb(214, 229, 255);

            if (selected_text == name)
            {
                fill_color = Qt::red;

                //p.fillRect(activity_rect, QColor::fromRgb(66, 134, 244));
                //p.setPen(QColor::fromRgb(66, 134, 244).darker(120));
            }
            else if (name == "planned")
            {
                fill_color = QColor::fromRgb(66, 134, 244);

                //p.fillRect(activity_rect, QColor::fromRgb(66, 134, 244));
                //p.setPen(QColor::fromRgb(66, 134, 244).darker(120));
            }

            p.setPen(fill_color.darker(120));
            p.fillRect(activity_rect, fill_color);
            p.drawLine(activity_rect.topLeft(), activity_rect.topRight());

            prev = curr;
        }

    }

    // Daily stats ****************************************************************
    p.setPen(Qt::gray);
    for (int i = 0; i < daily_stats.size(); ++i)
    {
        QRect day_rect(marg.left() + 2 + day_w * i, height() - marg.bottom() + 30 - font_height, day_w, font_height + 4);
        p.drawText(day_rect, daily_stats.at(i).total_time.toString("HH:mm:ss"), opt);

        day_rect.translate(0, 20);
        p.drawText(day_rect, daily_stats.at(i).work_time.toString("HH:mm:ss"), opt);

        day_rect.translate(0, 20);
        p.drawText(day_rect, daily_stats.at(i).longest_streak.toString("HH:mm:ss"), opt);
    }

    // Box ************************************************************************
    p.drawRect(g_rect);
}

void Graph::readEntries()
{
    QFile file(getLogPath());
    if (!file.open(QFile::ReadOnly | QFile::Text))
        return;

    while(!file.atEnd())
    {
        QStringList parts = QString::fromUtf8(file.readLine()).split('|');
        if (parts.size() == 2)
        {
            QDateTime dt = QDateTime::fromString(parts.front(), Qt::ISODate);
            QString name = parts.back().trimmed();

            if (dt.time() < min_time)
                min_time = dt.time();

            if (dt.time() > max_time)
                max_time = dt.time();

            if (entries.isEmpty())
            {
                entries.push_back(QVector<Entry>());
            }

            const QVector<Entry>& day = entries.back();
            if (!day.isEmpty() && day.back().date_time.date() != dt.date())
            {
                entries.push_back(QVector<Entry>());
            }

            Entry e { dt, name };
            entries.back().push_back(e);
        }
    }

    min_time = QTime(min_time.hour(), 0, 0);

    if (max_time.hour() == 23)
        max_time = QTime(23, 59, 59, 999);
    else
        max_time = QTime(max_time.hour() + 1, 0, 0);
}

void Graph::processEntries()
{
    daily_stats.resize(entries.size());

    for (int i = 0; i < entries.size(); ++i)
    {
        const auto& ent = entries.at(i);

        if (ent.isEmpty())
            continue;

        int longest = 0;

        for (int j = 1; j < ent.size(); ++j)
        {
            const auto& prev = ent.at(j - 1);
            const auto& curr = ent.at(j);

            if (prev.name == "Shutdown")
                continue;

            int secs = (int)prev.date_time.secsTo(curr.date_time);
            total_time += secs;

            QString task_name = fixupName(curr.name);

            total_stats[task_name] += secs;

            daily_stats[i].total_time = daily_stats[i].total_time.addSecs(secs);

            if (task_name == "planned")
            {
                daily_stats[i].work_time = daily_stats[i].work_time.addSecs(secs);

                if (secs > longest)
                {
                    QTime t(0, 0, 0, 0);
                    t = t.addSecs(secs);
                    daily_stats[i].longest_streak = t;
                    longest = secs;
                }
            }
        }
    }

    QList <QTreeWidgetItem*> items;
    items << new QTreeWidgetItem(QStringList() << "Total" << secToStr(total_time));
    items.last()->setData(1, Qt::TextAlignmentRole, (int)(Qt::AlignVCenter | Qt::AlignRight));

    QMapIterator<QString, int> it(total_stats);
    while(it.hasNext())
    {
        it.next();

        QString name = it.key().at(0).toUpper() + it.key().right(it.key().size() - 1);
        items << new QTreeWidgetItem(QStringList() << name << secToStr(it.value()));
        items.last()->setData(0, Qt::ToolTipRole, items.last()->text(0));
        items.last()->setData(1, Qt::TextAlignmentRole, (int)(Qt::AlignVCenter | Qt::AlignRight));
    }

    entries_tree->addTopLevelItems(items);
    entries_tree->sortItems(1, Qt::DescendingOrder);
}




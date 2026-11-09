#include <QTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include "InstanceList.h"
#include "BaseInstance.h"
#include "settings/INISettingsObject.h"

class InstanceRunningStateTest : public QObject
{
    Q_OBJECT
private slots:
    void runningState_data()
    {
        QTest::addColumn<bool>("recordPlaytime");
        QTest::newRow("recording-disabled") << false;
        QTest::newRow("recorded-before-post-exit") << true;
    }
    void runningState()
    {
        QFETCH(bool, recordPlaytime);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        auto global = std::make_shared<INISettingsObject>(temp.path() + "/global.cfg");
        for (const auto &key : {"PreLaunchCommand", "WrapperCommand", "PostExitCommand"})
            global->registerSetting(key, "");
        for (const auto &key : {"ShowConsole", "AutoCloseConsole", "ShowConsoleOnError", "LogPrePostOutput", "ConsoleOverflowStop"})
            global->registerSetting(key, false);
        global->registerSetting("ConsoleMaxLines", 1000);
        const auto dir = temp.path() + "/instances";
        QVERIFY(QDir().mkpath(dir + "/test"));
        QFile config(dir + "/test/instance.cfg");
        QVERIFY(config.open(QIODevice::WriteOnly));
        config.write("InstanceType=Unknown\nname=Test\n");
        config.close();
        InstanceList list(global, dir);
        QCOMPARE(list.loadList(), InstanceList::NoError);
        QCOMPARE(list.count(), 1);
        auto instance = list.at(0);
        instance->settings()->registerSetting("RecordGameTime", recordPlaytime);
        QSignalSpy changes(&list, &InstanceList::dataChanged);
        instance->setRunning(true);
        QVERIFY(instance->isRunning());
        QCOMPARE(changes.count(), 1);
        QCOMPARE(qvariant_cast<QModelIndex>(changes.takeFirst().at(0)), list.index(0));
        if (recordPlaytime)
        {
            instance->recordTimePlayed();
            changes.clear();
        }
        instance->setRunning(false);
        QVERIFY(!instance->isRunning());
        QCOMPARE(changes.count(), 1);
        QCOMPARE(qvariant_cast<QModelIndex>(changes.takeFirst().at(0)), list.index(0));
        instance->setRunning(false);
        QCOMPARE(changes.count(), 0);
    }
};
QTEST_GUILESS_MAIN(InstanceRunningStateTest)
#include "InstanceRunningState_test.moc"

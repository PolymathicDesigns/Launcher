#include <QTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QFile>
#include "InstanceList.h"
#include "settings/INISettingsObject.h"

class InstanceGroupsTest : public QObject
{
    Q_OBJECT
    bool write(const QString &path, const QByteArray &data)
    {
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
    }
private slots:
    void reloadExistingInstanceAssignment()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        auto settings = std::make_shared<INISettingsObject>(temp.path() + "/settings.cfg");
        for (const auto &key : {"PreLaunchCommand", "WrapperCommand", "PostExitCommand"})
            settings->registerSetting(key, "");
        for (const auto &key : {"ShowConsole", "AutoCloseConsole", "ShowConsoleOnError", "LogPrePostOutput", "ConsoleOverflowStop"})
            settings->registerSetting(key, false);
        settings->registerSetting("ConsoleMaxLines", 1000);
        const auto dir = temp.path() + "/instances";
        QVERIFY(QDir().mkpath(dir + "/test"));
        QVERIFY(write(dir + "/test/instance.cfg", "InstanceType=Unknown\nname=Test\n"));
        const auto path = dir + "/instgroups.json";
        QVERIFY(write(path, R"({"formatVersion":1,"groups":{"Old":{"instances":["test"]}}})"));
        InstanceList list(settings, dir);
        QCOMPARE(list.loadList(), InstanceList::NoError);
        QCOMPARE(list.count(), 1);
        QCOMPARE(list.data(list.index(0), InstanceList::GroupRole).toString(), QString("Old"));
        QSignalSpy changed(&list, &InstanceList::dataChanged);
        QVERIFY(write(path, R"({"formatVersion":1,"groups":{"New":{"instances":["test"]}}})"));
        list.loadList();
        list.reloadGroups();
        QCOMPARE(list.data(list.index(0), InstanceList::GroupRole).toString(), QString("New"));
        QCOMPARE(changed.count(), 1);
        QCOMPARE(qvariant_cast<QVector<int>>(changed.at(0).at(2)), QVector<int>{InstanceList::GroupRole});
    }
    void reloadGroups()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        auto settings = std::make_shared<INISettingsObject>(temp.path() + "/settings.cfg");
        InstanceList list(settings, temp.path() + "/instances");
        QDir().mkpath(temp.path() + "/instances");
        const auto path = temp.path() + "/instances/instgroups.json";
        QVERIFY(write(path, R"({"formatVersion":1,"groups":{"Old":{"hidden":true,"instances":[]}}})"));
        list.reloadGroups();
        QCOMPARE(list.getGroups(), QStringList{"Old"});
        QVERIFY(list.isGroupCollapsed("Old"));
        QSignalSpy changed(&list, &InstanceList::groupsChanged);
        QVERIFY(write(path, R"({"formatVersion":1,"groups":{"New":{"hidden":false,"instances":[]}}})"));
        list.reloadGroups();
        QCOMPARE(list.getGroups(), QStringList{"New"});
        QVERIFY(!list.isGroupCollapsed("Old"));
        QVERIFY(!list.isGroupCollapsed("New"));
        QCOMPARE(changed.count(), 1);
        QVERIFY(write(path, "{"));
        list.reloadGroups();
        QCOMPARE(list.getGroups(), QStringList{"New"});
        QVERIFY(QFile::remove(path));
        list.reloadGroups();
        QVERIFY(list.getGroups().isEmpty());
    }
};
QTEST_GUILESS_MAIN(InstanceGroupsTest)
#include "InstanceGroups_test.moc"

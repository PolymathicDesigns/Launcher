#include <QTest>
#include <QTemporaryDir>
#include <QFile>
#include <QMimeData>
#include <QUrl>
#include "GZip.h"
#include "WorldList.h"

class WorldListTest : public QObject
{
    Q_OBJECT
    bool createWorld(const QString &path)
    {
        if (!QDir().mkpath(path)) return false;
        QByteArray compressed;
        if (!GZip::zip(QByteArray::fromHex("0a00000a0004446174610800094c6576656c4e616d650004546573740000"), compressed)) return false;
        QFile file(path + "/level.dat");
        return file.open(QIODevice::WriteOnly) && file.write(compressed) == compressed.size();
    }
private slots:
    void rejectMove_data()
    {
        QTest::addColumn<bool>("includeWorld");
        QTest::addColumn<bool>("includeUnsupportedFile");
        QTest::newRow("unsupported") << false << true;
        QTest::newRow("world") << true << false;
        QTest::newRow("mixed") << true << true;
    }
    void rejectMove()
    {
        QFETCH(bool, includeWorld);
        QFETCH(bool, includeUnsupportedFile);
        QTemporaryDir source, destination;
        QVERIFY(source.isValid() && destination.isValid());
        QVERIFY(createWorld(source.path() + "/world"));
        QFile notes(source.path() + "/notes.txt");
        QVERIFY(notes.open(QIODevice::WriteOnly));
        notes.write("keep me");
        notes.close();
        QList<QUrl> urls;
        if (includeWorld) urls << QUrl::fromLocalFile(source.path() + "/world");
        if (includeUnsupportedFile) urls << QUrl::fromLocalFile(notes.fileName());
        QMimeData data;
        data.setUrls(urls);
        WorldList worlds(destination.path());
        QVERIFY(!worlds.dropMimeData(&data, Qt::MoveAction, -1, -1, QModelIndex()));
        QVERIFY(QFile::exists(source.path() + "/world/level.dat"));
        QVERIFY(QFile::exists(notes.fileName()));
        QVERIFY(QDir(destination.path()).entryList(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty());
    }
    void copyWorld()
    {
        QTemporaryDir source, destination;
        QVERIFY(source.isValid() && destination.isValid());
        QVERIFY(createWorld(source.path() + "/world"));
        QMimeData data;
        data.setUrls({QUrl::fromLocalFile(source.path() + "/world")});
        WorldList worlds(destination.path());
        QVERIFY(worlds.dropMimeData(&data, Qt::CopyAction, -1, -1, QModelIndex()));
        QVERIFY(QFile::exists(source.path() + "/world/level.dat"));
        QVERIFY(QFile::exists(destination.path() + "/Test/level.dat"));
        QCOMPARE(worlds.supportedDropActions(), Qt::DropActions(Qt::CopyAction));
    }
};
QTEST_GUILESS_MAIN(WorldListTest)
#include "WorldList_test.moc"

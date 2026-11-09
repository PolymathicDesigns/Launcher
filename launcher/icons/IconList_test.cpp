#include <QTest>
#include <QTemporaryDir>
#include <QImage>
#include <QFile>
#include "IconList.h"

class IconListTest : public QObject
{
    Q_OBJECT
private slots:
    void customDirectory_data()
    {
        QTest::addColumn<bool>("singleIcon");
        QTest::newRow("add-icons") << false;
        QTest::newRow("import-named-icon") << true;
    }
    void customDirectory()
    {
        QFETCH(bool, singleIcon);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QImage image(16, 16, QImage::Format_ARGB32);
        image.fill(Qt::red);
        const auto source = temp.path() + "/source.png";
        QVERIFY(image.save(source));
        IconList icons({}, temp.path() + "/initial/icons");
        icons.directoryChanged(temp.path() + "/selected/icons");
        const auto name = singleIcon ? "imported.png" : "source.png";
        if (singleIcon) icons.installIcon(source, name);
        else icons.installIcons({source});
        const auto target = temp.path() + "/selected/icons/" + name;
        QVERIFY(QFile::exists(target));
        QVERIFY(!QFile::exists(temp.path() + "/initial/icons/" + name));
        QCOMPARE(QImage(target), image);
    }
};
QTEST_MAIN(IconListTest)
#include "IconList_test.moc"

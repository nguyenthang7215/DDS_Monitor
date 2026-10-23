// Qt Charts in HostWorkspace requires QApplication, not the QGuiApplication
// used by the stock qmltestrunner. Link Qt6QuickTest, Qt6Widgets and Qt6Core.
// Run from the repository root:
// QT_QPA_PLATFORM=offscreen QSG_RHI_BACKEND=software <runner>
//     -input task3/tests/qml -import imports
#include <QApplication>
#include <QtQuickTest/quicktest.h>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    return quick_test_main(argc, argv, "NetworkScroll", nullptr);
}

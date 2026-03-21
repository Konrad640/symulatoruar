#include <QApplication>
#include "MainWindow.h"
#include "TestRunner.h"

// Ustaw na true, aby uruchomić testy w konsoli.
// Ustaw na false, aby uruchomić GUI.
#define RUN_TESTS_ONLY false

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    if (RUN_TESTS_ONLY) {
        TestRunner::runAllTests();

        return 0;
    } else {
        MainWindow w;
        w.setWindowTitle("Symulator UAR - Model ARX + PID");
        w.resize(1600, 1000);
        w.show();
        return a.exec();
    }
}

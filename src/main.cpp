#include <QApplication>
#include "widgets/MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("RacecarTools");

    MainWindow win;
    win.resize(1200, 800);
    win.show();

    return app.exec();
}

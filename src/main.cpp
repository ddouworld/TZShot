#include <QApplication>

#include "app/tzshot_app.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    TZShotApp tzshotApp(app);
    if (!tzshotApp.initialize()) {
        return 0;
    }

    return app.exec();
}

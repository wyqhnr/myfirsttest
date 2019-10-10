#include "widget.h"
#include <QApplication>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QString locale = QLocale::system().name();
    QTranslator translator;
    QString qmfile = QString(WORKING_DIRECTORY"/%1.qm").arg(locale);
    translator.load(qmfile);
    a.installTranslator(&translator);
    a.setWindowIcon(QIcon(":/pro.png"));
    Widget w;
    w.show();

    return a.exec();
}

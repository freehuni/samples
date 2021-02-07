#include <QtGui/QApplication>
#include <gst/gst.h>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    int retApp;
#if !GLIB_CHECK_VERSION(2,35,0)
    g_type_init ();
#endif

    QApplication a(argc, argv);

    gst_init (&argc, &argv);

    MainWindow w;

    w.show();
    
    retApp = a.exec();

    gst_deinit();
}

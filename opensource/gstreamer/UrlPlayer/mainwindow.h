#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>
#include <qtimer.h>

namespace Ui {
class MainWindow;
}


class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private slots:
    void on_btnPlay_clicked();

    void on_btnStop_clicked();

    void on_btnPause_clicked();

    void on_ctrlVolume_sliderMoved(int position);

    void on_MainWindow_destroyed();

    void on_timer();

private:
    QTimer* m_timer;
public:
    Ui::MainWindow *ui; /*!< Detailed description after the member */
    GstElement* pipeline;/*!< pipeline */
    GstElement* playbin2;/*!< playbin2s */

    static gboolean  call_RawGstBusMessage (GstBus *bus, GstMessage *msg, gpointer data);
    gboolean call_GstBusMessage(GstBus *bus, GstMessage *msg);
};

#endif // MAINWINDOW_H

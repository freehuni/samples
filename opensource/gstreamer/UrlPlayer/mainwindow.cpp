#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    pipeline = NULL;
    playbin2 = NULL;
    m_timer = NULL;
}

MainWindow::~MainWindow()
{

    delete ui;
}

gboolean  MainWindow::call_RawGstBusMessage (GstBus *bus, GstMessage *msg, gpointer data)
{
    MainWindow* mainWnd;

    mainWnd = static_cast<MainWindow*>(data);

    return mainWnd->call_GstBusMessage(bus, msg);
}


/*! doxygen test
*  call_gstBusMessage
\param bus GstBus*
\param msg GstMessage*
\retval success 성공
\retval fail  실패
*/
gboolean MainWindow::call_GstBusMessage(GstBus *bus, GstMessage *msg)
{
     g_print("message type:%s\n",GST_MESSAGE_TYPE_NAME(msg));

     const GstStructure *s;
     GstXOverlay *ov = NULL;
     gint64 duration,len;
     GstFormat format = GST_FORMAT_TIME;
     QString str;

     s = gst_message_get_structure (msg);
     if (s == NULL || gst_structure_has_name (s, "prepare-xwindow-id") == true)
     {
         // overlay window 설정
         ov = GST_X_OVERLAY (GST_MESSAGE_SRC (msg));

         WId xwinid = ui->frmPlayer->winId();
         QApplication::syncX();
         gst_x_overlay_set_window_handle(ov, ui->frmPlayer->winId());         
     }

     switch (GST_MESSAGE_TYPE (msg))
     {
         case GST_MESSAGE_DURATION:
            gst_message_parse_duration (msg, &format, &duration);
            if (format != GST_FORMAT_TIME)
                break;

            // 재생 시작시 duration 정보 1회 수신된다.
            g_print ("dur: %" GST_TIME_FORMAT "\n", GST_TIME_ARGS (duration));
            break;

         case GST_MESSAGE_EOS:
           g_print ("End of stream\n");

           if (m_timer != NULL)
           {
               m_timer->stop();
               delete m_timer;
               m_timer = NULL;
           }

           if (pipeline != NULL)
           {
               gst_element_set_state (pipeline, GST_STATE_NULL);
               gst_object_unref (pipeline);
           }

           break;

        case GST_MESSAGE_STATE_CHANGED:
            {
                GstState old, newState, pending;
                gst_message_parse_state_changed (msg, &old, &newState, &pending);
                if (newState == GST_STATE_PLAYING)
                {
                    printf("Status:GST_STATE_PLAYING~\n");
                    gst_element_query_duration (pipeline, &format, &len);
                    ui->lblDuration->setText(str.sprintf("%02u:%02u:%02u.%03u", GST_TIME_ARGS(len)));

                    if (m_timer == NULL)
                    {
                        m_timer = new QTimer(this);
                        connect(m_timer, SIGNAL(timeout()), this, SLOT(on_timer()));
                        m_timer->setInterval(500);
                        m_timer->start(1000);
                    }
                }
                else
                {
                    printf("Status:%d~\n", newState);
                }
            }
         break;

         case GST_MESSAGE_ERROR: {
           gchar  *debug;
           GError *error;

           gst_message_parse_error (msg, &error, &debug);
           g_free (debug);

           g_printerr ("Error: %s\n", error->message);
           g_error_free (error);

           break;
         }
         default:
            printf("Status: MsgType:%d\n", GST_MESSAGE_TYPE (msg));
           break;
     }

    return true;
}

void MainWindow::on_btnPlay_clicked()
{
    GstState state, pendingState;
    GstBus* bus;    

    if (pipeline == NULL)
    {
        pipeline = gst_pipeline_new("my playbin2");

        bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
        if (bus == NULL)
        {
            g_print("cannot create bus\n");
            return;
        }

        gst_bus_add_watch(bus, call_RawGstBusMessage, this);

        gst_object_unref(bus);

        playbin2 = gst_element_factory_make("playbin", "source");
        if (playbin2 == NULL)
        {
            g_print("cannot create playbin2\n");
            return;
        }
        gst_bin_add_many(GST_BIN(pipeline), playbin2, NULL);

        printf("url:%s\n", ui->editURL->text().toUtf8().data());

        g_object_set(G_OBJECT(playbin2),"uri",ui->editURL->text().toUtf8().data(), NULL);

        gst_element_set_state(pipeline, GST_STATE_READY);
        gst_element_set_state(pipeline, GST_STATE_PLAYING);

    }
    else
    {
        gst_element_get_state(pipeline, &state, NULL, GST_CLOCK_TIME_NONE);
        if (state == GST_STATE_PAUSED)
        {
            gst_element_set_state(pipeline, GST_STATE_PLAYING);
        }
    }



}

void MainWindow::on_btnStop_clicked()
{
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref (pipeline);
    pipeline = NULL;
}

void MainWindow::on_btnPause_clicked()
{
    gst_element_set_state(pipeline, GST_STATE_PAUSED);
}


void MainWindow::on_ctrlVolume_sliderMoved(int position)
{
    gdouble volume = position;

    if (playbin2 != NULL)
    {
        g_object_set(playbin2, "volume", volume, NULL);
    }
}

void MainWindow::on_MainWindow_destroyed()
{

}

void MainWindow::on_timer()
{
    gint64 pos = 0;
    GstFormat format = GST_FORMAT_TIME;
    QString str;
    static int idx = 0;

    gst_element_query_position (pipeline, &format, &pos);

    g_print("on timer(cnt:%d, pos:%lld)\n", idx++, pos);

    ui->lblPosition->setText(str.sprintf("%02u:%02u:%02u.%03u", GST_TIME_ARGS(pos)));
}

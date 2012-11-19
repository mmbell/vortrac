#ifndef BATCHWINDOW_H
#define BATCHWINDOW_H

#include <QMainWindow>
#include "DriverBatch.h"
#include "IO/Message.h"

class BatchWindow : public QMainWindow
{
    Q_OBJECT

public:
    BatchWindow(QWidget *parent, QDomDocument& xmlfile);
    ~BatchWindow();

public slots:
    void catchLog(const Message& message);

signals:
    void log(const Message& message);

protected:
    DriverBatch *driver;


};

#endif // BATCHWINDOW_H

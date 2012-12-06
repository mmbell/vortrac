#ifndef BATCHWINDOW_H
#define BATCHWINDOW_H

#include <QMainWindow>
#include "DriverBatch.h"
#include "IO/Message.h"

class BatchWindow : public QMainWindow
{
    Q_OBJECT

public:
    BatchWindow(QWidget *parent, const QString &fileName);
    ~BatchWindow();

public slots:
    void catchLog(const Message& message);
    void closeWindow();

signals:
    void log(const Message& message);
    void finished();

protected:
    DriverBatch *driver;


};

#endif // BATCHWINDOW_H

#ifndef BATCHWINDOW_H
#define BATCHWINDOW_H

#include <QMainWindow>
#include "DriverBatch.h"


class BatchWindow : public QMainWindow
{
    Q_OBJECT

public:
    BatchWindow(QWidget *parent, QDomDocument& xmlfile);
    ~BatchWindow();
protected:
    DriverBatch *driver;

};

#endif // BATCHWINDOW_H

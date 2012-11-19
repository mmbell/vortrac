#include "BatchWindow.h"

BatchWindow::BatchWindow(QWidget *parent, const QString &fileName)
    : QMainWindow(parent)
{
    this->setObjectName("Batch Window");

    qRegisterMetaType<Message>("Message");
    qRegisterMetaType<GriddedData>("GriddedData");
    qRegisterMetaType<VortexList>("VortexList");

    std::cout << "Starting main window ... \n";

    driver = new DriverBatch(this, fileName);

    driver->initialize();
    driver->run();
    driver->finalize();

}

BatchWindow::~BatchWindow()
{
    delete driver;
}

void BatchWindow::catchLog(const Message& message)
{
    emit log(message);
}

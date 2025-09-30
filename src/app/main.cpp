#include "mainwindow.h"
#include "scannerwaitdialog.h"

#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Set application properties
    a.setApplicationName("Korova Film Scanner");
    a.setApplicationVersion("1.0");
    a.setOrganizationName("Soke");

    // Create and show the scanner wait dialog
    ScannerWaitDialog waitDialog;

    // Connect signals
    QObject::connect(&waitDialog,
                     &ScannerWaitDialog::scannerDetected,
                     [&waitDialog, &a]()
                     {
                         // Scanner found, create and show main window
                         MainWindow *mainWindow = new MainWindow();
                         mainWindow->show();
                         waitDialog.close();

                         // Connect main window close to application quit
                         QObject::connect(
                             mainWindow, &MainWindow::destroyed, &a, &QApplication::quit);
                     });

    QObject::connect(&waitDialog,
                     &ScannerWaitDialog::dialogCancelled,
                     [&a]()
                     {
                         // User cancelled, exit application
                         a.quit();
                     });

    waitDialog.show();

    return a.exec();
}

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

    try
    {
        // Create and show the scanner wait dialog
        ScannerWaitDialog waitDialog;

        // Connect signals
        QObject::connect(&waitDialog,
                         &ScannerWaitDialog::scannerDetected,
                         [&waitDialog, &a]()
                         {
                             // Scanner found, create and show main window
                             std::cout << "Creating MainWindow..." << std::endl;
                             MainWindow *mainWindow = new MainWindow();
                             std::cout << "MainWindow created successfully" << std::endl;
                             mainWindow->show();
                             std::cout << "MainWindow shown successfully" << std::endl;
                             waitDialog.close();
                             std::cout << "Wait dialog closed successfully" << std::endl;

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
    catch (const std::exception &e)
    {
        QMessageBox::critical(nullptr, "Error", QString("Application error: %1").arg(e.what()));
        return 1;
    }
    catch (...)
    {
        QMessageBox::critical(nullptr, "Error", "Unknown application error");
        return 1;
    }
}

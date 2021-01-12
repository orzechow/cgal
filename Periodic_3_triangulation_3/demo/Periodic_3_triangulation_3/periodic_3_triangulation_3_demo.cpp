#include "MainWindow.h"

#include <QApplication>
#include <CGAL/Qt/Context_initialization.h>

int main(int argc, char *argv[])
{
  init_ogl_context(2,1);
  QApplication a(argc, argv);
  MainWindow w;
  //w.ui->setupUi(w);
  w.ui->viewer->restoreStateFromFile();

  w.show();

  a.connect(&a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()));
  a.connect(w.ui->actionExit, SIGNAL(triggered()), &a, SLOT(quit()));

  return a.exec();
}

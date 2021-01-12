#include "Viewer.h"
#include <qapplication.h>
#include <CGAL/Qt/Context_initialization.h>
int main(int argc, char** argv)
{
  CGAL::init_ogl_context(4,3);
  QApplication application(argc,argv);
  // Instantiate the viewer.
  Viewer viewer;
  viewer.setWindowTitle("Intersection points of randomly generated circles.");

  // Make the viewer window visible on screen.
  viewer.show();

  // Run main loop.
  return application.exec();
}

#include "rxclient.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[]) {
	QApplication a(argc, argv);
	rxclient w;
	w.show();
	return a.exec();
}

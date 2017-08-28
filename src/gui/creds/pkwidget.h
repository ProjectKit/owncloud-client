#ifndef PK_WIDGET_H
#define PK_WIDGET_H

#include <QWidget>
#include <iostream>
#include "httpcredentialsgui.h"
using namespace OCC;
using namespace std;

class PKWidget : public QWidget
{
public:
	PKWidget(AbstractCredentials* mother, QWidget* father = Q_NULLPTR) :
	QWidget(father),
	_mother(mother)
	{}

	AbstractCredentials* _mother;

protected:
	void hideEvent(QHideEvent* event)
	{
		emit _mother->asked();
	}
};

#endif
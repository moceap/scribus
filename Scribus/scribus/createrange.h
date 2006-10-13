/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
#ifndef CREATERANGE
#define CREATERANGE

#include "createrangebase.h"
#include "usertaskstructs.h"

#include <qstring.h>
#include <qwidget.h>

class CreateRange : public CreateRangeBase
{
	Q_OBJECT
	public:
		CreateRange(int pageCount, QWidget* parent, const char* name = "" , WFlags fl=0);
		~CreateRange();
		void getCreateRangeData(CreateRangeData&);
		
	protected slots:
		void selectRangeType(QWidget *);
		void basicAddToRange();
		void basicDelFromRange();
		void basicMoveUp();
		void basicMoveDown();
		void basicSelectRangeTypeConsec();
		void basicSelectRangeTypeComma();
		void basicSelectRangeType(int);
		void advSpinChange(int);
		
	protected:
		int m_PageCount;
		int m_RangeType;
		int m_BasicRangeType;
		QString m_PageString;
};

#endif

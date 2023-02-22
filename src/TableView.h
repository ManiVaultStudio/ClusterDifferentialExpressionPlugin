#pragma once
#include <QTableView>



class TableView : public QTableView
{
	Q_OBJECT
public:

	TableView(QWidget* parent);
	virtual ~TableView();
public slots:

	
	inline void SLOT_setColumnWidth(int column, int width)   { QTableView::setColumnWidth(column, width); }

protected:
	virtual void scrollContentsBy(int dx, int dy) override;


};
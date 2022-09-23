#pragma once
#include <QTableView>

class TableView : public QTableView
{
	Q_OBJECT
public:

	TableView(QWidget* parent);
	virtual ~TableView();

protected:
	virtual void scrollContentsBy(int dx, int dy) override;
};
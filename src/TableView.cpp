#include "TableView.h"
#include "WordWrapHeaderView.h"

TableView::TableView(QWidget* parent)
	:QTableView(parent)
{
	
}
TableView::~TableView()
{}


void TableView::scrollContentsBy(int dx, int dy) 
{
	QTableView::scrollContentsBy(dx,dy);
	if(dx != 0)
	{
		WordWrapHeaderView *header = qobject_cast<WordWrapHeaderView*>(horizontalHeader());
		if(header)
		{
			header->fixWidgetPositions();
		}
	}
}
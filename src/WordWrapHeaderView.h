#pragma once
#include <QHeaderView>

class WordWrapHeaderView : public QHeaderView
{
	
public:
	WordWrapHeaderView(Qt::Orientation orientation, QWidget* parent = nullptr);
	virtual ~WordWrapHeaderView();
protected:
	virtual QSize sectionSizeFromContents(int logicalIndex) const override;
};
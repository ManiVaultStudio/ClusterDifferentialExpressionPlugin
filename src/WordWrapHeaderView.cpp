#include "WordWrapHeaderView.h"
#include <iostream>
#include <qtextoption.h>

WordWrapHeaderView::WordWrapHeaderView(Qt::Orientation orientation, QWidget* parent)
 : QHeaderView(orientation,parent)
{}

WordWrapHeaderView::~WordWrapHeaderView()
{
	
}

QSize WordWrapHeaderView::sectionSizeFromContents(int logicalIndex) const 
{
    const auto alignment = defaultAlignment();
    if(alignment & Qt::Alignment(Qt::TextWordWrap))
    {
        
        const QString text = this->model()->headerData(logicalIndex, this->orientation(), Qt::DisplayRole).toString();
    	int maxWidth = this->sectionSize(logicalIndex);
        auto margins = this->contentsMargins();
        auto rect2 = this->contentsRect();
        const int maxHeight = 5000; // arbitrarily large

        const QFontMetrics metrics(this->fontMetrics());
		maxWidth -= metrics.horizontalAdvance("M"); // assume M is the widest character ??
        const QRect rect = metrics.boundingRect(QRect(0, 0, maxWidth, maxHeight), alignment, text);

        const QSize textMarginBuffer(2, 2); // buffer space around text preventing clipping
        if(logicalIndex == 1)
			std::cout << std::endl << maxWidth << "\t" << rect.left() << "\t " << rect.width() << "\t" << (2*rect.left()) + rect.width() << "\t" << rect.height() << std::endl;
        return rect.size() + textMarginBuffer;
    }
    else
    {
        return QHeaderView::sectionSizeFromContents(logicalIndex);
    }
    
    
}
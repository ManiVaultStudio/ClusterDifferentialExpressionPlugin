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
    	QRect rect = metrics.boundingRect(QRect(0, 0, maxWidth, maxHeight), alignment, text);
        const int marginCorrection = 2;
        const QSize textMarginBuffer(marginCorrection, marginCorrection); // buffer space around text preventing clipping

        
        // the bounding rectangle doesn't always give the correct height so we fix it here.
        if ((maxWidth - (rect.right() + 2*marginCorrection)) <= 0)
        {
            rect.setHeight(rect.height() + metrics.height());
        }
        return rect.size() + textMarginBuffer;
    }
    else
    {
        return QHeaderView::sectionSizeFromContents(logicalIndex);
    }
    
    
}
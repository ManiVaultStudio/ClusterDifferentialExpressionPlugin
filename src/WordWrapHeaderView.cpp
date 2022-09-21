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
        
        auto m = metrics.boundingRect("M");
    	QRect rect = metrics.boundingRect(QRect(0, 0, maxWidth, maxHeight), alignment, text);
        const int correction = 4;
        const QSize textMarginBuffer(2, 2); // buffer space around text preventing clipping

#ifdef _DEBUG
        if (logicalIndex == 1)
        {
            std::cout << "BR (" <<rect.width() << "," << rect.height() << ")\n";
            std::cout << "TE "<< maxWidth << ", " << (maxWidth - (rect.right() + correction)) << std::endl;
        }
#endif

        // the bounding rectangle doesn't always give the correct height so we fix it here.
        if ((maxWidth - (rect.right() + correction)) <= 0)
        {
            

            rect.setHeight(rect.height() + metrics.height());
#ifdef _DEBUG
            if (logicalIndex == 1)
            {
                std::cout << "FX (" << rect.width() << "," << rect.height() << ")\n";
            }
#endif
        }
        rect.setHeight(rect.height() + metrics.capHeight());// auto-correct the height a bit for letters like 'p' so the bottom part doesn't get chopped off

#ifdef _DEBUG
        if (logicalIndex == 1)
        {
            std::cout << "AC (" << rect.width() << "," << rect.height() << ")\n";
            std::cout << "---" << std::endl;
        }
#endif

        return rect.size() + textMarginBuffer;
    }
    else
    {
        return QHeaderView::sectionSizeFromContents(logicalIndex);
    }
    
    
}
#include "WordWrapHeaderView.h"
#include <iostream>
#include <qtextoption.h>

WordWrapHeaderView::WordWrapHeaderView(Qt::Orientation orientation, QWidget* parent)
 : QHeaderView(orientation,parent)
, _widgetSupportEnabled(false)
{
    
}

WordWrapHeaderView::~WordWrapHeaderView()
{
    
}


void WordWrapHeaderView::enableWidgetSupport(bool enabled)
{

    if(this->orientation() == Qt::Horizontal)
    {
        _widgetSupportEnabled = enabled;

        if(enabled)
        {
            connect(this, SIGNAL(sectionResized(int, int, int)), this,

                SLOT(handleSectionResized(int)));

            connect(this, SIGNAL(sectionMoved(int, int, int)), this,

                SLOT(handleSectionMoved(int, int, int)));

            setSectionsMovable(true);
        }
        else
        {
            disconnect(this, SIGNAL(sectionResized(int, int, int)), this,

                SLOT(handleSectionResized(int)));

            disconnect(this, SIGNAL(sectionMoved(int, int, int)), this,

                SLOT(handleSectionMoved(int, int, int)));

           
        }
    }
}

void WordWrapHeaderView::setWidget(unsigned i, QWidget *w)
{
    w->setParent(this);
    auto found = _widgets.find(i);
    if (found != _widgets.end())
    {
        delete (*found);
        *found = w;
    }
    else
        _widgets[i] = w;
}

void WordWrapHeaderView::fixWidgetPosition(int logicalIndex)
{
    auto found = _widgets.find(logicalIndex);
    if (found != _widgets.end())
    {
        auto x = sectionViewportPosition(logicalIndex);
        auto y = sectionSize(logicalIndex);
        auto h = height();
        auto o = horizontalOffset();
        auto s = sectionSizeFromContents(logicalIndex);
        std::cout << x << "\t" << y << "\t" << h << "\t" << o << "\t(" << s.width() << ","<< s.height() << ")"<< std::endl;

        found.value()->setFixedWidth(s.width()-5);
    	found.value()->setGeometry(sectionViewportPosition(logicalIndex), 5,

            s.width()-5, height()+5);
    }
}

void WordWrapHeaderView::fixWidgetPositions()
{
   
    for (auto widget_iterator = _widgets.begin(); widget_iterator != _widgets.end(); ++widget_iterator)
    {
        unsigned index = widget_iterator.key();
        fixWidgetPosition(index);
    }
}

void WordWrapHeaderView::handleSectionResized(int i)
{
    
    for (int j = visualIndex(i); j < count(); j++) {

        int logical = logicalIndex(j);
        fixWidgetPosition(logical);
    }
}

void WordWrapHeaderView::handleSectionMoved(int logical, int oldVisualIndex, int newVisualIndex)

{
    
    for (int i = qMin(oldVisualIndex, newVisualIndex); i < count(); i++) {

        int logical = logicalIndex(i);

        fixWidgetPosition(logical);
    }

}

QSize WordWrapHeaderView::sectionSizeFromContents(int logicalIndex) const 
{

    auto found = _widgets.find(logicalIndex);
    if(found != _widgets.end())
    {
        int maxWidth = this->sectionSize(logicalIndex);
        int height = found.value()->height();
        std::cout << "H: " << height << std::endl;
        return QSize(maxWidth+2, height+5);
    }
    else
    {
        const auto alignment = defaultAlignment();
        if (alignment & Qt::Alignment(Qt::TextWordWrap))
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



            // the bounding rectangle doesn't always give the correct height so we fix it here.
            if ((maxWidth - (rect.right() + correction)) <= 0)
            {


                rect.setHeight(rect.height() + metrics.height());

            }
            rect.setHeight(rect.height() + metrics.capHeight());// auto-correct the height a bit for letters like 'p' so the bottom part doesn't get chopped off



            return rect.size() + textMarginBuffer;
        }
        else
        {
            return QHeaderView::sectionSizeFromContents(logicalIndex);
        }
    }
    
    
    
}

void WordWrapHeaderView::showEvent(QShowEvent* event)
{
    if(_widgetSupportEnabled)
    {
        for(auto widget_iterator = _widgets.begin(); widget_iterator != _widgets.end(); ++widget_iterator)
        {
            unsigned index = widget_iterator.key();
            QWidget* widget = widget_iterator.value();
            fixWidgetPosition(index);
        }
        QHeaderView::showEvent(event);
    }
    else
        QHeaderView::showEvent(event);
    
}

void WordWrapHeaderView::updateGeometries()
{
    QHeaderView::updateGeometries();
    for (auto widget_iterator = _widgets.begin(); widget_iterator != _widgets.end(); ++widget_iterator)
    {
        unsigned index = widget_iterator.key();
        QWidget* widget = widget_iterator.value();
        fixWidgetPosition(index);
    }
}
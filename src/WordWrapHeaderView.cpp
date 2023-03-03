#include "WordWrapHeaderView.h"
#include <iostream>
#include <qlayout.h>
#include <qtextoption.h>

WordWrapHeaderView::WordWrapHeaderView(Qt::Orientation orientation, QWidget* parent, bool widgetSupport)
 : QHeaderView(orientation,parent)
, _widgetSupportEnabled(false)
, _columnOffset(0)
{
	if(widgetSupport)
	{
		 enableWidgetSupport(widgetSupport);
	}
}

WordWrapHeaderView::~WordWrapHeaderView()
{
    
}


void WordWrapHeaderView::enableWidgetSupport(bool enabled)
{
    if (enabled == _widgetSupportEnabled)
        return; // nothing to change
    
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
   // delete w;
    /*
    w->setParent(this);
    w->raise();
    auto found = _widgets.find(i);
    if (found != _widgets.end())
    {
        delete (*found);
        *found = w;
    }
    else
        _widgets[i] = w;
        */
}


void WordWrapHeaderView::setExtraLeftSideColumns(std::size_t offset)
{
    /*
    long long offsetChange = offset - _columnOffset;
    if (offsetChange == 0)
        return;
    QMap<unsigned, QWidget*> newMap;
    for(QMap<unsigned, QWidget*>::iterator i=_widgets.begin(); i != _widgets.end(); ++i)
    {
        if (i.key() == 0)
            newMap[0] = i.value();
        else
            newMap[i.key() + offsetChange] = i.value();
    }
    _widgets = newMap;
    _columnOffset = offset;
    */
	headerDataChanged(Qt::Horizontal, 0, count());
}


void WordWrapHeaderView::clearWidgets()
{
    /*
	for(auto w : _widgets)
	{
       
        delete w;
	}
    _widgets.clear();
    */
}

QWidget* WordWrapHeaderView::getWidget(int logicalIndex) const
{
    //return nullptr;
    if (_widgetSupportEnabled && (this->model()))
    {
        QVariant variant = this->model()->headerData(logicalIndex, Qt::Horizontal, Qt::DisplayRole);
        if(variant.metaType() == QMetaType::fromType<QObject*>())
        {
            QObject *result = variant.value<QObject*>();
      
            QWidget* widget = qobject_cast<QWidget*>(result);

            widget->show();
            return widget;
        }
    }
    return nullptr;
}

QWidget* WordWrapHeaderView::getWidget(int logicalIndex)
{
    QWidget* result = static_cast<const WordWrapHeaderView*>(this)->getWidget(logicalIndex);
    if (result)
    {
        if (result->parent() != this)
        {
            result->setParent(this);
            result->raise();
        }
        return result;
    }
    return nullptr;
}
void WordWrapHeaderView::fixWidgetPosition(int logicalIndex)
{
    
    //qDebug() << "fixWidgetPosition " << logicalIndex << " start";
    QWidget* foundWidget = getWidget(logicalIndex);
    if(foundWidget)
    {
        auto sectionPosition = sectionViewportPosition(logicalIndex);
        auto secSize = sectionSize(logicalIndex);
        auto sectionHeight = height();
        auto sectionOffset = horizontalOffset();
        auto sectionSizeContents = sectionSizeFromContents(logicalIndex);
        /*
        qDebug() << "sectionPosition: " << sectionPosition;
        qDebug() << "sectionSize: " << secSize;
        qDebug() << "sectionHeight: " << sectionHeight;
        qDebug() << "sectionOffset: " << sectionHeight;
        qDebug() << "sectionSizeContents: " << sectionHeight;
        */
        if(foundWidget->parent() != this)
        {
            foundWidget->setParent(this);
            foundWidget->raise();
        }
        foundWidget->setFixedWidth(sectionSizeContents.width() - 5);
        
        foundWidget->setGeometry(sectionPosition, 5, sectionSizeContents.width() - 5, sectionHeight + 5);

      //  qDebug() << "Geometry: (" << sectionPosition << "," << 5 << "," << sectionSizeContents.width() - 5 << "," << sectionHeight + 5 << ")";
    }

   // qDebug() << "fixWidgetPosition " << logicalIndex << " end";
   
}

void WordWrapHeaderView::fixWidgetPositions()
{
    if (this->model())
    {
        const auto  nrOfColumns = this->model()->columnCount();
        for (int logicalIdx = 0; logicalIdx < nrOfColumns; ++logicalIdx)
        {

            fixWidgetPosition(logicalIdx);
            /*
            QWidget* foundWidget = getWidget(logicalIdx);
            if (foundWidget)
            {
                foundWidget->setParent(this);
                foundWidget->raise();
                foundWidget->show();
                QRect geom(sectionViewportPosition(logicalIdx), 0, sectionSize(logicalIdx) - 5, height());
                qDebug() << "fixWidgetPositions " << logicalIdx << "\t" << geom;
                
                foundWidget->setGeometry(geom);
            }
            */
        }
    }
    /*
    if(this->model())
    {
        const auto  nrOfColumns = this->model()->columnCount();
        for (int column = 0; column < nrOfColumns; ++column)
        {
            fixWidgetPosition(column);
        }
    }
    */
    
    /*
    for (auto widget_iterator = _widgets.begin(); widget_iterator != _widgets.end(); ++widget_iterator)
    {
        unsigned index = widget_iterator.key();
        fixWidgetPosition(index);
    }
    */
}

void WordWrapHeaderView::handleSectionResized(int i)
{
    
    for (int j = visualIndex(i); j < count(); j++) {
        int logicalIdx = logicalIndex(j);
        fixWidgetPosition(logicalIdx);
        /*
        QWidget* foundWidget = getWidget(logicalIdx);
        if (foundWidget)
        {
            QRect geom(sectionViewportPosition(logicalIdx), 0, sectionSize(logicalIdx) - 5, height());
            qDebug() << "handleSectionResized " << logicalIdx << "\t" << geom;
            foundWidget->setGeometry(geom);
        }
        */
    }
    /*
    for (int j = visualIndex(i); j < count(); j++) {

        int logical = logicalIndex(j);
        fixWidgetPosition(logical);
    }
    */
}

void WordWrapHeaderView::handleSectionMoved(int logical, int oldVisualIndex, int newVisualIndex)

{
   
    for (int i = qMin(oldVisualIndex, newVisualIndex); i < count(); i++) {

        int logicalIdx = logicalIndex(i);
        fixWidgetPosition(logicalIdx);
        /*
        QWidget* foundWidget = getWidget(logicalIdx);
        if (foundWidget)
        {
            
            QRect geom(sectionViewportPosition(logicalIdx), 0, sectionSize(logicalIdx) - 5, height());
            qDebug() << "handleSectionMoved " << logical << "\t" << geom;
            foundWidget->setGeometry(geom);
        }
        */
    }

}

QSize WordWrapHeaderView::sectionSizeFromContents(int logicalIndex) const 
{
   
    QWidget* foundWidget = getWidget(logicalIndex);
    if(foundWidget)
    {
        foundWidget->show();
        int maxWidth = this->sectionSize(logicalIndex);
        int height = foundWidget->height();
        QSize result(maxWidth+2, height+5);
        return result;
	    
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
            const QSize textMarginBuffer(5, 2); // buffer space around text preventing clipping



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
        if (this->model())
        {
            const auto  nrOfColumns = this->model()->columnCount();
            for (int logicalIndex = 0; logicalIndex < nrOfColumns; ++logicalIndex)
            {
                QWidget* foundWidget = getWidget(logicalIndex);
                if (foundWidget)
                {
                    foundWidget->setGeometry(sectionViewportPosition(logicalIndex), 0, sectionSize(logicalIndex) - 5, height());
                    foundWidget->show();
                }
            }
        }
    }
    
	QHeaderView::showEvent(event);
    
}

void WordWrapHeaderView::updateGeometries()
{
    QHeaderView::updateGeometries();
    fixWidgetPositions();
    /*
    for (auto widget_iterator = _widgets.begin(); widget_iterator != _widgets.end(); ++widget_iterator)
    {
        unsigned index = widget_iterator.key();
        fixWidgetPosition(index);
    }
    */
}
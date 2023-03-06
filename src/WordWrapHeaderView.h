#pragma once
#include <QHeaderView>

class WordWrapHeaderView : public QHeaderView
{
	Q_OBJECT
public:
	WordWrapHeaderView(Qt::Orientation orientation, QWidget* parent = nullptr, bool widgetSupport=true);
	virtual ~WordWrapHeaderView();
	void enableWidgetSupport(bool value);
	
private:
	QWidget* getWidget(int logicalIndex) const;
	QWidget* getWidget(int logicalIndex);
	void fixWidgetPosition(int logicalIndex);

public:
	void fixWidgetPositions();

private slots:
	void handleSectionResized(int i);
	void handleSectionMoved(int logical, int oldVisualIndex, int newVisualIndex);

protected:
	virtual QSize sectionSizeFromContents(int logicalIndex) const override;
	virtual void showEvent(QShowEvent* event) override;
	virtual void updateGeometries() override;
private:
	bool _widgetSupportEnabled;
//	QMap<unsigned, QWidget*> _widgets;
	std::size_t _columnOffset;
};
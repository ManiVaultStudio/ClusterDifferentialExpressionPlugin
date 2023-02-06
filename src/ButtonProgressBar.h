#pragma once
#include <QWidget>
#include "QTableItemModel.h"
class QPushButton;
class QProgressBar;

class ButtonProgressBar : public QWidget
{
	Q_OBJECT
public:
	ButtonProgressBar(QWidget *parent = nullptr, QWidget* button = nullptr, QWidget* progressBar = nullptr);

	void setButtonText(const QString&, QColor color = Qt::black);
	void setProgressBarText(const QString&);

	QProgressBar* getProgressBar()
	{
		return _progressBar;
	}

public slots:
	void showStatus(QTableItemModel::Status status);
	
private:
	
	QProgressBar		*_progressBar;
	QPushButton			*_button;
};
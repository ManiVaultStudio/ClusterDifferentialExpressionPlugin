#include "ButtonProgressBar.h"

#include <QPushButton>
#include <QProgressBar>
#include <QGridLayout>
#include <QGraphicsColorizeEffect>
#include <stdexcept>

#include "QTableItemModel.h"

class InvalidProgressBarException : public std::runtime_error {
public:
    explicit InvalidProgressBarException(const std::string& message) : std::runtime_error(message) {}
};


ButtonProgressBar::ButtonProgressBar(QWidget *parent, QWidget* button, QWidget* progressBar)
	:QWidget(parent)
	, _button(nullptr)
	, _progressBar(nullptr)

{
    QGridLayout* newLayout = new QGridLayout();
    newLayout->setContentsMargins(0, 0, 0, 0);
    {
        if (button == nullptr)
        {
            _button = new QPushButton(this);
    
            newLayout->addWidget(_button, 0, 0);
        }
        else
        {
            _button = qobject_cast<QPushButton*>(button);
            if (_button == nullptr)
                _button = button->findChild<QPushButton*>();

            // if button is a QPushButton or contains a QPushButton add it
            if (_button)
            {
                newLayout->addWidget(button, 0, 0);
                _button->show();
            }
            else
            {
                delete newLayout;
                //throw std::exception("button is not a type of QPushButton and does not contain a QPushButton");
                throw InvalidProgressBarException("progressBar is not a type of QProgressBar and does not contain a QProgressBar");
            }
        }
    }
	
    {
        if (progressBar == nullptr)
        {
            _progressBar = new QProgressBar(this);
            newLayout->addWidget(_progressBar, 0, 0);
        }
        else
        {
            _progressBar = qobject_cast<QProgressBar*>(progressBar);
            if (_progressBar)
                _progressBar = progressBar->findChild<QProgressBar*>();

            if(_progressBar)
            {
                newLayout->addWidget(progressBar, 0, 0);
            }
            else
            {
                delete newLayout;
                //throw std::exception("progressBar is not a type of QProgressBar and does not contain a QProgressBar");
                throw InvalidProgressBarException("progressBar is not a type of QProgressBar and does not contain a QProgressBar");
            }
            
        }

        if(_progressBar)
        {
            _progressBar->setTextVisible(true);
            _progressBar->setAlignment(Qt::AlignCenter);
            _progressBar->setOrientation(Qt::Orientation::Horizontal);
            QGraphicsColorizeEffect* colorizeEffect = new QGraphicsColorizeEffect();
            colorizeEffect->setColor(Qt::red);
            colorizeEffect->setStrength(1);
            _progressBar->setGraphicsEffect(colorizeEffect);
            _progressBar->setFormat("No Selections");
            _progressBar->graphicsEffect()->setEnabled(false);
            _progressBar->hide();
        }
    }
    showStatus(QTableItemModel::Status::Undefined);
	delete layout();
    setLayout(newLayout);
}


void ButtonProgressBar::setButtonText(const QString &text, QColor color)
{
    if(_button)
    {
        QPalette pal = _button->palette();
        pal.setColor(QPalette::Button, Qt::red);
        pal.setColor(QPalette::ButtonText, Qt::red);
        _button->setPalette(pal);
        _button->setAutoFillBackground(true);
        _button->setText(text);
    }
		
}

void ButtonProgressBar::setProgressBarText(const QString& format)
{
    if (_progressBar)
        _progressBar->setFormat(format);
}

void ButtonProgressBar::showStatus(QTableItemModel::Status status)
{
	switch(status)
	{
		case QTableItemModel::Status::Undefined:
			{
				_button->hide();
				_progressBar->setFormat("No Data Available");
				_progressBar->setValue(0);
				_progressBar->show();
				break;;
			}
        case QTableItemModel::Status::OutDated:
	        {
		        _progressBar->hide();
                setButtonText("Out-Dated. Press to Compute", Qt::red);
                _button->show();

                break;
	        }
		case QTableItemModel::Status::Updating:
	        {
            _button->hide();
            _progressBar->setFormat("Updating...");
            _progressBar->setValue(0);
            _progressBar->show();
            break;;
	        }
		case QTableItemModel::Status::UpToDate:
	        {
            _button->hide();
            _progressBar->setFormat("Up-to-Date");
            _progressBar->setValue(_progressBar->maximum());
            _progressBar->show();
            break;
	        }
        
	}
}



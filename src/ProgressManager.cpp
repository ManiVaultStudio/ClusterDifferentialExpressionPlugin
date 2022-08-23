#include "ProgressManager.h"
#include <omp.h>
#include <QProgressDialog>
#include <iostream>
#include <iomanip>
#include <QWidgetAction>
#include <QProgressBar>
#include <QMenuBar>
#include "QApplication"
#include <QLabel>
#include <QMainWindow>

namespace local
{
    QMainWindow* getMainWindow()
    {
        foreach(QWidget * widget, qApp->topLevelWidgets())
            if (QMainWindow* mainWindow = qobject_cast<QMainWindow*>(widget))
                return mainWindow;
        return nullptr;
    }
}

ProgressManager::ProgressManager()
	:m_available(true)
	, m_progressDialog(nullptr)
	, m_progressBar(nullptr)
	, m_progressBarLabel(nullptr)
	, m_noProgressDialog(false)
	, m_cancelled(false)
	, m_autoRaise(true)
	, m_scaleFactor(0)
	, m_maxRange(0)
{
	
}

ProgressManager::~ProgressManager()
{
	m_cancelled = true;
	m_available = false;
	if (m_progressDialog)
	{
		while (!m_progressDialog->wasCanceled());
		delete m_progressDialog;
		m_progressDialog = nullptr;
	}
	else
	{
		m_progressBarLabel = nullptr;
		m_progressBar = nullptr;
	}
}

bool ProgressManager::available() const
{
	return m_available;
}

bool ProgressManager::autoRaise() const
{
	return m_autoRaise;
}

void ProgressManager::start(std::size_t size, const std::string &mesg)
{

	while (!m_available);

	m_available = false;
	
	
	
	if (m_progressBar)
	{
		
	}
	else if (!m_noProgressDialog)
	{
		if (omp_get_thread_num() == 0)
		{
			if (m_progressDialog == nullptr)
				m_progressDialog = new QProgressDialog(local::getMainWindow(), Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
			if (m_cancelled == false)
				m_progressDialog->setLabelText(mesg.c_str());
			m_progressDialog->setWindowModality(Qt::WindowModal);
			m_progressDialog->setCancelButton(nullptr);
			m_progressDialog->setMinimumDuration(150);
			m_progressDialog->show();
			if (m_autoRaise)
			{
				m_progressDialog->raise();
			}
			
		}
	}

	setLabelText(mesg.c_str());
	setRange(size);
	qApp->processEvents();
}

void ProgressManager::setRange( std::size_t size)
{
	m_maxRange = size;
	if (m_maxRange)
		m_scaleFactor = (1.0f * MAX_RANGE) / m_maxRange;
	else
		m_scaleFactor = 0;
	m_progress.assign(size, 0);
	if (m_progressBar)
	{
		if(size)
			m_progressBar->setRange(0, MAX_RANGE);
		else
			m_progressBar->setRange(0, 0);
		m_progressBar->setValue(0);
		m_progressBar->update();
	}
	else if (m_progressDialog)
	{
		if(size)
			m_progressDialog->setRange(0, MAX_RANGE);
		else
			m_progressDialog->setRange(0, 0);
		m_progressDialog->setValue(0);
		m_progressDialog->update();
	}
}

void ProgressManager::setAutoRaise(bool value)
{
	m_autoRaise = value;
}

void ProgressManager::setProgressBar(QProgressBar *progressBar)
{
	
	if (progressBar == m_progressBar)
		return;
	while (!m_available);
	m_progressBar = progressBar;
	
}

void ProgressManager::setProgressBarLabel(QLabel* label)
{
	m_progressBarLabel = label;
}

void ProgressManager::setNoProgressDialog(bool value)
{
	while (!m_available);
	if (value)
	{
		delete  m_progressDialog;
	}
	m_noProgressDialog = value;
}

void ProgressManager::setValue(std::size_t value)
{
	const int progressValue = (value == m_maxRange) ? MAX_RANGE : (m_scaleFactor * value);
	if (m_progressBar)
	{
		m_progressBar->setValue(progressValue);
		m_progressBar->update();
	}
	else if (m_progressDialog)
	{
		m_progressDialog->setValue(progressValue);
		m_progressDialog->update();
	}
	qApp->processEvents();
}

void ProgressManager::print(long long i)
{
	m_progress[i] = 1;
	if (omp_get_thread_num() == 0)
	{
		int64_t zero = 0;
		int64_t value = std::accumulate(m_progress.cbegin(), m_progress.cend(), zero);
		const int progressValue = (value == m_maxRange) ? MAX_RANGE : (m_scaleFactor * value);
		
// #ifndef HIDE_CONSOLE
// 		std::cout << "\r" << m_labelText.toStdString() << std::fixed << std::setprecision(3) << 100.0*value / m_maxRange << "%";
// #endif
		if (m_progressBar || m_progressDialog)
		{
			
			if (m_progressBar)
			{
				m_progressBar->setValue(progressValue);
			}
			else if (m_progressDialog)
			{
				m_progressDialog->setValue(progressValue);
				if (m_autoRaise)
				{
					m_progressDialog->update();
					m_progressDialog->raise();
				}
			}
		}		
#ifndef HIDE_CONSOLE
		else
			std::cout << "  print:: progressbar&m_progressDialog not set";
#endif		
	}
	qApp->processEvents();
}

void ProgressManager::end()
{
	m_available = true;
	
	if (omp_get_thread_num() == 0)
	{
// #ifndef HIDE_CONSOLE
// 		std::cout << "\r" << m_labelText.toStdString() << std::fixed <</* std::setw(11) <<*/ std::setprecision(3) << 100.0*std::accumulate(m_progress.cbegin(), m_progress.cend(), 0) / m_progress.size() << "%" << std::endl;
// #endif
	}

	if (m_progressBar)
	{
		m_progressBar->setValue(m_progressBar->maximum());
	}
	else if (m_progressDialog)
	{
		m_progressDialog->cancel();
		m_progressDialog->hide();
	}
	
}



void ProgressManager::setLabelText(const QString& mesg)
{
	m_labelText = mesg;
	if (m_progressBar)
	{
		if (m_progressBarLabel)
		{
			m_progressBarLabel->setText(m_labelText);
			m_progressBarLabel->update();
		}
		else
		{
			//m_labelText += " %p%";
			m_progressBar->setFormat(m_labelText);
			m_progressBar->update();
		}
	}
	else if (m_progressDialog)
	{
		m_progressDialog->setLabelText(m_labelText);
		m_progressDialog->update();
	}
}

void ProgressManager::update()
{
	if (m_progressBar)
		m_progressBar->update();
	else if (m_progressDialog)
	{
		m_progressDialog->update();
	}
}

void ProgressManager::setMaximum(int value)
{
	if (m_progressBar)
		m_progressBar->setMaximum(value);
	else if (m_progressDialog)
		m_progressDialog->setMaximum(value);
}

void ProgressManager::setMinimum(int value)
{
	if (m_progressBar)
		m_progressBar->setMinimum(value);
	else if (m_progressDialog)
		m_progressDialog->setMinimum(value);
}

int ProgressManager::maximum() const
{
	if (m_progressBar)
		return m_progressBar->maximum();
	if(m_progressDialog)
		return m_progressDialog->maximum();
	return 0;
}

int ProgressManager::minimum() const
{
	if (m_progressBar)
		return m_progressBar->minimum();
	if (m_progressDialog)
		return m_progressDialog->minimum();
	return 0;
}

void ProgressManager::setTextVisible(bool value)
{
	if (m_progressBar)
		m_progressBar->setTextVisible(value);
}

int ProgressManager::value() const
{
	if (m_progressBar)
		if (m_scaleFactor)
			return m_progressBar->value() / m_scaleFactor;
		else
			return m_progressBar->value();
	if (m_progressDialog)
		if (m_scaleFactor)
			return m_progressDialog->value() / m_scaleFactor;
		else
			return m_progressDialog->value();
	return -1;
}


int  ProgressManager::currentProgress() const
{
    if (omp_get_thread_num() == 0)
    {
        int64_t zero = 0;
        int64_t value = std::accumulate(m_progress.cbegin(), m_progress.cend(), zero);
        const int progressValue = (value == m_maxRange) ? MAX_RANGE : (m_scaleFactor * value);
        return progressValue;
    }
    return 0;
}

bool ProgressManager::canceled() const
{
	
	return m_cancelled;
}

void ProgressManager::setCanceled(bool value)
{
	if (value == m_cancelled)
		return;
	m_cancelled = value;
	if (omp_get_thread_num() == 0)
	{
		//std::cout << "ProgressManager::setCanceled(" << (value ? "true" : "false") << ")\n";
		m_labelText = "Canceling... ";
		if (m_cancelled)
		{
			if (m_progressBar)
			{
				m_progressBar->setFormat(m_labelText);
			}
// 			else if (m_progressDialog)
// 			{
// 				m_progressDialog->setLabelText(m_mesg.c_str());
// 			}
		}
		else
		{
			if (m_progressBar)
			{
				m_progressBar->setFormat("");
			}
			else if (m_progressDialog)
			{
				m_progressDialog->setLabelText("");
			}
		}
	}
}

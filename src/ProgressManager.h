
#pragma once

#include <atomic>
#include <vector>
#include <string>
#include "QSysInfo"
#include <QString>

class QProgressDialog;
class QProgressBar;
class QLabel;
class QWidgetAction;

class ProgressManager
{
	enum{MAX_RANGE=1000};
	std::vector<int8_t> m_progress;
	QString m_labelText;
	QProgressDialog *m_progressDialog;
	std::atomic<bool> m_available;
	QProgressBar *m_progressBar;
	QLabel* m_progressBarLabel;
	std::atomic<bool> m_cancelled;
	bool m_noProgressDialog;
	bool m_autoRaise;
	float m_scaleFactor;
	std::size_t m_maxRange;
public:
	ProgressManager();
	~ProgressManager();
	bool available() const;
	bool autoRaise() const;
	bool canceled() const;
	void setCanceled(bool value);
	void start(std::size_t, const std::string &mesg);
	void setRange(std::size_t size);
	void setAutoRaise(bool value);
	void setProgressBar(QProgressBar *progressBar);
	void setProgressBarLabel(QLabel* label);
	void setNoProgressDialog(bool value);
	void setValue(std::size_t value);
	void print(long long i);
	void end();
	void setLabelText(const QString& mesg);
	void update();
	void setMaximum(int value);
	void setMinimum(int value);
	int maximum() const;
	int minimum() const;
	void setTextVisible(bool value);
	int value() const;
    int currentProgress() const;
	

};

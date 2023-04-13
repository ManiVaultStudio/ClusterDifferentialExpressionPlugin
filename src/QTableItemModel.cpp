#include "QTableItemModel.h"
#include <QApplication>
#include <QClipboard>
#include <assert.h>
#include <QMetaType>
#include <QWidget>
#include <QLabel>

//#define TESTING

QTableItemModel::QTableItemModel(QObject *parent /*= Q_NULLPTR*/, bool checkable)
	:QAbstractTableModel(parent)
	, m_checkable(checkable)
	, m_columns(0)
	, m_status(Status::Undefined)
{
	
}

int QTableItemModel::rowCount(const QModelIndex &parent /*= QModelIndex()*/) const
{
	Q_UNUSED(parent);
	return m_data.size();
}

int QTableItemModel::columnCount(const QModelIndex &parent /*= QModelIndex()*/) const
{
	Q_UNUSED(parent);
#ifdef TESTING
	return m_columns + 1;
#else
	return m_columns;
#endif
}


QVariant QTableItemModel::data(const QModelIndex &index, int role /*= Qt::DisplayRole*/) const
{
	
	int r = index.row();
	int c = index.column();
	if (!index.isValid())
		return QVariant();
	if (index.row() >= m_data.size() || index.row() < 0)
		return QVariant();
	if (index.column() >= columnCount() || index.column() < 0)
		return QVariant();

	auto data = m_data[index.row()].data[index.column()];
	if(data.type() == QMetaType::QVariantMap)
	{
		QVariantMap map = data.toMap();
		QString roleString = QString::number(role);
		if(map.contains(roleString))
		{
			return map[roleString];
		}
	}
	else if (role == Qt::BackgroundRole)
	{
		if (m_status == Status::OutDated)
			return QBrush(QColor::fromRgb(227,227,227));
	}
	else if (role == Qt::DisplayRole)
	{
		return data;
	}
	else if  (m_checkable && (role == Qt::CheckStateRole && index.column() == 0))
	{
		return m_data[index.row()].checkState();
	}

	return QVariant();
}

bool QTableItemModel::setData(const QModelIndex & index, const QVariant & value, int role /*= Qt::DisplayRole*/)
{
	if (m_checkable &&( role == Qt::CheckStateRole && index.column() == 0))
	{
		Qt::CheckState state = static_cast<Qt::CheckState>(value.toUInt());
		if (m_data[index.row()].checkState() != state)
		{
			m_data[index.row()].setCheckState(state);
			emit(dataChanged(index, index));
			return true;
		}
		
		
	}
	else if (role == Qt::EditRole)
	{
		m_data[index.row()].data[index.column()] = value;
		emit(dataChanged(index, index));
		return true;
	}
	return false;
}

Qt::ItemFlags QTableItemModel::flags(const QModelIndex& index) const
{
	Qt::ItemFlags returnFlags = QAbstractTableModel::flags(index);

	returnFlags |= Qt::ItemIsEnabled;
	if (m_checkable && index.column() == 0)
	{
		returnFlags |= Qt::ItemIsUserCheckable;
	}

	return returnFlags;
}

void QTableItemModel::resize(std::size_t size)
{
	if (size != m_data.size())
	{
		m_data.resize(size);
	}

	for (std::size_t i = 0; i < size; ++i)
	{
#ifdef TESTING
		m_data[i].data.resize(m_columns + 1);
		m_data[i].data[m_columns] = i;
#endif
		if (m_data[i].data.size() != m_columns)
			m_data[i].data.resize(m_columns);

	}
}

QVariant& QTableItemModel::at(std::size_t row, std::size_t column)
{
	return m_data[row].data[column];
}



void QTableItemModel::setRow(std::size_t row, const std::vector<QVariant> &data, Qt::CheckState checked, bool silent/*=false*/)
{

	assert(data.size() == m_columns);
	std::size_t startColumn = m_columns;
	std::size_t endColumn = m_columns;
	
	if (m_checkable && (checked != m_data[row].checkState()))
	{
		startColumn = 0;
		endColumn = 0;
		m_data[row].setCheckState(checked);
	}
	
	for (std::size_t column = 0; column < m_columns; ++column)
	{
		if (m_data[row].data[column] != data[column])
		{
			if (startColumn == m_columns)
			{
				startColumn = column;
			}
			endColumn = column;
			m_data[row].data[column] = data[column];
		}
	}
	
	if (!silent)
	{
		if (startColumn < m_columns)
		{
			QModelIndex start = index(row, startColumn);
			QModelIndex end = index(row, endColumn);
			emit dataChanged(start, end);
		}
	}
}

QVariant QTableItemModel::headerData(int section, Qt::Orientation orientation, int role) const 
{
	if (section >= 0 && section < m_horizontalHeader.size())
	{
		if(orientation == Qt::Horizontal)
			if(role == Qt::DisplayRole)
				return m_horizontalHeader[section];
	}
	
    return QVariant();

}

void QTableItemModel::setCheckState(int row, Qt::CheckState state)
{
	setData(index(row, 0), state, Qt::CheckStateRole);
}

Qt::CheckState QTableItemModel::checkState(int row) const
{
	return m_data[row].checkState();
}

void QTableItemModel::clear()
{
	m_data.clear();
}

void QTableItemModel::startModelBuilding(qsizetype columns, qsizetype rows)
{
	beginResetModel();
	m_columns = columns;
	for(qsizetype i=0; i < m_horizontalHeader.size(); ++i)
	{
		QVariant variant = m_horizontalHeader[i];
		if (variant.metaType() == QMetaType::fromType<QObject*>())
		{
			QObject* result = variant.value<QObject*>();

			QWidget* widget = qobject_cast<QWidget*>(result);

			widget->lower();
			widget->hide();
			widget->setParent(nullptr);
		}
	}
	m_horizontalHeader.assign(m_columns, QVariantMap());
	resize(rows);
	setStatus(Status::Updating);
}

void QTableItemModel::endModelBuilding()
{
	setStatus(Status::UpToDate);
	emit headerDataChanged(Qt::Horizontal, 0, m_columns-1 );
	endResetModel();
}

void QTableItemModel::setHorizontalHeader(int index, QVariant &value)
{
	if (index >= m_horizontalHeader.size())
		m_horizontalHeader.resize(index+1);
	setHeaderData(index, Qt::Horizontal, value, Qt::DisplayRole);
}
void QTableItemModel::setHorizontalHeader(int index, const QString& value)
{
	QVariant variant(value);
	setHeaderData(index, Qt::Horizontal, variant, Qt::DisplayRole);
}



bool QTableItemModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role)
{
	assert(orientation == Qt::Horizontal);
	if(orientation == Qt::Horizontal)
	{
		if (section >= m_horizontalHeader.size())
			m_horizontalHeader.resize(section + 1);

		m_horizontalHeader[section] = value;
		return true;
	}
	return false;
}


void QTableItemModel::copyToClipboard() const
{
	const QString DisplayRoleKey = QString::number(Qt::DisplayRole);

	QString result;
	QChar quote = '"';
	for (std::size_t c = 0; c < m_columns; ++c)
	{
		if (c != 0)
			result += "\t";
		QVariant headerVariant  = m_horizontalHeader[c];
		if (headerVariant.metaType() == QMetaType::fromType<QString>())
		{
			QString header = headerVariant.toString();
			if (header != "_hidden_")
			{
				header.replace("\n", "_");
				header.replace(" ", "_");
				result += quote + header + quote;
			}
		}
		else if(headerVariant.metaType() == QMetaType::fromType<QObject*>())
		{
			QObject* object = headerVariant.value<QObject*>();

			QWidget* widget = qobject_cast<QWidget*>(object);
			QList<QLabel*> children = widget->findChildren<QLabel*>(Qt::FindChildrenRecursively);
			if(!children.isEmpty())
			{
				QString header;
				for (QLabel* c : children)
				{


					QString text = c->text();
					text.replace("\n", "_");
					text.replace(" ", "_");
					if (header.isEmpty())
						header = text;
					else
						header += "_" + text;
				}
				result += quote + header + quote;
			}
		}
		
	}
	result += "\n";

	const std::size_t rows = m_data.size();
	for (std::size_t r = 0; r < rows; ++r)
	{
		for (std::size_t c = 0; c < m_columns; ++c)
		{
			if (m_horizontalHeader[c].metaType() == QMetaType::fromType<QString>())
			{
				QString header = m_horizontalHeader[c].toString();

				if (header != "_hidden_")
				{
					if (c != 0)
						result += "\t";
					result += m_data[r].data[c].toString();
				}
			}
			else
			{
				if (c != 0)
					result += "\t";
				result += m_data[r].data[c].toString();
			}
		}
		result += "\n";
	}
	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText(result);
}

void QTableItemModel::invalidate()
{
	setStatus(QTableItemModel::Status::OutDated);
}

void QTableItemModel::setStatus(QTableItemModel::Status status)
{
	if (status != m_status)
	{
		layoutAboutToBeChanged();
		m_status = status;
		emit layoutChanged();
		emit statusChanged(status);
	}
}
QTableItemModel::Status QTableItemModel::status()
{
	return m_status;
}

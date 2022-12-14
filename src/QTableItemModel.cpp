#include "QTableItemModel.h"
#include <QApplication>
#include <QClipboard>
#include <assert.h>
#include <iostream>
#include <QMetaType>

//#define TESTING

QTableItemModel::QTableItemModel(QObject *parent /*= Q_NULLPTR*/, bool checkable, std::size_t columns)
	:QAbstractTableModel(parent)
	, m_checkable(checkable)
	, m_columns(columns)
	, m_outOfDate(false)
{
	m_horizontalHeader.resize(m_columns);
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
	/*
	if(role == Qt::TextAlignmentRole)
	{
		if (index.column() == 0)
			return Qt::AlignLeft;
		else
			return Qt::AlignRight;
	}
	else
	*/
	if(role == Qt::ForegroundRole)
	{
		auto temp = m_data[index.row()].data[index.column()];
		if (temp.type() == QMetaType::QVariantList)
		{
			auto temp2 = temp.toList();
			if (temp2.size() >= 2)
			{

				return temp2.at(1);
			}
		}
	}
	else if (role == Qt::BackgroundRole)
	{
		if (m_outOfDate)
			return QBrush(QColor::fromRgb(227,227,227));
		auto temp = m_data[index.row()].data[index.column()];
		if (temp.type() == QMetaType::QVariantList)
		{
			auto temp2 = temp.toList();
			if (temp2.size() >= 3)
			{

				return temp2.at(2);
			}
		}
	}
	else if (role == Qt::DisplayRole)
	{
		auto temp = m_data[index.row()].data[index.column()];
		if (temp.type() == QMetaType::QVariantList)
		{
			auto temp2 = temp.toList();
			if (temp2.size())
			{

				return temp2.at(0);
			}
		}
		return temp;
		//return m_data[index.row()].data[index.column()];
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
		for (std::size_t i = 0; i < size; ++i)
		{
#ifdef TESTING
			m_data[i].data.resize(m_columns+1);
			m_data[i].data[m_columns] = i;
#endif
			if (m_data[i].data.size() != m_columns)
				m_data[i].data.resize(m_columns);

		}
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
	if (section >= 0)
	{
		if (role == Qt::DisplayRole)
		{

			if (orientation == Qt::Horizontal)
			{
				return m_horizontalHeader[section];
			}
		}
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

void QTableItemModel::startModelBuilding()
{
	beginResetModel();
}

void QTableItemModel::endModelBuilding()
{
	setOutDated(false);
	endResetModel();
}

void QTableItemModel::setHorizontalHeader(int index, const QString &value)
{
	if (index >= m_horizontalHeader.size())
		m_horizontalHeader.resize(index);
	m_horizontalHeader[index] = value;
}

void QTableItemModel::copyToClipboard() const
{
	QString result;
	QChar quote = '"';
	for (std::size_t c = 0; c < m_columns; ++c)
	{
		if (c != 0)
			result += "\t";
		QString header = m_horizontalHeader[c];
		if (header != "_hidden_")
		{
			header.replace("\n", "_");
			header.replace(" ", "_");
			result += quote + header + quote;
		}
	}
	result += "\n";

	const std::size_t rows = m_data.size();
	for (std::size_t r = 0; r < rows; ++r)
	{
		for (std::size_t c = 0; c < m_columns; ++c)
		{
			if (m_horizontalHeader[c] != "_hidden_")
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

void QTableItemModel::setOutDated(bool value)
{
	
	if (value != m_outOfDate)
	{
		layoutAboutToBeChanged();
		m_outOfDate = value;
		emit layoutChanged();
	}
}

bool QTableItemModel::outDated() const
{
	return m_outOfDate;
}

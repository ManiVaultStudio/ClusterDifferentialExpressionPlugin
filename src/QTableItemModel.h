
#ifndef QTableItemModel_H
#define QTableItemModel_H
#include "QAbstractItemModel"
#include <vector>

#include "QStandardItemModel"

//template<std::size_t Columns>
class QTableItemModel : public QAbstractTableModel
{

	
public:
	enum class Status { Undefined, OutDated, Updating, UpToDate };

	class Row
	{
	public:
		std::vector<QVariant> data;
		Qt::CheckState checkState() const{ return m_checkState; }
		void setCheckState(Qt::CheckState &c){ m_checkState = c; }
	private:
		Qt::CheckState m_checkState;
		
	};
	Q_OBJECT

private:
	void clear();
	void resize(std::size_t size);
public:
	QTableItemModel(QObject *parent, bool checkable);
	virtual int	rowCount(const QModelIndex &parent = QModelIndex()) const override;
	virtual int	columnCount(const QModelIndex &parent = QModelIndex()) const override;
	virtual QVariant	data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	virtual bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::DisplayRole) override;
    virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

	void update();
	//virtual QModelIndex	index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
	//virtual QModelIndex	parent(const QModelIndex &index) const override;

	void setCheckState(int row, Qt::CheckState state);
	Qt::CheckState checkState(int row) const;

	
	QVariant& at(std::size_t row, std::size_t column);
	
	void setRow(std::size_t row, const std::vector<QVariant> &data, Qt::CheckState checked, bool silent=false);

	
	void startModelBuilding(qsizetype columns, qsizetype rows);

	void endModelBuilding();
	void setHorizontalHeader(int index, QVariant &value);
	void setHorizontalHeader(int index, const QString& value);
	void setHorizontalHeader(int index, QWidget *widget);
	

	void copyToClipboard() const;

	void invalidate();

	void setStatus(Status status);
public:
	Status status();

signals:
	void statusChanged(Status status);

private:
	
	std::vector < Row > m_data;
	std::vector < QVariant> m_horizontalHeader;
	std::vector < QWidget*> m_horizontalHeaderWidgets;
	bool m_checkable;
	std::size_t m_columns;
	Status m_status;
};

#endif


#ifndef QTableItemModel_H
#define QTableItemModel_H
#include "QAbstractItemModel"
#include <vector>

#include "QStandardItemModel"

//template<std::size_t Columns>
class QTableItemModel : public QAbstractTableModel
{
public:
	
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
public:
	QTableItemModel(QObject *parent, bool checkable, std::size_t columns);
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

	void resize(std::size_t size);
	QVariant& at(std::size_t row, std::size_t column);
	
	void setRow(std::size_t row, const std::vector<QVariant> &data, Qt::CheckState checked, bool silent=false);

	void clear();
	void startModelBuilding();
	void endModelBuilding();
	void setHorizontalHeader(int index, const QString &value);

	void copyToClipboard() const;

	void setOutDated(bool value);
	bool outDated() const;
private:
	
	std::vector < Row > m_data;
	std::vector < QString> m_horizontalHeader;
	bool m_checkable;
	std::size_t m_columns;
	bool m_outOfDate;
};

#endif

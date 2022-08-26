#pragma once
#include <QSortFilterProxyModel>
#include <QRegularExpression>

class SortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    SortFilterProxyModel(QObject* parent = nullptr);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool filterAcceptsColumn(int source_column, const QModelIndex& source_parent) const override;
    bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;

public slots:
    void nameFilterChanged(const QString& text);

private:
    QRegularExpression	m_nameRegExpFilter;
};
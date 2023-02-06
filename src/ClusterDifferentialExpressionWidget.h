#pragma once

#include <QWidget>
#include <QVector>

#include "actions/OptionAction.h"
#include "actions/StringAction.h"
#include "actions/ToggleAction.h"
#include "actions/DatasetPickerAction.h"
#include "actions/TriggerAction.h"
#include "SettingsAction.h"

// DE
class ClusterDifferentialExpressionPlugin;
class QTableItemModel;
class SortFilterProxyModel;
class TableView;

class ButtonProgressBar;

// HDPS
namespace hdps
{
    class DatasetImpl;
}


// QT
class QTableView;
class QComboBox;
class QLabel;
class QStringListModel;
class QPushButton;
class QProgressBar;
class QLabel;


class ClusterDifferentialExpressionWidget : public QWidget
{
    Q_OBJECT
public:
    ClusterDifferentialExpressionWidget(ClusterDifferentialExpressionPlugin* differentialExpressionPlugin);

    
    void setData(QSharedPointer<QTableItemModel> newModel);
    QProgressBar* getProgressBar();
    void setNrOfExtraColumns(std::size_t offset);

signals:
    void selectedRowChanged(int row);

    void computeDE();
    void specialMode1();

    

private slots:

    
    void tableView_Clicked(const QModelIndex&);
    void tableView_sectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

    

public slots:
    void resizeEvent(QResizeEvent* event) override;

public:
    

    void setDatasetTooltip(qsizetype index, const QString& label);
 
private:
    void initTableViewHeader();
    void initGui();
   
    
    
    bool                                        _modelIsUpToDate;
    ClusterDifferentialExpressionPlugin*        _differentialExpressionPlugin;
    TableView*                                  _tableView;
    
    QSharedPointer<QTableItemModel>             _differentialExpressionModel;
    
    QPointer<ButtonProgressBar>                 _buttonProgressBar;
    QVector<QWidget*>                           _datasetLabels;
};
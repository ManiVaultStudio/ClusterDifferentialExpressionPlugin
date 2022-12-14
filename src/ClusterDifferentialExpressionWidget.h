#pragma once

#include <QWidget>
#include <QVector>
#include "actions/OptionAction.h"
#include "actions/StringAction.h"
#include "actions/ToggleAction.h"
#include "actions/DatasetPickerAction.h"
#include "actions/TriggerAction.h"

// DE
class ClusterDifferentialExpressionPlugin;
class QTableItemModel;
class SortFilterProxyModel;
class TableView;

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


class ClusterDifferentialExpressionWidget : public QWidget
{
    Q_OBJECT
public:
    ClusterDifferentialExpressionWidget(ClusterDifferentialExpressionPlugin* differentialExpressionPlugin);

    void setClusters1( QStringList clusters);
    void setClusters2( QStringList clusters);
    void setClusterDatasets(const QVector<hdps::Dataset<hdps::DatasetImpl>> &datasets);
    
    void setData(QTableItemModel *newModel);
    void ShowProgressBar();
    void ShowComputeButton();
    void EnableAutoCompute(bool value);
    QProgressBar* getProgressBar();
    void setNrOfExtraColumns(std::size_t offset);




signals:
    void clusters1SelectionChanged(QList<int> selectedClusters);
    void clusters1DatasetChanged(const hdps::Dataset<hdps::DatasetImpl> &dataset);
    void clusters2SelectionChanged(QList<int> selectedClusters);
    void clusters2DatasetChanged(const hdps::Dataset<hdps::DatasetImpl>& dataset);
    void selectedRowChanged(int row);

    void computeDE();
    void specialMode1();

    

private slots:
    void clusters1Selection_CurrentIndexChanged(int index);
    void clusters2Selection_CurrentIndexChanged(int index);
    void updateStatisticsButtonPressed();
    void tableView_Clicked(const QModelIndex&);
    void tableView_sectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

public slots:
    void resizeEvent(QResizeEvent* event) override;

public:
    void selectClusterDataset1(const hdps::Dataset<hdps::DatasetImpl>& dataset);
    void selectClusterDataset2(const hdps::Dataset<hdps::DatasetImpl>& dataset);
    
 
private:
    void initTableViewHeader();
    void initGui();
   

    bool                                        _modelIsUpToDate;
    ClusterDifferentialExpressionPlugin*        _differentialExpressionPlugin;
    TableView*                                  _tableView;
    DatasetPickerAction                         _clusterDataset1Action;
    DatasetPickerAction                         _clusterDataset2Action;
    hdps::gui::OptionAction                     _clusters1SelectionAction;
    hdps::gui::OptionAction                     _clusters2SelectionAction;
    hdps::gui::ToggleAction                     _autoComputeToggleAction;
    hdps::gui::StringAction                     _filterOnIdAction;
    hdps::gui::StringAction                     _selectedIdAction;
    hdps::gui::TriggerAction                    _updateStatisticsAction;
    hdps::gui::TriggerAction::PushButtonWidget* _updateStatisticsButton;
    QTableItemModel*                            _differentialExpressionModel;
    SortFilterProxyModel*                       _sortFilterProxyModel;
    QProgressBar*                               _progressBar;
    QWidget*                                    _cluster1SectionLabelWidget;
    QWidget*                                    _cluster2SectionLabelWidget;
};
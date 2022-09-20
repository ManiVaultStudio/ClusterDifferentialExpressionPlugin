#pragma once

#include <QWidget>
#include "actions/OptionAction.h"
#include "actions/StringAction.h"
// DE
class ClusterDifferentialExpressionPlugin;
class QTableItemModel;
class SortFilterProxyModel;

// HDPS
class Cluster;

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
    void setFirstClusterLabel(QString);
    void setSecondClusterLabel(QString);
    void setData(QTableItemModel *newModel);
    void ShowUpToDate();
    void ShowOutOfDate();
    QProgressBar* getProgressBar();

    

signals:
    void clusters1SelectionChanged(QList<int> selectedClusters);
    void clusters2SelectionChanged(QList<int> selectedClusters);
    void computeDE();

private slots:
    void clusters1Selection_CurrentIndexChanged(int index);
    void clusters2Selection_CurrentIndexChanged(int index);
    void updateStatisticsButtonPressed();

private:
    void initGui();
   

    bool                                    _modelIsUpToDate;
    ClusterDifferentialExpressionPlugin*    _differentialExpressionPlugin;
    QStringListModel*                       _clusterModel1;
    QTableView*                             _tableView;
    QLabel*                                  _clusters1ParentName;
    QLabel*                                  _clusters2ParentName;
    hdps::gui::StringAction                 _clusterDataset1LabelAction;
    hdps::gui::StringAction                 _clusterDataset2LabelAction;
    hdps::gui::OptionAction                _clusters1SelectionAction;
    hdps::gui::OptionAction                _clusters2SelectionAction;
    QPushButton*                            _updateStatisticsButton;
    QTableItemModel*                        _differentialExpressionModel;
    SortFilterProxyModel*                    _sortFilterProxyModel;
    QProgressBar*                            _progressBar;
    QWidget* _cluster1SectionLabelWidget;
    QWidget* _cluster2SectionLabelWidget;
    
};
#pragma once

#include <QWidget>

// DE
class ClusterDifferentialExpressionPlugin;
class QTableItemModel;

// HDPS
class Cluster;

// QT
class QTableView;
class QComboBox;
class QStringListModel;
class QPushButton;
class QProgressBar;

class ClusterDifferentialExpressionWidget : public QWidget
{
    Q_OBJECT
public:
    ClusterDifferentialExpressionWidget(ClusterDifferentialExpressionPlugin* differentialExpressionPlugin);

    void setClusters1(const QStringList& clusters);
    void setClusters2(const QStringList& clusters);
    void setData(QTableItemModel *newModel);
    void ShowUpToDate();
    void ShowOutOfDate();

signals:
    void clusters1SelectionChanged(QList<int> selectedClusters);
    void clusters2SelectionChanged(QList<int> selectedClusters);
    

private slots:
    void clusters1Selection_CurrentIndexChanged(int index);
    void clusters2Selection_CurrentIndexChanged(int index);

private:
    void initGui();
   

    bool                                    _modelIsUpToDate;
    ClusterDifferentialExpressionPlugin*    _differentialExpressionPlugin;
    QStringListModel*                       _clusterModel1;
    QTableView*                             _tableView;
    QComboBox*                              _clusters1Selection;
    QComboBox*                              _clusters2Selection;
    QPushButton*                            _updateStatisticsButton;
    QProgressBar*                           _progressBar;
    QTableItemModel*                        _differentialExpressionModel;
    
};
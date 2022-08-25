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
class QLabel;
class QStringListModel;
class QPushButton;
class QProgressBar;
class QSortFilterProxyModel;

class ClusterDifferentialExpressionWidget : public QWidget
{
    Q_OBJECT
public:
    ClusterDifferentialExpressionWidget(ClusterDifferentialExpressionPlugin* differentialExpressionPlugin);

    void setClusters1( QStringList clusters);
    void setClusters2( QStringList clusters);
    void setClusters1ParentName(QString);
    void setClusters2ParentName(QString);
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
    QComboBox*                              _clusters1Selection;
    QComboBox*                              _clusters2Selection;
    QPushButton*                            _updateStatisticsButton;
    QTableItemModel*                        _differentialExpressionModel;
    QSortFilterProxyModel*                    _sortFilterProxyModel;
    QProgressBar*                            _progressBar;
    
};
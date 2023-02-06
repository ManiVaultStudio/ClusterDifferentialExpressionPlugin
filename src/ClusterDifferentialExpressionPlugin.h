#pragma once

#include "SettingsAction.h"
#include "ProgressManager.h"


// HDPS includes
#include <ViewPlugin.h>
#include <Dataset.h>
#include "widgets/DropWidget.h"
#include "actions/VariantAction.h"



using hdps::plugin::ViewPluginFactory;
using hdps::plugin::ViewPlugin;

class ClusterDifferentialExpressionWidget;
class QTableItemModel;
class SortFilterProxyModel;

class Points;
class Clusters;


namespace hdps {
    namespace gui {
        class DropWidget;
    }
}

class ClusterDifferentialExpressionPlugin : public ViewPlugin
{
    Q_OBJECT

        typedef std::pair<QString, std::pair<std::ptrdiff_t, std::ptrdiff_t>> DimensionNameMatch;
public:
    ClusterDifferentialExpressionPlugin(const hdps::plugin::PluginFactory* factory);

    void init() override;

    void onDataEvent(hdps::DataEvent* dataEvent);

    /**
    * Load one (or more datasets in the view)
    * @param datasets Dataset(s) to load
    */
    void loadData(const hdps::Datasets& datasets) override;

    // get functions. Some might be deleted later.
    ClusterDifferentialExpressionWidget& getClusterDifferentialExpressionWidget();
  

    QSharedPointer<LoadedDatasetsAction> getLoadedDatasetsAction()
    {
        return _loadedDatasetsAction;
    }

    QPointer<SortFilterProxyModel> &getSortFiltrProxyModel()
    {
        return _sortFilterProxyModel;
    }

    QWidget *getUpdateStatisticsWidget(QWidget *parent)
    {
        return _updateStatisticsAction.createWidget(parent);
    }

    StringAction &getSelectedIdAction()
    {
        return _selectedIdAction;
    }

    SettingsAction & getSettingsAction()
    {
        return _settingsAction;
    }

    StringAction& getInfoTextAction()
    {
        return _infoTextAction;
    }

private:
    void createMeanExpressionDataset(int dataset_index, int index);

    hdps::Dataset<Clusters> &getDataset(qsizetype index)
    {
        return _loadedDatasetsAction->getDataset(index);
    }

    const QStringList& getClusterSelection(qsizetype index)
    {
        return _loadedDatasetsAction->getClusterSelection(index);
    }

    void updateWindowTitle();
    void datasetChanged(qsizetype index, const hdps::Dataset<hdps::DatasetImpl>& dataset);
   

protected slots:
    void selectedRowChanged(int index);
public slots:
    
    void selectionChanged(const QStringList&);

private:
    
    std::ptrdiff_t get_DE_Statistics_Index(hdps::Dataset<Clusters> clusterDataset);
    hdps::Dataset<Points> get_DE_Statistics_Dataset(hdps::Dataset<Clusters> clusterDataset);
    std::vector<double> computeMeanExpressionsForSelectedClusters(hdps::Dataset<Clusters> clusterDataset, const QSet<unsigned>& selected_clusters);
    bool matchDimensionNames();
    //void updateData(int index);

public slots:
   
    void computeDE();



private:
    ClusterDifferentialExpressionWidget* _differentialExpressionWidget;      /** differential expression widget providing the GUI */
    

    hdps::gui::DropWidget*          _dropWidget;    /** Widget allowing users to drop in data */
   
    SettingsAction                              _settingsAction;

    std::vector<DimensionNameMatch> _matchingDimensionNames;
    ProgressManager                 _progressManager;       /** for handling multi-threaded progress updates either to a progress bar or progress dialog */
    bool                            _identicalDimensions;
  
    QSharedPointer<QTableItemModel>   _tableItemModel;
    QPointer<SortFilterProxyModel>      _sortFilterProxyModel;

    //actions
    QSharedPointer<LoadedDatasetsAction> _loadedDatasetsAction;
    QSharedPointer<StringAction>         _filterOnIdAction;
    QSharedPointer<ToggleAction>         _autoUpdateAction;
    StringAction                         _selectedIdAction;
    TriggerAction                        _updateStatisticsAction;
    StringAction                         _infoTextAction;
    QVector<QPointer<StringAction>>       _meanExpressionDatasetGuidAction;

    // Viewer Configuration Options
    VariantAction                       _preInfoVariantAction;
    VariantAction                       _postInfoVariantAction;
};
    


class ClusterDifferentialExpressionFactory : public ViewPluginFactory
{
    Q_INTERFACES(hdps::plugin::ViewPluginFactory hdps::plugin::PluginFactory)
        Q_OBJECT
        Q_PLUGIN_METADATA(IID   "nl.BioVault.ClusterDifferentialExpressionPlugin"
            FILE  "ClusterDifferentialExpressionPlugin.json")

public:
    ClusterDifferentialExpressionFactory() {}
    ~ClusterDifferentialExpressionFactory() override {}

    /** Returns the plugin icon */
    QIcon getIcon(const QColor& color = Qt::black) const override;

    ClusterDifferentialExpressionPlugin* produce() override;

    hdps::DataTypes supportedDataTypes() const override;


	hdps::gui::PluginTriggerActions getPluginTriggerActions(const hdps::Datasets & datasets) const override;
};
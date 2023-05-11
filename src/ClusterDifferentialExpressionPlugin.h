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

class TableView;
class ButtonProgressBar;
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

        typedef std::pair<QString, std::vector<ptrdiff_t>> DimensionNameMatch;

		
public:
    ClusterDifferentialExpressionPlugin(const hdps::plugin::PluginFactory* factory);

    QString getOriginalName() const;

    void init() override;

    void onDataEvent(hdps::DataEvent* dataEvent);

    /**
    * Load one (or more datasets in the view)
    * @param datasets Dataset(s) to load
    */
    void loadData(const hdps::Datasets& datasets) override;

    
    void addConfigurableWidget(const QString& name, QWidget* widget);
    QWidget* getConfigurableWidget(const QString& name);

public: // Serialization

/**
 * Load plugin from variant map
 * @param Variant map representation of the plugin
 */
    void fromVariantMap(const QVariantMap& variantMap) override;

    /**
     * Save plugin to variant map
     * @return Variant map representation of the plugin
     */
    QVariantMap toVariantMap() const override;


private:

    void serializeAction(WidgetAction* w);
    void publishAndSerializeAction(WidgetAction* w, bool serialize=true);
    void createMeanExpressionDataset(qsizetype dataset_index, qsizetype index);

    hdps::Dataset<Clusters> &getDataset(qsizetype index)
    {
        if(_dropWidget)
			_dropWidget->setShowDropIndicator(false);
        return _loadedDatasetsAction.getDataset(index);
    }

    QStringList getClusterSelection(qsizetype index)
    {
        return _loadedDatasetsAction.getClusterSelection(index);
    }

    void updateWindowTitle();
    void datasetChanged(qsizetype index, const hdps::Dataset<hdps::DatasetImpl>& dataset);
   
    void update_pairwiseDiffExpResultsAction(qsizetype dimension, const QString& nameToCheck);

protected slots:
    void selectedRowChanged(int index);


    void newCommandsReceived(const QVariant& commands);

    void datasetAdded(int index);

    void tableView_clicked(const QModelIndex& index);
    void tableView_selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

    void writeToCSV();

public slots:
    
    void clusterSelectionChanged(const QStringList&);

private:
    
    std::ptrdiff_t get_DE_Statistics_Index(hdps::Dataset<Clusters> clusterDataset);
    hdps::Dataset<Points> get_DE_Statistics_Dataset(hdps::Dataset<Clusters> clusterDataset);
    std::vector<double> computeMeanExpressionsForSelectedClusters(hdps::Dataset<Clusters> clusterDataset, const QSet<unsigned>& selected_clusters);
    bool matchDimensionNames();
    //void updateData(int index);

public slots:
   
    void computeDE();



private:
    const QString                       _originalName;
    TableView* _tableView;
    QPointer<ButtonProgressBar>                 _buttonProgressBar;
    hdps::gui::DropWidget*          _dropWidget;    /** Widget allowing users to drop in data */
    ProgressManager                 _progressManager;       /** for handling multi-threaded progress updates either to a progress bar or progress dialog */



    SettingsAction                              _settingsAction; // not a real action

   std::vector<std::pair<QString, QVector<qsizetype>>> _matchingDimensionNames;

    bool                            _identicalDimensions;
  
    QSharedPointer<QTableItemModel>   _tableItemModel;
    QPointer<SortFilterProxyModel>      _sortFilterProxyModel;

    //actions
    LoadedDatasetsAction                 _loadedDatasetsAction;
    StringAction                         _filterOnIdAction;
    ToggleAction                         _autoUpdateAction;
    StringAction                         _selectedIdAction;
    TriggerAction                        _updateStatisticsAction;
    QVector<QPointer<StringAction>>       _meanExpressionDatasetGuidAction;
    QVector<QPointer<StringAction>>       _DE_StatisticsDatasetGuidAction;
    TriggerAction                        _copyToClipboardAction;
    TriggerAction                        _saveToCsvAction;

    // Viewer Configuration Options
    VariantAction                       _preInfoVariantAction;
    VariantAction                       _postInfoVariantAction;
    StringAction                         _infoTextAction;
    //OptionsAction                        _selectedDatasetsAction;
    QVector<WidgetAction*>              _serializedActions;

    VariantAction                       _commandAction;

    QVector<QPointer<QWidget>>          _datasetTableViewHeader;
    QMap<QString, QWidget*>             _configurableWidgets;
    QByteArray                          _headerState;
    
    VariantAction                       _pairwiseDiffExpResultsAction;

    
    
};
    


class ClusterDifferentialExpressionFactory : public ViewPluginFactory
{
        Q_OBJECT
        Q_PLUGIN_METADATA(IID   "nl.BioVault.ClusterDifferentialExpressionPlugin"
            FILE  "ClusterDifferentialExpressionPlugin.json")
        Q_INTERFACES(hdps::plugin::ViewPluginFactory hdps::plugin::PluginFactory)

public:
    ClusterDifferentialExpressionFactory() {}
    ~ClusterDifferentialExpressionFactory() override {}

    /** Returns the plugin icon */
    QIcon getIcon(const QColor& color = Qt::black) const override;

    ClusterDifferentialExpressionPlugin* produce() override;

    hdps::DataTypes supportedDataTypes() const override;


	hdps::gui::PluginTriggerActions getPluginTriggerActions(const hdps::Datasets & datasets) const override;
};
#pragma once
#include <ViewPlugin.h>
#include "widgets/DropWidget.h"

#include "ProgressManager.h"

using hdps::plugin::ViewPluginFactory;
using hdps::plugin::ViewPlugin;
using hdps::plugin::PluginFactory;

class ClusterDifferentialExpressionWidget;
class QTableItemModel;

class SettingsAction;
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
    ClusterDifferentialExpressionPlugin(const PluginFactory* factory);

    void init() override;

    void onDataEvent(hdps::DataEvent* dataEvent);

    /**
    * Load one (or more datasets in the view)
    * @param datasets Dataset(s) to load
    */
    void loadData(const hdps::Datasets& datasets) override;

protected slots:
    void clusters1Selected(QList<int> selectedClusters);
    void clusters2Selected(QList<int> selectedClusters);
    void clusterDataset1Changed(const hdps::Dataset<hdps::DatasetImpl> &dataset);
    void clusterDataset2Changed(const hdps::Dataset<hdps::DatasetImpl>& dataset);
    
private:
    
    std::ptrdiff_t get_DE_Statistics_Index(hdps::Dataset<Clusters> clusterDataset);
    std::vector<double> computeMeanExpressionsForSelectedClusters(hdps::Dataset<Clusters> clusterDataset, const QList<int>& selected_clusters);
    bool matchDimensionNames();

public slots:
    void updateData();
    void computeDE();

public: // Action getters

    SettingsAction& getSettingsAction() { return *_settingsAction; }

private:
    ClusterDifferentialExpressionWidget* _differentialExpressionWidget;      /** differential expression widget providing the GUI */
    SettingsAction*                 _settingsAction;

    hdps::gui::DropWidget*          _dropWidget;    /** Widget allowing users to drop in data */
    hdps::Dataset<Clusters>         _clusterDataset1;     /** Currently loaded clusters dataset */
    hdps::Dataset<Clusters>         _clusterDataset2;     /** Currently loaded clusters dataset */


    QList<int>                      _clusterDataset1_selected_clusters; /** Currently selected clusters in clusters Dataset 1*/
    QList<int>                      _clusterDataset2_selected_clusters; /** Currently selected clusters in clusters Dataset 2*/
    std::vector<DimensionNameMatch> _matchingDimensionNames;
    ProgressManager                 _progressManager;       /** for handling multi-threaded progress updates either to a progress bar or progress dialog */
    bool                            _identicalDimensions;
};
    


class ClusterDifferentialExpressionFactory : public ViewPluginFactory
{
    Q_INTERFACES(hdps::plugin::ViewPluginFactory hdps::plugin::PluginFactory)
        Q_OBJECT
        Q_PLUGIN_METADATA(IID   "nl.BioVault.ClusterDifferentialExpressionPlugin"
            FILE  "ClusterDifferentialExpressionPlugin.json")

public:
    ClusterDifferentialExpressionFactory() {}
    ~ClusterDifferentialExpressionFactory() {}

    /** Returns the plugin icon */
    QIcon getIcon(const QColor& color = Qt::black) const override;

    ClusterDifferentialExpressionPlugin* produce() override;

    hdps::DataTypes supportedDataTypes() const override;
};
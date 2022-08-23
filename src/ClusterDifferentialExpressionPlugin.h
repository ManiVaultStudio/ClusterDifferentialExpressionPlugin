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

public:
    ClusterDifferentialExpressionPlugin(const PluginFactory* factory);

    void init() override;

    void onDataEvent(hdps::DataEvent* dataEvent);

protected slots:
    void clusters1Selected(QList<int> selectedClusters);
    void clusters2Selected(QList<int> selectedClusters);

public slots:
    void updateData();
    void computeDE();

public: // Action getters

    SettingsAction& getSettingsAction() { return *_settingsAction; }

private:

    

    ClusterDifferentialExpressionWidget* _differentialExpressionWidget;      /** differential expression widget displaying the result */
    SettingsAction* _settingsAction;

    hdps::gui::DropWidget*      _dropWidget;    /** Widget allowing users to drop in data */
    hdps::Dataset<Clusters>     _clusters1;     /** Currently loaded clusters dataset */
    
    QList<int>                  _clusters1_selection;
    QList<int>                  _clusters2_selection;

    ProgressManager             _progressManager;
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
    QIcon getIcon() const override;

    ClusterDifferentialExpressionPlugin* produce() override;

    hdps::DataTypes supportedDataTypes() const override;
};
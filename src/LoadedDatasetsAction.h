#pragma once

#include "PluginAction.h"

#include "actions/DatasetPickerAction.h"
#include "actions/OptionsAction.h"



using namespace hdps::gui;

class Clusters;

class LoadedDatasetsAction : public PluginAction
{
protected:

    struct Data
    {
    public:
        Data() = delete;
        Data(const Data&) = default;
        ~Data() = default;

        explicit Data(LoadedDatasetsAction* parent, int index = -1);

        DatasetPickerAction	      datasetPickerAction;
        OptionsAction             clusterOptionsAction;
        hdps::Dataset<Clusters>   currentDataset;
        StringAction              datasetNameStringAction;
        
    };
    class Widget : public WidgetActionWidget {
    public:
        Widget(QWidget* parent, LoadedDatasetsAction* currentDatasetAction, const std::int32_t& widgetFlags);
    };

    QWidget* getWidget(QWidget* parent, const std::int32_t& widgetFlags) override {
        return new Widget(parent, this, widgetFlags);
    };

public:
   
    LoadedDatasetsAction(ClusterDifferentialExpressionPlugin* plugin);

    hdps::gui::OptionsAction& getClusterSelectionAction(const std::size_t index);

    hdps::Dataset<Clusters>& getDataset(std::size_t index) const;

    QStringList getClusterOptions(std::size_t index) const;

    QStringList getClusterSelection(std::size_t index) const;

    QWidget* getClusterSelectionWidget(std::size_t index, QWidget* parent, const std::int32_t& flags);

    QWidget* getDatasetNameWidget(std::size_t index, QWidget* parent, const std::int32_t& flags);

    qsizetype size() const;

protected:
    QVector<QSharedPointer<Data>> _data;
    friend class Widget;
};
#pragma once

#include "PluginAction.h"

#include "actions/DatasetPickerAction.h"
#include "actions/OptionsAction.h"
#include "actions/TriggerAction.h"
#include "actions/VariantAction.h"


using namespace hdps::gui;

class Clusters;

class LoadedDatasetsAction : public PluginAction
{
    Q_OBJECT
protected:

    struct Data : QStandardItem
    {
    public:
        Data() = delete;
       // Data(const Data&) = default;
	        
        ~Data() = default;

        explicit Data(LoadedDatasetsAction* parent, int index = -1);

        
        virtual QStandardItem* clone() const;
        virtual QVariant data(int role = Qt::UserRole + 1) const override;
        virtual void 	setData(const QVariant& value, int role = Qt::UserRole + 1) override;

        DatasetPickerAction	      datasetPickerAction;
        OptionsAction             clusterOptionsAction;
        hdps::Dataset<Clusters>   currentDataset;
        StringAction              datasetNameStringAction;
        ToggleAction              datasetSelectedAction;
    };

    class Widget : public WidgetActionWidget {
    public:
        Widget(QWidget* parent, LoadedDatasetsAction* currentDatasetAction, const std::int32_t& widgetFlags);
    };

    QWidget* getWidget(QWidget* parent, const std::int32_t& widgetFlags) override {
        return new Widget(parent, this, widgetFlags);
    };

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

public:
   
    LoadedDatasetsAction(ClusterDifferentialExpressionPlugin* plugin);

    hdps::gui::ToggleAction& getDatasetSelectedAction(const std::size_t index);
    hdps::gui::OptionsAction& getClusterSelectionAction(const std::size_t index);

    hdps::Dataset<Clusters>& getDataset(std::size_t index) const;

    QStringList getClusterOptions(std::size_t index) const;

    QStringList getClusterSelection(std::size_t index) const;

    QWidget* getClusterSelectionWidget(std::size_t index, QWidget* parent, const std::int32_t& flags);

    QWidget* getDatasetNameWidget(std::size_t index, QWidget* parent, const std::int32_t& flags);

    qsizetype size() const;

    Data* data(qsizetype index) const;


    QStandardItemModel& model();

public slots:
    void addDataset();
signals:
    void datasetAdded(int index);
    void datasetOrClusterSelectionChanged();

protected:
    TriggerAction   _addDatasetTriggerAction;
    QStandardItemModel  _model;
    //std::vector<QSharedPointer<Data>> _data;
    friend class Widget;
};
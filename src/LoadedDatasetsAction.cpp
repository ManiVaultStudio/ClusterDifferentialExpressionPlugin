#include "LoadedDatasetsAction.h"
#include "ClusterDifferentialExpressionPlugin.h"


#include <ClusterData.h>
#include <cstdint>
#include <QMenu>


using namespace hdps;
using namespace hdps::gui;


LoadedDatasetsAction::Data:: Data(LoadedDatasetsAction* parent, int index)
    :datasetPickerAction(parent, "Dataset")
    ,clusterOptionsAction(parent, "Selected Clusters")
	,datasetNameStringAction(parent, "Dataset")
{
    if(index >=0)
    {
        {
            QString datasetGuiName = QString("Dataset ") + QString::number(index + 1);

            datasetPickerAction.setText(datasetGuiName);
            datasetNameStringAction.setString(datasetGuiName);
        }

        {
            QString guiName = parent->_plugin->getGuiName() + "::";
            {
                QString datasetPickerActionName = QString("Dataset") + QString::number(index + 1);
                datasetPickerAction.setConnectionPermissionsFlag(ConnectionPermissionFlag::All);
                datasetPickerAction.publish(guiName + datasetPickerActionName);
                datasetPickerAction.setSerializationName(datasetPickerActionName);
            }

            {
                QString datasetNameStringActionName = QString("DatasetName") + QString::number(index + 1);
                datasetNameStringAction.setConnectionPermissionsFlag(ConnectionPermissionFlag::All);
                datasetNameStringAction.publish(guiName + datasetNameStringActionName);
                datasetNameStringAction.setSerializationName(datasetNameStringActionName);
            }


            {
                QString clusterOptionsActionName = QString("SelectClusters") + QString::number(index + 1);
                clusterOptionsAction.setConnectionPermissionsFlag(ConnectionPermissionFlag::All);
                clusterOptionsAction.publish(guiName + clusterOptionsActionName);
                clusterOptionsAction.setSerializationName(clusterOptionsActionName);
            }
        }
        
        
        
        
        
    }
    datasetPickerAction.setDatasetsFilterFunction([](const hdps::Datasets& datasets) -> Datasets {
        Datasets clusterDatasets;

        for (auto dataset : datasets)
            if (dataset->getDataType() == ClusterType)
                clusterDatasets << dataset;

        return clusterDatasets;
        });

    connect(&datasetPickerAction, &DatasetPickerAction::datasetPicked, [this](Dataset<hdps::DatasetImpl> pickedDataset) -> void {
        currentDataset = pickedDataset;
        });


    
    connect(&currentDataset, &Dataset<Clusters>::changed,  [this](Dataset<hdps::DatasetImpl> dataset) -> void {


        if (datasetPickerAction.getCurrentDataset() != dataset)
            datasetPickerAction.setCurrentDataset(dataset);
        //else
        {
            Dataset<Clusters> clusterDataset = dataset;

            QStringList clusterNames;
            if (clusterDataset.isValid())
            {
                auto& clusters = clusterDataset->getClusters();
                for (auto cluster : clusters)
                {
                    clusterNames.append(cluster.getName());
                }
            }
            QStringList firstItemSelectedList;
            firstItemSelectedList.append(clusterNames.first());
            clusterOptionsAction.initialize(clusterNames, firstItemSelectedList);

        
        }
        });


	currentDataset = datasetPickerAction.getCurrentDataset();
        
        
}


QVariantMap LoadedDatasetsAction::toVariantMap() const
{
    auto variantMap = PluginAction::toVariantMap();

    variantMap["LoadedDatasetsActionVersion"] = 1;
    variantMap["NrOfDatasets"] = (qsizetype) _data.size();
    for(qsizetype i =0; i < _data.size(); ++i)
    {
        QVariantMap subMap;
        _data[i]->datasetPickerAction.insertIntoVariantMap(subMap);
        _data[i]->clusterOptionsAction.insertIntoVariantMap(subMap);
        _data[i]->datasetNameStringAction.insertIntoVariantMap(subMap);

        QString key = "Data" + QString::number(i);
        variantMap[key] = subMap;
    }
    return variantMap;
    return variantMap;
}

void LoadedDatasetsAction::fromVariantMap(const QVariantMap& variantMap)
{
    PluginAction::fromVariantMap(variantMap);

    auto version = variantMap.value("LoadedDatasetsActionVersion", QVariant::fromValue(uint(0))).toUInt();
    if(version > 0)
    {
        auto found = variantMap.find("NrOfDatasets");
        if (found == variantMap.cend())
            return;

        _data.resize(found->value<qsizetype>());
        for (auto i = 0; i < _data.size(); ++i)
        {
            QString key = "Data" + QString::number(i);
            auto found = variantMap.find(key);
            if (found != variantMap.cend())
            {
                QVariantMap subMap = found->value<QVariantMap>();
                _data[i]->datasetPickerAction.fromParentVariantMap(subMap);
                _data[i]->clusterOptionsAction.fromParentVariantMap(subMap);
                _data[i]->datasetNameStringAction.fromParentVariantMap(subMap);
            }
        }
    }

    
}

LoadedDatasetsAction::LoadedDatasetsAction(ClusterDifferentialExpressionPlugin* plugin) :
    PluginAction(plugin, plugin, "Selected clusters"),
_data(2)
{

    setSerializationName("LoadedDatasets");
    for (auto i = 0; i < _data.size();++i)
        _data[i].reset(new Data(this,i));
    setIcon(hdps::Application::getIconFont("FontAwesome").getIcon("database"));
    setToolTip("Manage clusters");
   
}

hdps::gui::OptionsAction& LoadedDatasetsAction::getClusterSelectionAction(const std::size_t index)
{
    return _data.at(index)->clusterOptionsAction;
}


hdps::Dataset<Clusters>& LoadedDatasetsAction::getDataset(std::size_t index) const
{
    return _data.at(index)->currentDataset;
}

 QStringList LoadedDatasetsAction::getClusterOptions(std::size_t index) const
{
    return _data.at(index)->clusterOptionsAction.getOptions();
}

QStringList LoadedDatasetsAction::getClusterSelection(std::size_t index) const
{
    return _data.at(index)->clusterOptionsAction.getSelectedOptions();
}

QWidget* LoadedDatasetsAction::getClusterSelectionWidget(std::size_t index, QWidget *parent, const std::int32_t &flags)
{
    return _data.at(index)->clusterOptionsAction.createWidget(parent, flags);
}

QWidget* LoadedDatasetsAction::getDatasetNameWidget(std::size_t index, QWidget* parent, const std::int32_t& flags)
{
    return _data.at(index)->datasetNameStringAction.createWidget(parent, flags);
}



qsizetype LoadedDatasetsAction::size() const
{
    return _data.size();
}


LoadedDatasetsAction::Widget::Widget(QWidget* parent, LoadedDatasetsAction* currentDatasetAction, const std::int32_t& widgetFlags) :
    WidgetActionWidget(parent, currentDatasetAction)
{
    
    if (widgetFlags & PopupLayout)
    {
        setFixedWidth(600);
        auto layout = new QGridLayout();

        for(qsizetype i = 0; i < currentDatasetAction->_data.size(); ++i)
        {
            layout->addWidget(currentDatasetAction->_data[i]->datasetPickerAction.createLabelWidget(this), i, 0);
            layout->addWidget(currentDatasetAction->_data[i]->datasetPickerAction.createWidget(this), i, 1);
            layout->addWidget(currentDatasetAction->_data[i]->clusterOptionsAction.createLabelWidget(this), i, 2);
            layout->addWidget(currentDatasetAction->_data[i]->clusterOptionsAction.createWidget(this, OptionsAction::ComboBox | OptionsAction::Selection), i, 3);
        }
       

        setPopupLayout(layout);
            
    } else {

        setFixedWidth(800);
        auto layout = new QHBoxLayout();

        for (qsizetype i = 0; i < currentDatasetAction->_data.size(); ++i)
        {
            layout->addWidget(currentDatasetAction->_data[i]->datasetPickerAction.createLabelWidget(this), 12);
            layout->addWidget(currentDatasetAction->_data[i]->datasetPickerAction.createWidget(this), 12);
            layout->addWidget(currentDatasetAction->_data[i]->clusterOptionsAction.createLabelWidget(this), 12);
            layout->addWidget(currentDatasetAction->_data[i]->clusterOptionsAction.createWidget(this, OptionsAction::ComboBox | OptionsAction::Selection), 12);
        }
        layout->setContentsMargins(0, 0, 0, 0);
        setLayout(layout);
    }
}
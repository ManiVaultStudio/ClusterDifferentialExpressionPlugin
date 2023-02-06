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
        QString datasetPickerActionName = QString("Dataset ") + QString::number(index + 1);
        datasetPickerAction.setText(datasetPickerActionName);
        datasetNameStringAction.setString(datasetPickerActionName);
        QString guiName = parent->_plugin->getGuiName() + "::";
        datasetPickerAction.publish(guiName + QString("Dataset_") + QString::number(index + 1));
        datasetNameStringAction.publish(guiName + QString("DatasetName_") + QString::number(index + 1));
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
            QStringList emptyList;
            emptyList.append(clusterNames.first());
            clusterOptionsAction.initialize(clusterNames, emptyList);

        
        }
        });


	currentDataset = datasetPickerAction.getCurrentDataset();
        
        
}

LoadedDatasetsAction::LoadedDatasetsAction(ClusterDifferentialExpressionPlugin* plugin) :
    PluginAction(plugin, plugin, "Selected clusters"),
_data(2)
	
   // _dataset1PickerAction(this, "Dataset 1"),
   // _dataset2PickerAction(this, "Dataset 2"),
	//_cluster1OptionsAction(this,"Selected Clusters"),
	//_cluster2OptionsAction(this, "Selected Clusters")
{
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
#include "LoadedDatasetsAction.h"
#include "ClusterDifferentialExpressionPlugin.h"


#include <ClusterData.h>
#include <cstdint>
#include <QMenu>


using namespace hdps;
using namespace hdps::gui;


LoadedDatasetsAction::Data:: Data(LoadedDatasetsAction* parent, int index)
	:QStandardItem()
    ,datasetPickerAction(parent, "Dataset")
    ,clusterOptionsAction(parent, "Selected Clusters")
	,datasetNameStringAction(parent, "Dataset")
	,datasetSelectedAction(parent, "Active Dataset",true,true)
{
    
    
    if(index >=0)
    {
        {
            QString datasetGuiName = QString("Dataset ") + QString::number(index + 1);

            datasetPickerAction.setText(datasetGuiName);
            datasetNameStringAction.setString(datasetGuiName);
            datasetNameStringAction.setText(datasetGuiName);
            datasetSelectedAction.setText("");
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

            {
                QString actionName = QString("SelectedDataset") + QString::number(index + 1);
                datasetSelectedAction.setConnectionPermissionsFlag(ConnectionPermissionFlag::All);
                datasetSelectedAction.publish(guiName + actionName);
                datasetSelectedAction.setSerializationName(actionName);
            }
        }
        QObject::connect(&currentDataset, &Dataset<Clusters>::changed, [this](const hdps::Dataset<hdps::DatasetImpl>& dataset) -> void {this->datasetNameStringAction.setText(dataset->getGuiName()); });
        
        
       // setCheckable(true);
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

    connect(&datasetNameStringAction, &StringAction::stringChanged, [this](const QString&)->void {this->emitDataChanged(); });
    connect(&datasetSelectedAction, &ToggleAction::changed, [this]()->void {this->emitDataChanged(); });

    
    
    setFlags(Qt::ItemIsUserCheckable  | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    if(datasetSelectedAction.isChecked())
		setData(Qt::Checked, Qt::CheckStateRole);
    else
        setData(Qt::Unchecked, Qt::CheckStateRole);
    
}

QStandardItem* LoadedDatasetsAction::Data::clone() const
{
    auto* parent = qobject_cast<LoadedDatasetsAction*>(datasetPickerAction.parent());
    if(parent)
    {
        return new Data(parent, parent->size());
    }
}



QVariant LoadedDatasetsAction::Data::data(int role) const
{
    if (role == Qt::DisplayRole)
        return datasetNameStringAction.getString();
    else if (role == Qt::CheckStateRole)
        if (datasetSelectedAction.isChecked())
            return Qt::Checked;
        else
            return  Qt::Unchecked;

    return QStandardItem::data(role);
}

void LoadedDatasetsAction::Data::setData(const QVariant& value, int role)
{
    if(role == Qt::DisplayRole)
    {
        datasetNameStringAction.setString(value.toString());
        emitDataChanged();
    }
    else if (role == Qt::CheckStateRole)
    {
        if(value == Qt::Checked)
			datasetSelectedAction.setChecked(true);
        else
            datasetSelectedAction.setChecked(false);
        emitDataChanged();
    }
    else
    {
        int x = 0;
        x++;

        QStandardItem::setData(value, role);
    }
	
}

QVariantMap LoadedDatasetsAction::toVariantMap() const
{
    auto variantMap = PluginAction::toVariantMap();

    qsizetype nrOfDatasets = _model.rowCount(); // _data.size();

    variantMap["LoadedDatasetsActionVersion"] = 1;
    variantMap["NrOfDatasets"] = nrOfDatasets;
    
    for(qsizetype i =0; i < nrOfDatasets; ++i)
    {

        Data* data = dynamic_cast<Data*>(_model.item(i, 0));
        
        QVariantMap subMap;

        data->datasetPickerAction.insertIntoVariantMap(subMap);
        data->clusterOptionsAction.insertIntoVariantMap(subMap);
        data->datasetNameStringAction.insertIntoVariantMap(subMap);
        data->datasetSelectedAction.insertIntoVariantMap(subMap);
        /*
        _data[i]->datasetPickerAction.insertIntoVariantMap(subMap);
        _data[i]->clusterOptionsAction.insertIntoVariantMap(subMap);
        _data[i]->datasetNameStringAction.insertIntoVariantMap(subMap);
        */
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
        const qsizetype NrOfDatasets = found->value<qsizetype>();
        
        const qsizetype datasetsToBeAdded = NrOfDatasets - _model.rowCount();//_data.size();
        for (qsizetype i = 0; i < datasetsToBeAdded; ++i)
            addDataset();
        for (auto i = 0; i < NrOfDatasets; ++i)
        {
            QString key = "Data" + QString::number(i);
            auto found = variantMap.find(key);
            if (found != variantMap.cend())
            {
                QVariantMap subMap = found->value<QVariantMap>();
                data(i)->datasetPickerAction.fromParentVariantMap(subMap);
                data(i)->clusterOptionsAction.fromParentVariantMap(subMap);
                data(i)->datasetNameStringAction.fromParentVariantMap(subMap);
               // _data[i]->datasetPickerAction.fromParentVariantMap(subMap);
                //_data[i]->clusterOptionsAction.fromParentVariantMap(subMap);
                //_data[i]->datasetNameStringAction.fromParentVariantMap(subMap);
            }
        }
    }

    
}

LoadedDatasetsAction::LoadedDatasetsAction(ClusterDifferentialExpressionPlugin* plugin)
    : PluginAction(plugin, plugin, "Selected clusters")
//    , _data(2)
    , _addDatasetTriggerAction(nullptr, "+")
{


    _addDatasetTriggerAction.setIcon(hdps::Application::getIconFont("FontAwesome").getIcon("plus"));
    setSerializationName("LoadedDatasets");
    for (auto i = 0; i < 2; ++i)
    {
        addDataset();
    }
   // for (auto i = 0; i < _data.size();++i)
   //     _data[i].reset(new Data(this,i));
    setIcon(hdps::Application::getIconFont("FontAwesome").getIcon("database"));
    setToolTip("Manage clusters");

    connect(&_addDatasetTriggerAction, &TriggerAction::triggered, this, &LoadedDatasetsAction::addDataset);
        
}

hdps::gui::ToggleAction& LoadedDatasetsAction::getDatasetSelectedAction(const std::size_t index)
{
    return data(index)->datasetSelectedAction;
}

hdps::gui::OptionsAction& LoadedDatasetsAction::getClusterSelectionAction(const std::size_t index)
{
    return data(index)->clusterOptionsAction;
    //return _data.at(index)->clusterOptionsAction;
}


hdps::Dataset<Clusters>& LoadedDatasetsAction::getDataset(std::size_t index) const
{
    return data(index)->currentDataset;
    //return _data.at(index)->currentDataset;
}

 QStringList LoadedDatasetsAction::getClusterOptions(std::size_t index) const
{
     return data(index)->clusterOptionsAction.getOptions();
    //return _data.at(index)->clusterOptionsAction.getOptions();
}

QStringList LoadedDatasetsAction::getClusterSelection(std::size_t index) const
{
    return data(index)->clusterOptionsAction.getSelectedOptions();
    //return _data.at(index)->clusterOptionsAction.getSelectedOptions();
}

QWidget* LoadedDatasetsAction::getClusterSelectionWidget(std::size_t index, QWidget *parent, const std::int32_t &flags)
{
    return data(index)->clusterOptionsAction.createWidget(parent, flags);
    //return _data.at(index)->clusterOptionsAction.createWidget(parent, flags);
}

QWidget* LoadedDatasetsAction::getDatasetNameWidget(std::size_t index, QWidget* parent, const std::int32_t& flags)
{
    return data(index)->datasetNameStringAction.createWidget(parent, flags);
    //return _data.at(index)->datasetNameStringAction.createWidget(parent, flags);
}



qsizetype LoadedDatasetsAction::size() const
{
    return _model.rowCount();
}

LoadedDatasetsAction::Data* LoadedDatasetsAction::data(qsizetype index) const
{
    return dynamic_cast<Data*>(_model.item(index, 0));
}

QStandardItemModel& LoadedDatasetsAction::model() 
{
    return _model;
}

void LoadedDatasetsAction::addDataset()
{
    int currentSize = _model.rowCount();// _data.size();

    _model.appendRow(new Data(this, currentSize));
//    _data.resize(currentSize + 1);
//    _data[currentSize].reset(new Data(this, currentSize));
    emit datasetAdded(currentSize);
}


LoadedDatasetsAction::Widget::Widget(QWidget* parent, LoadedDatasetsAction* currentDatasetAction, const std::int32_t& widgetFlags) :
    WidgetActionWidget(parent, currentDatasetAction)
{
    
    if (true/*widgetFlags & PopupLayout*/)
    {
        setFixedWidth(600);
        auto layout = new QGridLayout();

       
        
        
        QWidget* addButton = currentDatasetAction->_addDatasetTriggerAction.createWidget(this, TriggerAction::Icon);
        addButton->setFixedWidth(addButton->height());
        layout->addWidget(addButton,0,1);
        
        
        const int offset = 1;
        connect(currentDatasetAction, &LoadedDatasetsAction::datasetAdded, this,[this,layout,offset,currentDatasetAction]()->void
        {
                int i = currentDatasetAction->size()-1;
                int column = 0;
                
                QWidget* w = currentDatasetAction->data(i)->datasetSelectedAction.createWidget(this, ToggleAction::CheckBox);
        		w->setFixedWidth(16);
                layout->addWidget(w, i + offset, column++);
                layout->addWidget(currentDatasetAction->data(i)->datasetNameStringAction.createWidget(this), i + offset, column++);
                layout->addWidget(currentDatasetAction->data(i)->datasetPickerAction.createWidget(this), i + offset, column++);
                layout->addWidget(currentDatasetAction->data(i)->clusterOptionsAction.createLabelWidget(this), i + offset, column++);
                layout->addWidget(currentDatasetAction->data(i)->clusterOptionsAction.createWidget(this, OptionsAction::ComboBox), i + 1, column++);

         
        });

       
    	for (qsizetype i = 0; i < currentDatasetAction->size(); ++i)
        {
            int column = 0;
            QWidget* w = currentDatasetAction->data(i)->datasetSelectedAction.createWidget(this, ToggleAction::CheckBox);
            w->setFixedWidth(16);
            layout->addWidget(w, i + offset, column++);
            layout->addWidget(currentDatasetAction->data(i)->datasetNameStringAction.createWidget(this), i + offset, column++);
            layout->addWidget(currentDatasetAction->data(i)->datasetPickerAction.createWidget(this), i + offset, column++);
            layout->addWidget(currentDatasetAction->data(i)->clusterOptionsAction.createLabelWidget(this), i + offset, column++);
            layout->addWidget(currentDatasetAction->data(i)->clusterOptionsAction.createWidget(this, OptionsAction::ComboBox), i + 1, column++);
        }


        

        setPopupLayout(layout);
            
    } else {

        setFixedWidth(800);
        auto layout = new QHBoxLayout();


        QComboBox* datasetSelectionComboBox = new QComboBox(this);
        datasetSelectionComboBox->setModel(&currentDatasetAction->model());
        layout->addWidget(datasetSelectionComboBox);

        QMap<int, QList<QWidget*>> map;
        
        for (qsizetype i = 0; i < currentDatasetAction->size(); ++i)
        {
            if(currentDatasetAction->data(i)->datasetSelectedAction.isChecked())
            {
                layout->addWidget(currentDatasetAction->data(i)->datasetNameStringAction.createWidget(this), 12);
                layout->addWidget(currentDatasetAction->data(i)->datasetPickerAction.createWidget(this), 12);
                layout->addWidget(currentDatasetAction->data(i)->clusterOptionsAction.createLabelWidget(this), 12);
                layout->addWidget(currentDatasetAction->data(i)->clusterOptionsAction.createWidget(this, OptionsAction::ComboBox), 12);
            }
            
        }
        layout->setContentsMargins(0, 0, 0, 0);
        setLayout(layout);
    }
}
#include "LoadedDatasetsAction.h"

#include "ClusterDifferentialExpressionPlugin.h"

#include <ClusterData/ClusterData.h>
#include <util/StyledIcon.h>

#include <cstdint>
#include <QMenu>

using namespace mv;
using namespace mv::gui;


namespace localNamespace
{
    QString toCamelCase(const QString& s, QChar c = '_') {

        QStringList parts = s.split(c, Qt::SkipEmptyParts);
        for (int i = 1; i < parts.size(); ++i)
            parts[i].replace(0, 1, parts[i][0].toUpper());

        return parts.join("");

    }

    void initializeClusterOptions( const mv::Dataset<mv::DatasetImpl>& dataset, mv::gui::OptionsAction& optionsAction)
    {
        QStringList clusterNames;

        mv::Dataset<Clusters> clusterDataset = dataset;
        if (clusterDataset.isValid())
        {
            const auto& clusters = clusterDataset->getClusters();
            for (const auto& cluster : clusters)
                clusterNames.append(cluster.getName());
        }

        QStringList initialSelection;
        if (!clusterNames.isEmpty())
            initialSelection.append(clusterNames.first());

        optionsAction.initialize(clusterNames, initialSelection);
    }
}

//LoadedDatasetsAction::Data:: Data(LoadedDatasetsAction* parent, int index)
//	:QStandardItem()
//    ,datasetPickerAction(parent, "Dataset")
//    ,clusterOptionsAction(parent, "Selected Clusters")
//	,datasetNameStringAction(parent, "Dataset")
//	,datasetSelectedAction(parent, "Active Dataset",true)
//{
LoadedDatasetsAction::Data::Data(LoadedDatasetsAction* parent, int index)
    : QStandardItem()
    , datasetPickerAction(parent, "Cluster Dataset 1")
    , clusterOptionsAction(parent, "Cluster 1")
    , intersectionDatasetPickerAction(parent, "Cluster Dataset 2")
    , intersectionClusterOptionsAction(parent, "Cluster 2")
    , useIntersectionSelectionAction(parent, "Intersect", false)
    , datasetNameStringAction(parent, "Dataset")
    , datasetSelectedAction(parent, "Active Dataset", true)
{
    
    
    if(index >=0)
    {
        {
            QString datasetGuiName = QString("Dataset ") + QString::number(index + 1);

            datasetPickerAction.setText(datasetGuiName);
            datasetNameStringAction.setString(datasetGuiName);
            datasetNameStringAction.setText(datasetGuiName);
            datasetSelectedAction.setText(" ");
        }

        {
            QString baseName = parent->_plugin->getOriginalName() + "::";
            {
                QString datasetPickerActionName = QString("Dataset") + QString::number(index + 1);
                datasetPickerAction.setConnectionPermissionsFlag(ConnectionPermissionFlag::All);
                datasetPickerAction.publish(baseName + datasetPickerActionName);
                datasetPickerAction.setSerializationName(datasetPickerActionName);
            }

            {
                QString datasetNameStringActionName = QString("DatasetName") + QString::number(index + 1);
                datasetNameStringAction.setConnectionPermissionsFlag(ConnectionPermissionFlag::All);
                datasetNameStringAction.publish(baseName + datasetNameStringActionName);
                datasetNameStringAction.setSerializationName(datasetNameStringActionName);
            }


            {
                QString clusterOptionsActionName = QString("SelectClusters") + QString::number(index + 1);
                clusterOptionsAction.setConnectionPermissionsFlag(ConnectionPermissionFlag::All);
                clusterOptionsAction.publish(baseName + clusterOptionsActionName);
                clusterOptionsAction.setSerializationName(clusterOptionsActionName);
            }



            {
                QString actionName = QString("SelectedDataset") + QString::number(index + 1);
                datasetSelectedAction.setConnectionPermissionsFlag(ConnectionPermissionFlag::All);
                datasetSelectedAction.publish(baseName + actionName);
                datasetSelectedAction.setSerializationName(actionName);
            }

            {
                const QString actionName = QString("IntersectionDataset") + QString::number(index + 1);
                intersectionDatasetPickerAction.setConnectionPermissionsFlag(ConnectionPermissionFlag::All);
                intersectionDatasetPickerAction.publish(baseName + actionName);
                intersectionDatasetPickerAction.setSerializationName(actionName);
            }

            {
                const QString actionName = QString("SelectIntersectionClusters") + QString::number(index + 1);
                intersectionClusterOptionsAction.setConnectionPermissionsFlag(ConnectionPermissionFlag::All);
                intersectionClusterOptionsAction.publish(baseName + actionName);
                intersectionClusterOptionsAction.setSerializationName(actionName);
            }

            {
                const QString actionName = QString("UseIntersectionSelection") + QString::number(index + 1);

                useIntersectionSelectionAction.setConnectionPermissionsFlag( ConnectionPermissionFlag::All);

                useIntersectionSelectionAction.publish(baseName + actionName);
                useIntersectionSelectionAction.setSerializationName(actionName);
            }


        }
        QObject::connect(&currentDataset, &Dataset<Clusters>::changed, [this](const mv::Dataset<mv::DatasetImpl>& dataset) -> void {this->datasetNameStringAction.setText(dataset->getGuiName()); });
        QObject::connect(&intersectionDataset, &Dataset<Clusters>::changed, [this](const mv::Dataset<mv::DatasetImpl>& dataset) -> void {this->datasetNameStringAction.setText(dataset->getGuiName()); });
        
        
       // setCheckable(true);
    }
 //   datasetPickerAction.setFilterFunction([](const Dataset<DatasetImpl>& dataset) -> bool {
 //       return dataset->getDataType() == ClusterType;
	//});

 //   connect(&datasetPickerAction, &DatasetPickerAction::datasetPicked, [this](Dataset<mv::DatasetImpl> pickedDataset) -> void {
 //       currentDataset = pickedDataset;
 //       });


 //   
 //   connect(&currentDataset, &Dataset<Clusters>::changed,  [this](Dataset<mv::DatasetImpl> dataset) -> void {


 //       if (datasetPickerAction.getCurrentDataset() != dataset)
 //           datasetPickerAction.setCurrentDataset(dataset);
 //       //else
 //       {
 //           Dataset<Clusters> clusterDataset = dataset;

 //           QStringList clusterNames;
 //           if (clusterDataset.isValid())
 //           {
 //               auto& clusters = clusterDataset->getClusters();
 //               for (auto cluster : clusters)
 //               {
 //                   clusterNames.append(cluster.getName());
 //               }
 //           }
 //           QStringList firstItemSelectedList;
 //           firstItemSelectedList.append(clusterNames.first());
 //           clusterOptionsAction.initialize(clusterNames, firstItemSelectedList);

 //       
 //       }
 //       });


	//currentDataset = datasetPickerAction.getCurrentDataset();

    const auto clusterDatasetFilter =
        [](const Dataset<DatasetImpl>& dataset) -> bool
        {
            return dataset->getDataType() == ClusterType;
        };

    datasetPickerAction.setFilterFunction(clusterDatasetFilter);
    intersectionDatasetPickerAction.setFilterFunction(clusterDatasetFilter);

    connect(&datasetPickerAction, &DatasetPickerAction::datasetPicked, [this](Dataset<DatasetImpl> pickedDataset) { currentDataset = pickedDataset;});

    connect(&intersectionDatasetPickerAction, &DatasetPickerAction::datasetPicked,[this](Dataset<DatasetImpl> pickedDataset) { intersectionDataset = pickedDataset; });

    connect(&currentDataset, &Dataset<Clusters>::changed, [this](Dataset<DatasetImpl> dataset)
        {
            if (datasetPickerAction.getCurrentDataset() != dataset)
                datasetPickerAction.setCurrentDataset(dataset);

            localNamespace::initializeClusterOptions(dataset, clusterOptionsAction);

            // Preserve old behaviour by using the first dataset as the
            // intersection dataset until another one is explicitly selected.
            if (!intersectionDataset.isValid() && dataset.isValid())
                intersectionDataset = dataset;
        });

    connect(&intersectionDataset, &Dataset<Clusters>::changed, [this](Dataset<DatasetImpl> dataset)
        {
            if (intersectionDatasetPickerAction.getCurrentDataset() != dataset)
                intersectionDatasetPickerAction.setCurrentDataset(dataset);

            localNamespace::initializeClusterOptions(dataset, intersectionClusterOptionsAction);
        });

    intersectionDataset = intersectionDatasetPickerAction.getCurrentDataset();
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
    return nullptr;
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

    //variantMap["LoadedDatasetsActionVersion"] = 1;
    variantMap["LoadedDatasetsActionVersion"] = 2;
    variantMap["NrOfDatasets"] = nrOfDatasets;
    
    for(qsizetype i =0; i < nrOfDatasets; ++i)
    {

        Data* data = dynamic_cast<Data*>(_model.item(i, 0));
        
        QVariantMap subMap;

        data->datasetPickerAction.insertIntoVariantMap(subMap);
        data->clusterOptionsAction.insertIntoVariantMap(subMap);
        data->datasetNameStringAction.insertIntoVariantMap(subMap);
        data->datasetSelectedAction.insertIntoVariantMap(subMap);

        data->intersectionDatasetPickerAction.insertIntoVariantMap(subMap);
        data->intersectionClusterOptionsAction.insertIntoVariantMap(subMap);
        data->useIntersectionSelectionAction.insertIntoVariantMap(subMap);
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

                data(i)->datasetSelectedAction.fromParentVariantMap(subMap);

                if (version >= 2)
                {
                    data(i)->intersectionDatasetPickerAction.fromParentVariantMap(subMap);
                    data(i)->intersectionClusterOptionsAction.fromParentVariantMap(subMap);
                    data(i)->useIntersectionSelectionAction.fromParentVariantMap(subMap);
                }
                else
                    {
                    data(i)->useIntersectionSelectionAction.setChecked(false);
                }
            }
        }
    }

    
}

LoadedDatasetsAction::LoadedDatasetsAction(ClusterDifferentialExpressionPlugin* plugin)
    : PluginAction(plugin, plugin, "Selected clusters")
//    , _data(2)
    , _addDatasetTriggerAction(nullptr, "addDataset")
{
    
    setSerializationName("LoadedDatasets");
    for (auto i = 0; i < 2; ++i)
    {
        addDataset();
    }
   // for (auto i = 0; i < _data.size();++i)
   //     _data[i].reset(new Data(this,i));
    setIcon(mv::util::StyledIcon("database"));
    setToolTip("Manage clusters");

    connect(&_addDatasetTriggerAction, &TriggerAction::triggered, this, &LoadedDatasetsAction::addDataset);
    _addDatasetTriggerAction.setIcon(mv::util::StyledIcon("plus"));
    QString name = _addDatasetTriggerAction.text();
    assert(!name.isEmpty());
    QString apiName = localNamespace::toCamelCase(name, ' ');
    _addDatasetTriggerAction.setConnectionPermissionsFlag(ConnectionPermissionFlag::All);
    _addDatasetTriggerAction.publish(plugin->getOriginalName() + "::" + apiName);
    _addDatasetTriggerAction.setSerializationName(apiName);
    
}

mv::gui::ToggleAction& LoadedDatasetsAction::getDatasetSelectedAction(const std::size_t index)
{
    return data(index)->datasetSelectedAction;
}

mv::gui::OptionsAction& LoadedDatasetsAction::getClusterSelectionAction(const std::size_t index)
{
    return data(index)->clusterOptionsAction;
    //return _data.at(index)->clusterOptionsAction;
}


mv::Dataset<Clusters>& LoadedDatasetsAction::getDataset(std::size_t index) const
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

mv::gui::OptionsAction& LoadedDatasetsAction::getIntersectionClusterSelectionAction( const std::size_t index)
{
    return data(index)->intersectionClusterOptionsAction;
}

mv::Dataset<Clusters>& LoadedDatasetsAction::getIntersectionDataset(std::size_t index) const
{
    return data(index)->intersectionDataset;
}

QStringList LoadedDatasetsAction::getIntersectionClusterOptions(std::size_t index) const
{
    return data(index)->intersectionClusterOptionsAction.getOptions();
}

QStringList LoadedDatasetsAction::getIntersectionClusterSelection( std::size_t index) const
{
    return data(index)->intersectionClusterOptionsAction.getSelectedOptions();
}

QWidget* LoadedDatasetsAction::getIntersectionClusterSelectionWidget(std::size_t index, QWidget* parent, const std::int32_t& flags)
{
    return data(index)->intersectionClusterOptionsAction.createWidget(parent, flags);
}

mv::gui::ToggleAction& LoadedDatasetsAction::getUseIntersectionSelectionAction(const std::size_t index)
{
    return data(index)->useIntersectionSelectionAction;
}

bool LoadedDatasetsAction::isIntersectionSelectionEnabled(std::size_t index) const
{
    return data(index)->useIntersectionSelectionAction.isChecked();
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
     //   setFixedWidth(600);
     //   auto layout = new QGridLayout();

     //  
     //   
     //   
     //   QWidget* addButton = currentDatasetAction->_addDatasetTriggerAction.createWidget(this, TriggerAction::Icon);
     //   addButton->setFixedWidth(addButton->height());
     //   layout->addWidget(addButton,0,1);
     //   
     //   
     //   const int offset = 1;
     //   connect(currentDatasetAction, &LoadedDatasetsAction::datasetAdded, this,[this,layout,offset,currentDatasetAction]()->void
     //   {
     //           int i = currentDatasetAction->size()-1;
     //           int column = 0;
     //           
     //           QWidget* w = currentDatasetAction->data(i)->datasetSelectedAction.createWidget(this, ToggleAction::CheckBox);
     //   		w->setFixedWidth(16);
     //           layout->addWidget(w, i + offset, column++);
     //           layout->addWidget(currentDatasetAction->data(i)->datasetNameStringAction.createWidget(this), i + offset, column++);
     //           layout->addWidget(currentDatasetAction->data(i)->datasetPickerAction.createWidget(this), i + offset, column++);
     //           layout->addWidget(currentDatasetAction->data(i)->clusterOptionsAction.createLabelWidget(this), i + offset, column++);
     //           layout->addWidget(currentDatasetAction->data(i)->clusterOptionsAction.createWidget(this, OptionsAction::ComboBox), i + 1, column++);


     //           connect(&(currentDatasetAction->data(i)->datasetNameStringAction), &StringAction::stringChanged, [currentDatasetAction]() {emit currentDatasetAction->datasetOrClusterSelectionChanged(); });
     //           connect(&(currentDatasetAction->data(i)->datasetPickerAction), &DatasetPickerAction::currentTextChanged, [currentDatasetAction]() {emit currentDatasetAction->datasetOrClusterSelectionChanged(); });
     //           connect(&(currentDatasetAction->data(i)->clusterOptionsAction), &OptionsAction::selectedOptionsChanged, [currentDatasetAction]() {emit currentDatasetAction->datasetOrClusterSelectionChanged(); });
     //    
     //   });

     //  
    	//for (qsizetype i = 0; i < currentDatasetAction->size(); ++i)
     //   {
     //       int column = 0;
     //       QWidget* w = currentDatasetAction->data(i)->datasetSelectedAction.createWidget(this, ToggleAction::CheckBox);
     //       w->setFixedWidth(16);
     //       layout->addWidget(w, i + offset, column++);
     //       layout->addWidget(currentDatasetAction->data(i)->datasetNameStringAction.createWidget(this), i + offset, column++);
     //       layout->addWidget(currentDatasetAction->data(i)->datasetPickerAction.createWidget(this), i + offset, column++);
     //       layout->addWidget(currentDatasetAction->data(i)->clusterOptionsAction.createLabelWidget(this), i + offset, column++);
     //       layout->addWidget(currentDatasetAction->data(i)->clusterOptionsAction.createWidget(this, OptionsAction::ComboBox), i + 1, column++);
     //   }


     //   
     //   setLayout(layout);
     //   //setPopupLayout(layout);



        //setFixedWidth(800);
        setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

        auto layout = new QGridLayout();

        layout->setContentsMargins(0, 0, 0, 0);
        layout->setHorizontalSpacing(4);
        layout->setVerticalSpacing(2);
        layout->setColumnStretch(0, 0);

        QWidget* addButton = currentDatasetAction->_addDatasetTriggerAction.createWidget( this, TriggerAction::Icon);

        addButton->setFixedWidth(addButton->height());
        layout->addWidget(addButton, 0, 1);

        const int offset = 1;

        const auto addDatasetRow =[this, layout, offset]( LoadedDatasetsAction* action, qsizetype index)
            {
                int column = 0;

                QWidget* selectedWidget = action->data(index)->datasetSelectedAction.createWidget( this, ToggleAction::CheckBox);

                selectedWidget->setFixedWidth(16);

                layout->addWidget(selectedWidget, index + offset, column++);

                layout->addWidget(action->data(index)->datasetNameStringAction.createWidget(this), index + offset, column++);

                layout->addWidget(action->data(index)->datasetPickerAction.createWidget(this), index + offset, column++);

                layout->addWidget(action->data(index)->clusterOptionsAction.createLabelWidget(this), index + offset, column++);

                layout->addWidget(action->data(index)->clusterOptionsAction.createWidget(this, OptionsAction::ComboBox), index + offset, column++);

               /* layout->addWidget(new QLabel(QStringLiteral("∩"), this), index + offset, column++);

                layout->addWidget(action->data(index)->overlapDatasetPickerAction.createWidget(this), index + offset, column++);

                layout->addWidget(action->data(index)->overlapClusterOptionsAction.createLabelWidget(this), index + offset, column++);

                layout->addWidget(action->data(index)->overlapClusterOptionsAction.createWidget(this, OptionsAction::ComboBox), index + offset, column++);*/

                QWidget* intersectionToggleWidget = action->data(index)->useIntersectionSelectionAction.createWidget(this, ToggleAction::CheckBox);

                layout->addWidget(intersectionToggleWidget, index + offset, column++);

                QWidget* intersectionSeparator = new QLabel(QStringLiteral("∩"), this);

                layout->addWidget(intersectionSeparator, index + offset, column++);

                QWidget* intersectionDatasetWidget = action->data(index)->intersectionDatasetPickerAction.createWidget(this);

                layout->addWidget(intersectionDatasetWidget, index + offset, column++);

                QWidget* intersectionClusterLabelWidget = action->data(index)->intersectionClusterOptionsAction.createLabelWidget(this);

                layout->addWidget(intersectionClusterLabelWidget, index + offset, column++);

                QWidget* intersectionClusterWidget = action->data(index)->intersectionClusterOptionsAction.createWidget(this, OptionsAction::ComboBox);

                layout->addWidget(intersectionClusterWidget, index + offset, column++);

                const QList<QWidget*> intersectionWidgets = { intersectionSeparator, intersectionDatasetWidget, intersectionClusterLabelWidget, intersectionClusterWidget};

                const auto updateIntersectionVisibility = [intersectionWidgets](bool enabled)
                    {
                        for (QWidget* widget : intersectionWidgets)
                        {
                            if (widget != nullptr)
                                widget->setVisible(enabled);
                        }
                    };

                updateIntersectionVisibility(action->data(index)->useIntersectionSelectionAction.isChecked());

                connect(&action->data(index)->useIntersectionSelectionAction, &ToggleAction::toggled, this, [action, updateIntersectionVisibility](bool enabled)
                {updateIntersectionVisibility(enabled);
                emit action->datasetOrClusterSelectionChanged();
                 });

                connect(&action->data(index)->datasetNameStringAction, &StringAction::stringChanged, action, [action]() {emit action->datasetOrClusterSelectionChanged(); });

                connect(&action->data(index)->datasetPickerAction, &DatasetPickerAction::currentTextChanged, action, [action]() {emit action->datasetOrClusterSelectionChanged();   });

                connect(&action->data(index)->clusterOptionsAction, &OptionsAction::selectedOptionsChanged, action, [action]() {emit action->datasetOrClusterSelectionChanged();    });

                connect(&action->data(index)->intersectionDatasetPickerAction, &DatasetPickerAction::currentTextChanged, action, [action]() {emit action->datasetOrClusterSelectionChanged();    });

                connect(&action->data(index)->intersectionClusterOptionsAction, &OptionsAction::selectedOptionsChanged, action, [action]() {emit action->datasetOrClusterSelectionChanged();  });
            };

        connect(currentDatasetAction, &LoadedDatasetsAction::datasetAdded, this,[currentDatasetAction, addDatasetRow](int index)
            {
                addDatasetRow(currentDatasetAction, index);
            });

        for (qsizetype i = 0; i < currentDatasetAction->size(); ++i)
            addDatasetRow(currentDatasetAction, i);

        setLayout(layout);
            
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

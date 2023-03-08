#include "ClusterDifferentialExpressionPlugin.h"

// CDE includes

#include "SettingsAction.h"
#include "ProgressManager.h"
#include "QTableItemModel.h"
#include "SortFilterProxyModel.h"
#include "WordWrapHeaderView.h"
#include "ButtonProgressBar.h"
#include "TableView.h"

// HDPS includes
#include "PointData.h"
#include <QDebug>
#include "ClusterData.h"
#include "event/Event.h"

#include "DataHierarchyItem.h"
#include <actions/PluginTriggerAction.h>
#include <actions/WidgetAction.h>

// QT includes

#include <QMimeData>


#include <iostream>
#include <cassert>
#include <set>
#include <algorithm>
#include <execution>

//#ifdef __APPLE__
//#include "omp.h"
////#include </opt/homebrew/opt/libomp/include/omp.h>
//#else
//#include <omp.h>
//#endif


#include <omp.h>
#include <QHeaderView>

#include "TableView.h"


Q_PLUGIN_METADATA(IID "nl.BioVault.ClusterDifferentialExpressionPlugin")
//Q_DECLARE_METATYPE(QWidget*)
using namespace hdps;
using namespace hdps::gui;
using namespace hdps::plugin;
using namespace hdps::util;


namespace local
{
    bool is_valid_QByteArray(const QByteArray &state)
    {
        QByteArray data = state;
        QDataStream stream(&data, QIODevice::ReadOnly);
        const int dataStreamVersion = QDataStream::Qt_5_0;
        stream.setVersion(dataStreamVersion);
        int marker;
        int ver;
        stream >> marker;
        stream >> ver;
		bool result =  (stream.status() == QDataStream::Ok && (ver == 0));
        return result;
    }
    template<typename T> 
    bool is_exact_type(const QVariant& variant)
    {
        auto variantType = variant.metaType();
        auto requestedType = QMetaType::fromType<T>();
        return (variantType == requestedType);
    }
    template<typename T>
    T get_strict_value(const QVariant& variant)
    {
        if(is_exact_type<T>(variant))
            return variant.value<T>();
        else
        {
            qDebug() << "Error: requested " << QMetaType::fromType<T>().name() << " but value is of type " << variant.metaType().name();
            return T();
        }
    }

    template<typename T>
    QPair<bool,T> findAndGetAs(const QVariantMap &map, const QString & key)
    {
        auto found = map.constFind(key);
        if(found != map.constEnd())
        {
	       if(found->canConvert<T>())
	       {
               return QPair<bool, T>(true, found->value<T>());
	       }
        }
        return QPair<bool, T>(false, T());
    }

    template <typename T>
    float fround(T n, int d)
    {
        assert(!std::numeric_limits<T>::is_integer); // this function should not be called on integer types
        return static_cast<float>(floor(n * pow(10., d) + 0.5) / pow(10., d));
    }


    
    bool clusterDatset_has_computed_DE_Statistics(hdps::Dataset<Clusters> clusterDataset)
    {
        if(clusterDataset.isValid())
        {
            const QString child_DE_Statistics_DatasetName = "DE_Statistics";
            const auto& childDatasets = clusterDataset->getChildren({ PointType });
            for (qsizetype i = 0; i < childDatasets.size(); ++i)
            {
                if (childDatasets[i]->getGuiName() == child_DE_Statistics_DatasetName)
                {
                    return true;

                }
            }
        }
        return false;
    }

    bool clusterDataset_has_parent_Point_Dataset(hdps::Dataset<Clusters> clusterDataset)
    {
        return clusterDataset->getParent<Points>().isValid();
    }


    std::ptrdiff_t get_DE_Statistics_Index(hdps::Dataset<Clusters> clusterDataset)
    {
        const auto& clusters = clusterDataset->getClusters();
        const auto numClusters = clusters.size();

        hdps::Dataset<Points> points = clusterDataset->getParent<Points>();
        const std::ptrdiff_t numDimensions = points->getNumDimensions();

        // check if the basic DE_Statistics for the cluster dataset has already been computed
        std::ptrdiff_t child_DE_Statistics_DatasetIndex = -1;
        const QString child_DE_Statistics_DatasetName = "DE_Statistics";
        {
            const auto& childDatasets = clusterDataset->getChildren({ PointType });
            for (qsizetype i = 0; i < childDatasets.size(); ++i)
            {
                if (childDatasets[i]->getGuiName() == child_DE_Statistics_DatasetName)
                {
                    child_DE_Statistics_DatasetIndex = i;
                    break;
                }
            }
        }

        // if they are not available compute them now
        if (child_DE_Statistics_DatasetIndex < 0)
        {


            //compute the DE statistics for this cluster

            const auto numPoints = points->getNumPoints();

            std::vector<float> meanExpressions(numClusters * numDimensions, 0);

            int x = omp_get_max_threads();
            int y = omp_get_num_threads();

           
            points->visitData([&clusters, &meanExpressions, numClusters, numDimensions](auto vec)
                {

#pragma omp parallel for schedule(dynamic, 1)
                    for (int dimension = 0; dimension < numDimensions; ++dimension)
                    {
                        //#pragma omp parallel for schedule(dynamic, 1)
                        for (std::ptrdiff_t clusterIdx = 0; clusterIdx < numClusters; ++clusterIdx)
                        {
                            const auto& cluster = clusters[clusterIdx];
                            const auto& clusterIndices = cluster.getIndices();
                            std::size_t offset = (clusterIdx * numDimensions) + dimension;
                            for (auto row : clusterIndices)
                            {
                                meanExpressions[offset] += vec[row][dimension];
                            }
                            meanExpressions[offset] /= clusterIndices.size();
                        }
                       
                    }
                });

            





            auto *core = Application::core();
            hdps::Dataset<Points> newDataset = core->addDataset("Points", child_DE_Statistics_DatasetName, clusterDataset);
            events().notifyDatasetAdded(newDataset);
            newDataset->setDataElementType<float>();
            newDataset->setData(std::move(meanExpressions), numDimensions);
            newDataset->setDimensionNames(points->getDimensionNames());

            events().notifyDatasetChanged(newDataset);


            // now fild the child indices for this dataset
            child_DE_Statistics_DatasetIndex = -1;
            {
                const auto& childDatasets = clusterDataset->getChildren({ PointType });
                for (qsizetype i = 0; i < childDatasets.size(); ++i)
                {
                    if (childDatasets[i]->getGuiName() == child_DE_Statistics_DatasetName)
                    {
                        child_DE_Statistics_DatasetIndex = i;
                        break;
                    }
                }
            }

        }

        return child_DE_Statistics_DatasetIndex;
    }

    Dataset<Points> get_DE_Statistics_Dataset(hdps::Dataset<Clusters> clusterDataset)
    {
        auto clusterDataset_DE_Statitstics_Index = get_DE_Statistics_Index(clusterDataset);
        assert(clusterDataset_DE_Statitstics_Index >= 0);
        const auto& clusterDataset_Children = clusterDataset->getChildren({ PointType });
        Dataset<Points> DE_Statistics = clusterDataset_Children[clusterDataset_DE_Statitstics_Index];

        return DE_Statistics;
    }

    QSet<unsigned> getClusterIndices(const QStringList &clusters, const QStringList &selection)
    {
        QSet<unsigned> result;
	    for(auto s : selection)
	    {
            qsizetype index = clusters.indexOf(s);
            if (index != -1)
                result.insert(index);
	    }
        return result;
    }

    QString getFullGuiName(const Dataset<DatasetImpl> &dataset)
	{
        QString text;
        if (dataset.isValid())
        {
            Dataset<Points> points = dataset->getParent<Points>();
            if (points.isValid())
                text = points->getGuiName() + "\\";
            text += dataset->getGuiName();
        }
        else
        {
            text = "Unknown";
        }
        return text;
	}


    QString fromCamelCase(const QString& s, QChar c='_') {

        static QRegularExpression regExp1{ "(.)([A-Z][a-z]+)" };
        static QRegularExpression regExp2{ "([a-z0-9])([A-Z])" };

        QString result = s;
        
        QString s2("\\1");
    	s2 += QString(c);
    	s2+= "\\2";
        result.replace(regExp1, s2);
        result.replace(regExp2, s2);

        return result.toLower();

    }

    QString toCamelCase(const QString& s, QChar c='_') {

        QStringList parts = s.split(c, Qt::SkipEmptyParts);
        for (int i = 1; i < parts.size(); ++i)
            parts[i].replace(0, 1, parts[i][0].toUpper());

        return parts.join("");

    }

}
ClusterDifferentialExpressionPlugin::ClusterDifferentialExpressionPlugin(const hdps::plugin::PluginFactory* factory)
    : ViewPlugin(factory)

    
    , _dropWidget(nullptr)
    , _identicalDimensions(false)
    , _preInfoVariantAction(this, "TableViewLeftSideInfo")
    , _postInfoVariantAction(this, "TableViewRightSideInfo")
    , _settingsAction(this)
    , _loadedDatasetsAction(this)
    , _filterOnIdAction(this, "Filter on Id")
    , _autoUpdateAction(this, "auto update", false, false)
    , _selectedIdAction(this, "Last selected Id")
    , _updateStatisticsAction(this, "Calculate Differential Expression")
    , _sortFilterProxyModel(new SortFilterProxyModel)
    , _tableItemModel(new QTableItemModel(nullptr, false))
    , _infoTextAction(this, "IntoText")
    , _commandAction(this, "InvokeMethods")
	, _tableView(nullptr)
	, _buttonProgressBar(nullptr)
	//, _selectedDatasetsAction(this)
	

{
    setSerializationName(getGuiName());

    _sortFilterProxyModel->setSourceModel(_tableItemModel.get());
    _filterOnIdAction.setSearchMode(true);
    _filterOnIdAction.setClearable(true);
    _filterOnIdAction.setPlaceHolderString("Filter by ID");
    
    _updateStatisticsAction.setCheckable(false);
    _updateStatisticsAction.setChecked(false);
    
    
    publishAndSerializeAction(&_preInfoVariantAction);
    publishAndSerializeAction(&_postInfoVariantAction);
    publishAndSerializeAction(&_filterOnIdAction);
    publishAndSerializeAction(&_selectedIdAction);
    publishAndSerializeAction(&_updateStatisticsAction);
    publishAndSerializeAction(&_infoTextAction);
    publishAndSerializeAction(&_autoUpdateAction);
    publishAndSerializeAction(&_commandAction, false);
    _serializedActions.append(&_loadedDatasetsAction);
    
    
    connect(&_filterOnIdAction, &hdps::gui::StringAction::stringChanged, _sortFilterProxyModel, &SortFilterProxyModel::nameFilterChanged);

    connect(&_updateStatisticsAction, &hdps::gui::TriggerAction::triggered, this, &ClusterDifferentialExpressionPlugin::computeDE);


    //_settingsAction.addAction(_filterOnIdAction,100);
    _settingsAction.addAction(_loadedDatasetsAction, 1);

    _autoUpdateAction.setIcon(hdps::Application::getIconFont("FontAwesome").getIcon("check"));
    _settingsAction.addAction(_autoUpdateAction, 5);


    _meanExpressionDatasetGuidAction.reserve(_loadedDatasetsAction.size());
    _DE_StatisticsDatasetGuidAction.reserve(_loadedDatasetsAction.size());
    for (qsizetype i = 0; i < _loadedDatasetsAction.size(); ++i)
    {
        datasetAdded(i);
    }

    
    connect(&_commandAction, &VariantAction::variantChanged, this, &ClusterDifferentialExpressionPlugin::newCommandsReceived);

    connect(&_loadedDatasetsAction, &LoadedDatasetsAction::datasetAdded, this, &ClusterDifferentialExpressionPlugin::datasetAdded);

    //_selectedDatasetsAction.setOptionsModel(&_loadedDatasetsAction.model());
}



void ClusterDifferentialExpressionPlugin::init()
{
    QWidget& mainWidget = getWidget();
    auto mainLayout = new QGridLayout();
    delete mainWidget.layout();
    mainWidget.setLayout(mainLayout);

    mainWidget.setAcceptDrops(true);
    mainWidget.setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

    const int margin = 0;
    mainLayout->setContentsMargins(margin, margin, margin, margin);
    mainLayout->setSpacing(0);

    int currentRow = 0;
    { // toolbar
        QWidget* filterWidget = _filterOnIdAction.createWidget(&mainWidget);
        filterWidget->setContentsMargins(0, 3, 0, 3);
        addConfigurableWidget("FilterOnId", filterWidget);


      //  QWidget* selectedDatasetsWidget = _selectedDatasetsAction.createWidget(&mainWidget);
      //  selectedDatasetsWidget->setContentsMargins(0, 3, 0, 3);

        QWidget* settingsWidget = _settingsAction.createWidget(&mainWidget);
        addConfigurableWidget("LoadedDataSettings", settingsWidget);

        QHBoxLayout* toolBarLayout = new QHBoxLayout;
        toolBarLayout->addWidget(filterWidget);
     //   toolBarLayout->addWidget(selectedDatasetsWidget);
        toolBarLayout->addWidget(settingsWidget);
        mainLayout->addLayout(toolBarLayout, currentRow, 0);
        mainLayout->setRowStretch(currentRow++, 1);
    }

    { // table view

        _tableView = new TableView(&mainWidget);
        _tableView->setModel(_sortFilterProxyModel);
        _tableView->setSortingEnabled(true);
        _tableView->setSelectionMode(QAbstractItemView::SingleSelection);
        _tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        _tableView->setContextMenuPolicy(Qt::CustomContextMenu);
        //_tableView->setStyleSheet("QTableView::item:selected { background-color: #2c56ba; }");
        addConfigurableWidget("TableView", _tableView);
        connect(_tableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ClusterDifferentialExpressionPlugin::tableView_selectionChanged);


        WordWrapHeaderView* horizontalHeader = new WordWrapHeaderView(Qt::Horizontal, _tableView, true);
        //horizontalHeader->setStretchLastSection(true);
        horizontalHeader->setFirstSectionMovable(false);
        horizontalHeader->setSectionsMovable(true);
        horizontalHeader->setSectionsClickable(true);
        horizontalHeader->sectionResizeMode(QHeaderView::Stretch);
        horizontalHeader->setSectionResizeMode(QHeaderView::Stretch);
        horizontalHeader->setStretchLastSection(true);
        horizontalHeader->setSortIndicator(1, Qt::AscendingOrder);
        horizontalHeader->setDefaultAlignment(Qt::AlignBottom | Qt::AlignLeft | Qt::Alignment(Qt::TextWordWrap));
        _tableView->setHorizontalHeader(horizontalHeader);
        
        mainLayout->addWidget(_tableView, currentRow, 0);
        mainLayout->setRowStretch(currentRow++, 97);
    }

    { // infoWidget

        QLabel* infoTextWidget = new QLabel(&mainWidget);
        infoTextWidget->setContentsMargins(0, 3, 0, 3);
       
        const auto infoTextString = _infoTextAction.getString();
        infoTextWidget->setText(infoTextString);
        infoTextWidget->setVisible(!infoTextString.isEmpty());

        connect(&_infoTextAction, &StringAction::stringChanged, [infoTextWidget](const QString& text)
            {
                infoTextWidget->setText(text);
                infoTextWidget->setVisible(!text.isEmpty());
            });


        /*
        const auto debug = [infoTextWidget](bool b) -> void
        {
            bool x = b;
            infoTextWidget->setVisible(b);
        };
        */

        mainLayout->addWidget(infoTextWidget, currentRow, 0);
        mainLayout->setRowStretch(currentRow++, 1);
    }

    {// Progress bar and update button

        _buttonProgressBar = new ButtonProgressBar(&mainWidget, _updateStatisticsAction.createWidget(&mainWidget));
        _buttonProgressBar->setContentsMargins(0, 3, 0, 3);
        _buttonProgressBar->setProgressBarText("No Data Available");
        _buttonProgressBar->setButtonText("No Data Available", Qt::red);
        
        

        connect(_tableItemModel.get(), &QTableItemModel::statusChanged, _buttonProgressBar, &ButtonProgressBar::showStatus);

        mainLayout->addWidget(_buttonProgressBar, currentRow, 0);
        mainLayout->setRowStretch(currentRow++, 1);

    }

    { // dropwidget
        _dropWidget = new gui::DropWidget(&mainWidget);


        _dropWidget->setDropIndicatorWidget(new gui::DropWidget::DropIndicatorWidget(&getWidget(), "No data loaded", "Drag an item from the data hierarchy and drop it here to visualize data..."));
        _dropWidget->initialize([this](const QMimeData* mimeData) -> gui::DropWidget::DropRegions {
            gui::DropWidget::DropRegions dropRegions;

            const auto mimeText = mimeData->text();
            const auto tokens = mimeText.split("\n");

            if (tokens.count() == 1)
                return dropRegions;

            const auto datasetGuid = tokens[1];
            const auto dataType = DataType(tokens[2]);
            const auto dataTypes = DataTypes({ ClusterType });

            if (!dataTypes.contains(dataType))
                dropRegions << new gui::DropWidget::DropRegion(this, "Incompatible data", "This type of data is not supported", "exclamation-circle", false);

            if (dataType == ClusterType) {
                const auto candidateDataset = _core->requestDataset<Clusters>(datasetGuid);
                const auto description = QString("Select Clusters %1").arg(candidateDataset->getGuiName());

                if (!getDataset(0).isValid())
                {
                    dropRegions << new gui::DropWidget::DropRegion(this, "First Clusters Dataset", description, "th-large", true, [this, candidateDataset]() {

                        // _dropWidget->setShowDropIndicator(false);
                        getDataset(0) = candidateDataset;
                        getDataset(1) = candidateDataset;
                        });
                }
                else
                {
                    dropRegions << new gui::DropWidget::DropRegion(this, " First Clusters Dataset", description, "th-large", true, [this, candidateDataset]() {

                        //  _dropWidget->setShowDropIndicator(false);
                        getDataset(0) = candidateDataset;
                        });

                    dropRegions << new gui::DropWidget::DropRegion(this, " Second Clusters Dataset", description, "th-large", true, [this, candidateDataset]() {


                        //  _dropWidget->setShowDropIndicator(false);
                        getDataset(1) = candidateDataset;
                        });


                }
            }

            return dropRegions;
            });
    }


    _progressManager.setProgressBar(_buttonProgressBar->getProgressBar());
}


void ClusterDifferentialExpressionPlugin::onDataEvent(hdps::DataEvent* dataEvent)
{
    // Event which gets triggered when a dataset is added to the system.
    if (dataEvent->getType() == EventType::DataAdded)
    {
    //    _differentialExpressionWidget->addDataOption(dataEvent->getDataset()->getGuiName());
    }
    // Event which gets triggered when the data contained in a dataset changes.
    if (dataEvent->getType() == EventType::DataChanged)
    {
        //dataEvent->getDataset()
    }
}

void ClusterDifferentialExpressionPlugin::loadData(const hdps::Datasets& datasets)
{
    // Exit if there is nothing to load
    if (datasets.isEmpty())
        return;

    // Load the first dataset
    getDataset(0) = datasets.first();

    if (datasets.size() > 1)
        getDataset(1) = datasets[1];
}

void ClusterDifferentialExpressionPlugin::addConfigurableWidget(const QString& name, QWidget* widget)
{
    if (!_configurableWidgets.contains(name))
        _configurableWidgets[name] = widget;
}

QWidget* ClusterDifferentialExpressionPlugin::getConfigurableWidget(const QString& name)
{
    if (_configurableWidgets.contains(name))
        return _configurableWidgets[name];
    return nullptr;
}

void ClusterDifferentialExpressionPlugin::fromVariantMap(const QVariantMap& variantMap)
{
    ViewPlugin::fromVariantMap(variantMap);
    auto version = variantMap.value("ClusterDifferentialExpressionPluginVersion", QVariant::fromValue(uint(0))).toUInt();
    if(version > 0)
    {
        for (auto action : _serializedActions)
        {
            if(variantMap.contains(action->getSerializationName()))
				action->fromParentVariantMap(variantMap);

        }
    }

    
    if(version > 1)
    {

    	QVariantMap propertiesMap = local::get_strict_value<QVariantMap>(variantMap.value("#Properties"));
        if(!propertiesMap.isEmpty())
        {
            {
                auto found = propertiesMap.constFind("TableViewHeaderState");
                if (found != propertiesMap.constEnd())
                {

                    QVariant value = found.value();
                    QString stateAsQString = local::get_strict_value<QString>(value);
                    // When reading a QByteArray back from jsondocument it's a QString. to Convert it back to a QByteArray we need to use .toUtf8().
                    QByteArray state = QByteArray::fromBase64(stateAsQString.toUtf8());
                    assert(local::is_valid_QByteArray(state));
                	_headerState = state;
                }
            }
            
        }
    }

    
    
}

QVariantMap ClusterDifferentialExpressionPlugin::toVariantMap() const
{
    QVariantMap variantMap = ViewPlugin::toVariantMap();
    variantMap["ClusterDifferentialExpressionPluginVersion"] = 2;
    for(auto action : _serializedActions)
    {
        assert(action->getSerializationName()!="#Properties");
        action->insertIntoVariantMap(variantMap);
    }
    
    // properties map
    QVariantMap propertiesMap;

    QByteArray headerState = _tableView->horizontalHeader()->saveState();
    propertiesMap["TableViewHeaderState"] = QString::fromUtf8(headerState.toBase64()); // encode the state with toBase64() and put it in a Utf8 QString since it will do that anyway. Best to be explicit in case it changes in the future
    variantMap["#Properties"] = propertiesMap;
    
    
    return variantMap;
    
}



void ClusterDifferentialExpressionPlugin::publishAndSerializeAction(WidgetAction* w, bool serialize)
{
    assert(w != nullptr);
    if(w==nullptr)
        return;
    QString name = w->text();
    assert(!name.isEmpty());
    QString apiName = local::toCamelCase(name,' ');
    w->setConnectionPermissionsFlag(ConnectionPermissionFlag::All);
    w->publish(getGuiName() + "::" + apiName);
    w->setSerializationName(apiName);
    if(serialize)
		_serializedActions.append(w);
}

void ClusterDifferentialExpressionPlugin::createMeanExpressionDataset(qsizetype dataset_index, qsizetype index)
{
    
    assert(dataset_index >= 0);
    const auto& clusters = getDataset(dataset_index)->getClusters();
    std::size_t nrOfClusters = clusters.size();
    std::size_t nrOfPoints = 0;
    for (auto cluster : clusters)
    {
        nrOfPoints += cluster.getNumberOfIndices();
    }
    std::vector<float> meanExpressionData(nrOfPoints,std::numeric_limits<float>::quiet_NaN());

    if(index >=0)
    {
        assert(_identicalDimensions || (!_matchingDimensionNames.empty()));

        qsizetype de_Statistics_dimension = _identicalDimensions ? index : _matchingDimensionNames[index].second[dataset_index];

        if (de_Statistics_dimension >= 0)
        {
            auto de_Statistics_clusterDataset = get_DE_Statistics_Dataset(getDataset(dataset_index));
            const Points* p = de_Statistics_clusterDataset.get();
            for (qsizetype clusterIndex = 0; clusterIndex < clusters.size(); ++clusterIndex)
            {
                std::size_t point_index = (clusterIndex * p->getNumDimensions()) + de_Statistics_dimension;
                float value = p->getValueAt(point_index);
                const auto& clusterIndices = clusters[clusterIndex].getIndices();
                for (auto i : clusterIndices)
                {
                    meanExpressionData[i] = value;
                }
            }
        }
    }
    
    QString meanExpressionDatasetGuid = _meanExpressionDatasetGuidAction[dataset_index]->getString();
    Dataset<Points> meanExpressionDataset = _core->requestDataset(meanExpressionDatasetGuid);
    meanExpressionDataset->setData(meanExpressionData, 1);
    events().notifyDatasetChanged(meanExpressionDataset);
       
   
}

void ClusterDifferentialExpressionPlugin::updateWindowTitle()
{
    QString windowTitle = getGuiName() + ": ";
    for(qsizetype i =0; i < _loadedDatasetsAction.size(); ++i)
    {
        if (i > 0)
            windowTitle += " vs ";
        windowTitle += local::getFullGuiName(getDataset(i));
    }
    getWidget().setWindowTitle(windowTitle);
}

void ClusterDifferentialExpressionPlugin::datasetChanged(qsizetype index, const hdps::Dataset<hdps::DatasetImpl>& dataset)
{
    _tableItemModel->invalidate();
    
    updateWindowTitle();

    _identicalDimensions = false;
    _matchingDimensionNames.clear();

    
    createMeanExpressionDataset(index, -1);

    if (index == 0 && !(getDataset(1).isValid()))
        getDataset(1) = dataset;
    else
    {
        if(_autoUpdateAction.isChecked())
        {
            bool condition = true;
            for (qsizetype i = 0; condition && (i < _loadedDatasetsAction.size()); ++i)
            {
                condition &= (local::clusterDatset_has_computed_DE_Statistics(_loadedDatasetsAction.getDataset(i)));
            }
            if (condition)
                computeDE();
        }
    }
}




void ClusterDifferentialExpressionPlugin::clusterSelectionChanged(const QStringList&)
{
    if(_autoUpdateAction.isChecked())
    {
        bool condition = true;
        for (qsizetype i = 0; condition && (i < _loadedDatasetsAction.size()); ++i)
        {
            condition &= (local::clusterDatset_has_computed_DE_Statistics(_loadedDatasetsAction.getDataset(i)));
        }
        if (condition)
        {
            computeDE();
            return;
        }
    }
	if (_tableItemModel)
		_tableItemModel->invalidate();
}




void ClusterDifferentialExpressionPlugin::selectedRowChanged(int index)
{
    if (index < 0)
        return;

    for(qsizetype i=0; i < _loadedDatasetsAction.size(); ++i)
    {
        createMeanExpressionDataset(i, index);
    }
    


    /*
    QVariantList commands;
    {
        QVariantList command;
        command << QString("TableView") << QString("SLOT_setColumnWidth") << int(1) << int(20);
        commands.push_back(command);
    }
    {
        QVariantList command;
        command << QString("TableView") << QString("hideColumn") << int(2);
        commands.push_back(command);

    }


    {
        QVariantList command;
        command << QString("TableViewClusterSelection1") << QString("setDisabled") << bool(true);
        commands.push_back(command);

    }

    commands.push_back(QString());
    
	_commandAction.setVariant(commands);
	*/

}


void ClusterDifferentialExpressionPlugin::datasetAdded(int index)
{
    connect(&_loadedDatasetsAction.getDataset(index), &Dataset<Clusters>::changed, this, [this, index](const hdps::Dataset<hdps::DatasetImpl>& dataset) {datasetChanged(index, dataset); });
    connect(&_loadedDatasetsAction.getClusterSelectionAction(index), &OptionsAction::selectedOptionsChanged, this, &ClusterDifferentialExpressionPlugin::clusterSelectionChanged);
    connect(&_loadedDatasetsAction.getDatasetSelectedAction(index), &ToggleAction::toggled, this, [this, index](bool)
        {
            const hdps::Dataset<hdps::DatasetImpl>& dataset = this->_loadedDatasetsAction.getDataset(index);
            datasetChanged(index, dataset);
        });

    _meanExpressionDatasetGuidAction.resize(_loadedDatasetsAction.size(), nullptr);
    std::vector<float> meanExpressionData(1, 0);
    const QString guiName = getGuiName();
    
   
    {
        QString actionName = "SelectedIDMeanExpressionsDataset " + QString::number(index);
        _meanExpressionDatasetGuidAction[index] = new StringAction(this, "SelectedIDMeanExpressionsDataset " + QString::number(index));
        publishAndSerializeAction(_meanExpressionDatasetGuidAction[index]);

        QString datasetName = guiName + QString("::") + actionName;

        auto& allDatasets = _core->getDataManager().allSets();
        bool found = false;
        for (auto d = allDatasets.cbegin(); d != allDatasets.cend(); ++d)
        {
            if (d->isValid() && ((*d)->getGuiName() == datasetName))
            {
                _meanExpressionDatasetGuidAction[index]->setString(d->getDatasetGuid());
                Dataset<Points> meanExpressionDataset = *d;
                meanExpressionDataset->setData(meanExpressionData, 1);
                found = true;
                break;
            }
        }
        if (!found)
        {
            Dataset<Points> meanExpressionDataset = _core->addDataset("Points", datasetName);
            _meanExpressionDatasetGuidAction[index]->setString(meanExpressionDataset.getDatasetGuid());
            meanExpressionDataset->setData(meanExpressionData, 1);
        }
    }

    {
        _DE_StatisticsDatasetGuidAction.resize(_loadedDatasetsAction.size(), nullptr);
        //for (qsizetype i = 0; i < _DE_StatisticsDatasetGuidAction.size(); ++i)
        {
            QString actionName = "DE_ExpressionsDataset " + QString::number(index);
            _DE_StatisticsDatasetGuidAction[index] = new StringAction(this, "DE_StatisticsDataset " + QString::number(index));
            publishAndSerializeAction(_DE_StatisticsDatasetGuidAction[index]);
        }
    }
    
    //_differentialExpressionWidget->addDatasetToHeader(index, false);

    _datasetTableViewHeader.resize(_loadedDatasetsAction.size());
    {
        QWidget* clusterHeaderWidget = new QWidget;
        QGridLayout* clusterHeaderWidgetLayout = new QGridLayout(clusterHeaderWidget);
        clusterHeaderWidgetLayout->setContentsMargins(1, 1, 1, 1);
        clusterHeaderWidgetLayout->setHorizontalSpacing(0);
        clusterHeaderWidgetLayout->setVerticalSpacing(0);
        clusterHeaderWidgetLayout->setSizeConstraint(QLayout::SetFixedSize);

        QWidget* datasetNameWidget = _loadedDatasetsAction.getDatasetNameWidget(index, clusterHeaderWidget, 1);
        clusterHeaderWidgetLayout->addWidget(datasetNameWidget, 0, 0, Qt::AlignTop);
        _configurableWidgets[QString("TableViewDatasetName") + QString::number(index + 1)] = datasetNameWidget;
        QWidget* widget = _loadedDatasetsAction.getClusterSelectionWidget(index, clusterHeaderWidget, 1);
        _configurableWidgets[QString("TableViewClusterSelection") + QString::number(index + 1)] = widget;
        clusterHeaderWidgetLayout->addWidget(widget, 1, 0, Qt::AlignTop);
        clusterHeaderWidgetLayout->addWidget(new QLabel("Mean", clusterHeaderWidget), 2, 0, Qt::AlignLeft);


    	clusterHeaderWidget->setLayout(clusterHeaderWidgetLayout);
        clusterHeaderWidget->setParent(nullptr);
        clusterHeaderWidget->hide();
        _datasetTableViewHeader[index]= clusterHeaderWidget;
    }

    
   
    
}

void ClusterDifferentialExpressionPlugin::tableView_clicked(const QModelIndex& index)
{
    if (_tableItemModel->status() != QTableItemModel::Status::UpToDate)
        return;
    try
    {
        QModelIndex firstColumn = index.sibling(index.row(), 0);

        QString selectedGeneName = firstColumn.data().toString();
        QModelIndex temp = _sortFilterProxyModel->mapToSource(firstColumn);
        auto row = temp.row();
       _selectedIdAction.setString(selectedGeneName);
        emit selectedRowChanged(row);
    }
    catch (...)
    {
        // catch everything
    }
}

void ClusterDifferentialExpressionPlugin::tableView_selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    tableView_clicked(selected.indexes().first());
}

void ClusterDifferentialExpressionPlugin::newCommandsReceived(const QVariant& variant)
{
    if (!variant.isValid())
    {
        _commandAction.setVariant(bool(false)); // return false
        return;
    }

    QVariantList commands = local::get_strict_value<QVariantList>(variant);
    if (commands.isEmpty())
    {
        _commandAction.setVariant(bool(false)); // return false
        return;
    }

    enum { OBJECT_ID = 0, METHOD_ID = 1, ARGUMENT_OFFSET = 2 };
    qsizetype successfulCommands = 0;
    for (auto item : commands)
    {
        QVariantList  command = local::get_strict_value<QVariantList>(item);

        if (command.size() >= 2)
        {
            QString objectID = local::get_strict_value<QString>(command[OBJECT_ID]);
            if (!objectID.isEmpty())
            {
                QObject* object = this->getConfigurableWidget(objectID);
                if (object)
                {
                    QString method = local::get_strict_value<QString>(command[METHOD_ID]);
                    if (!method.isEmpty())
                    {
                        QString method_signature = method + '(';
                        for (qsizetype i = ARGUMENT_OFFSET; i < command.size(); ++i)
                        {
                            if (i > ARGUMENT_OFFSET)
                                method_signature += ',';
                            method_signature += command[i].typeName();
                        }
                        method_signature += ')';

                        QString message = QString("calling ") + objectID + "->" + method_signature + " ";
                        if ((object->metaObject()->indexOfMethod(method_signature.toLocal8Bit().data()) != -1))
                        {
                            const qsizetype nrOfArguments = command.size() - ARGUMENT_OFFSET;
                            bool result = false;
                            switch (nrOfArguments)
                            {
                            case 0:  result = QMetaObject::invokeMethod(object, method.toLocal8Bit().data(), Qt::DirectConnection); break;
                            case 1:  result = QMetaObject::invokeMethod(object, method.toLocal8Bit().data(), Qt::DirectConnection, QGenericArgument(command[ARGUMENT_OFFSET + 0].typeName(), command[ARGUMENT_OFFSET + 0].data()));  break;
                            case 2:  result = QMetaObject::invokeMethod(object, method.toLocal8Bit().data(), Qt::DirectConnection, QGenericArgument(command[ARGUMENT_OFFSET + 0].typeName(), command[ARGUMENT_OFFSET + 0].data()), QGenericArgument(command[ARGUMENT_OFFSET + 1].typeName(), command[ARGUMENT_OFFSET + 1].data()));  break;
                            case 3:  result = QMetaObject::invokeMethod(object, method.toLocal8Bit().data(), Qt::DirectConnection, QGenericArgument(command[ARGUMENT_OFFSET + 0].typeName(), command[ARGUMENT_OFFSET + 0].data()), QGenericArgument(command[ARGUMENT_OFFSET + 1].typeName(), command[ARGUMENT_OFFSET + 1].data()), QGenericArgument(command[ARGUMENT_OFFSET + 2].typeName(), command[ARGUMENT_OFFSET + 2].data()));  break;
                            default: break;
                            }


                            if (result)
                            {
                                message += " --> OK";
                                successfulCommands++;
                            }
                            else
                            {
                                message += " --> Failed";
                            }
                        }
                        else
                        {
                            message += " --> signal/slot does not exist";

                        }
                        qDebug() << message;
                    }
                }
            }
        }

    } //for (auto item : commands)
    
    _commandAction.setVariant(bool(successfulCommands == commands.size())); // return value ?
}


bool ClusterDifferentialExpressionPlugin::matchDimensionNames()
{
    _matchingDimensionNames.clear();

    const qsizetype nrOfDatasets = _loadedDatasetsAction.size();

   
    std::vector<std::vector<QString>> dimensionNames(nrOfDatasets);
    std::vector<QVector<QString>> sortedDimensionNames(nrOfDatasets);
    
	//#pragma omp parallel for schedule(dynamic, 1)
    for(qsizetype datasetIndex=0; datasetIndex < nrOfDatasets; ++datasetIndex)
    {
        
        if (local::clusterDatset_has_computed_DE_Statistics(getDataset(0)))
        {
            dimensionNames[datasetIndex] = get_DE_Statistics_Dataset(getDataset(datasetIndex))->getDimensionNames();
        }
        else
        {
            auto parentDataset = getDataset(datasetIndex)->getParent<Points>();
            if (parentDataset.isValid())
	            dimensionNames[datasetIndex] = parentDataset->getDimensionNames();
        }
        sortedDimensionNames[datasetIndex]= std::move(QVector<QString>(dimensionNames[datasetIndex].cbegin(), dimensionNames[datasetIndex].cend()));
        std::sort(std::execution::par_unseq, sortedDimensionNames[datasetIndex].begin(), sortedDimensionNames[datasetIndex].end());
    }

    bool identicalOrder = true;
    std::size_t totalNumberOfDimensions = dimensionNames[0].size();
    for (qsizetype datasetIndex = 1; datasetIndex < nrOfDatasets; ++datasetIndex)
    {
        identicalOrder &= (dimensionNames[0] == dimensionNames[datasetIndex]);
        totalNumberOfDimensions += dimensionNames[datasetIndex].size();
    }
    if (identicalOrder)
        return true;


    
    QVector<QString> allDimensionNames = sortedDimensionNames[0];
    for (qsizetype datasetIndex = 1; datasetIndex < nrOfDatasets; ++datasetIndex)
    {
        QVector<QString>result;
        std::merge(std::execution::par_unseq, allDimensionNames.cbegin(), allDimensionNames.cend(), sortedDimensionNames[datasetIndex].cbegin(), sortedDimensionNames[datasetIndex].cend(), result.begin());
        std::unique(std::execution::par_unseq, result.begin(), result.end());
        allDimensionNames = std::move(result);
        

    }
    

    std::vector<std::vector<QString>::const_iterator> begin(nrOfDatasets);
    std::vector<std::vector<QString>::const_iterator> end(nrOfDatasets);
	#pragma omp parallel for
    for(qsizetype datasetIndex=0; datasetIndex < nrOfDatasets; ++datasetIndex)
    {
        begin[datasetIndex] = dimensionNames[datasetIndex].begin();
        end[datasetIndex] = dimensionNames[datasetIndex].end();
    }

    _matchingDimensionNames.resize(allDimensionNames.size());
    _progressManager.start(allDimensionNames.size(), "Matching Dimensions");
	#pragma  omp parallel for schedule(dynamic,1)
	for(qsizetype i=0; i < allDimensionNames.size(); ++i)
	{
        const QString name = allDimensionNames[i];
        _matchingDimensionNames[i].first = name;
        QVector<qsizetype>& value = _matchingDimensionNames[i].second;
		value.resize(nrOfDatasets, -1);
        for (qsizetype datasetIndex = 0; datasetIndex < nrOfDatasets; ++datasetIndex)
        {
            auto found  = std::find(std::execution::par_unseq, begin[datasetIndex], end[datasetIndex],name);
            if(found != end[datasetIndex])
            {
                value[datasetIndex] = found - begin[datasetIndex];
            }
        }
        _progressManager.print(i);
	}
   
    _progressManager.end();
	
}




std::ptrdiff_t ClusterDifferentialExpressionPlugin::get_DE_Statistics_Index(hdps::Dataset<Clusters> clusterDataset)
{
    const auto& clusters = clusterDataset->getClusters();
    const auto numClusters = clusters.size();

    hdps::Dataset<Points> points = clusterDataset->getParent<Points>();
    const std::ptrdiff_t numDimensions = points->getNumDimensions();

    // check if the basic DE_Statistics for the cluster dataset has already been computed
    std::ptrdiff_t child_DE_Statistics_DatasetIndex = -1;
    const QString child_DE_Statistics_DatasetName = "DE_Statistics";
    {
        const auto& childDatasets = clusterDataset->getChildren({ PointType });
        for (qsizetype i = 0; i < childDatasets.size(); ++i)
        {
            if (childDatasets[i]->getGuiName() == child_DE_Statistics_DatasetName)
            {
                child_DE_Statistics_DatasetIndex = i;
                break;
            }
        }
    }

    // if they are not available compute them now
    if (child_DE_Statistics_DatasetIndex < 0)
    {


        //compute the DE statistics for this cluster

        const auto numPoints = points->getNumPoints();

        std::vector<float> meanExpressions(numClusters * numDimensions, 0);

        int x = omp_get_max_threads();
        int y = omp_get_num_threads();

        std::string message = QString("Computing DE Statistics for %1 - %2").arg(points->getGuiName(),clusterDataset->getGuiName()).toStdString();
        _progressManager.start(numDimensions, message);
        points->visitData([this, &clusters, &meanExpressions, numClusters, numDimensions](auto vec)
            {
                
                #pragma omp parallel for schedule(dynamic, 1)
                for (int dimension = 0; dimension < numDimensions; ++dimension)
                {
                    //#pragma omp parallel for schedule(dynamic, 1)
                    for (std::ptrdiff_t clusterIdx = 0; clusterIdx < numClusters; ++clusterIdx)
                    {
                        const auto& cluster = clusters[clusterIdx];
                        const auto& clusterIndices = cluster.getIndices();
                        std::size_t offset = (clusterIdx * numDimensions) + dimension;
                        for (auto row : clusterIndices)
                        {
                            meanExpressions[offset] += vec[row][dimension];
                        }
                        meanExpressions[offset] /= clusterIndices.size();
                    }
                    _progressManager.print(dimension);
                }
            });
        

        
        hdps::Dataset<Points> newDataset = _core->addDataset("Points", child_DE_Statistics_DatasetName, clusterDataset);

        events().notifyDatasetAdded(newDataset);
        
        newDataset->setDataElementType<float>();
        newDataset->setData(std::move(meanExpressions), numDimensions);
        newDataset->setDimensionNames(points->getDimensionNames());
        
        events().notifyDatasetChanged(newDataset);
       

        // now fild the child indices for this dataset
        child_DE_Statistics_DatasetIndex = -1;
        {
            const auto& childDatasets = clusterDataset->getChildren({ PointType });
            for (qsizetype i = 0; i < childDatasets.size(); ++i)
            {
                if (childDatasets[i]->getGuiName() == child_DE_Statistics_DatasetName)
                {
                    child_DE_Statistics_DatasetIndex = i;
                    break;
                }
            }
        }
    }

    _progressManager.end();
    return child_DE_Statistics_DatasetIndex;
}


Dataset<Points> ClusterDifferentialExpressionPlugin::get_DE_Statistics_Dataset(hdps::Dataset<Clusters> clusterDataset)
{
    auto clusterDataset_DE_Statitstics_Index = get_DE_Statistics_Index(clusterDataset);
    assert(clusterDataset_DE_Statitstics_Index >= 0);
    const auto& clusterDataset_Children = clusterDataset->getChildren({ PointType });
    Dataset<Points> DE_Statistics = clusterDataset_Children[clusterDataset_DE_Statitstics_Index];
    return DE_Statistics;
}

std::vector<double> ClusterDifferentialExpressionPlugin::computeMeanExpressionsForSelectedClusters(hdps::Dataset<Clusters> clusterDataset, const QSet<unsigned>& selected_clusters)
{

   
    
    std::vector<double> meanExpressions_cluster1;
    auto clusterDataset_DE_Statitstics_Index = get_DE_Statistics_Index(clusterDataset);
    if (clusterDataset_DE_Statitstics_Index < 0)
        return meanExpressions_cluster1;

    const auto& clusterDataset_Children = clusterDataset->getChildren({ PointType });
    Dataset<Points> DE_Statistics = clusterDataset_Children[clusterDataset_DE_Statitstics_Index];

    const auto& clusters = clusterDataset->getClusters();
    auto numDimensions = DE_Statistics->getNumDimensions();

    //_progressManager.start((numDimensions * selected_clusters.size()) + numDimensions, QString("Computing Mean Expressions for %1").arg(clusterDataset->getGuiName()).toStdString());
    meanExpressions_cluster1.assign(numDimensions,0);
    { // for each selected cluster in selection 1
        std::size_t sumOfClusterSizes = 0;
        std::size_t numProcessedClusters = 0;
        for (auto clusterIdx : selected_clusters)
        {
            const auto& cluster = clusters[clusterIdx];
            auto clusterName = cluster.getName();
            const auto clusterSize = cluster.getIndices().size();

            const std::size_t clusterIndexOffset = clusterIdx * numDimensions;
#pragma omp parallel for schedule(dynamic,1)
            for (std::ptrdiff_t dimension = 0; dimension < numDimensions; ++dimension)
            {

                meanExpressions_cluster1[dimension] += DE_Statistics->getValueAt(clusterIndexOffset + dimension) * clusterSize;
   //            _progressManager.print(  (numProcessedClusters * numDimensions) + dimension);
            }
            sumOfClusterSizes += clusterSize;
            ++numProcessedClusters;
        }

        std::size_t progressOffset = (selected_clusters.size() * numDimensions);

#pragma omp parallel for schedule(dynamic,1)
        for (std::ptrdiff_t dimension = 0; dimension < numDimensions; ++dimension)
        {
            meanExpressions_cluster1[dimension] /= sumOfClusterSizes;
  //          _progressManager.print(progressOffset + dimension);
        }
        progressOffset += numDimensions;
    }

   // _progressManager.end();
    return meanExpressions_cluster1;
}

void ClusterDifferentialExpressionPlugin::computeDE()
{
    if (_tableItemModel->status() == QTableItemModel::Status::UpToDate)
        return;
    _tableItemModel->setStatus(QTableItemModel::Status::Updating);
    const qsizetype NrOfDatasets = _loadedDatasetsAction.size();
    assert(NrOfDatasets >= 2);

    qsizetype NrOfSelectedDatasets = 0;
    for(qsizetype i=0; i < NrOfDatasets; ++i)
    {
        if (_loadedDatasetsAction.data(i)->datasetSelectedAction.isChecked())
        {
            NrOfSelectedDatasets++;
            if (_loadedDatasetsAction.getClusterSelection(i).size() == 0)
                return;
        }
    }
    
    /*
    static bool demo = false;
    if (demo)
    {
        QVariantMap demoMap;
        QVariantMap demoHARS;
        QVariantMap demoHCONDELS;

        
        QVariantMap item1;
        item1[QString::number(Qt::BackgroundRole)] = QBrush(QColor::fromRgb(128, 128, 128));
        
       
        demoHARS["A1BG"] = item1;
        demoHCONDELS["A1BG"] = QString("*");

        QVariantMap item2;
        item1[QString::number(Qt::DecorationRole)] = Application::getIconFont("FontAwesome").getIcon("square", Qt::black);
        

        demoHARS["PVALB"] = item2;
    	demoHCONDELS["PVALB"] = QString("*");

        demoMap["HARS"] = demoHARS;
        demoMap["HCONDELS"] = demoHCONDELS;
        _preInfoVariantAction.setVariant(demoMap);

        QVariantMap emptColumn;
        emptColumn[""] = QVariantMap();
        _postInfoVariantAction.setVariant(emptColumn);
    }
    */
    

    std::vector<std::vector<double>> meanExpressionValues(NrOfDatasets);
	//#pragma omp parallel for schedule(dynamic,1)
    for (qsizetype i = 0; i < NrOfDatasets; ++i)
    {
        if(_loadedDatasetsAction.data(i)->datasetSelectedAction.isChecked())
        {
            QStringList clusterStrings = _loadedDatasetsAction.getClusterOptions(i);
            QStringList clusterSelectionStrings = _loadedDatasetsAction.getClusterSelection(i);
            meanExpressionValues[i] = computeMeanExpressionsForSelectedClusters(getDataset(i), local::getClusterIndices(clusterStrings, clusterSelectionStrings));

            auto DE_StatisticsDataset = get_DE_Statistics_Dataset(_loadedDatasetsAction.getDataset(i));
            if (DE_StatisticsDataset.isValid())
                _DE_StatisticsDatasetGuidAction[i].data()->setString(DE_StatisticsDataset.getDatasetGuid());
        }
    }
    
    enum{ID, MEAN_DE};
    auto preInfoMap = _preInfoVariantAction.getVariant().toMap();
    auto postInfoMap = _postInfoVariantAction.getVariant().toMap();
    qsizetype columnOffset =  preInfoMap.size();
    std::size_t totalColumnCount = columnOffset + 2 + NrOfSelectedDatasets + postInfoMap.size();
    if (NrOfSelectedDatasets > 2)
        totalColumnCount += 2; // for min and max DE

 
    if(!_identicalDimensions)
        if (_matchingDimensionNames.empty())
        {
            _identicalDimensions = matchDimensionNames();
        	assert(_identicalDimensions || (!_matchingDimensionNames.empty()));
        }

    std::vector<QString> unifiedDimensionNames;
    for (qsizetype i = 0; i < NrOfDatasets; ++i)
    {
        if (_loadedDatasetsAction.data(i)->datasetSelectedAction.isChecked())
        {
            unifiedDimensionNames = get_DE_Statistics_Dataset(getDataset(0))->getDimensionNames();
            break;
        }
    }
    std::ptrdiff_t numDimensions =(std::ptrdiff_t) (_identicalDimensions ? unifiedDimensionNames.size() : _matchingDimensionNames.size());
   
    
    _tableItemModel->startModelBuilding(totalColumnCount, numDimensions);
    _progressManager.start(numDimensions, "Computing Differential Expresions ");
	#pragma omp  parallel for schedule(dynamic,1)
    for (std::ptrdiff_t dimension = 0; dimension < numDimensions; ++dimension)
    {
        std::vector<QVariant> dataVector(totalColumnCount);
        QString dimensionName = _identicalDimensions ?  unifiedDimensionNames[dimension] : _matchingDimensionNames[dimension].first;

        std::vector<double> mean(NrOfDatasets);
        if(_identicalDimensions)
        {
            for(qsizetype datasetIndex =0; datasetIndex < NrOfDatasets; ++datasetIndex)
            {
                if (_loadedDatasetsAction.data(datasetIndex)->datasetSelectedAction.isChecked())
                {
                    mean[datasetIndex] = meanExpressionValues[datasetIndex][dimension];
                }
            }
        }
        else
        {
            for (qsizetype datasetIndex = 0; datasetIndex < NrOfDatasets; ++datasetIndex)
            {
                if(_loadedDatasetsAction.data(datasetIndex)->datasetSelectedAction.isChecked())
                {
                    qsizetype dimensionIndex = _matchingDimensionNames[dimension].second[datasetIndex];
                    if (dimensionIndex >= 0)
                        mean[datasetIndex] = meanExpressionValues[datasetIndex][dimensionIndex];
                    else
                        mean[datasetIndex] = std::numeric_limits<double>::quiet_NaN();
                }
            }
        }

        double mean_DE = 0;
        double min_DE = std::numeric_limits<double >::max();
        double max_DE = std::numeric_limits<double>::lowest();
        if(NrOfSelectedDatasets > 2)
        {
            std::size_t counter = 0;
            for (qsizetype datasetIndex1 = 0; datasetIndex1 < NrOfDatasets; ++datasetIndex1)
            {
                if (_loadedDatasetsAction.data(datasetIndex1)->datasetSelectedAction.isChecked())
                {
                    double mean1 = mean[datasetIndex1];
                    if (!isnan(mean1))
                    {
                        for (qsizetype datasetIndex2 = (datasetIndex1 + 1); datasetIndex2 < NrOfDatasets; ++datasetIndex2)
                        {
                            if (_loadedDatasetsAction.data(datasetIndex2)->datasetSelectedAction.isChecked())
                            {
                                const double diffExp = fabs(mean1 - mean[datasetIndex2]);
                                if (diffExp > max_DE)
                                    max_DE = diffExp;
                                if (diffExp < min_DE)
                                    min_DE = diffExp;
                                mean_DE += diffExp;
                                ++counter;
                            }
                        }
                    }
                }
            }
            mean_DE /= counter;
        }
        else
        {
            if(NrOfSelectedDatasets ==2)
				mean_DE = mean[0] - mean[1]; // no fabs since we want to preserve the sign
        }
        

        dataVector[ID] = dimensionName;
        std::size_t columnNr = 1;
        for (auto info = preInfoMap.cbegin(); info != preInfoMap.cend(); ++info, ++columnNr)
        {
            auto infoMap = info.value().toMap();
            auto found = infoMap.constFind(dimensionName);
            if (found != infoMap.cend())
            {
                dataVector[columnNr] = found.value();
            }
        }

        dataVector[columnNr++] = local::fround(mean_DE, 3);
        if(NrOfSelectedDatasets >2)
        {
            dataVector[columnNr++] = local::fround(min_DE, 3);
            dataVector[columnNr++] = local::fround(max_DE, 3);
        }
       
        for (qsizetype datasetIndex = 0; datasetIndex < NrOfDatasets; ++datasetIndex)
        {
            if(_loadedDatasetsAction.data(datasetIndex)->datasetSelectedAction.isChecked())
            {
                double meanValue = mean[datasetIndex];
                if (isnan(meanValue))
                {
                    dataVector[columnNr++] = "N/A";
                }
                else
                {
                    dataVector[columnNr++] = local::fround(meanValue, 3);
                }
            }
        }
		
        
        for (auto info = postInfoMap.cbegin(); info != postInfoMap.cend(); ++info, ++columnNr)
        {
            auto infoMap = info.value().toMap();
            auto found = infoMap.constFind(dimensionName);
            if (found != infoMap.cend())
            {
                dataVector[columnNr] = found.value();
            }
        }

        _tableItemModel->setRow(dimension, dataVector, Qt::Unchecked, true);
        _progressManager.print(dimension);
    }
   
    
    QString emptyString;

 
    
    _tableItemModel->setHorizontalHeader(ID, QString("ID"));
   int columnNr = 1;
    for(auto i  = preInfoMap.constBegin(); i!= preInfoMap.constEnd(); ++i, ++columnNr)
    {
        _tableItemModel->setHorizontalHeader(columnNr, i.key());
    }

    int means_offset = MEAN_DE + 1;
    if (NrOfSelectedDatasets > 2)
    {
        _tableItemModel->setHorizontalHeader(columnNr++, "Mean Differential Expression");
        _tableItemModel->setHorizontalHeader(columnNr++, "Min Differential Expression");
        _tableItemModel->setHorizontalHeader(columnNr++, "Max Differential Expression");
    }
    else
        _tableItemModel->setHorizontalHeader(columnNr++, "Differential Expression");

    
    for (qsizetype datasetIndex = 0; datasetIndex < NrOfDatasets; ++datasetIndex)
    {
       
        if (_loadedDatasetsAction.data(datasetIndex)->datasetSelectedAction.isChecked())
        {
            //_datasetTableViewHeader[datasetIndex]->setParent(_differentialExpressionWidget->getTableView()->horizontalHeader());
            //_datasetTableViewHeader[datasetIndex]->raise();
           // QVariant variant(QMetaType::fromType<QWidget*>(), static_cast<void*>(_datasetTableViewHeader[datasetIndex].get()));
            QWidget* widget = _datasetTableViewHeader[datasetIndex].get();
            widget->setParent(nullptr);
            widget->hide();
            QVariant variant = QVariant::fromValue((QObject*)widget);
            //   qDebug() << columnOffset + MEANS_OFFSET + datasetIndex << " _datasetTableViewHeader[" << datasetIndex << "]." << _datasetTableViewHeader[datasetIndex].get();
            if (variant.isValid())
                _tableItemModel->setHorizontalHeader(columnNr++, variant);
        }
    }
    
    for (auto i = postInfoMap.constBegin(); i != postInfoMap.constEnd(); ++i, ++columnNr)
    {
        _tableItemModel->setHorizontalHeader(columnNr, i.key());
    }
    _tableItemModel->endModelBuilding();
    _progressManager.end();


  
    if (_tableView && _tableView->horizontalHeader())
    {
        _tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);

        if (_headerState.size())
        {
            if (qobject_cast<WordWrapHeaderView*>(_tableView->horizontalHeader())->restoreState(_headerState))
            {
                _headerState.clear();
            }
        }
    }

  

}

QIcon ClusterDifferentialExpressionFactory::getIcon(const QColor& color /*= Qt::black*/) const
{
    return Application::getIconFont("FontAwesome").getIcon("table", color);
}

ClusterDifferentialExpressionPlugin* ClusterDifferentialExpressionFactory::produce()
{
    return new ClusterDifferentialExpressionPlugin(this);
}

hdps::DataTypes ClusterDifferentialExpressionFactory::supportedDataTypes() const
{
    DataTypes supportedTypes;
    supportedTypes.append(ClusterType);
    return supportedTypes;
}

hdps::gui::PluginTriggerActions ClusterDifferentialExpressionFactory::getPluginTriggerActions(const hdps::Datasets& datasets) const
{

    PluginTriggerActions pluginTriggerActions;
   
    /*
     * temporary functions to help create data for simian viewer. won't work with normal HDPS core.
     *
     **
     **/
    
     
    /*
    const auto& fontAwesome = Application::getIconFont("FontAwesome");

    Datasets clusterDatasets;
    for (const auto& dataset : datasets)
    {
        if (dataset->getDataType() == hdps::DataType(QString("Clusters")))
        {
            clusterDatasets << dataset;
        }
    }

    if (clusterDatasets.count())
    {
        auto pluginTriggerAction = new PluginTriggerAction(const_cast<ClusterDifferentialExpressionFactory*>(this),this,"Compute DE Statistics...", "Compute DE Statistics", fontAwesome.getIcon("braille"), [this, clusterDatasets](PluginTriggerAction& pluginTriggerAction) -> void {
            for (const auto& dataset : clusterDatasets)
            {
                local::get_DE_Statistics_Index(dataset);
            }

            });

        pluginTriggerActions << pluginTriggerAction;
    }

    Datasets  pointDatasets;
    for (const auto& dataset : datasets)
    {
        if (dataset->getDataType() == hdps::DataType(QString("Points")))
        {
            pointDatasets << dataset;
        }
    }

    if(pointDatasets.count())
    {
        Datasets numericalMetaDataDatasets;
	    for(const auto pdataset : pointDatasets)
	    {
		    if(pdataset->getGuiName() =="Numerical MetaData")
			{
                numericalMetaDataDatasets << pdataset;
			}
	    }

        if (numericalMetaDataDatasets.size())
        {
            auto pluginTriggerAction = new PluginTriggerAction(const_cast<ClusterDifferentialExpressionFactory*>(this), this,"Clean-up data for SimianViewer", "Swap content of Datasets and remove the 2nd", fontAwesome.getIcon("braille"), [this, numericalMetaDataDatasets](PluginTriggerAction& pluginTriggerAction) -> void {

                for(auto numDataset : numericalMetaDataDatasets)
                {
                    Dataset<Points>numericalMetaDataset = numDataset;
                    Dataset<Points> parentDataset = numericalMetaDataset.get()->getParent<Points>();
                    Dataset<Points> temp = parentDataset;
                    Points* p = parentDataset.get();
                    Points* p2 = numericalMetaDataset.get();
                    p->swap(*p2);
                    events().notifyDatasetChanged(parentDataset);
                    events().notifyDatasetChanged(numericalMetaDataset);
                    
                    auto* core = Application::core();
                    auto& item = core->getDataHierarchyItem(numericalMetaDataset->getGuid());
                    auto& parent = item.getParent();
                    core->removeDataset(numDataset);
                    parent.removeChild(item);
                }
                });
            pluginTriggerActions << pluginTriggerAction;
        }
    }
    
  
   
    
    */
        
    
   

    return pluginTriggerActions;
}


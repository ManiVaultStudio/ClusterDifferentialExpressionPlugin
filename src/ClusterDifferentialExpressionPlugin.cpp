#include "ClusterDifferentialExpressionPlugin.h"

// CDE includes
#include "ClusterDifferentialExpressionWidget.h"
#include "SettingsAction.h"
#include "ProgressManager.h"
#include "QTableItemModel.h"
#include "SortFilterProxyModel.h"

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




#include <omp.h>




Q_PLUGIN_METADATA(IID "nl.BioVault.ClusterDifferentialExpressionPlugin")

using namespace hdps;
using namespace hdps::gui;
using namespace hdps::plugin;
using namespace hdps::util;


namespace local
{
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

    , _differentialExpressionWidget(nullptr)
    , _dropWidget(nullptr)
    , _identicalDimensions(false)
    , _preInfoVariantAction(nullptr, "TableViewLeftSideInfo")
    , _postInfoVariantAction(nullptr,"TableViewRightSideInfo")
    , _settingsAction(this)
    , _loadedDatasetsAction(new LoadedDatasetsAction(this))
    , _filterOnIdAction(new StringAction(this, "Filter on Id"))
    , _autoUpdateAction(new ToggleAction(this, "auto update", true, true))
    , _selectedIdAction(this, "Last selected Id")
    , _updateStatisticsAction(this, "Calculate Differential Expression")
    , _sortFilterProxyModel(new SortFilterProxyModel)
    , _tableItemModel(new QTableItemModel(nullptr, false))
	, _infoTextAction(nullptr,"IntoText")
{
    setSerializationName(getGuiName());

    _sortFilterProxyModel->setSourceModel(_tableItemModel.get());
    _filterOnIdAction->setSearchMode(true);
    _filterOnIdAction->setClearable(true);
    _filterOnIdAction->setPlaceHolderString("Filter by ID");
    
    _updateStatisticsAction.setCheckable(false);
    _updateStatisticsAction.setChecked(false);
    
    
    initAction(&_preInfoVariantAction);
    initAction(&_postInfoVariantAction);
    initAction(_filterOnIdAction.get());
    initAction(&_selectedIdAction);
    initAction(&_updateStatisticsAction);
    initAction(&_infoTextAction);
    initAction(_autoUpdateAction.get());
    _serializedActions.append(_loadedDatasetsAction.get());
    
    
    _infoTextAction.setCheckable(true);
    
    connect(_filterOnIdAction.get(), &hdps::gui::StringAction::stringChanged, _sortFilterProxyModel, &SortFilterProxyModel::nameFilterChanged);

    connect(&_updateStatisticsAction, &hdps::gui::TriggerAction::triggered, this, &ClusterDifferentialExpressionPlugin::computeDE);


    for(qsizetype i =0; i < _loadedDatasetsAction->size();++i)
    {
        connect(&_loadedDatasetsAction->getDataset(i), &Dataset<Clusters>::changed, this, [this,i](const hdps::Dataset<hdps::DatasetImpl>& dataset) {datasetChanged(i, dataset); });
        connect(&_loadedDatasetsAction->getClusterSelectionAction(i), &OptionsAction::selectedOptionsChanged, this, &ClusterDifferentialExpressionPlugin::selectionChanged);
    }

    _settingsAction.addAction(_filterOnIdAction,100);
    _settingsAction.addAction(_loadedDatasetsAction.objectCast<WidgetAction>(),1);

    _autoUpdateAction->setIcon(hdps::Application::getIconFont("FontAwesome").getIcon("check"));
    _settingsAction.addAction(_autoUpdateAction,5);


    _meanExpressionDatasetGuidAction.resize(_loadedDatasetsAction->size(), nullptr);
    std::vector<float> meanExpressionData(1, 0);
    const QString guiName = getGuiName();
    for (qsizetype i = 0; i < _meanExpressionDatasetGuidAction.size(); ++i)
    {
        QString actionName = "SelectedIDMeanExpressionsDataset " + QString::number(i);
        _meanExpressionDatasetGuidAction[i] = new StringAction(this, "SelectedIDMeanExpressionsDataset " + QString::number(i));
        initAction(_meanExpressionDatasetGuidAction[i]);
        
        QString datasetName = guiName + QString("::") + actionName;

        auto &allDatasets = _core->getDataManager().allSets();
        bool found = false;
        for(auto d = allDatasets.cbegin(); d!= allDatasets.cend(); ++d)
        {
	        if(d->isValid() && ((*d)->getGuiName() == datasetName))
	        {
	        	_meanExpressionDatasetGuidAction[i]->setString(d->getDatasetGuid());
                Dataset<Points> meanExpressionDataset = *d;
                meanExpressionDataset->setData(meanExpressionData, 1);
                found = true;
                break;
	        }
        }
        if(!found)
        {
            Dataset<Points> meanExpressionDataset = _core->addDataset("Points", datasetName);
            _meanExpressionDatasetGuidAction[i]->setString(meanExpressionDataset.getDatasetGuid());
            meanExpressionDataset->setData(meanExpressionData, 1);
        }
    }
}


void ClusterDifferentialExpressionPlugin::init()
{
   
   
    getWidget().setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

    _differentialExpressionWidget = new ClusterDifferentialExpressionWidget(this);
    _differentialExpressionWidget->setData(_tableItemModel);
    

    _progressManager.setProgressBar(_differentialExpressionWidget->getProgressBar());

   // connect(_differentialExpressionWidget, &ClusterDifferentialExpressionWidget::clusters1SelectionChanged, &_selected_clusters1, &CDE::Selection::setSingleSelection);
  //  connect(_differentialExpressionWidget, &ClusterDifferentialExpressionWidget::clusters2SelectionChanged, &_selected_clusters2, &CDE::Selection::setSingleSelection);
 //   connect(_differentialExpressionWidget, &ClusterDifferentialExpressionWidget::computeDE, this, &ClusterDifferentialExpressionPlugin::computeDE);
    connect(_differentialExpressionWidget, &ClusterDifferentialExpressionWidget::selectedRowChanged, this, &ClusterDifferentialExpressionPlugin::selectedRowChanged);
    

    _dropWidget = new gui::DropWidget(_differentialExpressionWidget);
    
   
    _dropWidget->setDropIndicatorWidget(new gui::DropWidget::DropIndicatorWidget(&getWidget(), "No data loaded", "Drag an item from the data hierarchy and drop it here to visualize data..."));
    _dropWidget->initialize([this](const QMimeData* mimeData) -> gui::DropWidget::DropRegions {
        gui::DropWidget::DropRegions dropRegions;

        const auto mimeText = mimeData->text();
        const auto tokens = mimeText.split("\n");
       
        if (tokens.count() == 1)
            return dropRegions;

        const auto datasetGuid = tokens[1];
        const auto dataType = DataType(tokens[2]);
        const auto dataTypes = DataTypes({ClusterType });

        if (!dataTypes.contains(dataType))
            dropRegions << new gui::DropWidget::DropRegion(this, "Incompatible data", "This type of data is not supported", "exclamation-circle", false);

        if (dataType == ClusterType) {
            const auto candidateDataset = _core->requestDataset<Clusters>(datasetGuid);
            const auto description = QString("Select Clusters %1").arg(candidateDataset->getGuiName());

            if (!getDataset(0).isValid())
            {
                dropRegions << new gui::DropWidget::DropRegion(this, "First Clusters Dataset", description, "th-large", true, [this, candidateDataset]() {
                    
                    _dropWidget->setShowDropIndicator(false);
                    getDataset(0) = candidateDataset;
                    getDataset(1) = candidateDataset;
                    });
            }
            else
            {
                dropRegions << new gui::DropWidget::DropRegion(this, " First Clusters Dataset", description, "th-large", true, [this, candidateDataset]() {
                
                    _dropWidget->setShowDropIndicator(false);
                    getDataset(0) = candidateDataset;
                    });

                dropRegions << new gui::DropWidget::DropRegion(this, " Second Clusters Dataset", description, "th-large", true, [this, candidateDataset]() {
                    
                   
                    _dropWidget->setShowDropIndicator(false);
                    getDataset(1) = candidateDataset;
                    });

               
            }
        }

        return dropRegions;
        });

       


        auto mainLayout = new QVBoxLayout();

        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);
     //   mainLayout->addWidget(_settingsAction->createWidget(&_widget));
        mainLayout->addWidget(_differentialExpressionWidget, 1);

        getWidget().setLayout(mainLayout);


        
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

ClusterDifferentialExpressionWidget& ClusterDifferentialExpressionPlugin::getClusterDifferentialExpressionWidget()
{
    return *_differentialExpressionWidget;
}


void ClusterDifferentialExpressionPlugin::fromVariantMap(const QVariantMap& variantMap)
{
    ViewPlugin::fromVariantMap(variantMap);
    auto version = variantMap.value("ClusterDifferentialExpressionPluginVersion", QVariant::fromValue(uint(0))).toUInt();
    if(version > 0)
    {
        for (auto action : _serializedActions)
        {
            action->fromParentVariantMap(variantMap);
        }
    }
}

QVariantMap ClusterDifferentialExpressionPlugin::toVariantMap() const
{
    QVariantMap variantMap = ViewPlugin::toVariantMap();
    variantMap["ClusterDifferentialExpressionPluginVersion"] = 1;
    for(auto action : _serializedActions)
    {
        action->insertIntoVariantMap(variantMap);
    }
    return variantMap;
}


void ClusterDifferentialExpressionPlugin::initAction(WidgetAction* w)
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
    _serializedActions.append(w);
}

void ClusterDifferentialExpressionPlugin::createMeanExpressionDataset(int dataset_index, int index)
{
    
    assert(dataset_index >= 1);
    const auto& clusters = getDataset(dataset_index-1)->getClusters();
    std::size_t nrOfClusters = clusters.size();
    std::size_t nrOfPoints = 0;
    for (auto cluster : clusters)
    {
        nrOfPoints += cluster.getNumberOfIndices();
    }
    std::vector<float> meanExpressionData(nrOfPoints,0);

    if(index >=0)
    {
        assert(dataset_index == 1 || dataset_index == 2);
        assert(_identicalDimensions || (!_matchingDimensionNames.empty()));

        int de_Statistics_dimension = (_identicalDimensions) ? index : (dataset_index == 1) ? _matchingDimensionNames[index].second.first : _matchingDimensionNames[index].second.second;

        if (de_Statistics_dimension >= 0)
        {
            auto de_Statistics_clusterDataset = (dataset_index == 1) ? get_DE_Statistics_Dataset(getDataset(0)) : get_DE_Statistics_Dataset(getDataset(1));
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
    
    QString meanExpressionDatasetGuid = _meanExpressionDatasetGuidAction[dataset_index - 1]->getString();
    Dataset<Points> meanExpressionDataset = _core->requestDataset(meanExpressionDatasetGuid);
    meanExpressionDataset->setData(meanExpressionData, 1);
    events().notifyDatasetChanged(meanExpressionDataset);
       
   
}

void ClusterDifferentialExpressionPlugin::updateWindowTitle()
{
    QString windowTitle = getGuiName() + ": ";
    for(qsizetype i =0; i < _loadedDatasetsAction->size(); ++i)
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
    _differentialExpressionWidget->setDatasetTooltip(index, local::getFullGuiName(dataset));
    updateWindowTitle();

    _identicalDimensions = false;
    _matchingDimensionNames.clear();

    createMeanExpressionDataset(index + 1, -1);

    if (index == 0 && !(getDataset(1).isValid()))
        getDataset(1) = dataset;
    else
    {
        if(_autoUpdateAction->isChecked())
        {
            bool condition = true;
            for (qsizetype i = 0; condition && (i < _loadedDatasetsAction->size()); ++i)
            {
                condition &= (local::clusterDatset_has_computed_DE_Statistics(_loadedDatasetsAction->getDataset(i)));
            }
            if (condition)
                computeDE();
        }
    }
}




void ClusterDifferentialExpressionPlugin::selectionChanged(const QStringList&)
{
    if(_autoUpdateAction->isChecked())
    {
        bool condition = true;
        for (qsizetype i = 0; condition && (i < _loadedDatasetsAction->size()); ++i)
        {
            condition &= (local::clusterDatset_has_computed_DE_Statistics(_loadedDatasetsAction->getDataset(i)));
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

    
    createMeanExpressionDataset(1, index);
    createMeanExpressionDataset(2, index);

}


bool ClusterDifferentialExpressionPlugin::matchDimensionNames()
{
    _matchingDimensionNames.clear();

    typedef std::pair<QString, std::pair<std::ptrdiff_t, std::ptrdiff_t>> DimensionNameMatch;
    std::vector<std::vector<DimensionNameMatch>> dimensionNameMatchesPerThread(omp_get_max_threads());

    std::vector<QString> clusterDataset1_dimensionNames;
    if(local::clusterDatset_has_computed_DE_Statistics(getDataset(0)))
    {
        clusterDataset1_dimensionNames = get_DE_Statistics_Dataset(getDataset(0))->getDimensionNames();
    }
    else 
    {
        auto clusterDataset1Parent = getDataset(0)->getParent<Points>();
        if (!clusterDataset1Parent.isValid())
            return false;
        clusterDataset1_dimensionNames = clusterDataset1Parent->getDimensionNames();
    }

    std::vector<QString> clusterDataset2_dimensionNames;
    if (local::clusterDatset_has_computed_DE_Statistics(getDataset(1)))
    {
        clusterDataset2_dimensionNames = get_DE_Statistics_Dataset(getDataset(1))->getDimensionNames();
    }
    else
    {
        auto clusterDataset2Parent = getDataset(1)->getParent<Points>();
        if (!clusterDataset2Parent.isValid())
            return false;
        clusterDataset1_dimensionNames = clusterDataset2Parent->getDimensionNames();
    }
    
    if(clusterDataset1_dimensionNames == clusterDataset2_dimensionNames)
    {
        return true;
    }

    bool identicalDimensionNames = true;
    _progressManager.start(clusterDataset2_dimensionNames.size(), "Matching Dimensions");
    ptrdiff_t clusterDataset1_dimensionNames_size = (ptrdiff_t) clusterDataset1_dimensionNames.size();
#pragma omp parallel for schedule(dynamic,1)
    for (ptrdiff_t i = 0; i < clusterDataset1_dimensionNames_size; ++i)
    {
        QString dimensionName = clusterDataset1_dimensionNames[i];
        if (clusterDataset2_dimensionNames[i] == dimensionName)
        {
            DimensionNameMatch newMatch;
            newMatch.first = dimensionName;
            newMatch.second.first = i;
            newMatch.second.second = i;
            dimensionNameMatchesPerThread[omp_get_thread_num()].push_back(newMatch);
        }
        else
        {
            identicalDimensionNames = false;
            auto found = std::find(clusterDataset2_dimensionNames.cbegin(), clusterDataset2_dimensionNames.cend(), dimensionName);
            if (found != clusterDataset2_dimensionNames.cend())
            {
                DimensionNameMatch newMatch;
                newMatch.first = dimensionName;
                newMatch.second.first = i;
                newMatch.second.second = found - clusterDataset2_dimensionNames.cbegin();
                dimensionNameMatchesPerThread[omp_get_thread_num()].push_back(newMatch);
            }
        }
        _progressManager.print(i);
    }
    _progressManager.end();
    if (identicalDimensionNames)
        return true;
    std::size_t numDimensions = 0;
    for(auto d : dimensionNameMatchesPerThread)
    {
        numDimensions += d.size();
    }
    _matchingDimensionNames.reserve(numDimensions);
    for (auto d : dimensionNameMatchesPerThread)
    {
        _matchingDimensionNames.insert(_matchingDimensionNames.cend(), d.cbegin(), d.cend());
    }
    return false;
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

    _tableItemModel->setStatus(QTableItemModel::Status::Updating);
    const qsizetype NrOfDatasets = _loadedDatasetsAction->size();
    assert(NrOfDatasets >= 2);

    for(qsizetype i=0; i < NrOfDatasets; ++i)
    {
        if (_loadedDatasetsAction->getClusterSelection(i).size() == 0)
            return;
    }
    

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
    

    std::vector<std::vector<double>> meanExpressionValues(NrOfDatasets);
    for (qsizetype i = 0; i < NrOfDatasets; ++i)
    {
        QStringList clusterStrings = _loadedDatasetsAction->getClusterOptions(i);
        QStringList clusterSelectionStrings = _loadedDatasetsAction->getClusterSelection(i);
        meanExpressionValues[i] = computeMeanExpressionsForSelectedClusters(getDataset(i), local::getClusterIndices(clusterStrings, clusterSelectionStrings));
    }
    
    enum{ID, DE, MEAN1, MEAN2, COLUMN_COUNT};
    auto preInfoMap = _preInfoVariantAction.getVariant().toMap();
    auto postInfoMap = _postInfoVariantAction.getVariant().toMap();
    qsizetype columnOffset =  preInfoMap.size();
    std::size_t totalColumnCount = columnOffset + COLUMN_COUNT + postInfoMap.size();

 
    if(!_identicalDimensions)
        if (_matchingDimensionNames.empty())
        {
            _identicalDimensions = matchDimensionNames();
        	assert(_identicalDimensions || (!_matchingDimensionNames.empty()));
        }

    std::vector<QString> unifiedDimensionNames = get_DE_Statistics_Dataset(getDataset(0))->getDimensionNames();
    std::ptrdiff_t numDimensions =(std::ptrdiff_t) (_identicalDimensions ? unifiedDimensionNames.size() : _matchingDimensionNames.size());
   
    
    _tableItemModel->startModelBuilding(totalColumnCount, numDimensions);
    _progressManager.start(numDimensions, "Computing Differential Expresions ");
	#pragma omp  parallel for schedule(dynamic,1)
    for (std::ptrdiff_t dimension = 0; dimension < numDimensions; ++dimension)
    {
        std::vector<QVariant> dataVector(totalColumnCount);
        QString dimensionName = _identicalDimensions ?  unifiedDimensionNames[dimension] : _matchingDimensionNames[dimension].first;

        double mean1 = 0;
        double mean2 = 0;
        if(_identicalDimensions)
        {
            mean1 = meanExpressionValues[0][dimension];
            mean2 = meanExpressionValues[1][dimension];
        }
        else
        {
            std::ptrdiff_t clusterDataset1_Index = _matchingDimensionNames[dimension].second.first;
            std::ptrdiff_t clusterDataset2_Index = _matchingDimensionNames[dimension].second.second;
            mean1 = meanExpressionValues[0][clusterDataset1_Index];
            mean2 = meanExpressionValues[1][clusterDataset2_Index];
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
        dataVector[columnOffset + DE] = local::fround(mean1 - mean2, 3);
        dataVector[columnOffset + MEAN1] = local::fround(mean1, 3);
        dataVector[columnOffset + MEAN2] = local::fround(mean2, 3);

        columnNr = columnOffset + MEAN2 + 1;
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

 
    
    _tableItemModel->setHorizontalHeader(ID, "ID");
   int columnNr = 1;
    for(auto i  = preInfoMap.constBegin(); i!= preInfoMap.constEnd(); ++i, ++columnNr)
    {
        _tableItemModel->setHorizontalHeader(columnNr, i.key());
    }
    _tableItemModel->setHorizontalHeader(columnOffset+DE, "Differential Expression");
    _tableItemModel->setHorizontalHeader(columnOffset+MEAN1, emptyString);
    _tableItemModel->setHorizontalHeader(columnOffset+MEAN2, emptyString);
     columnNr = columnOffset + MEAN2 + 1;
    for (auto i = postInfoMap.constBegin(); i != postInfoMap.constEnd(); ++i, ++columnNr)
    {
        _tableItemModel->setHorizontalHeader(columnNr, i.key());
    }
    _tableItemModel->endModelBuilding();
    _progressManager.end();

    _differentialExpressionWidget->setNrOfExtraColumns(columnOffset);
    
    _differentialExpressionWidget->setData(_tableItemModel);
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


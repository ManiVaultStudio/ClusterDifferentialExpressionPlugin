#include "ClusterDifferentialExpressionPlugin.h"

// CDE includes
#include "ClusterDifferentialExpressionWidget.h"
#include "SettingsAction.h"
#include "ProgressManager.h"
#include "QTableItemModel.h"

// HDPS includes
#include "PointData.h"
#include "ClusterData.h"
#include "event/Event.h"

// QT includes
#include <QMimeData>
#include <iostream>





#include <omp.h>

Q_PLUGIN_METADATA(IID "nl.BioVault.ClusterDifferentialExpressionPlugin")

using namespace hdps;
using namespace hdps::gui;
using namespace hdps::plugin;
using namespace hdps::util;

ClusterDifferentialExpressionPlugin::ClusterDifferentialExpressionPlugin(const PluginFactory* factory)
    : ViewPlugin(factory)
    , _differentialExpressionWidget(nullptr)
    , _settingsAction(nullptr)
    , _dropWidget(nullptr)
    , _identicalDimensions(false)
{
 
}

void ClusterDifferentialExpressionPlugin::init()
{
    _widget.setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

    _differentialExpressionWidget = new ClusterDifferentialExpressionWidget(this);
    _progressManager.setProgressBar(_differentialExpressionWidget->getProgressBar());

    connect(_differentialExpressionWidget, &ClusterDifferentialExpressionWidget::clusters1SelectionChanged, this, &ClusterDifferentialExpressionPlugin::clusters1Selected);
    connect(_differentialExpressionWidget, &ClusterDifferentialExpressionWidget::clusters2SelectionChanged, this, &ClusterDifferentialExpressionPlugin::clusters2Selected);
    connect(_differentialExpressionWidget, &ClusterDifferentialExpressionWidget::computeDE, this, &ClusterDifferentialExpressionPlugin::computeDE);

    _dropWidget = new gui::DropWidget(_differentialExpressionWidget);
    _settingsAction = new SettingsAction(*this);
   
    _dropWidget->setDropIndicatorWidget(new gui::DropWidget::DropIndicatorWidget(&_widget, "No data loaded", "Drag an item from the data hierarchy and drop it here to visualize data..."));
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

            if (!_clusterDataset1.isValid())
            {
                dropRegions << new gui::DropWidget::DropRegion(this, "First Clusters Dataset", description, "th-large", true, [this, candidateDataset]() {
                    _differentialExpressionWidget->ShowOutOfDate();
                    _dropWidget->setShowDropIndicator(false);
                    _clusterDataset1 = candidateDataset;
                    });
            }
            else
            {
                dropRegions << new gui::DropWidget::DropRegion(this, " First Clusters Dataset", description, "th-large", true, [this, candidateDataset]() {
                    _differentialExpressionWidget->ShowOutOfDate();
                    _dropWidget->setShowDropIndicator(false);
                    _clusterDataset1 = candidateDataset;
                    });

                dropRegions << new gui::DropWidget::DropRegion(this, " Second Clusters Dataset", description, "th-large", true, [this, candidateDataset]() {
                    
                    _differentialExpressionWidget->ShowOutOfDate();
                    _dropWidget->setShowDropIndicator(false);
                    _clusterDataset2 = candidateDataset;
                    });

               
            }
        }

        return dropRegions;
        });

        const auto updateWindowTitle = [this]() -> void {
            QString firstClusterInfo;
            QString secondClusterInfo;
            if (_clusterDataset1.isValid())
            {
                Dataset<Points> points1 = _clusterDataset1->getParent<Points>();
                if (points1.isValid())
                    firstClusterInfo = points1->getGuiName() + " - ";
                firstClusterInfo += _clusterDataset1->getGuiName();
            }
            else
            {
                firstClusterInfo = "Unknown";
            }
            if (_clusterDataset2.isValid())
            {
                Dataset<Points> points2 = _clusterDataset2->getParent<Points>();
                if (points2.isValid())
                    secondClusterInfo = points2->getGuiName() + " - ";
                secondClusterInfo += _clusterDataset1->getGuiName();
            }
            else
            {
                secondClusterInfo = "Unknown";
            }
            _widget.setWindowTitle(QString("%1: %2 vs %3").arg(getGuiName(), firstClusterInfo, secondClusterInfo));
         };

        

        // Load clusters when the dataset name of the clusters dataset reference changes
        connect(&_clusterDataset1, &Dataset<Clusters>::changed, this, [this, updateWindowTitle]() {
            updateWindowTitle();
        updateData();
            //TODO: Delete DE_Statistics
        });

        connect(&_clusterDataset2, &Dataset<Clusters>::changed, this, [this, updateWindowTitle]() {
            updateWindowTitle();
            updateData();
            //TODO: Delete DE_Statistics
            });

        //connect(_differentialExpressionWidget, SIGNAL(clusterSelectionChanged(QList<int>)), SLOT(clusterSelected(QList<int>)));
        //connect(_differentialExpressionWidget, SIGNAL(dataSetPicked(QString)), SLOT(dataSetPicked(QString)));


        auto mainLayout = new QVBoxLayout();

        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);
     //   mainLayout->addWidget(_settingsAction->createWidget(&_widget));
        mainLayout->addWidget(_differentialExpressionWidget, 1);

        _widget.setLayout(mainLayout);
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
        updateData();
    }
}

void ClusterDifferentialExpressionPlugin::loadData(const hdps::Datasets& datasets)
{
    // Exit if there is nothing to load
    if (datasets.isEmpty())
        return;

    // Load the first dataset
    _clusterDataset1 = datasets.first();
}

void ClusterDifferentialExpressionPlugin::clusters1Selected(QList<int> selectedClusters)
{
    if(selectedClusters != _clusterDataset1_selected_clusters)
    {
        _clusterDataset1_selected_clusters = selectedClusters;
        _differentialExpressionWidget->ShowOutOfDate();
    }
    
}
void ClusterDifferentialExpressionPlugin::clusters2Selected(QList<int> selectedClusters)
{
    if(selectedClusters != _clusterDataset2_selected_clusters)
    {
        _clusterDataset2_selected_clusters = selectedClusters;
        _differentialExpressionWidget->ShowOutOfDate();
    }
    
}

bool ClusterDifferentialExpressionPlugin::matchDimensionNames()
{
    _matchingDimensionNames.clear();

    typedef std::pair<QString, std::pair<std::ptrdiff_t, std::ptrdiff_t>> DimensionNameMatch;
    std::vector<std::vector<DimensionNameMatch>> dimensionNameMatchesPerThread(omp_get_max_threads());
    
    auto clusterDataset1Parent = _clusterDataset1->getParent<Points>();
    auto clusterDataset2Parent = _clusterDataset2->getParent<Points>();
    if (!clusterDataset1Parent.isValid())
        return false;
    if (!clusterDataset2Parent.isValid())
        return false;
    const auto clusterDataset1_dimensionNames = clusterDataset1Parent->getDimensionNames();
    const auto clusterDataset2_dimensionNames = clusterDataset2Parent->getDimensionNames();
    
    if(clusterDataset1_dimensionNames == clusterDataset2_dimensionNames)
    {
        return true;
    }
    bool identicalDimensionNames = true;
    _progressManager.start(clusterDataset2_dimensionNames.size(), "Checking Dimensions");
#pragma omp parallel for schedule(dynamic,1)
    for (ptrdiff_t i = 0; i < clusterDataset1_dimensionNames.size(); ++i)
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

void ClusterDifferentialExpressionPlugin::updateData()
{
    // if only 1 cluster dataset is set, set the first also as the second for convenience
    if(_clusterDataset1.isValid() && !_clusterDataset2.isValid())
    {
        _clusterDataset2 = _clusterDataset1;
        
        _identicalDimensions = true;
    }
    else
    {
        auto _clusterDataset1Parent = _clusterDataset1->getParent<Points>();
        auto _clusterDataset2Parent = _clusterDataset2->getParent<Points>();
        if(_clusterDataset1Parent.isValid() && _clusterDataset2Parent.isValid() && (_clusterDataset1Parent.getDatasetGuid() == _clusterDataset2Parent.getDatasetGuid()))
        {
               // if they have the same parent, they should have the same dimensions
            _identicalDimensions = true;
        }
        else
        {
            _identicalDimensions = matchDimensionNames();
        }
        
    }

    

    QStringList clusterNames1;
    if(_clusterDataset1.isValid())
    {
        auto& clusters = _clusterDataset1->getClusters();
        for (auto cluster : clusters)
        {
            clusterNames1.append(cluster.getName());
        }
    }
    

    QStringList clusterNames2;
    if(_clusterDataset2.isValid())
    {
        auto& clusters = _clusterDataset2->getClusters();
        for (auto cluster : clusters)
        {
            clusterNames2.append(cluster.getName());
        }
    }
    

    _differentialExpressionWidget->setClusters1(clusterNames1);
    _differentialExpressionWidget->setClusters2(clusterNames2);
    auto clusterDataset1Parent = _clusterDataset1->getParent<Points>();
    if(clusterDataset1Parent.isValid())
        _differentialExpressionWidget->setFirstClusterLabel(QString("%1 - %2").arg(clusterDataset1Parent->getGuiName(), _clusterDataset1->getGuiName()));
    else
        _differentialExpressionWidget->setFirstClusterLabel("Unknown");
    auto clusterDataset2Parent = _clusterDataset2->getParent<Points>();
    if(clusterDataset2Parent.isValid())
        _differentialExpressionWidget->setSecondClusterLabel(QString("%1 - %2").arg(clusterDataset2Parent->getGuiName(), _clusterDataset2->getGuiName()));
    else
        _differentialExpressionWidget->setSecondClusterLabel("Unknown");
    _differentialExpressionWidget->ShowOutOfDate();
}


std::ptrdiff_t ClusterDifferentialExpressionPlugin::get_DE_Statistics_Index(hdps::Dataset<Clusters> clusterDataset)
{
    const auto& clusters = clusterDataset->getClusters();
    const auto numClusters = clusters.size();

    hdps::Dataset<Points> _points1 = clusterDataset->getParent<Points>();
    const std::ptrdiff_t numDimensions = _points1->getNumDimensions();

    // check if the basic DE_Statistics for the cluster dataset has already been computed
    std::ptrdiff_t child_DE_Statistics_DatasetIndex = -1;
    const QString child_DE_Statistics_DatasetName = "DE_Statistics";
    {
        const auto& childDatasets = clusterDataset->getChildren({ PointType });
        for (std::size_t i = 0; i < childDatasets.size(); ++i)
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

        const auto numPoints = _points1->getNumPoints();

        std::vector<float> meanExpressions(numClusters * numDimensions, 0);

        int x = omp_get_max_threads();
        int y = omp_get_num_threads();

        std::string message = QString("Computing DE Statistics for %1 - %2").arg(_points1->getGuiName(),clusterDataset->getGuiName()).toStdString();
        _progressManager.start(numDimensions/**numPoints*/, message);
        _points1->visitData([this, &clusters, &meanExpressions, numClusters, numDimensions](auto vec)
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
                           // this->_progressManager.print((row * numDimensions) + dimension);
                           // if(omp_get_thread_num()==0)
                            //    std::cout << "D: " << dimension << "\t" << clusterIdx << "\t" << row << std::endl;
                        }
                        meanExpressions[offset] /= clusterIndices.size();
                    }
                    _progressManager.print(dimension);
                }
            });

        _progressManager.end();

        

       

        
        hdps::Dataset<Points> newDataset = _core->addDataset("Points", child_DE_Statistics_DatasetName, clusterDataset);
        _core->notifyDatasetAdded(newDataset);
        newDataset->setDataElementType<float>();
        newDataset->setData(std::move(meanExpressions), numDimensions);
        _core->notifyDatasetChanged(newDataset);
       

        // now fild the child indices for this dataset
        child_DE_Statistics_DatasetIndex = -1;
        {
            const auto& childDatasets = clusterDataset->getChildren({ PointType });
            for (std::size_t i = 0; i < childDatasets.size(); ++i)
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

std::vector<double> ClusterDifferentialExpressionPlugin::computeMeanExpressionsForSelectedClusters(hdps::Dataset<Clusters> clusterDataset, const QList<int>& selected_clusters)
{
    std::vector<double> meanExpressions_cluster1;
    auto clusterDataset_DE_Statitstics_Index = get_DE_Statistics_Index(clusterDataset);
    if (clusterDataset_DE_Statitstics_Index < 0)
        return meanExpressions_cluster1;

    const auto& clusterDataset_Children = clusterDataset->getChildren({ PointType });
    Dataset<Points> DE_Statistics = clusterDataset_Children[clusterDataset_DE_Statitstics_Index];

    const auto& clusters = clusterDataset->getClusters();
    auto numDimensions = DE_Statistics->getNumDimensions();

    _progressManager.start((numDimensions * selected_clusters.size()) + numDimensions, QString("Computing Mean Expressions for %1").arg(clusterDataset->getGuiName()).toStdString());
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
                _progressManager.print(  (numProcessedClusters * numDimensions) + dimension);
            }
            sumOfClusterSizes += clusterSize;
            ++numProcessedClusters;
        }

        std::size_t progressOffset = (selected_clusters.size() * numDimensions);

#pragma omp parallel for schedule(dynamic,1)
        for (std::ptrdiff_t dimension = 0; dimension < numDimensions; ++dimension)
        {
            meanExpressions_cluster1[dimension] /= sumOfClusterSizes;
            _progressManager.print(progressOffset + dimension);
        }
        progressOffset += numDimensions;
    }

    _progressManager.end();
    return meanExpressions_cluster1;
}

void ClusterDifferentialExpressionPlugin::computeDE()
{
    if(_clusterDataset1_selected_clusters.empty() || _clusterDataset2_selected_clusters.empty())
    {
        return;
    }

    
    const auto& clusters = _clusterDataset1->getClusters();
    const auto numClusters = clusters.size();
    if (numClusters == 0)
        return;

    std::vector<double> meanExpressions_cluster1 = computeMeanExpressionsForSelectedClusters(_clusterDataset1, _clusterDataset1_selected_clusters);
    std::vector<double> meanExpressions_cluster2 = computeMeanExpressionsForSelectedClusters(_clusterDataset2, _clusterDataset2_selected_clusters);
    
    
    enum{NAME, DE, MEAN1, MEAN2, COLUMN_COUNT};
    QTableItemModel* resultModel = new QTableItemModel(nullptr, false, COLUMN_COUNT);
    
    if(_identicalDimensions)
    {
        std::vector<QString> dimensionNames = _clusterDataset1->getParent<Points>()->getDimensionNames();
        std::size_t numDimensions = dimensionNames.size();
        resultModel->resize(numDimensions);
        resultModel->startModelBuilding();
        _progressManager.start(numDimensions, "Computing Diferential Expressions");
        #pragma omp  parallel for schedule(dynamic,1)
        for (std::ptrdiff_t dimension = 0; dimension < numDimensions; ++dimension)
        {
            std::vector<QVariant> dataVector(COLUMN_COUNT);
            QString dimensionName = dimensionNames[dimension];
            const double mean1 = meanExpressions_cluster1[dimension];
            const double mean2 = meanExpressions_cluster2[dimension];
            dataVector[NAME] = dimensionName;
            dataVector[DE] = mean1 - mean2;
            dataVector[MEAN1] = mean1;
            dataVector[MEAN2] = mean2;
            resultModel->setRow(dimension, dataVector, Qt::Unchecked, true);
            _progressManager.print(dimension);
        }
    }
    else
    {
        std::size_t numDimensions = _matchingDimensionNames.size();
        resultModel->resize(numDimensions);
        resultModel->startModelBuilding();
        _progressManager.start(numDimensions, "Computing Diferential Expressions");
        #pragma omp  parallel for schedule(dynamic,1)
        for (std::ptrdiff_t dimension = 0; dimension < numDimensions; ++dimension)
        {
            std::vector<QVariant> dataVector(COLUMN_COUNT);
            QString dimensionName = _matchingDimensionNames[dimension].first;
            std::ptrdiff_t clusterDataset1_Index = _matchingDimensionNames[dimension].second.first;
            std::ptrdiff_t clusterDataset2_Index = _matchingDimensionNames[dimension].second.second;
            dataVector[NAME] = dimensionName;
            const double mean1 = meanExpressions_cluster1[clusterDataset1_Index];
            const double mean2 = meanExpressions_cluster2[clusterDataset2_Index];
            dataVector[DE] = mean1 - mean2;
            dataVector[MEAN1] = mean1;
            dataVector[MEAN2] = mean2;
            resultModel->setRow(dimension, dataVector, Qt::Unchecked, true);

            _progressManager.print(dimension);
        }
    }
    
    
    
    QString cluster1_mean_header = "Selection 1 Mean";
    /*
    for (std::size_t c=0; c < _clusterDataset1_selected_clusters.size(); ++c)
    {
        const auto& cluster = clusters[_clusterDataset1_selected_clusters[c]];
        auto clusterName = cluster.getName();
        if (c > 0)
            cluster1_mean_header += "+";
        cluster1_mean_header += clusterName;
    }
    */

    QString cluster2_mean_header = "Selection 2 Mean";
    /*
    for (std::size_t c = 0; c < _clusterDataset2_selected_clusters.size(); ++c)
    {
        const auto& cluster = clusters[_clusterDataset2_selected_clusters[c]];
        auto clusterName = cluster.getName();
        if (c > 0)
            cluster1_mean_header += "+";
        cluster2_mean_header += clusterName;
    }
    */
    resultModel->setHorizontalHeader(NAME, "Gene");
    resultModel->setHorizontalHeader(DE, "Differential Expression");
    resultModel->setHorizontalHeader(MEAN1, cluster1_mean_header);
    resultModel->setHorizontalHeader(MEAN2, cluster2_mean_header);
    resultModel->endModelBuilding();
    _progressManager.end();
    
    _differentialExpressionWidget->setData(resultModel);
    
}

QIcon ClusterDifferentialExpressionFactory::getIcon() const
{
    return Application::getIconFont("FontAwesome").getIcon("table");
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
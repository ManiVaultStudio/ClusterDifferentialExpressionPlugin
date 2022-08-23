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

ClusterDifferentialExpressionPlugin::ClusterDifferentialExpressionPlugin(const PluginFactory* factory) :
    ViewPlugin(factory),
    _differentialExpressionWidget(nullptr),
    _settingsAction(nullptr),
    _dropWidget(nullptr)
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

            if (candidateDataset == _clusters1) {
                dropRegions << new gui::DropWidget::DropRegion(this, "Clusters", "Cluster set is already in use", "exclamation-circle", false, [this]() {});
            }
            else {
                dropRegions << new gui::DropWidget::DropRegion(this, "Clusters", description, "th-large", true, [this, candidateDataset]() {
                    _clusters1 = candidateDataset;
                    _differentialExpressionWidget->ShowOutOfDate();
                    _dropWidget->setShowDropIndicator(false);
                    });
            }
        }

        return dropRegions;
        });

        const auto updateWindowTitle = [this]() -> void {
         if (!_clusters1.isValid())
              _widget.setWindowTitle(getGuiName());
          else
             _widget.setWindowTitle(QString("%1: %2").arg(getGuiName(), _clusters1->getGuiName()));
         };

        

        // Load clusters when the dataset name of the clusters dataset reference changes
        connect(&_clusters1, &Dataset<Clusters>::changed, this, [this, updateWindowTitle]() {
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


void ClusterDifferentialExpressionPlugin::clusters1Selected(QList<int> selectedClusters)
{
    if(selectedClusters != _clusters1_selection)
    {
        _clusters1_selection = selectedClusters;
        _differentialExpressionWidget->ShowOutOfDate();
    }
    
}
void ClusterDifferentialExpressionPlugin::clusters2Selected(QList<int> selectedClusters)
{
    if(selectedClusters != _clusters2_selection)
    {
        _clusters2_selection = selectedClusters;
        _differentialExpressionWidget->ShowOutOfDate();
    }
    
}

void ClusterDifferentialExpressionPlugin::updateData()
{
    QStringList clusterNames;
    auto &clusters = _clusters1->getClusters();
    for(auto cluster : clusters)
    {
        clusterNames.append(cluster.getName());
    }
    _differentialExpressionWidget->setClusters1(clusterNames);
    _differentialExpressionWidget->setClusters2(clusterNames);
    _differentialExpressionWidget->ShowOutOfDate();
}

void ClusterDifferentialExpressionPlugin::computeDE()
{
    if(_clusters1_selection.empty() || _clusters2_selection.empty())
    {
        return;
    }

    Dataset<Points> _points1 = _clusters1->getParent<Points>();
    const auto& clusters = _clusters1->getClusters();
    const auto numClusters = clusters.size();
    const std::ptrdiff_t numDimensions = _points1->getNumDimensions();
    if (numClusters == 0)
        return;

    // check if the basic DE_Statistics for the cluster dataset has already been computed
    std::ptrdiff_t child_DE_Statistics_DatasetIndex = -1;
    const QString child_DE_Statistics_DatasetName = "DE_Statistics";
    {
        const auto& childDatasets = _clusters1->getChildren({ PointType });
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

        std::vector<float> meanExpressions(numClusters*numDimensions,0);

        _progressManager.start(meanExpressions.size(), "Computing DE Statistis Dataset");

        for (std::size_t clusterIdx = 0; clusterIdx < numClusters; ++clusterIdx)
        {

            const auto& cluster = clusters[clusterIdx];
            const auto& clusterIndices = cluster.getIndices();

            #pragma omp parallel for schedule(dynamic, 1)
            for (std::ptrdiff_t dimension = 0; dimension < numDimensions; ++dimension)
            {
                
                auto resultIdx = (clusterIdx * numDimensions) + dimension;
                for (auto row : clusterIndices)
                {
                    meanExpressions[resultIdx] += _points1->getValueAt((row * numDimensions) + dimension);
                }
                meanExpressions[resultIdx] /= numPoints;

                _progressManager.print(resultIdx);
            }
        }

        

        hdps::Dataset<Points> newDataset = _core->addDataset("Points", child_DE_Statistics_DatasetName, _clusters1);
        _core->notifyDatasetAdded(newDataset);
        newDataset->setDataElementType<float>();
        newDataset->setData(meanExpressions, numDimensions);
        _core->notifyDatasetChanged(newDataset);

        // now fild the child indices for this dataset
        child_DE_Statistics_DatasetIndex = -1;
        {
            const auto& childDatasets = _clusters1->getChildren({PointType});
            for (std::size_t i = 0; i < childDatasets.size(); ++i)
            {
                if (childDatasets[i]->getGuiName() == child_DE_Statistics_DatasetName)
                {
                    child_DE_Statistics_DatasetIndex = i;
                    break;
                }
            }
        }
        

        _progressManager.end();
    }

    if (child_DE_Statistics_DatasetIndex < 0)
        return;
    const auto& childDatasets = _clusters1->getChildren({ PointType });
    Dataset<Points> DE_Statistics = childDatasets[child_DE_Statistics_DatasetIndex];

    _progressManager.start((_clusters1_selection.size()+ _clusters2_selection.size() + 3)*numDimensions, "Compute Differential Expressions");
    std::size_t progressOffset = 0;

    std::vector<double> meanExpressions_cluster1(numDimensions);
    { // for each selected cluster in selection 1
        std::size_t sumOfClusterSizes = 0;
        std::size_t numProcessedClusters = 0;
        for (auto clusterIdx : _clusters1_selection)
        {
            const auto& cluster = clusters[clusterIdx];
            auto clusterName = cluster.getName();
            const auto clusterSize = cluster.getIndices().size();

            const std::size_t clusterIndexOffset = clusterIdx * numDimensions;
            #pragma omp parallel for schedule(dynamic,1)
            for (std::ptrdiff_t dimension = 0; dimension < numDimensions; ++dimension)
            {
                
                meanExpressions_cluster1[dimension] += DE_Statistics->getValueAt(clusterIndexOffset+dimension) * clusterSize;
                _progressManager.print(progressOffset + (numProcessedClusters*numDimensions) + dimension);
            }
            sumOfClusterSizes += clusterSize;
            ++numProcessedClusters;
        }

        progressOffset += (_clusters2_selection.size() * numDimensions);

#pragma omp parallel for schedule(dynamic,1)
        for (std::ptrdiff_t dimension = 0; dimension < numDimensions; ++dimension)
        {
            meanExpressions_cluster1[dimension] /= sumOfClusterSizes;
            _progressManager.print(progressOffset + dimension);
        }
        progressOffset += numDimensions;
    }

    std::vector<double> meanExpressions_cluster2(numDimensions);
    { // for each selected cluster in selection 1
        std::size_t sumOfClusterSizes = 0;
        std::size_t numProcessedClusters = 0;
        for (auto clusterIdx : _clusters2_selection)
        {
            const auto& cluster = clusters[clusterIdx];
            auto clusterName = cluster.getName();
            const auto clusterSize = cluster.getIndices().size();

            const std::size_t clusterIndexOffset = clusterIdx * numDimensions;
            #pragma omp parallel for schedule(dynamic,1)
            for (std::ptrdiff_t dimension = 0; dimension < numDimensions; ++dimension)
            {
                meanExpressions_cluster2[dimension] += DE_Statistics->getValueAt(clusterIndexOffset+dimension) * clusterSize;
                _progressManager.print(progressOffset + (numProcessedClusters * numDimensions) + dimension);
            }
            sumOfClusterSizes += clusterSize;
            ++numProcessedClusters;
        }
        progressOffset += (_clusters2_selection.size() * numDimensions);
        #pragma omp parallel for schedule(dynamic,1)
        for (std::ptrdiff_t dimension = 0; dimension < numDimensions; ++dimension)
        {
            meanExpressions_cluster2[dimension] /= sumOfClusterSizes;
            _progressManager.print(progressOffset + dimension);
        }
        progressOffset += numDimensions;
    }

    enum{NAME, DE, MEAN1, MEAN2, COLUMN_COUNT};
    QTableItemModel* resultModel = new QTableItemModel(nullptr, false, COLUMN_COUNT);

    const auto& dimensionNames = _points1->getDimensionNames();
    resultModel->resize(numDimensions);
    resultModel->startModelBuilding();

    #pragma omp pragma parallel for(schedule,dynamic,1)
    for (std::ptrdiff_t dimension = 0; dimension < numDimensions; ++dimension)
    {
        std::vector<QVariant> dataVector(COLUMN_COUNT);
        dataVector[NAME] = dimensionNames[dimension];
        dataVector[MEAN1] = meanExpressions_cluster1[dimension];
        dataVector[MEAN2] = meanExpressions_cluster2[dimension];
        dataVector[DE] = meanExpressions_cluster1[dimension] - meanExpressions_cluster2[dimension];
        resultModel->setRow(dimension, dataVector, Qt::Unchecked, true);

        _progressManager.print(progressOffset + dimension);
    }

    QString cluster1_mean_header = "Mean ";
    for (std::size_t c=0; c < _clusters1_selection.size(); ++c)
    {
        const auto& cluster = clusters[_clusters1_selection[c]];
        auto clusterName = cluster.getName();
        if (c > 0)
            cluster1_mean_header += "+";
        cluster1_mean_header += clusterName;
    }
    QString cluster2_mean_header = "Mean ";
    for (std::size_t c = 0; c < _clusters2_selection.size(); ++c)
    {
        const auto& cluster = clusters[_clusters2_selection[c]];
        auto clusterName = cluster.getName();
        if (c > 0)
            cluster1_mean_header += "+";
        cluster2_mean_header += clusterName;
    }
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
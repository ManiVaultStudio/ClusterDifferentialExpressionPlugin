#include "ClusterDifferentialExpressionPlugin.h"

// CDE includes
#include "ClusterDifferentialExpressionWidget.h"
#include "SettingsAction.h"

// HDPS includes
#include "PointData.h"
#include "ClusterData.h"
#include "event/Event.h"

// QT includes
#include <QMimeData>
#include <iostream>

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
                    _points1 = candidateDataset->getParent<Points>();
                    });
            }
        }

        return dropRegions;
        });

        const auto updateWindowTitle = [this]() -> void {
         if (!_clusters1.isValid())
              _differentialExpressionWidget->setWindowTitle(getGuiName());
          else
             _differentialExpressionWidget->setWindowTitle(QString("%1: %2").arg(getGuiName(), _clusters1->getGuiName()));
         };

         // Load points when the dataset name of the points dataset reference changes
        connect(&_points1, &Dataset<Points>::changed, this, [this, updateWindowTitle]() {
            updateWindowTitle();
        });

        // Load clusters when the dataset name of the clusters dataset reference changes
        connect(&_clusters1, &Dataset<Clusters>::changed, this, [this, updateWindowTitle]() {
            updateWindowTitle();
        updateData();
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
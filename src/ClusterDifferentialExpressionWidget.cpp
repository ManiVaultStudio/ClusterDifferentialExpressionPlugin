#include "ClusterDifferentialExpressionWidget.h"
#include "ClusterDifferentialExpressionPlugin.h"

#include "QTableItemModel.h"
#include "SortFilterProxyModel.h"
#include "WordWrapHeaderView.h"

#include <QComboBox>
#include <QTableView>
#include <QPushButton>
#include <QHeaderView>
#include <QProgressBar>
#include <QGraphicsColorizeEffect>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QHBoxLayout>

#include <ClusterData.h>

namespace local
{
	bool updateOptionAction(hdps::gui::OptionAction &action, const QStringList &options)
	{
        if (action.hasOptions() && (!options.isEmpty()))
        {
            auto currentText = action.getCurrentText();
            if (options.contains(currentText))
            {
                action.initialize(options, currentText, options[0]);
                action.setEnabled(true);
                return true;
            }
        }
        if(options.isEmpty())
            action.initialize(options);
        else
			action.initialize(options, options[0], options[0]);

        action.setEnabled(action.hasOptions());

        return false;
	}

    bool updateDatasetPickerAction(DatasetPickerAction& action, const QVector < hdps::Dataset <hdps::DatasetImpl>> options)
	{
        auto current = action.getCurrentDataset();
       
        action.setDatasets(options);
        if (options.contains(current))
        {
            if(action.getCurrentDataset() != current)
				action.setCurrentDataset(current);
            return true;
        }
        return false;
	}
}

ClusterDifferentialExpressionWidget::ClusterDifferentialExpressionWidget(ClusterDifferentialExpressionPlugin* differentialExpressionPlugin)
    :_differentialExpressionPlugin(differentialExpressionPlugin)
    , _modelIsUpToDate(false)
    , _clusterDataset1Action(this, "Dataset")
    , _clusterDataset2Action(this, "Dataset")
    , _clusters1SelectionAction(this, "Clusters")
    , _clusters2SelectionAction(this, "Clusters")
    , _tableView(nullptr)
    , _differentialExpressionModel(nullptr)
    , _sortFilterProxyModel(nullptr)
    , _clusters1ParentName(nullptr)
    , _clusters2ParentName(nullptr)
    , _cluster1SectionLabelWidget(nullptr)
    , _cluster2SectionLabelWidget(nullptr)
    , _autoComputeToggleAction(this, "Automatic Update")
{
    initGui();
    
}

void ClusterDifferentialExpressionWidget::initGui()
{

    setAcceptDrops(true);
    const int NumberOfColums = 2;

    QGridLayout* layout = new QGridLayout;
    setLayout(layout);

    int currentRow = 0;

    {
        QGridLayout* gridLayout = new QGridLayout;
        {
            gridLayout->addWidget(_clusterDataset1Action.createLabelWidget(this), 0, 0);
            gridLayout->addWidget(_clusterDataset1Action.createWidget(this), 0, 1);
            connect(&_clusterDataset1Action, &DatasetPickerAction::datasetPicked, this, [this](const hdps::Dataset<hdps::DatasetImpl>& dataset) { emit clusters1DatasetChanged(dataset); });
        }
       
        {
            _clusters1SelectionAction.setDefaultWidgetFlags(hdps::gui::OptionAction::ComboBox);

            _clusters1SelectionAction.setPlaceHolderString("Choose Selection 1 Cluster");
            _clusters1SelectionAction.setDefaultIndex(0);
            connect(&_clusters1SelectionAction, &hdps::gui::OptionAction::currentIndexChanged, this, &ClusterDifferentialExpressionWidget::clusters1Selection_CurrentIndexChanged);
            _cluster1SectionLabelWidget = _clusters1SelectionAction.createLabelWidget(this);
            _cluster1SectionLabelWidget->setDisabled(true);
            gridLayout->addWidget(_cluster1SectionLabelWidget, 1, 0);
            gridLayout->addWidget(_clusters1SelectionAction.createWidget(this), 1, 1);
        }
        
        QGroupBox* newGroupBox = new QGroupBox("Selection 1");
        newGroupBox->setLayout(gridLayout);
        layout->addWidget(newGroupBox, currentRow, 0, 1, 1);
    }

    {
        QGridLayout* gridLayout = new QGridLayout;

        {
            gridLayout->addWidget(_clusterDataset2Action.createLabelWidget(this), 0, 0);
            gridLayout->addWidget(_clusterDataset2Action.createWidget(this), 0, 1);
            connect(&_clusterDataset2Action, &DatasetPickerAction::datasetPicked, this, [this](const hdps::Dataset<hdps::DatasetImpl>& dataset) { emit clusters2DatasetChanged(dataset); });
        }
        
        
        {
            _clusters2SelectionAction.setDefaultWidgetFlags(hdps::gui::OptionAction::ComboBox);
            _clusters2SelectionAction.setPlaceHolderString("Choose Selection 2 Cluster");
            _cluster2SectionLabelWidget = _clusters2SelectionAction.createLabelWidget(this);
            _cluster2SectionLabelWidget->setDisabled(true); // disabled until the options are added.
            gridLayout->addWidget(_cluster2SectionLabelWidget, 1, 0);
            gridLayout->addWidget(_clusters2SelectionAction.createWidget(this), 1, 1);
            connect(&_clusters2SelectionAction, &hdps::gui::OptionAction::currentIndexChanged, this, &ClusterDifferentialExpressionWidget::clusters2Selection_CurrentIndexChanged);
        }
        

        QGroupBox* newGroupBox = new QGroupBox("Selection 2");
        newGroupBox->setLayout(gridLayout);
        layout->addWidget(newGroupBox, currentRow++, 1, 1, 1);
    }
    
    
    { // table view
        _sortFilterProxyModel = new SortFilterProxyModel;
        _tableView = new QTableView;
        _tableView->setModel(_sortFilterProxyModel);
        _tableView->setSortingEnabled(true);
        _tableView->setSelectionMode(QAbstractItemView::SingleSelection);
        _tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        _tableView->setContextMenuPolicy(Qt::CustomContextMenu);
        
        
        

        WordWrapHeaderView* horizontalHeader = new WordWrapHeaderView(Qt::Horizontal);
        //horizontalHeader->setStretchLastSection(true);
        horizontalHeader->setFirstSectionMovable(false);
        horizontalHeader->setSectionsMovable(true);
        horizontalHeader->setSectionsClickable(true);
        horizontalHeader->sectionResizeMode(QHeaderView::Interactive);
        horizontalHeader->setSectionResizeMode(QHeaderView::Stretch);
		horizontalHeader->setSortIndicator(0, Qt::AscendingOrder);
        horizontalHeader->setDefaultAlignment(Qt::AlignCenter | Qt::Alignment(Qt::TextWordWrap));
        _tableView->setHorizontalHeader(horizontalHeader);

        layout->addWidget(_tableView, currentRow++, 0, 1, NumberOfColums);
    }

    {// Progress bar and update button
        _progressBar = new QProgressBar(this);
        _progressBar->setTextVisible(true);
        _progressBar->setAlignment(Qt::AlignCenter);
        _progressBar->setOrientation(Qt::Orientation::Horizontal);
        QGraphicsColorizeEffect* colorizeEffect = new QGraphicsColorizeEffect();
        colorizeEffect->setColor(Qt::red);
        colorizeEffect->setStrength(1);
        _progressBar->setGraphicsEffect(colorizeEffect);
        _progressBar->setFormat("No Selections");
        _progressBar->graphicsEffect()->setEnabled(false);

        layout->addWidget(_progressBar, currentRow, 0, 1, NumberOfColums);
        _progressBar->hide();

        

        _updateStatisticsButton = new QPushButton("Calculate Differential Expression", this);
        layout->addWidget(_updateStatisticsButton, currentRow++, 0, 1, NumberOfColums);
        QObject::connect(_updateStatisticsButton, &QPushButton::clicked, this, &ClusterDifferentialExpressionWidget::updateStatisticsButtonPressed);
        {
            QHBoxLayout* layout2 = new QHBoxLayout;
            layout2->addWidget(new QLabel("Filter on ID "),1,Qt::AlignLeft);
            QLineEdit* nameFilter = new QLineEdit(this);
            nameFilter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            connect(nameFilter, SIGNAL(textChanged(QString)), _sortFilterProxyModel, SLOT(nameFilterChanged(QString)));
            layout2->addWidget(nameFilter,99 ,Qt::AlignLeft);
            layout->addLayout(layout2, currentRow++, 0, 1, NumberOfColums);
        }

        {
            QHBoxLayout* layout2 = new QHBoxLayout;
            _autoComputeToggleAction.setChecked(false);
            _autoComputeToggleAction.setEnabled(false);
            layout2->addWidget(_autoComputeToggleAction.createLabelWidget(this), 1, Qt::AlignLeft);
            _autoComputeToggleAction.setText("");
            layout2->addWidget(_autoComputeToggleAction.createWidget(this),99, Qt::AlignLeft);
            layout->addLayout(layout2, currentRow++, 0, 1, NumberOfColums);
        }

       
      
    }
    
}

void ClusterDifferentialExpressionWidget::setClusters1(QStringList clusters)
{
    auto result = local::updateOptionAction(_clusters1SelectionAction, clusters);
    if(_autoComputeToggleAction.isEnabled())
        _autoComputeToggleAction.setChecked(!clusters.empty());

}

void ClusterDifferentialExpressionWidget::setClusters2( QStringList clusters)
{
    auto result = local::updateOptionAction(_clusters2SelectionAction, clusters);
    if (_autoComputeToggleAction.isEnabled())
        _autoComputeToggleAction.setChecked(!clusters.empty());
}

void ClusterDifferentialExpressionWidget::setClusterDatasets(const QVector<hdps::Dataset<hdps::DatasetImpl>> &datasets)
{
    local::updateDatasetPickerAction(_clusterDataset1Action,datasets);
    local::updateDatasetPickerAction(_clusterDataset2Action, datasets);
}


void ClusterDifferentialExpressionWidget::setData(QTableItemModel* newModel)
{
    auto* oldModel = _sortFilterProxyModel->sourceModel();
    _differentialExpressionModel = newModel;
    _sortFilterProxyModel->setSourceModel(_differentialExpressionModel);
    if(oldModel)
        delete oldModel;

    
    QHeaderView* horizontalHeader = _tableView->horizontalHeader();
    horizontalHeader->resizeSections(QHeaderView::ResizeToContents);
    

    ShowUpToDate();
}

void ClusterDifferentialExpressionWidget::ShowUpToDate()
{
    _updateStatisticsButton->hide();
    _progressBar->show();
    if(_differentialExpressionModel)
    {
        _differentialExpressionModel->setOutDated(false);
        if (_differentialExpressionModel->rowCount() != 0)
        {
            _progressBar->setFormat("Results Up-to-Date");
        }
    }
}

QProgressBar* ClusterDifferentialExpressionWidget::getProgressBar()
{
    return _progressBar;
}


void ClusterDifferentialExpressionWidget::ShowOutOfDate()
{
    _progressBar->hide();
    _updateStatisticsButton->show();
    if (_differentialExpressionModel)
    {
        if (_differentialExpressionModel->rowCount() > 0)
        {
            _differentialExpressionModel->setOutDated(true);
            const QString recomputeText = "Out-of-Date - Recalculate";
            if (_updateStatisticsButton->text() != recomputeText)
            {
                QPalette pal = _updateStatisticsButton->palette();
                // 			QBrush b(Qt::red);
                // 			pal.setBrush(QPalette::Button, b);
                pal.setColor(QPalette::Button, Qt::red);
                pal.setColor(QPalette::ButtonText, Qt::red);
                _updateStatisticsButton->setPalette(pal);
                _updateStatisticsButton->setAutoFillBackground(true);
                _updateStatisticsButton->setText(recomputeText);
            }
        }
    }
}

void  ClusterDifferentialExpressionWidget::EnableAutoCompute(bool value)
{
    if(false == value)
    {
        _autoComputeToggleAction.setChecked(false);
    }
    _autoComputeToggleAction.setEnabled(value);
}

void ClusterDifferentialExpressionWidget::clusters1Selection_CurrentIndexChanged(int index)
{
    if(index >=0)
    {
        QList<int> selection = { index };
        emit clusters1SelectionChanged(selection);
        if (_autoComputeToggleAction.isChecked())
            updateStatisticsButtonPressed();
    }
}

void ClusterDifferentialExpressionWidget::clusters2Selection_CurrentIndexChanged(int index)
{
    if(index >=0)
    {
        QList<int> selection = { index };
        emit clusters2SelectionChanged(selection);
        if (_autoComputeToggleAction.isChecked())
            updateStatisticsButtonPressed();
    }
}

void ClusterDifferentialExpressionWidget::selectClusterDataset1(const hdps::Dataset<hdps::DatasetImpl>&dataset)
{
    if(_clusterDataset1Action.getCurrentDataset() != dataset)
		_clusterDataset1Action.setCurrentDataset(dataset);
}

void ClusterDifferentialExpressionWidget::selectClusterDataset2(const hdps::Dataset<hdps::DatasetImpl>& dataset)
{
    if (_clusterDataset2Action.getCurrentDataset() != dataset)
		_clusterDataset2Action.setCurrentDataset(dataset);
}


void ClusterDifferentialExpressionWidget::updateStatisticsButtonPressed()
{
    ShowUpToDate();
    emit computeDE();
}
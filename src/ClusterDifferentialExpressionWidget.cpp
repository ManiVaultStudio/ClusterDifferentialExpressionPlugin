#include "ClusterDifferentialExpressionWidget.h"
#include "ClusterDifferentialExpressionPlugin.h"

#include "QTableItemModel.h"
#include "SortFilterProxyModel.h"
#include "WordWrapHeaderView.h"
#include "TableView.h"

#include <QComboBox>
#include <QTableView>
#include <QPushButton>
#include <QHeaderView>
#include <QProgressBar>
#include <QGraphicsColorizeEffect>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QResizeEvent>
#include <QGroupBox>
#include <QHBoxLayout>

#include <ClusterData.h>

namespace local
{

    void setLabelWidgetIcon(QWidget *labelWidget, const QString &name)
    {
        QLabel* internalLabel = labelWidget->findChild<QLabel*>();
        if (internalLabel)
        {
            const auto& fontAwesome = Application::getIconFont("FontAwesome");
            internalLabel->setFont(fontAwesome.getFont(10));
            internalLabel->setText(Application::getIconFont("FontAwesome").getIconCharacter(name));
           // internalLabel->setAlignment(Qt::AlignBottom);
        }
    }
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
        action.setShowFullPathName(true);
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
    , _filterOnIdAction(this, "Filter on Id")
    , _selectedIdAction(this, "Last selected Id")
	, _updateStatisticsAction(this, "Calculate Differential Expression")
    , _tableView(nullptr)
    , _differentialExpressionModel(nullptr)
    , _sortFilterProxyModel(nullptr)
    , _cluster1SectionLabelWidget(nullptr)
    , _cluster2SectionLabelWidget(nullptr)
    , _autoComputeToggleAction(this, "Automatic Update")
	, _updateStatisticsButton(nullptr)
{
    initGui();
    QString pluginID = differentialExpressionPlugin->getGuiName();
    _selectedIdAction.publish(pluginID+": selectedId");
    _updateStatisticsAction.setCheckable(false);
    _updateStatisticsAction.setChecked(false);
    _updateStatisticsAction.publish(pluginID+": calculateStatistics");
}


void ClusterDifferentialExpressionWidget::initTableViewHeader()
{
    WordWrapHeaderView* horizontalHeader = new WordWrapHeaderView(Qt::Horizontal);
    //horizontalHeader->setStretchLastSection(true);
    horizontalHeader->setFirstSectionMovable(false);
    horizontalHeader->setSectionsMovable(true);
    horizontalHeader->setSectionsClickable(true);
    horizontalHeader->sectionResizeMode(QHeaderView::Stretch);
    horizontalHeader->setSectionResizeMode(QHeaderView::Stretch);
    //horizontalHeader->setSectionResizeMode(0,QHeaderView::Interactive);
    horizontalHeader->setSortIndicator(0, Qt::AscendingOrder);
    horizontalHeader->setDefaultAlignment(Qt::AlignBottom | Qt::AlignHCenter | Qt::Alignment(Qt::TextWordWrap));
    _tableView->setHorizontalHeader(horizontalHeader);
    
    { // search action for ID column

        QWidget* clusterHeaderWidget = new QWidget;
        QGridLayout* clusterHeaderWidgetLayout = new QGridLayout(clusterHeaderWidget);
        clusterHeaderWidgetLayout->setContentsMargins(0, 0, 0, 0);
        clusterHeaderWidgetLayout->setHorizontalSpacing(0);
        clusterHeaderWidgetLayout->setVerticalSpacing(0);
        clusterHeaderWidgetLayout->setSpacing(0);
        clusterHeaderWidgetLayout->setSizeConstraint(QLayout::SetFixedSize);

        auto* w1 = _filterOnIdAction.createLabelWidget(this);
        local::setLabelWidgetIcon(w1, "search");
        clusterHeaderWidgetLayout->addWidget(w1, 0, 0, 1, 1, Qt::AlignTop);


        auto* w2 = _filterOnIdAction.createWidget(this);
        clusterHeaderWidgetLayout->addWidget(w2, 0, 1, 1, 1, Qt::AlignTop);

        clusterHeaderWidgetLayout->addWidget(new QLabel(""), 1, 0, 1, 2, Qt::AlignCenter);
        clusterHeaderWidgetLayout->addWidget(new QLabel("ID"), 2, 0, 1, 2, Qt::AlignCenter);
        clusterHeaderWidget->setLayout(clusterHeaderWidgetLayout);

        connect(&_filterOnIdAction, &hdps::gui::StringAction::stringChanged, _sortFilterProxyModel, &SortFilterProxyModel::nameFilterChanged);

        horizontalHeader->setWidget(0, clusterHeaderWidget);
    }

    {
        QWidget* clusterHeaderWidget = new QWidget;
        QGridLayout* clusterHeaderWidgetLayout = new QGridLayout(clusterHeaderWidget);
        clusterHeaderWidgetLayout->setContentsMargins(1, 1, 1, 1);
        clusterHeaderWidgetLayout->setHorizontalSpacing(0);
        clusterHeaderWidgetLayout->setVerticalSpacing(0);
        clusterHeaderWidgetLayout->setSizeConstraint(QLayout::SetFixedSize);

        _clusterDataset1Action.setText(":");
        _clusterDataset1Action.setShowIcon(false);
        _clusterDataset1Action.setShowFullPathName(true);
        _clusters1SelectionAction.setText(":");
        auto* w1 = _clusterDataset1Action.createLabelWidget(horizontalHeader);
        local::setLabelWidgetIcon(w1, "th-large"); // symbol for cluster datasets

        auto* w2 = _clusterDataset1Action.createWidget(horizontalHeader);
        auto* w3 = _clusters1SelectionAction.createLabelWidget(horizontalHeader);
        local::setLabelWidgetIcon(w3, "layer-group");
        auto* w4 = _clusters1SelectionAction.createWidget(horizontalHeader);
        auto* w5 = new QLabel("Mean", horizontalHeader);
        clusterHeaderWidgetLayout->addWidget(w1, 0, 0, 1, 1, Qt::AlignTop);
        clusterHeaderWidgetLayout->addWidget(w2, 0, 1, 1, 1, Qt::AlignTop);
        clusterHeaderWidgetLayout->addWidget(w3, 1, 0, 1, 1, Qt::AlignTop);
        clusterHeaderWidgetLayout->addWidget(w4, 1, 1, 1, 1, Qt::AlignTop);
        clusterHeaderWidgetLayout->addWidget(w5, 2, 0, 1, 2, Qt::AlignCenter);

        clusterHeaderWidget->setLayout(clusterHeaderWidgetLayout);
        horizontalHeader->setWidget(2, clusterHeaderWidget);
    }

    {
        QWidget* clusterHeaderWidget = new QWidget;
        QGridLayout* clusterHeaderWidgetLayout = new QGridLayout(clusterHeaderWidget);
        clusterHeaderWidgetLayout->setContentsMargins(0, 0, 0, 0);
        clusterHeaderWidgetLayout->setHorizontalSpacing(0);
        clusterHeaderWidgetLayout->setVerticalSpacing(0);
        clusterHeaderWidgetLayout->setSpacing(0);
        clusterHeaderWidgetLayout->setSizeConstraint(QLayout::SetFixedSize);

        _clusterDataset2Action.setText(":");
        _clusterDataset2Action.setShowIcon(false);
        _clusters2SelectionAction.setText(":");
        auto* w1 = _clusterDataset2Action.createLabelWidget(horizontalHeader);
        local::setLabelWidgetIcon(w1, "th-large");
        auto* w2 = _clusterDataset2Action.createWidget(horizontalHeader);
        auto* w3 = _clusters2SelectionAction.createLabelWidget(horizontalHeader);
        local::setLabelWidgetIcon(w3, "layer-group");
        auto* w4 = _clusters2SelectionAction.createWidget(horizontalHeader);
        auto* w5 = new QLabel("Mean", horizontalHeader);
        int h11 = w1->height();
        int h12 = w2->height();
        int w21 = w3->height();
        int w22 = w4->height();
        auto* c = new QComboBox(horizontalHeader);
        int wc = c->height();
        delete c;
        int h1 = std::max(h11, h12);
        int h2 = std::max(w21, w22);
        int h3 = w5->height();

        clusterHeaderWidgetLayout->addWidget(w1, 0, 0, 1, 1, Qt::AlignTop);
        clusterHeaderWidgetLayout->addWidget(w2, 0, 1, 1, 1, Qt::AlignTop);
        clusterHeaderWidgetLayout->addWidget(w3, 1, 0, 1, 1, Qt::AlignTop);
        clusterHeaderWidgetLayout->addWidget(w4, 1, 1, 1, 1, Qt::AlignTop);
        clusterHeaderWidgetLayout->addWidget(w5, 2, 0, 1, 2, Qt::AlignCenter);
        clusterHeaderWidget->setLayout(clusterHeaderWidgetLayout);



        horizontalHeader->setWidget(3, clusterHeaderWidget);
        QCoreApplication::processEvents();
    }
	horizontalHeader->enableWidgetSupport(true);

    //horizontalHeader->setCascadingSectionResizes(true);
    
}

void ClusterDifferentialExpressionWidget::initGui()
{

    setAcceptDrops(true);
    const int NumberOfColums = 2;

    QGridLayout* layout = new QGridLayout;
    setLayout(layout);

    int currentRow = 0;
    connect(&_clusterDataset1Action, &DatasetPickerAction::datasetPicked, this, [this](const hdps::Dataset<hdps::DatasetImpl>& dataset) { emit clusters1DatasetChanged(dataset); });
    connect(&_clusters1SelectionAction, &hdps::gui::OptionAction::currentIndexChanged, this, &ClusterDifferentialExpressionWidget::clusters1Selection_CurrentIndexChanged);
    connect(&_clusterDataset2Action, &DatasetPickerAction::datasetPicked, this, [this](const hdps::Dataset<hdps::DatasetImpl>& dataset) { emit clusters2DatasetChanged(dataset); });
    connect(&_clusters2SelectionAction, &hdps::gui::OptionAction::currentIndexChanged, this, &ClusterDifferentialExpressionWidget::clusters2Selection_CurrentIndexChanged);
    
    connect(&_clusterDataset1Action, &DatasetPickerAction::datasetPicked, this, [this](const hdps::Dataset<hdps::DatasetImpl>& dataset) { emit clusters1DatasetChanged(dataset); });
    connect(&_clusters1SelectionAction, &hdps::gui::OptionAction::currentIndexChanged, this, &ClusterDifferentialExpressionWidget::clusters1Selection_CurrentIndexChanged);
    connect(&_clusterDataset2Action, &DatasetPickerAction::datasetPicked, this, [this](const hdps::Dataset<hdps::DatasetImpl>& dataset) { emit clusters2DatasetChanged(dataset); });
    connect(&_clusters2SelectionAction, &hdps::gui::OptionAction::currentIndexChanged, this, &ClusterDifferentialExpressionWidget::clusters2Selection_CurrentIndexChanged);

    { // table view
        _sortFilterProxyModel = new SortFilterProxyModel;
        _tableView = new TableView(this);
        _tableView->setModel(_sortFilterProxyModel);
        _tableView->setSortingEnabled(true);
        _tableView->setSelectionMode(QAbstractItemView::SingleSelection);
        _tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        _tableView->setContextMenuPolicy(Qt::CustomContextMenu);
        

        
        connect(_tableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ClusterDifferentialExpressionWidget::tableView_sectionChanged);

        initTableViewHeader();

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

        
        {
            
            QWidget* w = _updateStatisticsAction.createWidget(this);
            _updateStatisticsButton = w->findChild<hdps::gui::TriggerAction::PushButtonWidget*>();// new QPushButton("Calculate Differential Expression", this);
            _updateStatisticsButton->setText("Calculate Differential Expression");
            layout->addWidget(w, currentRow++, 0, 1, NumberOfColums);
            QObject::connect(&_updateStatisticsAction, &hdps::gui::TriggerAction::triggered, this, [this]() -> void { this->updateStatisticsButtonPressed(); });
            //QObject::connect(updateStatisticsButton, &QPushButton::clicked, this, &ClusterDifferentialExpressionWidget::updateStatisticsButtonPressed);
        }
        
       

        {
            QHBoxLayout* layout2 = new QHBoxLayout;
            
            _autoComputeToggleAction.setChecked(false);
            _autoComputeToggleAction.setEnabled(false);
            layout2->addWidget(_autoComputeToggleAction.createLabelWidget(this), 1, Qt::AlignLeft);
            _autoComputeToggleAction.setText("");
            layout2->addWidget(_autoComputeToggleAction.createWidget(this),99, Qt::AlignLeft);

            _selectedIdAction.setPlaceHolderString("");
            _selectedIdAction.setEnabled(true);
            layout2->addWidget(_selectedIdAction.createLabelWidget(this), 1, Qt::AlignLeft);
            auto* w = _selectedIdAction.createWidget(this);
            w->setEnabled(false);
            layout2->addWidget(w, 99, Qt::AlignLeft);

            

        	layout->addLayout(layout2, currentRow++, 0, 1, NumberOfColums);
        }



      
    }
    
}

void ClusterDifferentialExpressionWidget::setClusters1(QStringList clusters)
{
    auto result = local::updateOptionAction(_clusters1SelectionAction, clusters);
    if(_autoComputeToggleAction.isEnabled())
        if (_autoComputeToggleAction.isChecked())
			_autoComputeToggleAction.setChecked(!clusters.empty());

}

void ClusterDifferentialExpressionWidget::setClusters2( QStringList clusters)
{
    auto result = local::updateOptionAction(_clusters2SelectionAction, clusters);
    if (_autoComputeToggleAction.isEnabled())
        if(_autoComputeToggleAction.isChecked())
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
    bool firstTime = (oldModel == nullptr);
    _differentialExpressionModel = newModel;
    _sortFilterProxyModel->setSourceModel(_differentialExpressionModel);
    if(oldModel)
        delete oldModel;

	//update header    
    QHeaderView* horizontalHeader = _tableView->horizontalHeader();
    if (firstTime)
    {
        horizontalHeader->setSectionResizeMode(QHeaderView::ResizeToContents);
        horizontalHeader->setStretchLastSection(true);
    }
    
    
    emit horizontalHeader->headerDataChanged(Qt::Horizontal, 0, newModel->columnCount());

    QCoreApplication::processEvents();
    if(firstTime)
        _tableView->resizeColumnsToContents();
    horizontalHeader->setSectionResizeMode(QHeaderView::Interactive);
   
    
    
    ShowProgressBar();
    
}

void ClusterDifferentialExpressionWidget::ShowProgressBar()
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

void ClusterDifferentialExpressionWidget::setNrOfExtraColumns(std::size_t offset)
{
    qobject_cast<WordWrapHeaderView*>(_tableView->horizontalHeader())->setExtraLeftSideColumns(offset);
}



void ClusterDifferentialExpressionWidget::ShowComputeButton()
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
      //  if (_autoComputeToggleAction.isChecked())
       //     updateStatisticsButtonPressed();
    }
}

void ClusterDifferentialExpressionWidget::clusters2Selection_CurrentIndexChanged(int index)
{
    if(index >=0)
    {
        QList<int> selection = { index };
        emit clusters2SelectionChanged(selection);
        //if (_autoComputeToggleAction.isChecked())
          //  updateStatisticsButtonPressed();
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
    ShowProgressBar();
    emit computeDE();
}

void ClusterDifferentialExpressionWidget::tableView_Clicked(const QModelIndex &index)
{
    QModelIndex firstColumn = index.sibling(index.row(),  0);
  
    QString selectedGeneName = firstColumn.data().toString();
    _selectedIdAction.setString(selectedGeneName);
    QModelIndex temp = _sortFilterProxyModel->mapToSource(firstColumn);
    auto row = temp.row();
    emit selectedRowChanged(row);
}

void ClusterDifferentialExpressionWidget::tableView_sectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    tableView_Clicked(selected.indexes().first());
}

void ClusterDifferentialExpressionWidget::resizeEvent(QResizeEvent* event) 
{
    QWidget::resizeEvent(event);
    QHeaderView* horizontalHeader = _tableView->horizontalHeader();
    horizontalHeader->setSectionResizeMode(QHeaderView::ResizeToContents);
    QCoreApplication::processEvents();
    horizontalHeader->setSectionResizeMode(QHeaderView::Interactive);
}
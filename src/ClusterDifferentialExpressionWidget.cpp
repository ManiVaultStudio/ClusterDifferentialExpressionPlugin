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



ClusterDifferentialExpressionWidget::ClusterDifferentialExpressionWidget(ClusterDifferentialExpressionPlugin* differentialExpressionPlugin)
    :_differentialExpressionPlugin(differentialExpressionPlugin)
    , _modelIsUpToDate(false)
    , _clusterDataset1LabelAction(this, "Dataset")
    , _clusterDataset2LabelAction(this, "Dataset")
    , _clusters1SelectionAction(this,"Clusters")
    , _clusters2SelectionAction(this, "Clusters")
    , _tableView(nullptr)
    , _differentialExpressionModel(nullptr)
    , _sortFilterProxyModel(nullptr)
    , _clusters1ParentName(nullptr)
    , _clusters2ParentName(nullptr)
	, _cluster1SectionLabelWidget(nullptr)
    , _cluster2SectionLabelWidget(nullptr)
	
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
            gridLayout->addWidget(_clusterDataset1LabelAction.createLabelWidget(this), 0, 0);
            QWidget* w = _clusterDataset1LabelAction.createWidget(this);
            {
                QLineEdit* f = w->findChild<QLineEdit*>();
                f->setDisabled(true); // no manual editing    
            }
            gridLayout->addWidget(w, 0, 1);
            connect(&_clusterDataset1LabelAction, &hdps::gui::StringAction::stringChanged, this, [this](const QString& id) { emit clusters1DatasetChanged(id); });
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
            gridLayout->addWidget(_clusterDataset2LabelAction.createLabelWidget(this), 0, 0);
            gridLayout->addWidget(_clusterDataset2LabelAction.createWidget(this), 0, 1);

            auto* w = _clusterDataset2LabelAction.createWidget(this);
            QLineEdit* f = w->findChild<QLineEdit*>();
            f->setDisabled(true); // no manual editing
            gridLayout->addWidget(w, 0, 1);

            connect(&_clusterDataset1LabelAction, &hdps::gui::StringAction::stringChanged, this, [this](const QString& id) { emit clusters1DatasetChanged(id); });
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
        _tableView->setSortingEnabled(true);
        
        

        WordWrapHeaderView* horizontalHeader = new WordWrapHeaderView(Qt::Horizontal);
        //horizontalHeader->setStretchLastSection(true);
        horizontalHeader->setFirstSectionMovable(false);
        horizontalHeader->setSectionsMovable(true);
        horizontalHeader->sectionResizeMode(QHeaderView::Interactive);
        horizontalHeader->setSectionResizeMode(QHeaderView::Stretch);
        horizontalHeader->setSortIndicator(0, Qt::SortOrder::AscendingOrder);
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

        layout->addWidget(new QLabel("Filter on Name "), currentRow, 0, 1, 1);
        QLineEdit* nameFilter = new QLineEdit(this);
        nameFilter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        connect(nameFilter, SIGNAL(textChanged(QString)), _sortFilterProxyModel, SLOT(nameFilterChanged(QString)));
        layout->addWidget(nameFilter, currentRow++, 1, 1, NumberOfColums-1);
    }
    
}

void ClusterDifferentialExpressionWidget::setClusters1(QStringList clusters)
{
    if(clusters.size())
		_clusters1SelectionAction.initialize(clusters, clusters[0], clusters[0]);
    else
        _clusters1SelectionAction.initialize(clusters);
    
    _cluster1SectionLabelWidget->setEnabled(true);
}

void ClusterDifferentialExpressionWidget::setClusters2( QStringList clusters)
{
    if (clusters.size())
        _clusters2SelectionAction.initialize(clusters, clusters[0], clusters[0]);
    else
        _clusters2SelectionAction.initialize(clusters);
    _cluster2SectionLabelWidget->setEnabled(true);
}

void ClusterDifferentialExpressionWidget::setFirstClusterLabel(QString name)
{
    _clusterDataset1LabelAction.setString(name);
}

void ClusterDifferentialExpressionWidget::setSecondClusterLabel(QString name)
{
    _clusterDataset2LabelAction.setString(name);
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

void ClusterDifferentialExpressionWidget::clusters1Selection_CurrentIndexChanged(int index)
{
    if(index >=0)
    {
        QList<int> selection = { index };
        emit clusters1SelectionChanged(selection);
    }
}

void ClusterDifferentialExpressionWidget::clusters2Selection_CurrentIndexChanged(int index)
{
    if(index >=0)
    {
        QList<int> selection = { index };
        emit clusters2SelectionChanged(selection);
    }
}

void ClusterDifferentialExpressionWidget::updateStatisticsButtonPressed()
{
    ShowUpToDate();
    emit computeDE();
}
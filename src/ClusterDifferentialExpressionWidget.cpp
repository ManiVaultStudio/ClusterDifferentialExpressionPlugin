#include "ClusterDifferentialExpressionWidget.h"
#include "ClusterDifferentialExpressionPlugin.h"

#include "QTableItemModel.h"
#include "SortFilterProxyModel.h"


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
    , _clusters1Selection(nullptr)
    , _clusters2Selection(nullptr)
    , _tableView(nullptr)
    , _differentialExpressionModel(nullptr)
    , _sortFilterProxyModel(nullptr)
    , _clusters1ParentName(nullptr)
    , _clusters2ParentName(nullptr)
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
        QVBoxLayout *vboxLayout = new QVBoxLayout;
        _clusters1ParentName = new QLabel();
        vboxLayout->addWidget(_clusters1ParentName);
        _clusters1Selection = new QComboBox;
        vboxLayout->addWidget(_clusters1Selection);
        connect(_clusters1Selection, &QComboBox::currentIndexChanged, this, &ClusterDifferentialExpressionWidget::clusters1Selection_CurrentIndexChanged);
        QGroupBox* newGroupBox = new QGroupBox("Selection 1");
        newGroupBox->setLayout(vboxLayout);
        layout->addWidget(newGroupBox, currentRow, 0, 1, 1);
        
    }

    {
        QVBoxLayout* vboxLayout = new QVBoxLayout;
        _clusters2ParentName = new QLabel();
        vboxLayout->addWidget(_clusters2ParentName);
        _clusters2Selection = new QComboBox;
        vboxLayout->addWidget(_clusters2Selection);
        connect(_clusters2Selection, &QComboBox::currentIndexChanged, this, &ClusterDifferentialExpressionWidget::clusters2Selection_CurrentIndexChanged);
        QGroupBox* newGroupBox = new QGroupBox("Selection 2");
        newGroupBox->setLayout(vboxLayout);
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

        QHeaderView* horizontalHeader = _tableView->horizontalHeader();
        //horizontalHeader->setStretchLastSection(true);
        horizontalHeader->setFirstSectionMovable(false);
        horizontalHeader->setSectionsMovable(true);
        horizontalHeader->sectionResizeMode(QHeaderView::Interactive);
        horizontalHeader->setSectionResizeMode(QHeaderView::Interactive);
        horizontalHeader->setSortIndicator(0, Qt::SortOrder::AscendingOrder);
        

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
    _clusters1Selection->clear();
    if (clusters.size())
    {
        std::vector < std::pair<QString, std::size_t>> sortedClusters(clusters.size());
        for (std::size_t i = 0; i < sortedClusters.size(); ++i)
        {
            sortedClusters[i].first = clusters[i];
            sortedClusters[i].second = i;
        }
        std::sort(sortedClusters.begin(), sortedClusters.end());
        for (auto cluster : sortedClusters)
        {
            _clusters1Selection->addItem(cluster.first, cluster.second);
        }
        if (_clusters1Selection->count())
            _clusters1Selection->setCurrentIndex(0);
    }
}

void ClusterDifferentialExpressionWidget::setClusters2( QStringList clusters)
{
    _clusters2Selection->clear();
    if(clusters.size())
    {
        std::vector < std::pair<QString, std::size_t>> sortedClusters(clusters.size());
        for(std::size_t i=0; i < sortedClusters.size(); ++i)
        {
            sortedClusters[i].first = clusters[i];
            sortedClusters[i].second = i;
        }
        std::sort(sortedClusters.begin(), sortedClusters.end());
        for (auto cluster : sortedClusters)
        {
            _clusters2Selection->addItem(cluster.first, cluster.second);
        }
        if (_clusters2Selection->count())
            _clusters2Selection->setCurrentIndex(0);
    }
}

void ClusterDifferentialExpressionWidget::setFirstClusterLabel(QString name)
{
    name += ": ";
    if(_clusters1ParentName)
        _clusters1ParentName->setText(name);
}

void ClusterDifferentialExpressionWidget::setSecondClusterLabel(QString name)
{
    name += ": ";
    if (_clusters2ParentName)
        _clusters2ParentName->setText(name);
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
    QList<int> selection = { _clusters1Selection->currentData().toInt() };
    emit clusters1SelectionChanged(selection);
}

void ClusterDifferentialExpressionWidget::clusters2Selection_CurrentIndexChanged(int index)
{
    QList<int> selection = { _clusters2Selection->currentData().toInt() };
    emit clusters2SelectionChanged(selection);
}

void ClusterDifferentialExpressionWidget::updateStatisticsButtonPressed()
{
    ShowUpToDate();
    emit computeDE();
}
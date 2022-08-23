#include "ClusterDifferentialExpressionWidget.h"
#include "ClusterDifferentialExpressionPlugin.h"

#include "QTableItemModel.h"

#include <QComboBox>
#include <QTableView>
#include <QPushButton>
#include <QHeaderView>
#include <QProgressBar>
#include <QGraphicsColorizeEffect>
#include <QSortFilterProxyModel>

ClusterDifferentialExpressionWidget::ClusterDifferentialExpressionWidget(ClusterDifferentialExpressionPlugin* differentialExpressionPlugin)
    :_differentialExpressionPlugin(differentialExpressionPlugin)
    , _modelIsUpToDate(false)
    , _clusters1Selection(nullptr)
    , _clusters2Selection(nullptr)
    , _tableView(nullptr)
    , _differentialExpressionModel(nullptr)
    , _sortFilterProxyModel(nullptr)
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

    { // cluster selection
        _clusters1Selection = new QComboBox;
        layout->addWidget(_clusters1Selection, currentRow, 0, 1, 1);
        connect(_clusters1Selection, &QComboBox::currentIndexChanged, this, &ClusterDifferentialExpressionWidget::clusters1Selection_CurrentIndexChanged);

        _clusters2Selection = new QComboBox;
        layout->addWidget(_clusters2Selection, currentRow++, 1, 1, 1);
        connect(_clusters2Selection, &QComboBox::currentIndexChanged, this, &ClusterDifferentialExpressionWidget::clusters2Selection_CurrentIndexChanged);
    }
    
    { // table view
        _sortFilterProxyModel = new QSortFilterProxyModel;
        _tableView = new QTableView;
        _tableView->setModel(_sortFilterProxyModel);
        _tableView->setSortingEnabled(true);
        _tableView->setSelectionMode(QAbstractItemView::SingleSelection);
        _tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        _tableView->setContextMenuPolicy(Qt::CustomContextMenu);
        _tableView->setSortingEnabled(true);

        QHeaderView* horizontalHeader = _tableView->horizontalHeader();
        horizontalHeader->setStretchLastSection(true);
        horizontalHeader->setSectionsMovable(true);
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
    }
    
}

void ClusterDifferentialExpressionWidget::setClusters1(QStringList clusters)
{
    _clusters1Selection->clear();
    if(clusters.size())
    {
        clusters.sort();
        _clusters1Selection->addItems(clusters);
        if (_clusters1Selection->count())
            _clusters1Selection->setCurrentIndex(0);
    }
}

void ClusterDifferentialExpressionWidget::setClusters2( QStringList clusters)
{
    _clusters2Selection->clear();
    if(clusters.size())
    {
        clusters.sort();
        _clusters2Selection->addItems(clusters);
        if (_clusters2Selection->count() > 1)
            _clusters2Selection->setCurrentIndex(1);
        else if (_clusters2Selection->count())
            _clusters2Selection->setCurrentIndex(0);
    }
}

void ClusterDifferentialExpressionWidget::setData(QTableItemModel* newModel)
{
    auto* oldModel = _sortFilterProxyModel->sourceModel();
    _differentialExpressionModel = newModel;
    _sortFilterProxyModel->setSourceModel(_differentialExpressionModel);
    if(oldModel)
        delete oldModel;

    
    QHeaderView* horizontalHeader = _tableView->horizontalHeader();
    for (auto c = 0; c < horizontalHeader->count(); ++c)
        horizontalHeader->setSectionResizeMode(c, QHeaderView::ResizeToContents);

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
    QList<int> selection = { index };
    emit clusters1SelectionChanged(selection);
}

void ClusterDifferentialExpressionWidget::clusters2Selection_CurrentIndexChanged(int index)
{
    QList<int> selection = { index };
    emit clusters2SelectionChanged(selection);
}

void ClusterDifferentialExpressionWidget::updateStatisticsButtonPressed()
{
    ShowUpToDate();
    emit computeDE();
}
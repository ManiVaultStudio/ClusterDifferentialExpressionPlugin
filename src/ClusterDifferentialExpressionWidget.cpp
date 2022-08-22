#include "ClusterDifferentialExpressionWidget.h"
#include "ClusterDifferentialExpressionPlugin.h"

#include "QTableItemModel.h"

#include <QComboBox>
#include <QTableView>
#include <QPushButton>

#include <QProgressBar>
#include <QGraphicsColorizeEffect>

namespace local
{
    void setClusters(const QStringList& clusters, QComboBox &clusterSelection, ClusterDifferentialExpressionWidget &deWidget)
    {
        clusterSelection.blockSignals(true);
        clusterSelection.clear();
        for (auto c : clusters)
        {
            clusterSelection.addItem(c,clusterSelection.count());
        }
        clusterSelection.setCurrentText(QString()); // make sure nothing is selected
        clusterSelection.blockSignals(false);
        deWidget.ShowOutOfDate();
    }
}



ClusterDifferentialExpressionWidget::ClusterDifferentialExpressionWidget(ClusterDifferentialExpressionPlugin* differentialExpressionPlugin)
    :_differentialExpressionPlugin(differentialExpressionPlugin)
    , _modelIsUpToDate(false)
    , _clusters1Selection(nullptr)
    , _clusters2Selection(nullptr)
    , _tableView(nullptr)
{
    initGui();
}

void ClusterDifferentialExpressionWidget::initGui()
{
    const int NumberOfColums = 2;

    QGridLayout* layout = new QGridLayout;
    setLayout(layout);

    int currentRow = 0;

    { // cluster selection
        _clusters1Selection = new QComboBox;
        layout->addWidget(_clusters1Selection, currentRow, 0, 1, 1);

        _clusters2Selection = new QComboBox;
        layout->addWidget(_clusters2Selection, currentRow++, 1, 1, 1);
    }
    
    _differentialExpressionModel = new QTableItemModel(this, false, 2);
    { // table view
        _tableView = new QTableView;
        _tableView->setModel(_differentialExpressionModel);
        _tableView->setSortingEnabled(true);
        _tableView->setSelectionMode(QAbstractItemView::SingleSelection);
        _tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        _tableView->setContextMenuPolicy(Qt::CustomContextMenu);
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
        QObject::connect(_updateStatisticsButton, &QPushButton::clicked, _differentialExpressionPlugin, &ClusterDifferentialExpressionPlugin::updateData);
    }
    
}

void ClusterDifferentialExpressionWidget::setClusters1(const QStringList & clusters)
{
    local::setClusters(clusters, *_clusters1Selection, *this);
}

void ClusterDifferentialExpressionWidget::setClusters2(const QStringList& clusters)
{
    local::setClusters(clusters, *_clusters2Selection, *this);
}

void ClusterDifferentialExpressionWidget::setData(QTableItemModel* newModel)
{
    auto *oldModel = _tableView->model();
    _differentialExpressionModel = newModel;
    _tableView->setModel(_differentialExpressionModel);
    delete oldModel;
}

void ClusterDifferentialExpressionWidget::ShowUpToDate()
{
    _differentialExpressionModel->setOutDated(false);
    _updateStatisticsButton->hide();
    _progressBar->show();
    if(_differentialExpressionModel->rowCount() != 0)
    {
        _progressBar->setFormat("Results Up-to-Date");
    }
}

void ClusterDifferentialExpressionWidget::ShowOutOfDate()
{
    if (_differentialExpressionModel->rowCount()== 0)
        return;
    _differentialExpressionModel->setOutDated(true);
    _progressBar->hide();
    const QString recomputeText = "Out-of-Date - Recalculate";
    if(_updateStatisticsButton->text() != recomputeText)
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
    _updateStatisticsButton->show();
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

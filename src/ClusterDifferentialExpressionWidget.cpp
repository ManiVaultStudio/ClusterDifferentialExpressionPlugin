#include "ClusterDifferentialExpressionWidget.h"
#include "ClusterDifferentialExpressionPlugin.h"

#include "QTableItemModel.h"
#include "SortFilterProxyModel.h"
#include "WordWrapHeaderView.h"
#include "TableView.h"
#include "ButtonProgressBar.h"

#include <QComboBox>
#include <QTableView>
#include <QPushButton>
#include <QHeaderView>
#include <QProgressBar>
#include <QGraphicsColorizeEffect>

#include <QVBoxLayout>
#include <QResizeEvent>



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
    , _tableView(nullptr)
	,_differentialExpressionModel(nullptr)
	,_buttonProgressBar(nullptr)
{
    initGui();
   
}


void ClusterDifferentialExpressionWidget::setDatasetTooltip(qsizetype index, const QString& label)
{
    if (index < _datasetLabels.size())
        _datasetLabels[index]->setToolTip(label);
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
    horizontalHeader->setDefaultAlignment(Qt::AlignBottom | Qt::AlignLeft | Qt::Alignment(Qt::TextWordWrap));
    _tableView->setHorizontalHeader(horizontalHeader);

    const auto NrOfDatasets = _differentialExpressionPlugin->getLoadedDatasetsAction()->size();
    _datasetLabels.resize(NrOfDatasets);
    for(qsizetype datasetIndex = 0; datasetIndex < NrOfDatasets; ++datasetIndex)
    {
        QWidget* clusterHeaderWidget = new QWidget;
        QGridLayout* clusterHeaderWidgetLayout = new QGridLayout(clusterHeaderWidget);
        clusterHeaderWidgetLayout->setContentsMargins(1, 1, 1, 1);
        clusterHeaderWidgetLayout->setHorizontalSpacing(0);
        clusterHeaderWidgetLayout->setVerticalSpacing(0);
        clusterHeaderWidgetLayout->setSizeConstraint(QLayout::SetFixedSize);

        {
            QWidget* datasetNameWidget = _differentialExpressionPlugin->getLoadedDatasetsAction()->getDatasetNameWidget(datasetIndex, horizontalHeader, 1);
            QLineEdit* internalWidget = datasetNameWidget->findChild<hdps::gui::StringAction::LineEditWidget*>();
            internalWidget->setReadOnly(true);
            internalWidget->setFrame(false);
            /*
            internalWidget->setAttribute(Qt::WA_TranslucentBackground, true);
            internalWidget->setWindowFlag(Qt::FramelessWindowHint, true);// required on Windows for WA_TranslucentBackground
            internalWidget->setAlignment(Qt::AlignHCenter);
            */
            _datasetLabels[datasetIndex] = datasetNameWidget;
            clusterHeaderWidgetLayout->addWidget(datasetNameWidget, 0, 0, Qt::AlignTop);
        }

        {
            QWidget* widget = _differentialExpressionPlugin->getLoadedDatasetsAction()->getClusterSelectionWidget(datasetIndex, horizontalHeader, 1);
            /*
            QComboBox* internalWidget = widget->findChild<hdps::gui::OptionsAction::ComboBoxWidget*>();
            
            if (!internalWidget->lineEdit())
            {
                internalWidget->setEditable(true);
                internalWidget->lineEdit()->setReadOnly(true);
            }
        	if (internalWidget->lineEdit())
            {
                internalWidget->lineEdit()->setAlignment(Qt::AlignHCenter);
                for (int i = 0; i < internalWidget->count(); ++i)
                {
                    internalWidget->setItemData(i, Qt::AlignHCenter, Qt::TextAlignmentRole);
                }
            }
            */
            clusterHeaderWidgetLayout->addWidget(widget, 1, 0,  Qt::AlignTop);
        }


       
        clusterHeaderWidgetLayout->addWidget(new QLabel("Mean", horizontalHeader), 2, 0,  Qt::AlignLeft);

        clusterHeaderWidget->setLayout(clusterHeaderWidgetLayout);
        horizontalHeader->setWidget(2+datasetIndex, clusterHeaderWidget);
    }
	QCoreApplication::processEvents();
	horizontalHeader->enableWidgetSupport(true);

    
    
}

void ClusterDifferentialExpressionWidget::initGui()
{

    setAcceptDrops(true);
    const int NumberOfColums = 1;

    delete layout();

    QGridLayout* newLayout = new QGridLayout;
   
    setLayout(newLayout);

    int currentRow = 0;
  
    {
        QWidget* w = _differentialExpressionPlugin->getSettingsAction().createWidget(this);
        newLayout->addWidget(w, currentRow,0);
        newLayout->setRowStretch(currentRow++, 1);
    }

    { // table view
        
        _tableView = new TableView(this);
        _tableView->setModel(_differentialExpressionPlugin->getSortFiltrProxyModel());
        _tableView->setSortingEnabled(true);
        _tableView->setSelectionMode(QAbstractItemView::SingleSelection);
        _tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        _tableView->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(_tableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ClusterDifferentialExpressionWidget::tableView_sectionChanged);

        initTableViewHeader();

        newLayout->addWidget(_tableView, currentRow, 0);
        newLayout->setRowStretch(currentRow++, 97);
    }

    {



        QLabel* infoTextWidget = new QLabel(this);
        
        StringAction& infoTextAction = _differentialExpressionPlugin->getInfoTextAction();
        
        infoTextWidget->setText(infoTextAction.getString());
        infoTextWidget->setVisible(infoTextAction.isChecked());
        
        connect(&infoTextAction, &StringAction::stringChanged, infoTextWidget, &QLabel::setText);
        connect(&infoTextAction, &StringAction::toggled, infoTextWidget, &QLabel::setVisible);

        /*
        const auto debug = [infoTextWidget](bool b) -> void
        {
            bool x = b;
            infoTextWidget->setVisible(b);
        };
        */
        
        newLayout->addWidget(infoTextWidget, currentRow, 0);
    	newLayout->setRowStretch(currentRow++, 1);
    }
    
    {// Progress bar and update button

        _buttonProgressBar = new ButtonProgressBar(this, _differentialExpressionPlugin->getUpdateStatisticsWidget(this));

        _buttonProgressBar->setProgressBarText("No Data Available");
        _buttonProgressBar->setButtonText("No Data Available", Qt::red);

        newLayout->addWidget(_buttonProgressBar, currentRow, 0 );
        newLayout->setRowStretch(currentRow++, 1);

    }
}



void ClusterDifferentialExpressionWidget::setData(QSharedPointer<QTableItemModel> model)
{

    const auto debug = [](bool b) -> void
    {
        bool x = b;
        if(b)
        {
            int x = 0;
            x++;
        }
        else
        {
            int x = 0;
            x++;
        }
    };

    const bool firstTime = (_differentialExpressionModel == nullptr);
    _differentialExpressionModel = model;
    QHeaderView* horizontalHeader = _tableView->horizontalHeader();
    if(firstTime)
    {
	    // first time
        connect(_differentialExpressionModel.get(), &QTableItemModel::statusChanged, _buttonProgressBar, &ButtonProgressBar::showStatus);
        horizontalHeader->setSectionResizeMode(QHeaderView::ResizeToContents);
        horizontalHeader->setStretchLastSection(true);
    }

    
        
        
	
    emit horizontalHeader->headerDataChanged(Qt::Horizontal, 0, _differentialExpressionModel->columnCount());

    QCoreApplication::processEvents();
    if(firstTime)
        _tableView->resizeColumnsToContents();
    horizontalHeader->setSectionResizeMode(QHeaderView::Interactive);
}



QProgressBar* ClusterDifferentialExpressionWidget::getProgressBar()
{
    return _buttonProgressBar->getProgressBar();
}

void ClusterDifferentialExpressionWidget::setNrOfExtraColumns(std::size_t offset)
{
    qobject_cast<WordWrapHeaderView*>(_tableView->horizontalHeader())->setExtraLeftSideColumns(offset);
}


void ClusterDifferentialExpressionWidget::tableView_Clicked(const QModelIndex &index)
{
    if (_differentialExpressionModel->status() != QTableItemModel::Status::UpToDate)
        return;
    try
    {
        QModelIndex firstColumn = index.sibling(index.row(), 0);

        QString selectedGeneName = firstColumn.data().toString();
        QModelIndex temp = _differentialExpressionPlugin->getSortFiltrProxyModel()->mapToSource(firstColumn);
        auto row = temp.row();
        _differentialExpressionPlugin->getSelectedIdAction().setString(selectedGeneName);
        emit selectedRowChanged(row);
    }
    catch(...)
    {
	    // catch everything
    }
    
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
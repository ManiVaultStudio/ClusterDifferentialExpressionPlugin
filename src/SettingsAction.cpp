/*
#include "SettingsAction.h"
#include "Application.h"
#include "ClusterDifferentialExpressionPlugin.h"
#include "PointData/PointData.h"

#include <QMenu>

using namespace hdps::gui;

SettingsAction::SettingsAction(ClusterDifferentialExpressionPlugin* plugin) :
    PluginAction(plugin, plugin, "Settings")
{
    setText("Settings");

    const auto updateEnabled = [this]() {
       // const auto enabled = _scatterplotPlugin->getPositionDataset().isValid();

       
    };

    //connect(&plugin->getPositionDataset(), &Dataset<Points>::changed, this, updateEnabled);

    updateEnabled();

   // _exportAction.setIcon(hdps::Application::getIconFont("FontAwesome").getIcon("camera"));
   // _exportAction.setDefaultWidgetFlags(TriggerAction::Icon);

}

QMenu* SettingsAction::getContextMenu()
{
    
    auto menu = new QMenu();
    
    return menu;
}

void SettingsAction::addAction(WidgetAction &action, qsizetype priority)
{
    _actions.push_back(&action);
    _priorities.push_back(priority);
}

SettingsAction::Widget::Widget(QWidget* parent, SettingsAction* settingsAction) :
    WidgetActionWidget(parent, settingsAction, Widget::State::Standard),
    _layout(),
    _toolBarWidget(),
    _toolBarLayout(),
    _stateWidgets(),
    _spacerWidgets()
{
    setAutoFillBackground(true);

    _toolBarLayout.setContentsMargins(0, 3, 0, 3);
    _toolBarLayout.setSpacing(4);
    _toolBarLayout.setSizeConstraint(QLayout::SetFixedSize);

    for(qsizetype i=0; i < settingsAction->_actions.size(); ++i)
    {
        addStateWidget(settingsAction->_actions[i], settingsAction->_priorities[i]);
    }
    //addStateWidget(&settingsAction->_currentDatasetAction, 0);
  

    _toolBarLayout.addStretch(1);

    _toolBarWidget.setLayout(&_toolBarLayout);
    
    _layout.addWidget(&_toolBarWidget);
    _layout.addStretch(1);

    setLayout(&_layout);

    _layout.setContentsMargins(4, 4, 4, 4);

    this->installEventFilter(this);
    _toolBarWidget.installEventFilter(this);
}

bool SettingsAction::Widget::eventFilter(QObject* object, QEvent* event)
{
    switch (event->type())
    {
        case QEvent::Resize:
            updateLayout();
            break;

        default:
            break;
    }

    return QObject::eventFilter(object, event);
}

void SettingsAction::Widget::addStateWidget(WidgetAction* widgetAction, const std::int32_t& priority )
{
    _stateWidgets << new WidgetActionStateWidget(this, widgetAction, priority);

   
    _toolBarLayout.addWidget(_stateWidgets.back());
}

void SettingsAction::Widget::updateLayout()
{
    QMap<WidgetActionStateWidget*, Widget::State> states;

    for (auto stateWidget : _stateWidgets)
        states[stateWidget] = Widget::State::Collapsed;

    const auto getWidth = [this, &states]() -> std::uint32_t {
        std::uint32_t width = 2 * _layout.contentsMargins().left();

        for (auto stateWidget : _stateWidgets)
            width += stateWidget->getSizeHint(states[stateWidget]).width();

        for (auto spacerWidget : _spacerWidgets) {
            const auto spacerWidgetIndex    = _spacerWidgets.indexOf(spacerWidget);
            const auto stateWidgetLeft      = _stateWidgets[spacerWidgetIndex];
            const auto stateWidgetRight     = _stateWidgets[spacerWidgetIndex + 1];
            const auto spacerWidgetType     = SpacerWidget::getType(states[stateWidgetLeft], states[stateWidgetRight]);
            const auto spacerWidgetWidth    = SpacerWidget::getWidth(spacerWidgetType);

            width += spacerWidgetWidth;
        }

        return width;
    };

    auto prioritySortedStateWidgets = _stateWidgets;

    std::sort(prioritySortedStateWidgets.begin(), prioritySortedStateWidgets.end(), [](WidgetActionStateWidget* stateWidgetA, WidgetActionStateWidget* stateWidgetB) {
        return stateWidgetA->getPriority() > stateWidgetB->getPriority();
    });

    for (auto stateWidget : prioritySortedStateWidgets) {
        auto cachedStates = states;

        states[stateWidget] = Widget::State::Standard;

        if (getWidth() > static_cast<std::uint32_t>(width())) {
            states = cachedStates;
            break;
        }
    }

    for (auto stateWidget : _stateWidgets)
        stateWidget->setState(states[stateWidget]);

    for (auto spacerWidget : _spacerWidgets) {
        const auto spacerWidgetIndex    = _spacerWidgets.indexOf(spacerWidget);
        const auto stateWidgetLeft      = _stateWidgets[spacerWidgetIndex];
        const auto stateWidgetRight     = _stateWidgets[spacerWidgetIndex + 1];
        const auto spacerWidgetType     = SpacerWidget::getType(states[stateWidgetLeft], states[stateWidgetRight]);

        spacerWidget->setType(spacerWidgetType);
    }
}

SettingsAction::SpacerWidget::SpacerWidget(const Type& type ) :
    QWidget(),
    _type(Type::Divider),
    _layout(new QHBoxLayout())
   // _verticalLine(new QFrame())
{
   // _verticalLine->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
   // _verticalLine->setFrameShape(QFrame::VLine);
   // _verticalLine->setFrameShadow(QFrame::Sunken);

    _layout->setContentsMargins(1, 1, 1, 1);
    _layout->setSpacing(0);
    _layout->setAlignment(Qt::AlignCenter);
  //  _layout->addWidget(_verticalLine);
    
    setType(type);
}

SettingsAction::SpacerWidget::Type SettingsAction::SpacerWidget::getType(const WidgetActionWidget::State& widgetTypeLeft, const WidgetActionWidget::State& widgetTypeRight)
{
    return widgetTypeLeft == WidgetActionWidget::State::Collapsed && widgetTypeRight == WidgetActionWidget::State::Collapsed ? Type::Spacer : Type::Divider;
}

SettingsAction::SpacerWidget::Type SettingsAction::SpacerWidget::getType(const WidgetActionStateWidget* stateWidgetLeft, const WidgetActionStateWidget* stateWidgetRight)
{
    return getType(stateWidgetLeft->getState(), stateWidgetRight->getState());
}

void SettingsAction::SpacerWidget::setType(const Type& type)
{
    _type = type;

    setLayout(_layout);
    setFixedWidth(getWidth(_type));

  //  _verticalLine->setVisible(_type == Type::Divider ? true : false);
}

std::int32_t SettingsAction::SpacerWidget::getWidth(const Type& type)
{
    switch (type)
    {
        case Type::Divider:
            return 14;

        case Type::Spacer:
            return 6;

        default:
            break;
    }

    return 0;
}
*/
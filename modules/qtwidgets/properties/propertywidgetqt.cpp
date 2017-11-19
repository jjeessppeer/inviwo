/*********************************************************************************
 *
 * Inviwo - Interactive Visualization Workshop
 *
 * Copyright (c) 2012-2017 Inviwo Foundation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *********************************************************************************/

#include <modules/qtwidgets/properties/propertywidgetqt.h>
#include <inviwo/core/common/inviwoapplication.h>
#include <inviwo/core/metadata/containermetadata.h>
#include <inviwo/core/util/settings/systemsettings.h>
#include <inviwo/core/properties/property.h>
#include <inviwo/core/properties/propertywidgetfactory.h>
#include <inviwo/core/properties/propertyowner.h>
#include <inviwo/core/properties/propertypresetmanager.h>
#include <inviwo/core/network/networklock.h>
#include <inviwo/core/network/workspacemanager.h>
#include <modules/qtwidgets/inviwoqtutils.h>
#include <inviwo/core/common/moduleaction.h>
#include <inviwo/core/common/inviwomodule.h>

#include <modules/qtwidgets/propertylistwidget.h>

#include <warn/push>
#include <warn/ignore/all>
#include <QApplication>
#include <QDesktopWidget>
#include <QStyleOption>
#include <QPainter>
#include <QToolTip>
#include <QHelpEvent>
#include <QInputDialog>
#include <QClipboard>
#include <QMenu>
#include <QLayout>
#include <QMimeData>
#include <QMessageBox>
#include <warn/pop>

namespace inviwo {

const int PropertyWidgetQt::minimumWidth = 250;
const int PropertyWidgetQt::spacing = 7;
const int PropertyWidgetQt::margin = 0;

PropertyWidgetQt::PropertyWidgetQt(Property* property)
    : QWidget()
    , PropertyWidget(property)
    , parent_(nullptr)
    , baseContainer_(nullptr)
    , maxNumNestedShades_(4)
    , nestedDepth_(0) {

    if (property_) {
        property_->addObserver(this);
        if (auto app = util::getInviwoApplication(property_)) {
            auto& settings = app->getSystemSettings();
            appModeCallback_ = settings.applicationUsageMode_.onChange(
                [this]() { onSetUsageMode(property_->getUsageMode()); });
        }
    }

    setNestedDepth(nestedDepth_);
    setObjectName("PropertyWidget");
    setContextMenuPolicy(Qt::PreventContextMenu);
}

PropertyWidgetQt::~PropertyWidgetQt() {
    if (property_) {
        if (auto app = util::getInviwoApplication(property_)) {
            app->getSystemSettings().applicationUsageMode_.removeOnChange(appModeCallback_);
        }
    }
}

void PropertyWidgetQt::initState() {
    if (property_) {
        setReadOnly(property_->getReadOnly());
        setVisible(property_->getVisible());
    }
}

void PropertyWidgetQt::setVisible(bool visible) {
    bool wasVisible = QWidget::isVisible();
    UsageMode appMode = getApplicationUsageMode();
    if (visible && property_ && property_->getUsageMode() == UsageMode::Development &&
        appMode == UsageMode::Application)
        visible = false;

    QWidget::setVisible(visible);

    if (visible != wasVisible && parent_) parent_->onChildVisibilityChange(this);
}

void PropertyWidgetQt::onSetVisible(bool visible) { setVisible(visible); }

void PropertyWidgetQt::onChildVisibilityChange(PropertyWidgetQt* /*child*/) {
    if (property_) {
        setVisible(property_->getVisible());
    }
}

void PropertyWidgetQt::setReadOnly(bool readonly) { setDisabled(readonly); }

void PropertyWidgetQt::onSetUsageMode(UsageMode /*usageMode*/) {
    setVisible(property_->getVisible());
}

void PropertyWidgetQt::onSetReadOnly(bool readonly) { setReadOnly(readonly); }

void PropertyWidgetQt::onSetSemantics(const PropertySemantics& /*semantics*/) {
    emit updateSemantics(this);
}

std::unique_ptr<QMenu> PropertyWidgetQt::getContextMenu() {
    std::unique_ptr<QMenu> menu = std::make_unique<QMenu>();

    if (auto app = util::getInviwoApplication(property_)) {
        menu->addAction(QString::fromStdString(property_->getDisplayName()));
        menu->addSeparator();

        {  // View mode actions (Developer / Application)
            auto usageModeItem = menu->addMenu(tr("&Usage mode"));
            auto developerUsageModeAction = usageModeItem->addAction(tr("&Developer"));
            developerUsageModeAction->setCheckable(true);
            auto applicationUsageModeAction = usageModeItem->addAction(tr("&Application"));
            applicationUsageModeAction->setCheckable(true);
            auto usageModeActionGroup = new QActionGroup(usageModeItem);
            usageModeActionGroup->addAction(developerUsageModeAction);
            usageModeActionGroup->addAction(applicationUsageModeAction);

            // Set the current selection.
            if (property_->getUsageMode() == UsageMode::Development) {
                developerUsageModeAction->setChecked(true);
            } else if (property_->getUsageMode() == UsageMode::Application) {
                applicationUsageModeAction->setChecked(true);
            }

            // Disable the view mode buttons in Application mode
            developerUsageModeAction->setEnabled(getApplicationUsageMode() ==
                                                 UsageMode::Development);
            applicationUsageModeAction->setEnabled(getApplicationUsageMode() ==
                                                   UsageMode::Development);

            connect(developerUsageModeAction, &QAction::triggered, this,
                    [this]() { property_->setUsageMode(UsageMode::Development); });

            connect(applicationUsageModeAction, &QAction::triggered, this,
                    [this]() { property_->setUsageMode(UsageMode::Application); });
        }
        {
            auto copyAction = menu->addAction("&Copy");
            connect(copyAction, &QAction::triggered, this, [this]() {
                if (!property_) return;

                Serializer serializer("");
                std::vector<Property*> properties = {property_};
                serializer.serialize("Properties", properties, "Property");

                std::stringstream ss;
                serializer.writeFile(ss);
                auto str = ss.str();
                QByteArray data(str.c_str(), static_cast<int>(str.length()));

                auto mimedata = util::make_unique<QMimeData>();
                mimedata->setData(QString("application/x.vnd.inviwo.property+xml"), data);
                mimedata->setData(QString("text/plain"), data);
                QApplication::clipboard()->setMimeData(mimedata.release());
            });

            auto pasteAction = menu->addAction("&Paste");
            if (property_->getReadOnly()) pasteAction->setEnabled(false);
            connect(pasteAction, &QAction::triggered, this, [this, app]() {
                if (!property_) return;

                auto clipboard = QApplication::clipboard();
                auto mimeData = clipboard->mimeData();
                QByteArray data;
                if (mimeData->formats().contains(
                        QString("application/x.vnd.inviwo.property+xml"))) {
                    data = mimeData->data(QString("application/x.vnd.inviwo.property+xml"));
                } else if (mimeData->formats().contains(QString("text/plain"))) {
                    data = mimeData->data(QString("text/plain"));
                }
                std::stringstream ss;
                for (auto d : data) ss << d;

                try {
                    auto d = app->getWorkspaceManager()->createWorkspaceDeserializer(ss, "");
                    std::vector<std::unique_ptr<Property>> properties;
                    d.deserialize("Properties", properties, "Property");
                    if (!properties.empty() && properties.front()) {
                        NetworkLock lock(property_);
                        property_->set(properties.front().get());
                    }
                } catch (AbortException&) {
                }
            });

            auto copyPathAction = menu->addAction("Copy Path");
            connect(copyPathAction, &QAction::triggered, this, [this]() {
                if (!property_) return;
                std::string path = joinString(property_->getPath(), ".");
                QApplication::clipboard()->setText(path.c_str());
            });
        }

        {
            auto factory = app->getPropertyWidgetFactory();
            auto semantics = factory->getSupportedSemanicsForProperty(property_);
            if (semantics.size() > 1) {
                auto semanicsMenu = menu->addMenu(tr("&Semantics"));
                auto semanticsGroup = new QActionGroup(semanicsMenu);

                for (auto& semantic : semantics) {
                    auto action =
                        semanicsMenu->addAction(utilqt::toQString(semantic.getString()));
                    semanticsGroup->addAction(action);
                    action->setCheckable(true);
                    action->setChecked(semantic == property_->getSemantics());
                    action->setData(utilqt::toQString(semantic.getString()));
                }

                connect(
                    semanticsGroup, &QActionGroup::triggered,
                    this, [prop = property_](QAction* action) {
                        PropertySemantics semantics(utilqt::fromQString(action->data().toString()));
                        prop->setSemantics(semantics);
                    });
            }
        }

        menu->addSeparator();
        addPresetMenuActions(menu.get(), app);
        menu->addSeparator();

        auto resetAction = menu->addAction(tr("&Reset to default"));
        resetAction->setToolTip(tr("&Reset the property back to it's initial state"));
        connect(resetAction, &QAction::triggered, this,
                [this]() { property_->resetToDefaultState(); });

        // Module actions.
        addModuleMenuActions(menu.get(), app);
    }

    return menu;
}

void PropertyWidgetQt::addModuleMenuActions(QMenu* menu, InviwoApplication* app) {
    std::map<std::string, std::vector<const ModuleCallbackAction*>> callbackMapPerModule;
    for (auto& moduleAction : app->getCallbackActions()) {
        auto moduleName = moduleAction->getModule()->getIdentifier();
        callbackMapPerModule[moduleName].push_back(moduleAction.get());
    }

    for (auto& elem : callbackMapPerModule) {
        const auto& moduleName = elem.first;
        const auto& actions = elem.second;
        if (actions.empty()) continue;
        auto submenu = menu->addMenu(QString::fromStdString(moduleName));
        for (const auto& mAction : actions) {
            auto actionName = mAction->getActionName();
            auto action = submenu->addAction(QString::fromStdString(actionName));
            connect(action, &QAction::triggered, this,
                    [ app, actionName, property = property_ ](bool /*checked*/) {
                        const auto& mActions = app->getCallbackActions();
                        auto it = std::find_if(mActions.begin(), mActions.end(), [&](auto& item) {
                            return item->getActionName() == actionName;
                        });
                        if (it != mActions.end()) {
                            (*it)->getCallBack().invoke(property);
                        }
                    });
        }
    }
}

void PropertyWidgetQt::addPresetMenuActions(QMenu* menu, InviwoApplication* app) {
    if (!property_) return;

    auto propertyPresetMenu = menu->addMenu("Property Presets");
    auto workspacePresetMenu = menu->addMenu("Workspace Presets");
    auto appPresetMenu = menu->addMenu("Application Presets");

    auto presetManager = app->getPropertyPresetManager();

    {
        auto addPresetToMenu = [presetManager, this](QMenu* menu, const std::string& name,
                                                     Property* property, PropertyPresetType type) {
            auto action = menu->addAction(QString::fromStdString(name));
            if (property->getReadOnly()) action->setEnabled(false);
            connect(action, &QAction::triggered, menu, [presetManager, name, property, type]() {
                presetManager->loadPreset(name, property, type);
            });
        };
        auto savePreset =
            [ presetManager, property = property_ ](QMenu * menu, PropertyPresetType type) {
            // will prompt the user to enter a preset name.
            // returns false if a preset with the same name already exits
            // and the user does not want to overwrite it.
            bool ok;
            QString text = QInputDialog::getText(menu, tr("Save Preset"), tr("Name:"),
                                                 QLineEdit::Normal, "", &ok);
            if (ok && !text.isEmpty()) {
                const auto presetName = text.toStdString();
                bool actionExists = false;
                for (auto p : presetManager->getAvailablePresets(property, type)) {
                    if (p == presetName) {
                        actionExists = true;
                        break;
                    }
                }
                if (actionExists) {
                    if (QMessageBox::question(
                            menu, "Overwrite Preset?",
                            QString("Preset named \"%1\" already exists.<br><br>Overwrite?")
                                .arg(text)) == QMessageBox::No) {
                        return false;
                    }
                }
                // save current state as a preset
                presetManager->savePreset(presetName, property, type);
            }
            return true;
        };
        auto clearPresets = [ presetManager, property = property_ ](PropertyPresetType type) {
            switch (type) {
                case PropertyPresetType::Property:
                    presetManager->clearPropertyPresets(property);
                    break;
                case PropertyPresetType::Workspace:
                    presetManager->clearWorkspacePresets();
                    break;
                case PropertyPresetType::Application:
                    break;
                default:
                    return;
            }
        };

        {
            // application presets
            auto saveAction = appPresetMenu->addAction("Save...");
            connect(saveAction, &QAction::triggered, [=]() {
                while (!savePreset(appPresetMenu, PropertyPresetType::Application))
                    ;
            });
            appPresetMenu->addSeparator();
            for (auto preset :
                 presetManager->getAvailablePresets(property_, PropertyPresetType::Application)) {
                addPresetToMenu(appPresetMenu, preset, property_, PropertyPresetType::Application);
            }
        }
        {
            // workspace presets
            auto saveAction = workspacePresetMenu->addAction("Save...");
            connect(saveAction, &QAction::triggered, [=]() {
                while (!savePreset(workspacePresetMenu, PropertyPresetType::Workspace))
                    ;
            });

            auto clearAction = workspacePresetMenu->addAction("Clear");
            connect(clearAction, &QAction::triggered,
                    [=]() { clearPresets(PropertyPresetType::Workspace); });
            workspacePresetMenu->addSeparator();
            for (auto preset :
                 presetManager->getAvailablePresets(property_, PropertyPresetType::Workspace)) {
                addPresetToMenu(workspacePresetMenu, preset, property_,
                                PropertyPresetType::Workspace);
            }
        }
        {
            // property presets
            auto saveAction = propertyPresetMenu->addAction("Save...");
            connect(saveAction, &QAction::triggered, [=]() {
                while (!savePreset(propertyPresetMenu, PropertyPresetType::Property))
                    ;
            });
            auto clearAction = propertyPresetMenu->addAction("Clear");
            connect(clearAction, &QAction::triggered,
                    [=]() { clearPresets(PropertyPresetType::Property); });
            propertyPresetMenu->addSeparator();

            // add menu entries for already existing presets
            for (auto preset :
                 presetManager->getAvailablePresets(property_, PropertyPresetType::Property)) {
                addPresetToMenu(propertyPresetMenu, preset, property_,
                                PropertyPresetType::Property);
            }
        }
    }
}

UsageMode PropertyWidgetQt::getApplicationUsageMode() {
    return InviwoApplication::getPtr()->getApplicationUsageMode();
}

bool PropertyWidgetQt::event(QEvent* event) {
    if (event->type() == QEvent::ToolTip && property_) {
        auto helpEvent = static_cast<QHelpEvent*>(event);
        QToolTip::showText(helpEvent->globalPos(), utilqt::toQString(property_->getDescription()));
        return true;
    } else if (event->type() == QEvent::MouseButtonRelease) {
        auto mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::RightButton) {
            if (auto menu = getContextMenu()) {
                menu->exec(mouseEvent->globalPos());
                mouseEvent->accept();
                return true;
            }
        }
    }
    return QWidget::event(event);
}

void PropertyWidgetQt::setSpacingAndMargins(QLayout* layout) {
    layout->setContentsMargins(margin, margin, margin, margin);
    layout->setSpacing(spacing);
}

QSize PropertyWidgetQt::minimumSizeHint() const { return PropertyWidgetQt::sizeHint(); }

QSize PropertyWidgetQt::sizeHint() const { return layout()->sizeHint(); }

void PropertyWidgetQt::setNestedDepth(int depth) {
    nestedDepth_ = depth;
    if (nestedDepth_ == 0) {
        // special case for depth zero
        QObject::setProperty("bgType", "toplevel");
    } else {
        QObject::setProperty("bgType", nestedDepth_ % maxNumNestedShades_);
    }
}

int PropertyWidgetQt::getNestedDepth() const { return nestedDepth_; }

PropertyWidgetQt* PropertyWidgetQt::getParentPropertyWidget() const { return parent_; }

InviwoDockWidget* PropertyWidgetQt::getBaseContainer() const { return baseContainer_; }

void PropertyWidgetQt::setParentPropertyWidget(PropertyWidgetQt* parent, InviwoDockWidget* widget) {
    parent_ = parent;
    baseContainer_ = widget;
}

void PropertyWidgetQt::paintEvent(QPaintEvent*) {
    QStyleOption o;
    o.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &o, &p, this);
}

}  // namespace inviwo

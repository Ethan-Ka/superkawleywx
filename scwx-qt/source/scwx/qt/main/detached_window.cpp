#include "detached_window.hpp"

#include <scwx/qt/config/radar_site.hpp>
#include <scwx/qt/ui/collapsible_group.hpp>
#include <scwx/qt/ui/level2_products_widget.hpp>
#include <scwx/qt/ui/level2_settings_widget.hpp>
#include <scwx/qt/ui/level3_products_widget.hpp>
#include <scwx/util/logger.hpp>

#include <fmt/format.h>

#include <QCloseEvent>
#include <QDockWidget>
#include <QTimer>
#include <QScrollArea>
#include <QShowEvent>
#include <QVBoxLayout>
#include <QWidget>

namespace scwx
{
namespace qt
{
namespace main
{

static const std::string logPrefix_ = "scwx::qt::main::detached_window";
static const auto        logger_    = util::Logger::Create(logPrefix_);

// Detached windows reuse slot 0 for settings/layer-model look-ups.
// They are independent at the UI level and do not participate in the
// main-window toolbox, so the shared slot is an acceptable simplification.
static constexpr std::size_t kDetachedId = 0u;

class DetachedWindowImpl : public QObject
{
   Q_OBJECT

public:
   explicit DetachedWindowImpl(DetachedWindow*                self,
                               map::MapWidget*                sourcePane,
                               const QMapLibre::Settings&     settings,
                               std::shared_ptr<gl::GlContext> glContext) :
       self_ {self},
   mapWidget_ {nullptr},
       level2ProductsWidget_ {nullptr},
       level2SettingsWidget_ {nullptr},
       level3ProductsWidget_ {nullptr},
       level2SettingsGroup_ {nullptr},
       // Snapshot source state at construction time
       initialSite_ {nullptr},
       initialGroup_ {common::RadarProductGroup::Unknown},
       initialProduct_ {},
       initialElevation_ {}
   {
      logger_->info("DetachedWindowImpl ctor: begin self={} source={} glContext={}"
                    ,
                    static_cast<const void*>(self_),
                    static_cast<const void*>(sourcePane),
                    static_cast<const void*>(glContext.get()));

      logger_->info("DetachedWindowImpl ctor: creating map widget");
      mapWidget_ = new map::MapWidget(kDetachedId, settings, glContext);
      logger_->info("DetachedWindowImpl ctor: map widget created {}",
                    static_cast<const void*>(mapWidget_));

      if (sourcePane != nullptr)
      {
         logger_->info("DetachedWindowImpl ctor: snapshot source state begin");
         initialSite_      = sourcePane->GetRadarSite();
         initialGroup_     = sourcePane->GetRadarProductGroup();
         initialProduct_   = sourcePane->GetRadarProductName();
         initialElevation_ = sourcePane->GetElevation();
         logger_->info("DetachedWindowImpl ctor: snapshot source state complete");
      }
      else
      {
         logger_->warn("DetachedWindowImpl constructed with null source pane");
      }

      logger_->info("DetachedWindowImpl ctor: complete");
   }

   void BuildToolbox()
   {
      // NOLINTBEGIN(cppcoreguidelines-owning-memory)

      // Scrollable toolbox dock
      auto* dock = new QDockWidget(QObject::tr("Radar Toolbox"), self_);
      dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea |
                            Qt::BottomDockWidgetArea);
      dock->setFeatures(QDockWidget::DockWidgetMovable |
                        QDockWidget::DockWidgetFloatable |
                        QDockWidget::DockWidgetClosable);

      auto* scrollArea           = new QScrollArea(dock);
      auto* scrollAreaContents   = new QWidget(scrollArea);
      auto* scrollAreaLayout     = new QVBoxLayout(scrollAreaContents);
      scrollAreaLayout->setContentsMargins(0, 0, 0, 0);
      scrollAreaLayout->setAlignment(Qt::AlignTop);

      // Level 2 Products
      auto* level2Group = new ui::CollapsibleGroup(QObject::tr("Level 2 Products"),
                                                   scrollAreaContents);
      level2ProductsWidget_ = new ui::Level2ProductsWidget(level2Group);
      level2Group->GetContentsLayout()->addWidget(level2ProductsWidget_);
      scrollAreaLayout->addWidget(level2Group);

      // Level 3 Products
      auto* level3Group = new ui::CollapsibleGroup(QObject::tr("Level 3 Products"),
                                                   scrollAreaContents);
      level3ProductsWidget_ = new ui::Level3ProductsWidget(level3Group);
      level3Group->GetContentsLayout()->addWidget(level3ProductsWidget_);
      scrollAreaLayout->addWidget(level3Group);

      // Level 2 Settings (tilt selector, etc.)
      level2SettingsGroup_ =
         new ui::CollapsibleGroup(QObject::tr("Level 2 Settings"), scrollAreaContents);
      level2SettingsWidget_ = new ui::Level2SettingsWidget(level2SettingsGroup_);
      level2SettingsGroup_->GetContentsLayout()->addWidget(level2SettingsWidget_);
      scrollAreaLayout->addWidget(level2SettingsGroup_);
      level2SettingsGroup_->setVisible(false);

      scrollAreaLayout->addStretch();
      scrollArea->setWidgetResizable(true);
      scrollArea->setWidget(scrollAreaContents);
      dock->setWidget(scrollArea);

      self_->addDockWidget(Qt::RightDockWidgetArea, dock);

      // NOLINTEND(cppcoreguidelines-owning-memory)
   }

   void ConnectSignals()
   {
      // Product selection from toolbox → map
      connect(level2ProductsWidget_,
              &ui::Level2ProductsWidget::RadarProductSelected,
              mapWidget_,
              [this](common::RadarProductGroup group,
                     const std::string&        productName,
                     int16_t                   productCode)
              {
                 mapWidget_->SelectRadarProduct(group, productName, productCode);
              });

      connect(level3ProductsWidget_,
              &ui::Level3ProductsWidget::RadarProductSelected,
              mapWidget_,
              [this](common::RadarProductGroup group,
                     const std::string&        productName,
                     int16_t                   productCode)
              {
                 mapWidget_->SelectRadarProduct(group, productName, productCode);
              });

      connect(level2SettingsWidget_,
              &ui::Level2SettingsWidget::ElevationSelected,
              mapWidget_,
              &map::MapWidget::SelectElevation);

      // Map → toolbox (update UI when product changes)
      connect(mapWidget_,
              &map::MapWidget::RadarSweepUpdated,
              self_,
              [this]()
              {
                 const auto group   = mapWidget_->GetRadarProductGroup();
                 const auto product = mapWidget_->GetRadarProductName();

                 level2ProductsWidget_->UpdateProductSelection(group, product);
                 level3ProductsWidget_->UpdateProductSelection(group, product);

                 const bool isL2 =
                    group == common::RadarProductGroup::Level2;
                 level2SettingsGroup_->setVisible(isL2);
                 if (isL2)
                 {
                    level2SettingsWidget_->UpdateSettings(mapWidget_);
                 }

                 UpdateTitle();
              },
              Qt::QueuedConnection);

      connect(mapWidget_,
              &map::MapWidget::Level3ProductsChanged,
              self_,
              [this]()
              {
                 level3ProductsWidget_->UpdateAvailableProducts(
                    mapWidget_->GetAvailableLevel3Categories());
              },
              Qt::QueuedConnection);

      connect(mapWidget_,
              &map::MapWidget::RadarSiteUpdated,
              self_,
              [this](std::shared_ptr<config::RadarSite> /* site */)
              { UpdateTitle(); },
              Qt::QueuedConnection);

      connect(mapWidget_,
              &map::MapWidget::IncomingLevel2ElevationChanged,
              self_,
              [this](std::optional<float> incoming)
              {
                 level2SettingsWidget_->UpdateIncomingElevation(incoming);
              },
              Qt::QueuedConnection);

   }

   void ApplyInitialState()
   {
      // Switch to source radar site without jumping the map coordinates
      // (the map centres on the site by default in the constructor)
      if (initialSite_ != nullptr)
      {
         mapWidget_->SelectRadarSite(initialSite_, false);
      }

      // Select the source product/group
      mapWidget_->SelectRadarProduct(initialGroup_, initialProduct_);

      // Select elevation for Level 2
      if (initialGroup_ == common::RadarProductGroup::Level2 &&
          initialElevation_.has_value())
      {
         mapWidget_->SelectElevation(*initialElevation_);
      }
   }

   void UpdateTitle()
   {
      auto        radarSite = mapWidget_->GetRadarSite();
      std::string site =
         radarSite != nullptr ? radarSite->id() : std::string("?");
      std::string product = mapWidget_->GetRadarProductName();

      std::string title;

      auto elevation = mapWidget_->GetElevation();
      if (elevation.has_value() &&
          mapWidget_->GetRadarProductGroup() ==
             common::RadarProductGroup::Level2)
      {
         title = fmt::format(
            "SuperKawley Wx \u2014 {} \u2014 {} {:.1f}\u00b0", site, product, *elevation);
      }
      else
      {
         title =
            fmt::format("SuperKawley Wx \u2014 {} \u2014 {}", site, product);
      }

      self_->setWindowTitle(QString::fromStdString(title));
   }

   DetachedWindow* self_;
   map::MapWidget* mapWidget_;

   ui::Level2ProductsWidget* level2ProductsWidget_;
   ui::Level2SettingsWidget* level2SettingsWidget_;
   ui::Level3ProductsWidget* level3ProductsWidget_;
   ui::CollapsibleGroup*     level2SettingsGroup_;

   // Snapshot of source state taken at construction
   std::shared_ptr<config::RadarSite> initialSite_;
   common::RadarProductGroup          initialGroup_;
   std::string                        initialProduct_;
   std::optional<float>               initialElevation_;
};

// ---------------------------------------------------------------------------

DetachedWindow::DetachedWindow(map::MapWidget*                sourcePane,
                               const QMapLibre::Settings&     settings,
                               std::shared_ptr<gl::GlContext> glContext,
                               QWidget*                       parent) :
   QMainWindow(parent),
   p {}
{
   logger_->info("DetachedWindow ctor begin: this={} source={} parent={}",
                 static_cast<const void*>(this),
                 static_cast<const void*>(sourcePane),
                 static_cast<const void*>(parent));

   p = std::make_unique<DetachedWindowImpl>(this, sourcePane, settings, glContext);

   logger_->info("DetachedWindow pimpl created: this={} pimpl={} mapWidget={}"
             ,
             static_cast<const void*>(this),
             static_cast<const void*>(p.get()),
             static_cast<const void*>(p->mapWidget_));

   setAttribute(Qt::WA_DeleteOnClose);
   setWindowTitle(tr("SuperKawley Wx"));
   resize(900, 700);

   // Set MapWidget as the central widget
   setCentralWidget(p->mapWidget_);

   p->BuildToolbox();

   QTimer::singleShot(
      0,
      this,
      [this]()
      {
         if (p == nullptr)
         {
            return;
         }

         logger_->info("DetachedWindow deferred init begin: this={}",
                       static_cast<const void*>(this));

         p->ApplyInitialState();
         p->ConnectSignals();
         p->UpdateTitle();

         logger_->info("DetachedWindow deferred init complete: this={}",
                       static_cast<const void*>(this));
      });

   logger_->info("DetachedWindow ctor complete: this={}",
                 static_cast<const void*>(this));
}

DetachedWindow::~DetachedWindow() = default;

void DetachedWindow::showEvent(QShowEvent* event)
{
   logger_->info("DetachedWindow showEvent: this={}",
                 static_cast<const void*>(this));
   QMainWindow::showEvent(event);
}

void DetachedWindow::closeEvent(QCloseEvent* event)
{
   logger_->info("Detached window closed");
   QMainWindow::closeEvent(event);
}

} // namespace main
} // namespace qt
} // namespace scwx

#include "detached_window.moc"

#include "mirror_window.hpp"

#include <scwx/qt/config/radar_site.hpp>
#include <scwx/qt/ui/collapsible_group.hpp>
#include <scwx/qt/ui/level2_products_widget.hpp>
#include <scwx/qt/ui/level2_settings_widget.hpp>
#include <scwx/qt/ui/level3_products_widget.hpp>
#include <scwx/util/logger.hpp>

#include <fmt/format.h>

#include <QCheckBox>
#include <QCloseEvent>
#include <QDockWidget>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QPointer>
#include <QResizeEvent>
#include <QScrollArea>
#include <QShowEvent>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

namespace scwx
{
namespace qt
{
namespace main
{

static const std::string logPrefix_ = "scwx::qt::main::mirror_window";
static const auto        logger_    = util::Logger::Create(logPrefix_);

static constexpr std::size_t kDetachedId = 0u;

class MirrorWindowImpl : public QObject
{
   Q_OBJECT

public:
   explicit MirrorWindowImpl(MirrorWindow*                  self,
                             map::MapWidget*                sourcePane,
                             const QMapLibre::Settings&     settings,
                             std::shared_ptr<gl::GlContext> glContext) :
       self_ {self},
       source_ {sourcePane},
   mapWidget_ {nullptr},
       level2ProductsWidget_ {nullptr},
       level2SettingsWidget_ {nullptr},
       level3ProductsWidget_ {nullptr},
       level2SettingsGroup_ {nullptr},
       syncProductTilt_ {nullptr},
       syncZoom_ {nullptr},
       syncPan_ {nullptr},
       disconnectedLabel_ {nullptr},
       sourceConnected_ {true},
       // Sync connections — stored so we can disconnect selectively
       productTiltConn_ {},
       zoomPanConn_ {}
   {
      logger_->info("MirrorWindowImpl ctor: begin self={} source={} glContext={}"
                    ,
                    static_cast<const void*>(self_),
                    static_cast<const void*>(sourcePane),
                    static_cast<const void*>(glContext.get()));

      logger_->info("MirrorWindowImpl ctor: creating map widget");
      mapWidget_ = new map::MapWidget(kDetachedId, settings, glContext);
      logger_->info("MirrorWindowImpl ctor: map widget created {}",
                    static_cast<const void*>(mapWidget_));

      if (sourcePane == nullptr)
      {
         logger_->warn("MirrorWindowImpl constructed with null source pane");
      }

      logger_->info("MirrorWindowImpl ctor: complete");
   }

   void BuildToolbox()
   {
      // NOLINTBEGIN(cppcoreguidelines-owning-memory)

      auto* dock = new QDockWidget(QObject::tr("Mirror Controls"), self_);
      dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea |
                            Qt::BottomDockWidgetArea);
      dock->setFeatures(QDockWidget::DockWidgetMovable |
                        QDockWidget::DockWidgetFloatable |
                        QDockWidget::DockWidgetClosable);

      auto* scrollArea         = new QScrollArea(dock);
      auto* scrollAreaContents = new QWidget(scrollArea);
      auto* scrollAreaLayout   = new QVBoxLayout(scrollAreaContents);
      scrollAreaLayout->setContentsMargins(4, 4, 4, 4);
      scrollAreaLayout->setAlignment(Qt::AlignTop);

      // --- Sync toggles ---
      auto* syncGroup  = new QGroupBox(QObject::tr("Sync with Source"), scrollAreaContents);
      auto* syncLayout = new QVBoxLayout(syncGroup);

      syncProductTilt_ = new QCheckBox(QObject::tr("Product / Tilt"), syncGroup);
      syncProductTilt_->setChecked(true);

      syncZoom_ = new QCheckBox(QObject::tr("Zoom"), syncGroup);
      syncZoom_->setChecked(false);

      syncPan_ = new QCheckBox(QObject::tr("Pan"), syncGroup);
      syncPan_->setChecked(false);

      syncLayout->addWidget(syncProductTilt_);
      syncLayout->addWidget(syncZoom_);
      syncLayout->addWidget(syncPan_);
      syncGroup->setLayout(syncLayout);
      scrollAreaLayout->addWidget(syncGroup);

      // Separator
      auto* sep = new QFrame(scrollAreaContents);
      sep->setFrameShape(QFrame::HLine);
      sep->setFrameShadow(QFrame::Sunken);
      scrollAreaLayout->addWidget(sep);

      // --- Local toolbox (only active when sync is off) ---
      auto* level2Group =
         new ui::CollapsibleGroup(QObject::tr("Level 2 Products"), scrollAreaContents);
      level2ProductsWidget_ = new ui::Level2ProductsWidget(level2Group);
      level2Group->GetContentsLayout()->addWidget(level2ProductsWidget_);
      scrollAreaLayout->addWidget(level2Group);

      auto* level3Group =
         new ui::CollapsibleGroup(QObject::tr("Level 3 Products"), scrollAreaContents);
      level3ProductsWidget_ = new ui::Level3ProductsWidget(level3Group);
      level3Group->GetContentsLayout()->addWidget(level3ProductsWidget_);
      scrollAreaLayout->addWidget(level3Group);

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

   void BuildDisconnectedOverlay()
   {
      // NOLINTBEGIN(cppcoreguidelines-owning-memory)
      disconnectedLabel_ = new QLabel(self_);
      disconnectedLabel_->setText(QObject::tr("Source disconnected"));
      disconnectedLabel_->setAlignment(Qt::AlignCenter);
      disconnectedLabel_->setStyleSheet(
         "QLabel { background-color: rgba(0,0,0,180); color: #ff6666; "
         "font-size: 16pt; font-weight: bold; }");
      disconnectedLabel_->setVisible(false);
      // NOLINTEND(cppcoreguidelines-owning-memory)
   }

   // Connect product/tilt sync from source → mirror
   void ConnectProductTilt()
   {
      if (source_ == nullptr)
      {
         return;
      }

      productTiltConn_ =
         connect(source_,
                 &map::MapWidget::RadarSweepUpdated,
                 self_,
                 [this]()
                 {
                    if (source_ == nullptr)
                    {
                       return;
                    }
                    const auto group   = source_->GetRadarProductGroup();
                    const auto product = source_->GetRadarProductName();
                    mapWidget_->SelectRadarProduct(group, product);

                    const auto elevation = source_->GetElevation();
                    if (elevation.has_value() &&
                        group == common::RadarProductGroup::Level2)
                    {
                       mapWidget_->SelectElevation(*elevation);
                    }

                    UpdateTitle();
                 },
                 Qt::QueuedConnection);
   }

   void DisconnectProductTilt()
   {
      if (productTiltConn_)
      {
         disconnect(productTiltConn_);
         productTiltConn_ = QMetaObject::Connection {};
      }
   }

   // Connect zoom/pan sync from source → mirror
   void ConnectZoomPan()
   {
      if (source_ == nullptr)
      {
         return;
      }

      zoomPanConn_ =
         connect(source_,
                 &map::MapWidget::MapParametersChanged,
                 self_,
                 [this](double latitude,
                        double longitude,
                        double zoom,
                        double bearing,
                        double pitch)
                 {
                    // Only forward axes that are individually synced
                    const bool doZoom = syncZoom_ != nullptr && syncZoom_->isChecked();
                    const bool doPan  = syncPan_ != nullptr && syncPan_->isChecked();

                    if (doZoom || doPan)
                    {
                       // Use current mirror values for axes that are not synced
                       mapWidget_->SetMapParameters(latitude, longitude, zoom,
                                                    bearing, pitch);
                    }
                 },
                 Qt::QueuedConnection);
   }

   void DisconnectZoomPan()
   {
      if (zoomPanConn_)
      {
         disconnect(zoomPanConn_);
         zoomPanConn_ = QMetaObject::Connection {};
      }
   }

   void ConnectSignals()
   {
      // Toggle signals — connect/disconnect sync slots at runtime
      connect(syncProductTilt_,
              &QCheckBox::checkStateChanged,
              self_,
              [this](Qt::CheckState state)
              {
                 if (state == Qt::Checked)
                 {
                    ConnectProductTilt();
                    // Immediately sync
                    if (source_ != nullptr)
                    {
                      auto radarSite = source_->GetRadarSite();
                      if (radarSite != nullptr)
                      {
                         mapWidget_->SelectRadarSite(radarSite, false);
                      }

                       mapWidget_->SelectRadarProduct(
                          source_->GetRadarProductGroup(),
                          source_->GetRadarProductName());
                       auto elev = source_->GetElevation();
                       if (elev.has_value() &&
                           source_->GetRadarProductGroup() ==
                              common::RadarProductGroup::Level2)
                       {
                          mapWidget_->SelectElevation(*elev);
                       }
                    }
                    // When product is synced, local toolbox controls are disabled
                    level2ProductsWidget_->setEnabled(false);
                    level3ProductsWidget_->setEnabled(false);
                    level2SettingsWidget_->setEnabled(false);
                 }
                 else
                 {
                    DisconnectProductTilt();
                    level2ProductsWidget_->setEnabled(true);
                    level3ProductsWidget_->setEnabled(true);
                    level2SettingsWidget_->setEnabled(true);
                 }
              });

      connect(syncZoom_,
              &QCheckBox::checkStateChanged,
              self_,
              [this](Qt::CheckState state)
              {
                 // If either zoom or pan is checked, maintain the connection
                 const bool panChecked =
                    syncPan_ != nullptr && syncPan_->isChecked();
                 if (state == Qt::Checked || panChecked)
                 {
                    if (!zoomPanConn_)
                    {
                       ConnectZoomPan();
                    }
                 }
                 else
                 {
                    DisconnectZoomPan();
                 }
              });

      connect(syncPan_,
              &QCheckBox::checkStateChanged,
              self_,
              [this](Qt::CheckState state)
              {
                 const bool zoomChecked =
                    syncZoom_ != nullptr && syncZoom_->isChecked();
                 if (state == Qt::Checked || zoomChecked)
                 {
                    if (!zoomPanConn_)
                    {
                       ConnectZoomPan();
                    }
                 }
                 else
                 {
                    DisconnectZoomPan();
                 }
              });

      // Local product selection (active when sync is OFF)
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

      // Mirror map → toolbox updates
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

      if (source_ != nullptr)
      {
         connect(source_,
                 &map::MapWidget::RadarSiteUpdated,
                 self_,
                 [this](std::shared_ptr<config::RadarSite> site)
                 {
                    const bool syncProductTilt =
                       syncProductTilt_ != nullptr &&
                       syncProductTilt_->isChecked();

                    if (!syncProductTilt || site == nullptr)
                    {
                       return;
                    }

                    mapWidget_->SelectRadarSite(site, false);
                    UpdateTitle();
                 },
                 Qt::QueuedConnection);
      }

      connect(mapWidget_,
              &map::MapWidget::IncomingLevel2ElevationChanged,
              self_,
              [this](std::optional<float> incoming)
              {
                 level2SettingsWidget_->UpdateIncomingElevation(incoming);
              },
              Qt::QueuedConnection);

      // Detect source pane destruction
      if (source_ != nullptr)
      {
         connect(source_,
                 &QObject::destroyed,
                 self_,
                 [this]()
                 {
                    logger_->info("Mirror source pane destroyed");
                    // QPointer has already been zeroed before destroyed fires,
                    // so source_ is already null here — just record the state.
                    sourceConnected_ = false;

                    DisconnectProductTilt();
                    DisconnectZoomPan();

                    if (disconnectedLabel_ != nullptr)
                    {
                       disconnectedLabel_->setVisible(true);
                       disconnectedLabel_->raise();
                    }
                    UpdateTitle();
                 });
      }

      // Apply initial sync state (product/tilt is on by default)
      ConnectProductTilt();
      level2ProductsWidget_->setEnabled(false);
      level3ProductsWidget_->setEnabled(false);
      level2SettingsWidget_->setEnabled(false);
   }

   void ApplyInitialState()
   {
      if (source_ == nullptr)
      {
         return;
      }

      auto radarSite = source_->GetRadarSite();
      if (radarSite != nullptr)
      {
         mapWidget_->SelectRadarSite(radarSite, false);
      }

      mapWidget_->SelectRadarProduct(source_->GetRadarProductGroup(),
                                     source_->GetRadarProductName());

      auto elevation = source_->GetElevation();
      if (elevation.has_value() &&
          source_->GetRadarProductGroup() == common::RadarProductGroup::Level2)
      {
         mapWidget_->SelectElevation(*elevation);
      }
   }

   void UpdateTitle()
   {
      auto        radarSite = mapWidget_->GetRadarSite();
      std::string site =
         radarSite != nullptr ? radarSite->id() : std::string("?");
      std::string product = mapWidget_->GetRadarProductName();

      std::string title;
      auto        elevation = mapWidget_->GetElevation();
      if (elevation.has_value() &&
          mapWidget_->GetRadarProductGroup() ==
             common::RadarProductGroup::Level2)
      {
         title = fmt::format(
            "SuperKawley Wx \u2014 {} \u2014 {} {:.1f}\u00b0 [Mirror]",
            site, product, *elevation);
      }
      else
      {
         title = fmt::format(
            "SuperKawley Wx \u2014 {} \u2014 {} [Mirror]", site, product);
      }

      QString qtitle = QString::fromStdString(title);
      if (!sourceConnected_)
      {
         qtitle += QObject::tr(" [Source disconnected]");
      }

      self_->setWindowTitle(qtitle);
   }

   MirrorWindow*                    self_;
   QPointer<map::MapWidget>         source_;
   map::MapWidget*                  mapWidget_;

   ui::Level2ProductsWidget* level2ProductsWidget_;
   ui::Level2SettingsWidget* level2SettingsWidget_;
   ui::Level3ProductsWidget* level3ProductsWidget_;
   ui::CollapsibleGroup*     level2SettingsGroup_;

   QCheckBox* syncProductTilt_;
   QCheckBox* syncZoom_;
   QCheckBox* syncPan_;
   QLabel*    disconnectedLabel_;

   bool sourceConnected_;

   QMetaObject::Connection productTiltConn_;
   QMetaObject::Connection zoomPanConn_;
};

// ---------------------------------------------------------------------------

MirrorWindow::MirrorWindow(map::MapWidget*                sourcePane,
                           const QMapLibre::Settings&     settings,
                           std::shared_ptr<gl::GlContext> glContext,
                           QWidget*                       parent) :
   QMainWindow(parent),
   p {}
{
   logger_->info("MirrorWindow ctor begin: this={} source={} parent={}",
                 static_cast<const void*>(this),
                 static_cast<const void*>(sourcePane),
                 static_cast<const void*>(parent));

   p = std::make_unique<MirrorWindowImpl>(this, sourcePane, settings, glContext);

   logger_->info("MirrorWindow pimpl created: this={} pimpl={} mapWidget={}"
             ,
             static_cast<const void*>(this),
             static_cast<const void*>(p.get()),
             static_cast<const void*>(p->mapWidget_));

   setAttribute(Qt::WA_DeleteOnClose);
   setWindowTitle(tr("SuperKawley Wx [Mirror]"));
   resize(900, 700);

   setCentralWidget(p->mapWidget_);

   p->BuildToolbox();
   p->BuildDisconnectedOverlay();

   QTimer::singleShot(
      0,
      this,
      [this]()
      {
         if (p == nullptr)
         {
            return;
         }

         logger_->info("MirrorWindow deferred init begin: this={}",
                       static_cast<const void*>(this));

         p->ApplyInitialState();
         p->ConnectSignals();
         p->UpdateTitle();

         logger_->info("MirrorWindow deferred init complete: this={}",
                       static_cast<const void*>(this));
      });

   logger_->info("MirrorWindow ctor complete: this={}",
                 static_cast<const void*>(this));
}

MirrorWindow::~MirrorWindow() = default;

void MirrorWindow::resizeEvent(QResizeEvent* event)
{
   QMainWindow::resizeEvent(event);

   // Keep the disconnected overlay covering the central widget on every resize
   if (p->disconnectedLabel_ != nullptr && centralWidget() != nullptr)
   {
      p->disconnectedLabel_->setGeometry(centralWidget()->geometry());
   }
}

void MirrorWindow::showEvent(QShowEvent* event)
{
   logger_->info("MirrorWindow showEvent: this={}",
                 static_cast<const void*>(this));
   QMainWindow::showEvent(event);

   if (p->disconnectedLabel_ != nullptr && centralWidget() != nullptr)
   {
      p->disconnectedLabel_->setGeometry(centralWidget()->geometry());
   }
}

void MirrorWindow::closeEvent(QCloseEvent* event)
{
   logger_->info("Mirror window closed");
   QMainWindow::closeEvent(event);
}

} // namespace main
} // namespace qt
} // namespace scwx

#include "mirror_window.moc"

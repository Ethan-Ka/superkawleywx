#pragma once

#include <scwx/qt/gl/gl_context.hpp>
#include <scwx/qt/map/map_widget.hpp>

#include <memory>

#include <qmaplibre.hpp>

#include <QMainWindow>

namespace scwx
{
namespace qt
{
namespace main
{

class MirrorWindowImpl;

/**
 * @brief A radar window that mirrors a source pane with selectable sync axes.
 *
 * Opened via right-click → "Mirror in new window". Tracks source pane changes
 * on selectable axes (product/tilt, loop, zoom, pan) via signal/slot pairs
 * that are connected or disconnected at runtime when toggles change.
 *
 * Default sync state: product/tilt ON, loop ON, zoom OFF, pan OFF.
 * When the source pane is closed the window shows a "Source disconnected"
 * overlay but does not close automatically.
 */
class MirrorWindow : public QMainWindow
{
   Q_OBJECT

public:
   /**
    * @param sourcePane  The pane to mirror. Connections are made at
    *                    construction.
    * @param settings    QMapLibre settings (map provider, API keys, etc.)
    * @param glContext   Shared OpenGL context from the main window.
    * @param parent      Optional Qt parent.
    */
   MirrorWindow(map::MapWidget*                sourcePane,
                const QMapLibre::Settings&     settings,
                std::shared_ptr<gl::GlContext> glContext,
                QWidget*                       parent = nullptr);
   ~MirrorWindow();

   void resizeEvent(QResizeEvent* event) override;
   void showEvent(QShowEvent* event) override;
   void closeEvent(QCloseEvent* event) override;

private:
   std::unique_ptr<MirrorWindowImpl> p;
};

} // namespace main
} // namespace qt
} // namespace scwx

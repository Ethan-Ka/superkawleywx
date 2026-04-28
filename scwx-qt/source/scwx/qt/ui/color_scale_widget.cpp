#include <scwx/qt/ui/color_scale_widget.hpp>
#include <scwx/qt/util/product_labels.hpp>

#include <QAction>
#include <QLinearGradient>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QSettings>
#include <QString>

namespace scwx::qt::ui
{

// ---------------------------------------------------------------------------
// Product classification
// ---------------------------------------------------------------------------

enum class ProductType
{
   Reflectivity,
   Velocity,
   ZDR,
   CC,
   KDP,
   Hydrometeor,
   Unknown,
};

static ProductType ClassifyProduct(const QString& code)
{
   // Reflectivity family
   static const QStringList reflCodes {
      "N0B","N1B","N2B","N3B",
      "N0Q","N1Q","N2Q","N3Q",
      "NCR","NET","EET","NVL","DVL",
      "NLA","NML","NHL","DHR","DPA","N0F"
   };
   // Velocity family
   static const QStringList velCodes {
      "N0G","N1G","NAG",
      "N0U","N1U","N2U","N3U",
      "N0S","N1S","N2S","N3S",
      "NSW"
   };
   // Dual-pol
   static const QStringList zdrCodes {"N0X","N1X","N2X","N3X"};
   static const QStringList ccCodes  {"N0C","N1C","N2C","N3C"};
   static const QStringList kdpCodes {"N0K","N1K","N2K","N3K"};
   static const QStringList hmcCodes {"N0H","N1H","N2H","N3H"};

   if (reflCodes.contains(code)) return ProductType::Reflectivity;
   if (velCodes.contains(code))  return ProductType::Velocity;
   if (zdrCodes.contains(code))  return ProductType::ZDR;
   if (ccCodes.contains(code))   return ProductType::CC;
   if (kdpCodes.contains(code))  return ProductType::KDP;
   if (hmcCodes.contains(code))  return ProductType::Hydrometeor;
   return ProductType::Unknown;
}

// ---------------------------------------------------------------------------
// Gradient helpers
// ---------------------------------------------------------------------------

/// Standard NEXRAD reflectivity colour ramp (simplified, 16-step).
static QLinearGradient MakeReflGradient(const QRectF& r)
{
   QLinearGradient g(r.left(), r.top(), r.right(), r.top());
   g.setColorAt(0.00, QColor(0x64, 0x64, 0x64)); // ND / grey
   g.setColorAt(0.06, QColor(0x04, 0xE9, 0xE7)); // 5 dBZ
   g.setColorAt(0.12, QColor(0x01, 0x9F, 0xF4)); // 10
   g.setColorAt(0.19, QColor(0x02, 0x80, 0xFB)); // 15
   g.setColorAt(0.25, QColor(0x02, 0xFD, 0x02)); // 20
   g.setColorAt(0.31, QColor(0x01, 0xC5, 0x01)); // 25
   g.setColorAt(0.38, QColor(0x00, 0x8C, 0x01)); // 30
   g.setColorAt(0.44, QColor(0xFD, 0xFD, 0x01)); // 35
   g.setColorAt(0.50, QColor(0xE7, 0xC0, 0x00)); // 40
   g.setColorAt(0.56, QColor(0xFF, 0x88, 0x00)); // 45
   g.setColorAt(0.63, QColor(0xFF, 0x00, 0x00)); // 50
   g.setColorAt(0.69, QColor(0xD6, 0x00, 0x00)); // 55
   g.setColorAt(0.75, QColor(0xC0, 0x00, 0x00)); // 60
   g.setColorAt(0.81, QColor(0xFF, 0x00, 0xFF)); // 65
   g.setColorAt(0.88, QColor(0x99, 0x55, 0xC8)); // 70
   g.setColorAt(1.00, QColor(0xFF, 0xFF, 0xFF)); // 75+
   return g;
}

/// Bipolar velocity ramp: strong inbound (green) → zero (black) → outbound (red).
static QLinearGradient MakeVelGradient(const QRectF& r)
{
   QLinearGradient g(r.left(), r.top(), r.right(), r.top());
   g.setColorAt(0.00, QColor(0x00, 0xFF, 0x00)); // strong inbound
   g.setColorAt(0.25, QColor(0x00, 0x88, 0x00));
   g.setColorAt(0.50, QColor(0x00, 0x00, 0x00)); // zero
   g.setColorAt(0.75, QColor(0x88, 0x00, 0x00));
   g.setColorAt(1.00, QColor(0xFF, 0x00, 0x00)); // strong outbound
   return g;
}

static QLinearGradient MakeZDRGradient(const QRectF& r)
{
   QLinearGradient g(r.left(), r.top(), r.right(), r.top());
   g.setColorAt(0.00, QColor(0x7F, 0x00, 0x7F)); // very negative
   g.setColorAt(0.40, QColor(0x00, 0x00, 0xAA)); // slightly negative
   g.setColorAt(0.50, QColor(0x00, 0xFF, 0x00)); // zero
   g.setColorAt(0.75, QColor(0xFF, 0xCC, 0x00)); // moderate positive
   g.setColorAt(1.00, QColor(0xFF, 0x00, 0x00)); // high positive
   return g;
}

static QLinearGradient MakeCCGradient(const QRectF& r)
{
   QLinearGradient g(r.left(), r.top(), r.right(), r.top());
   g.setColorAt(0.00, QColor(0xFF, 0x00, 0x00)); // low CC (debris / mixed)
   g.setColorAt(0.50, QColor(0xFF, 0xFF, 0x00));
   g.setColorAt(0.85, QColor(0x00, 0xFF, 0x00)); // high CC (uniform precip)
   g.setColorAt(1.00, QColor(0xFF, 0xFF, 0xFF));
   return g;
}

static QLinearGradient MakeKDPGradient(const QRectF& r)
{
   QLinearGradient g(r.left(), r.top(), r.right(), r.top());
   g.setColorAt(0.00, QColor(0x00, 0x00, 0x80));
   g.setColorAt(0.40, QColor(0x00, 0xFF, 0xFF));
   g.setColorAt(0.70, QColor(0xFF, 0xFF, 0x00));
   g.setColorAt(1.00, QColor(0xFF, 0x00, 0x00));
   return g;
}

/// Hydrometeor classification — discrete category colours.
static QLinearGradient MakeHmcGradient(const QRectF& r)
{
   QLinearGradient g(r.left(), r.top(), r.right(), r.top());
   g.setColorAt(0.00,  QColor(0x00, 0xA0, 0xFF)); // Rain
   g.setColorAt(0.17,  QColor(0x00, 0x40, 0xFF)); // Heavy Rain
   g.setColorAt(0.33,  QColor(0x00, 0xFF, 0x80)); // Big Drops
   g.setColorAt(0.50,  QColor(0xFF, 0xCC, 0x00)); // Graupel
   g.setColorAt(0.67,  QColor(0xFF, 0x66, 0x00)); // Small Hail
   g.setColorAt(0.83,  QColor(0xFF, 0x00, 0x00)); // Large Hail
   g.setColorAt(1.00,  QColor(0xFF, 0x00, 0xFF)); // Ice Crystals
   return g;
}

// ---------------------------------------------------------------------------
// Category band definitions
// ---------------------------------------------------------------------------

struct Band
{
   double     start; ///< fraction of gradient width [0..1]
   double     end;
   QString    label;
};

static QList<Band> BandsForProduct(ProductType type)
{
   switch (type)
   {
   case ProductType::Reflectivity:
      return {
         {0.00, 0.25, "Light"},
         {0.25, 0.50, "Moderate"},
         {0.50, 0.75, "Heavy"},
         {0.75, 1.00, "Extreme"},
      };
   case ProductType::Velocity:
      return {
         {0.00, 0.45, "Inbound"},
         {0.45, 0.55, "Zero"},
         {0.55, 1.00, "Outbound"},
      };
   case ProductType::ZDR:
      return {
         {0.00, 0.45, "Ice / Neg."},
         {0.45, 0.55, "Near-Zero"},
         {0.55, 1.00, "Rain / Large drops"},
      };
   case ProductType::CC:
      return {
         {0.00, 0.35, "Mixed / Debris"},
         {0.35, 0.75, "Transitional"},
         {0.75, 1.00, "Uniform Precip"},
      };
   case ProductType::KDP:
      return {
         {0.00, 0.40, "Low"},
         {0.40, 0.70, "Moderate"},
         {0.70, 1.00, "Heavy Rain"},
      };
   case ProductType::Hydrometeor:
      return {
         {0.00, 0.17, "Rain"},
         {0.17, 0.33, "Heavy Rain"},
         {0.33, 0.50, "Big Drops"},
         {0.50, 0.67, "Graupel"},
         {0.67, 0.83, "Small Hail"},
         {0.83, 1.00, "Large Hail"},
      };
   default:
      return {};
   }
}

static QString UnitLabel(ProductType type)
{
   switch (type)
   {
   case ProductType::Reflectivity: return "dBZ";
   case ProductType::Velocity:     return "kts";
   case ProductType::ZDR:          return "dB";
   case ProductType::CC:           return "CC (0–1)";
   case ProductType::KDP:          return "°/km";
   case ProductType::Hydrometeor:  return "HMC";
   default:                        return "";
   }
}

// ---------------------------------------------------------------------------
// Heights
// ---------------------------------------------------------------------------

static constexpr int kCompactHeight  = 18;
static constexpr int kExpandedHeight = 60;

// ---------------------------------------------------------------------------
// Impl
// ---------------------------------------------------------------------------

class ColorScaleWidgetImpl
{
public:
   QString     productCode_ {};
   ProductType productType_ {ProductType::Unknown};
   bool        expanded_    {false};

   void LoadSettings()
   {
      if (productCode_.isEmpty())
         return;
      QSettings s;
      expanded_ =
         s.value(QString("colorScale/%1/expanded").arg(productCode_), false)
            .toBool();
   }

   void SaveSettings() const
   {
      if (productCode_.isEmpty())
         return;
      QSettings s;
      s.setValue(QString("colorScale/%1/expanded").arg(productCode_), expanded_);
   }
};

// ---------------------------------------------------------------------------
// ColorScaleWidget
// ---------------------------------------------------------------------------

ColorScaleWidget::ColorScaleWidget(QWidget* parent) :
    QWidget(parent), p(std::make_unique<ColorScaleWidgetImpl>())
{
   setFixedHeight(kCompactHeight);
   setToolTip(tr("Left-click to expand / collapse. Right-click for options."));
   setCursor(Qt::PointingHandCursor);
}

ColorScaleWidget::~ColorScaleWidget() = default;

void ColorScaleWidget::UpdateProduct(const QString& productCode)
{
   if (p->productCode_ == productCode)
      return;

   p->productCode_ = productCode;
   p->productType_ = ClassifyProduct(productCode);
   p->LoadSettings();

   const int h = p->expanded_ ? kExpandedHeight : kCompactHeight;
   setFixedHeight(h);
   update();
}

// ---------------------------------------------------------------------------
// paintEvent
// ---------------------------------------------------------------------------

void ColorScaleWidget::paintEvent(QPaintEvent* /*event*/)
{
   QPainter painter(this);
   painter.setRenderHint(QPainter::Antialiasing, false);

   const QRect r = rect();

   // ── Background ──────────────────────────────────────────────────────────
   painter.fillRect(r, QColor(0x18, 0x18, 0x18));

   if (p->productType_ == ProductType::Unknown || p->productCode_.isEmpty())
   {
      painter.setPen(QColor(0x88, 0x88, 0x88));
      painter.drawText(r, Qt::AlignCenter, p->productCode_.isEmpty()
                                              ? tr("No product")
                                              : p->productCode_);
      return;
   }

   // ── Gradient strip ──────────────────────────────────────────────────────
   constexpr int kLabelColW = 46; // width reserved on the left for unit label
   const QRect   gradRect(kLabelColW, 2, r.width() - kLabelColW - 4,
                          kCompactHeight - 4);

   QLinearGradient grad = [&]() -> QLinearGradient
   {
      switch (p->productType_)
      {
      case ProductType::Velocity:     return MakeVelGradient(gradRect);
      case ProductType::ZDR:          return MakeZDRGradient(gradRect);
      case ProductType::CC:           return MakeCCGradient(gradRect);
      case ProductType::KDP:          return MakeKDPGradient(gradRect);
      case ProductType::Hydrometeor:  return MakeHmcGradient(gradRect);
      default:                        return MakeReflGradient(gradRect);
      }
   }();

   painter.fillRect(gradRect, grad);

   // ── Unit label (left side) ───────────────────────────────────────────────
   painter.setPen(QColor(0xDD, 0xDD, 0xDD));
   QFont font = painter.font();
   font.setPointSize(7);
   font.setBold(true);
   painter.setFont(font);
   painter.drawText(QRect(0, 0, kLabelColW - 2, kCompactHeight),
                    Qt::AlignVCenter | Qt::AlignRight,
                    UnitLabel(p->productType_));

   // ── Compact: product label on right ─────────────────────────────────────
   if (!p->expanded_)
   {
      font.setBold(false);
      font.setPointSize(7);
      painter.setFont(font);
      painter.setPen(QColor(0xAA, 0xAA, 0xAA));
      const QString lbl = util::ProductLabel(p->productCode_);
      painter.drawText(QRect(gradRect.right() + 2, 0,
                             r.width() - gradRect.right() - 4, kCompactHeight),
                       Qt::AlignVCenter | Qt::AlignLeft,
                       lbl);
      return;
   }

   // ── Expanded: tick marks + category bands ────────────────────────────────
   const QList<Band> bands = BandsForProduct(p->productType_);
   const int         gx    = gradRect.left();
   const int         gw    = gradRect.width();

   // Draw category band labels in the lower portion (below gradient strip)
   constexpr int kTickRow  = kCompactHeight - 1; // y where ticks begin
   constexpr int kLabelRow = kCompactHeight + 4;  // y where band labels begin

   painter.setPen(QColor(0xCC, 0xCC, 0xCC));
   font.setPointSize(7);
   font.setBold(false);
   painter.setFont(font);

   for (const auto& band : bands)
   {
      const int x0 = gx + static_cast<int>(band.start * gw);
      const int x1 = gx + static_cast<int>(band.end   * gw);

      // Tick at band start
      painter.drawLine(x0, kTickRow, x0, kTickRow + 5);

      // Band label centred
      const int   labelW  = x1 - x0;
      const QRect labelR(x0, kLabelRow, labelW, r.height() - kLabelRow - 2);
      painter.drawText(labelR,
                       Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap,
                       band.label);
   }

   // Tick at far right
   painter.drawLine(gx + gw, kTickRow, gx + gw, kTickRow + 5);

   // Separator line
   painter.setPen(QColor(0x44, 0x44, 0x44));
   painter.drawLine(0, kCompactHeight - 1, r.width(), kCompactHeight - 1);
}

// ---------------------------------------------------------------------------
// mousePressEvent
// ---------------------------------------------------------------------------

void ColorScaleWidget::mousePressEvent(QMouseEvent* event)
{
   if (event->button() == Qt::LeftButton)
   {
      p->expanded_ = !p->expanded_;
      p->SaveSettings();
      setFixedHeight(p->expanded_ ? kExpandedHeight : kCompactHeight);
      update();
      event->accept();
      return;
   }

   if (event->button() == Qt::RightButton)
   {
      QMenu menu(this);

      QAction* alwaysLabels = menu.addAction(tr("Always show labels"));
      alwaysLabels->setCheckable(true);
      alwaysLabels->setChecked(p->expanded_);

      QAction* intensityGuide = menu.addAction(tr("Show intensity guide"));
      menu.addSeparator();
      QAction* editPalette = menu.addAction(tr("Edit color palette"));
      editPalette->setEnabled(false); // Phase 2

      QAction* selected = menu.exec(event->globalPosition().toPoint());

      if (selected == alwaysLabels)
      {
         p->expanded_ = alwaysLabels->isChecked();
         p->SaveSettings();
         setFixedHeight(p->expanded_ ? kExpandedHeight : kCompactHeight);
         update();
      }
      else if (selected == intensityGuide)
      {
         // Toggle to expanded state if not already
         if (!p->expanded_)
         {
            p->expanded_ = true;
            p->SaveSettings();
            setFixedHeight(kExpandedHeight);
            update();
         }
      }

      event->accept();
      return;
   }

   QWidget::mousePressEvent(event);
}

} // namespace scwx::qt::ui

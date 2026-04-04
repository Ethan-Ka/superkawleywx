#include <scwx/qt/ui/help_dock_widget.hpp>
#include <scwx/qt/util/product_labels.hpp>

#include <QLineEdit>
#include <QScrollBar>
#include <QTextBrowser>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

namespace scwx::qt::ui
{

// ---------------------------------------------------------------------------
// Static product-help content
// ---------------------------------------------------------------------------

/// Returns the HTML help article for a given product code family.
/// Falls back to a generic "unknown product" article.
static QString HelpHtml(const QString& productCode)
{
   // Map product code → article key so families share content.
   static const QMap<QString, QString> codeToKey {
      // Reflectivity
      {"N0B","refl"},{"N1B","refl"},{"N2B","refl"},{"N3B","refl"},
      {"N0Q","refl"},{"N1Q","refl"},{"N2Q","refl"},{"N3Q","refl"},
      {"NCR","refl"},{"NLA","refl"},{"NML","refl"},{"NHL","refl"},
      {"DHR","refl"},{"DPA","refl"},
      // Echo tops
      {"NET","tops"},{"EET","tops"},
      // VIL
      {"NVL","vil"},{"DVL","vil"},
      // Velocity
      {"N0G","vel"},{"N1G","vel"},{"NAG","vel"},
      {"N0U","vel"},{"N1U","vel"},{"N2U","vel"},{"N3U","vel"},
      // Storm-relative vel
      {"N0S","srm"},{"N1S","srm"},{"N2S","srm"},{"N3S","srm"},
      // Spectrum width
      {"NSW","sw"},
      // ZDR
      {"N0X","zdr"},{"N1X","zdr"},{"N2X","zdr"},{"N3X","zdr"},
      // CC
      {"N0C","cc"},{"N1C","cc"},{"N2C","cc"},{"N3C","cc"},
      // KDP
      {"N0K","kdp"},{"N1K","kdp"},{"N2K","kdp"},{"N3K","kdp"},
      // Hydrometeor
      {"N0H","hmc"},{"N1H","hmc"},{"N2H","hmc"},{"N3H","hmc"},
      // Derived
      {"NMD","meso"},{"NTV","tvs"},{"NHI","hail"},{"NST","track"},
      {"N0F","noise"},
   };

   static const QMap<QString, QString> articles {
      {"refl", R"(
<h2>Base Reflectivity (dBZ)</h2>
<p>Reflectivity measures how much of the radar beam is scattered back by
precipitation particles. Higher values mean more or larger particles.</p>
<table>
<tr><th>Range</th><th>Description</th></tr>
<tr><td>&lt; 20 dBZ</td><td>Light precipitation or ground clutter</td></tr>
<tr><td>20–30 dBZ</td><td>Light rain</td></tr>
<tr><td>30–40 dBZ</td><td>Moderate rain</td></tr>
<tr><td>40–50 dBZ</td><td>Heavy rain, possible small hail</td></tr>
<tr><td>50–60 dBZ</td><td>Very heavy rain, likely hail</td></tr>
<tr><td>60–65 dBZ</td><td>Extreme precipitation, large hail likely</td></tr>
<tr><td>&gt; 65 dBZ</td><td>Destructive hail core</td></tr>
</table>
<p><b>What to look for:</b> Hook echoes (rotating supercell), bow echoes
(damaging wind), three-body scatter spike (large hail artifact).</p>
)"},
      {"tops", R"(
<h2>Echo Tops (kft)</h2>
<p>The altitude of the highest detected radar echo (≥ 18 dBZ). Higher tops
indicate stronger updrafts and more vigorous convection.</p>
<p><b>What to look for:</b> Rapidly rising tops indicate intensifying storms.
Tops &gt; 50 kft are associated with severe supercells.</p>
)"},
      {"vil", R"(
<h2>Vertically Integrated Liquid (kg/m²)</h2>
<p>VIL sums the liquid water content through the entire depth of the storm.
High VIL is associated with large hail and extreme precipitation.</p>
<p><b>What to look for:</b> VIL density (VIL / echo top height) &gt; 3.5 g/m³
is a useful large-hail indicator.</p>
)"},
      {"vel", R"(
<h2>Base Velocity (kts)</h2>
<p>Measures the speed of precipitation moving toward (inbound, green) or
away from (outbound, red/warm) the radar. Velocity aliasing can produce
false reversals when winds exceed the Nyquist velocity.</p>
<p><b>What to look for:</b>
<ul>
<li><b>Gate-to-gate shear couplet</b> — adjacent strong inbound/outbound gates
indicate rotation. Tight, high-magnitude couplets indicate mesocyclones or
tornadoes.</li>
<li><b>Velocity aliasing</b> — abrupt colour reversal without physical reason.
Cross-check with higher tilts.</li>
</ul></p>
)"},
      {"srm", R"(
<h2>Storm-Relative Velocity (kts)</h2>
<p>Same as base velocity, but the storm motion vector is subtracted. This
removes the translation of the storm and makes rotation features clearer.</p>
<p><b>What to look for:</b> Tighter and more symmetric inbound/outbound couplets
compared to base velocity.</p>
)"},
      {"sw", R"(
<h2>Spectrum Width (kts)</h2>
<p>Measures the spread of radial velocities within each radar sample volume.
High spectrum width indicates turbulence or strong wind shear.</p>
<p><b>What to look for:</b> Broad spectrum width at low levels near a rotating
updraft indicates strong shear associated with mesocyclone development.</p>
)"},
      {"zdr", R"(
<h2>Differential Reflectivity — ZDR (dB)</h2>
<p>ZDR is the ratio of horizontal to vertical returned power. Large positive
values mean oblate (flattened) rain drops. Near-zero means tumbling hail or
ice crystals.</p>
<table>
<tr><th>ZDR value</th><th>Interpretation</th></tr>
<tr><td>&gt; 2 dB</td><td>Large rain drops or size sorting</td></tr>
<tr><td>0–2 dB</td><td>Small rain drops or mixed phase</td></tr>
<tr><td>~0 dB</td><td>Tumbling hail (cross-check with high dBZ)</td></tr>
<tr><td>&lt; 0 dB</td><td>Vertically oriented ice crystals</td></tr>
</table>
<p><b>What to look for:</b> ZDR column (high ZDR extending above the freezing
level) indicates a strong, sustained updraft. ZDR arc on the right forward
flank indicates strong low-level shear — a tornadogenesis contributor.</p>
)"},
      {"cc", R"(
<h2>Correlation Coefficient — CC</h2>
<p>CC measures how similar successive radar pulses are within a sample volume.
High CC (≥ 0.97) indicates uniform precipitation particles. Low CC indicates
mixed or non-meteorological targets.</p>
<table>
<tr><th>CC range</th><th>Interpretation</th></tr>
<tr><td>0.97–1.05</td><td>Uniform liquid precipitation</td></tr>
<tr><td>0.90–0.97</td><td>Mixed phase (rain / hail, melting layer)</td></tr>
<tr><td>0.60–0.80</td><td>Debris, biological, or ground clutter</td></tr>
<tr><td>&lt; 0.60</td><td>Non-meteorological</td></tr>
</table>
<p><b>What to look for:</b> <b>Tornadic Debris Signature (TDS)</b> — low CC
(0.60–0.80) collocated with a velocity couplet at low altitude confirms a
tornado in contact with the ground.</p>
)"},
      {"kdp", R"(
<h2>Specific Differential Phase — KDP (°/km)</h2>
<p>KDP measures the rate of phase difference between horizontal and vertical
pulses. High KDP indicates dense liquid water; it is immune to hail
contamination unlike ZDR and reflectivity.</p>
<table>
<tr><th>KDP</th><th>Interpretation</th></tr>
<tr><td>&gt; 3 °/km</td><td>Very heavy rain rate</td></tr>
<tr><td>1–3 °/km</td><td>Moderate rain</td></tr>
<tr><td>~0 °/km</td><td>Hail or dry ice (no liquid water)</td></tr>
</table>
<p><b>What to look for:</b> High KDP with high dBZ but near-zero ZDR = heavy
rain with hail. KDP foot/column indicates extreme liquid water flux in the
updraft.</p>
)"},
      {"hmc", R"(
<h2>Hydrometeor Classification (HMC)</h2>
<p>An algorithm combines all dual-pol inputs to classify precipitation into
categories: Rain, Heavy Rain, Big Drops, Graupel, Small Hail, Large Hail,
Ice Crystals, and Non-Meteorological.</p>
<p><b>Important:</b> HMC is an estimate, not ground truth. Always cross-reference
with the underlying products (ZDR, CC, KDP). The "Large Hail" category is
particularly prone to false positives with ground clutter.</p>
)"},
      {"meso", R"(
<h2>Mesocyclone Detection (NMD)</h2>
<p>An algorithm that identifies rotating mesocyclone signatures from velocity
data and outputs strength and depth information.</p>
<p><b>What to look for:</b> Mesocyclone strength ≥ 0.5 (50%) and depth ≥ 3 km
are associated with significant tornado risk.</p>
)"},
      {"tvs", R"(
<h2>Tornado Vortex Signature (TVS)</h2>
<p>An algorithm that detects gate-to-gate shear meeting TVS thresholds. A TVS
does not confirm a tornado on the ground — it confirms a tight velocity couplet
meeting algorithmic criteria.</p>
<p><b>Important:</b> Always cross-reference with CC for a Tornadic Debris
Signature (TDS) to confirm ground contact.</p>
)"},
      {"hail", R"(
<h2>Hail Index (NHI)</h2>
<p>Probability of hail (POH) and probability of severe hail (POSH) derived
from the height of the 45 dBZ and 50 dBZ echo tops relative to the freezing
and −20°C levels.</p>
)"},
      {"track", R"(
<h2>Storm Tracking (NST / SCIT)</h2>
<p>Storm Cell Identification and Tracking (SCIT) identifies individual storm
cells, labels them, and extrapolates their future positions based on recent
motion. Used by the escape route planner.</p>
)"},
      {"noise", R"(
<h2>Power Removed Control (N0F)</h2>
<p>An engineering product that shows areas where the signal processor has
removed power (clutter filtering). Not a meteorological product.</p>
)"},
   };

   const QString key = codeToKey.value(productCode, {});
   if (!key.isEmpty() && articles.contains(key))
      return articles[key];

   return QStringLiteral("<h2>%1</h2><p>No guide entry for this product yet.</p>")
      .arg(productCode.isEmpty() ? "No product selected" : productCode);
}

// ---------------------------------------------------------------------------
// Index page
// ---------------------------------------------------------------------------

static QString IndexHtml()
{
   QString html =
      "<h2>Product Guide</h2>"
      "<p>Select a product on the radar pane and right-click → "
      "<b>What am I looking at?</b>, or click a link below.</p>"
      "<h3>Reflectivity</h3><ul>"
      "<li><a href='#N0Q'>Base Reflectivity (N0Q)</a></li>"
      "<li><a href='#N0B'>Base Refl. Hi-res (N0B)</a></li>"
      "<li><a href='#NCR'>Composite Reflectivity (NCR)</a></li>"
      "<li><a href='#NET'>Echo Tops (NET/EET)</a></li>"
      "<li><a href='#NVL'>Vert. Integrated Liquid (NVL/DVL)</a></li>"
      "</ul><h3>Velocity</h3><ul>"
      "<li><a href='#N0U'>Base Velocity (N0U)</a></li>"
      "<li><a href='#N0G'>Base Velocity Hi-res (N0G)</a></li>"
      "<li><a href='#N0S'>Storm-Relative Velocity (N0S)</a></li>"
      "<li><a href='#NSW'>Spectrum Width (NSW)</a></li>"
      "</ul><h3>Dual-Pol</h3><ul>"
      "<li><a href='#N0X'>Differential Refl. ZDR (N0X)</a></li>"
      "<li><a href='#N0C'>Correlation Coeff. CC (N0C)</a></li>"
      "<li><a href='#N0K'>Spec. Diff. Phase KDP (N0K)</a></li>"
      "<li><a href='#N0H'>Hydrometeor Classification (N0H)</a></li>"
      "</ul><h3>Derived / Alerts</h3><ul>"
      "<li><a href='#NMD'>Mesocyclone Detection (NMD)</a></li>"
      "<li><a href='#NTV'>Tornado Vortex Sig. (NTV)</a></li>"
      "<li><a href='#NHI'>Hail Index (NHI)</a></li>"
      "<li><a href='#NST'>Storm Tracking (NST)</a></li>"
      "</ul>";
   return html;
}

// ---------------------------------------------------------------------------
// Impl
// ---------------------------------------------------------------------------

class HelpDockWidgetImpl : public QObject
{
   Q_OBJECT

public:
   explicit HelpDockWidgetImpl(HelpDockWidget* self) :
       self_ {self}, browser_ {nullptr}, searchEdit_ {nullptr}
   {
   }

   HelpDockWidget* self_;
   QTextBrowser*   browser_;
   QLineEdit*      searchEdit_;

   void SetupUi()
   {
      QWidget*     contents = new QWidget(self_);
      QVBoxLayout* layout   = new QVBoxLayout(contents);
      layout->setContentsMargins(4, 4, 4, 4);
      layout->setSpacing(4);

      searchEdit_ = new QLineEdit(contents);
      searchEdit_->setPlaceholderText(tr("Search…"));
      searchEdit_->setClearButtonEnabled(true);
      layout->addWidget(searchEdit_);

      browser_ = new QTextBrowser(contents);
      browser_->setOpenLinks(false); // handle internally
      browser_->setHtml(IndexHtml());
      layout->addWidget(browser_);

      self_->setWidget(contents);

      // Internal link navigation (e.g. #N0Q)
      connect(browser_, &QTextBrowser::anchorClicked,
              this, &HelpDockWidgetImpl::OnAnchorClicked);

      // Live search
      connect(searchEdit_, &QLineEdit::textChanged,
              this, &HelpDockWidgetImpl::OnSearchChanged);
   }

public slots:
   void OnAnchorClicked(const QUrl& url)
   {
      const QString fragment = url.fragment();
      if (!fragment.isEmpty())
      {
         self_->ShowProductHelp(fragment);
      }
   }

   void OnSearchChanged(const QString& text)
   {
      if (text.isEmpty())
      {
         browser_->setHtml(IndexHtml());
         return;
      }
      // Simple: highlight matches using browser find
      browser_->setHtml(IndexHtml());
      browser_->find(text);
   }
};

// ---------------------------------------------------------------------------
// HelpDockWidget
// ---------------------------------------------------------------------------

HelpDockWidget::HelpDockWidget(QWidget* parent) :
    QDockWidget(tr("Product Guide"), parent),
    p(std::make_unique<HelpDockWidgetImpl>(this))
{
   setObjectName("helpDockWidget");
   setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
   p->SetupUi();
}

HelpDockWidget::~HelpDockWidget() = default;

void HelpDockWidget::ShowProductHelp(const QString& productCode)
{
   if (!isVisible())
   {
      show();
      raise();
   }

   const QString label = util::ProductLabel(productCode);
   const QString html  = HelpHtml(productCode);

   p->browser_->setHtml(
      QStringLiteral("<h1>%1</h1><p style='color:#aaa;font-size:small'>%2</p>%3")
         .arg(label, productCode, html));

   // Clear search when navigating directly
   if (!p->searchEdit_->text().isEmpty())
      p->searchEdit_->clear();
}

} // namespace scwx::qt::ui

#include "help_dock_widget.moc"

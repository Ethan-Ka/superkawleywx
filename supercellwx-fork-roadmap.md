# Supercell Wx Fork — Roadmap and Feature Spec

> Fork target: `dpaulat/supercell-wx`
> Status: Planning
> Last updated: 2026-04-02

---

## Table of contents

1. [Feature dependency graph](#dependency-graph)
2. [Phase 1 — Core UI, beginner QOL, and onboarding](#phase-1)
3. [Phase 2 — Analysis overlays and intermediate QOL](#phase-2)
4. [Phase 3 — Chase operations](#phase-3)
5. [Phase 4 — Radar guide and signature reference](#phase-4)
6. [Phase 5 — Broadcast and annotation tools](#phase-5)
7. [Phase 6 — Documentation, extensibility, and advanced features](#phase-6)
8. [Detailed spec: duplicate and mirror windows](#spec-windows)
9. [Detailed spec: product name labels](#spec-labels)
10. [Detailed spec: interactive color scale](#spec-colorscale)
11. [Detailed spec: vertical cross-section / RHI slice](#spec-crosssection)
12. [Detailed spec: waypoint planner and escape route logic](#spec-waypoints)
13. [Detailed spec: chase log](#spec-chaselog)
14. [Detailed spec: offline tile caching](#spec-offline)
15. [Detailed spec: SpotterNetwork native integration](#spec-spotter)
16. [Detailed spec: radar guide and signature library](#spec-guide)
17. [Detailed spec: broadcast window and annotation system](#spec-broadcast)

---

## Feature dependency graph {#dependency-graph}

The diagram below shows which features must be complete before others can begin.
Features with no incoming arrows can be started immediately.

```
Product label map ──────────────────────────────────────┐
                                                         ▼
Interactive color scale ──────────────────────────► Tooltip system ──► Help panel
                                                         │
                                                         ▼
Context menu on pane ──────────────────────────────► Duplicate window
                          │                              │
                          │                              ▼
                          └──────────────────────────► Mirror window
                                                         │
                                                         ▼
Annotation canvas (shared) ──────────────────┬──► Operational annotations
                                             │
                                             └──► Broadcast window
                                                         │
                                                         ▼
                                                  Broadcast overlays

GPS HUD improvements ─────────────────────────────────┐
                                                       ▼
Waypoint planner ──────────────────────────────► Escape route logic
        │                                              │
        │                                              ▼
        └──────────────────────────────────────► Chase log

Offline tile cache ────────────────────────────────────┐
                                                        ▼
SpotterNetwork integration (standalone — no deps)  Disconnected mode

SCIT overlay ──────────────────────────────────────────┐
                                                        ▼
Storm motion overlay ──────────────────────────► Escape route cutoff detection

Cross-section tool (standalone — no deps)
Lightning layer (standalone — no deps)
Warning enhancements (standalone — no deps)
Ground clutter filter (standalone — no deps)
SPC outlook overlay (standalone — no deps)
Road condition overlay (standalone — no deps)
Signature guide (standalone — no deps)
Plugin API (requires: all Phase 1–5 APIs stable)
```

**Build order recommendation (minimizes blocking):**

| Order | Feature | Reason |
|---|---|---|
| 1 | Product label map | Zero risk, isolated, feeds tooltips and title bar |
| 2 | Interactive color scale | Self-contained QWidget subclass |
| 3 | Tooltip system + help panel | Depends on label map |
| 4 | Context menu on pane | Unlocks duplicate and mirror |
| 5 | Duplicate window | Depends on context menu |
| 6 | Mirror window | Depends on duplicate infrastructure |
| 7 | Warning polygon enhancements | Standalone, high value |
| 8 | Cross-section tool | Standalone, high value |
| 9 | Lightning layer | Standalone |
| 10 | GPS HUD improvements | Upstream GPS exists, extending it |
| 11 | Waypoint planner | Depends on GPS HUD |
| 12 | SCIT / storm motion overlay | Needed for escape route cutoff |
| 13 | Escape route logic | Depends on waypoints + SCIT |
| 14 | Chase log | Depends on GPS + waypoints |
| 15 | Offline tile cache | Standalone |
| 16 | SpotterNetwork integration | Standalone |
| 17 | Annotation canvas | Shared infrastructure |
| 18 | Operational annotations | Depends on canvas |
| 19 | Broadcast window | Depends on canvas |
| 20 | Broadcast overlays | Depends on broadcast window |
| 21 | SPC outlook overlay | Standalone |
| 22 | Road condition overlay | Standalone |
| 23 | Signature guide | Standalone |
| 24 | Plugin API | All prior phases stable |

---

## Phase 1 — Core UI, beginner QOL, and onboarding {#phase-1}

### Detachable radar popout window

Right-click any pane in the radar grid → **"Duplicate in new window"**

- Original pane stays in the grid untouched
- Duplicate opens as a full independent window initialized from the source pane's state
  at the moment of duplication (site, product, tilt, zoom, pan all copied over)
- After opening, fully decoupled — no shared state with anything
- Unlimited duplicates, limited only by system resources
- Title bar: `Supercell Wx — {SITE} — {PRODUCT} {TILT}°`, updates live
- Full toolbox: Level 2 / Level 3 product dropdowns, map settings, timeline, tilt selector
- Toolbox docked by default, draggable to floating panel

**Acceptance criteria:**
- Right-clicking any pane shows context menu with "Duplicate in new window"
- Duplicate opens with correct site, product, tilt, zoom, and pan copied from source
- Changing product in duplicate does not affect source pane
- Closing duplicate does not affect source pane or any other window
- Title bar updates when product, site, or tilt changes in the duplicate
- Multiple duplicates can be open simultaneously without performance issues

### Mirror window

Right-click any pane → **"Mirror in new window"**

- Individual sync toggles: product/tilt (on), loop playback (on), zoom (off), pan (off)
- Controls active only for unsynced axes
- Source disconnected state shown if source pane closes
- Title bar: `Supercell Wx — {SITE} — {PRODUCT} {TILT}° [Mirror]`

**Mirror use cases**

| Use case | Zoom sync | Pan sync | Notes |
|---|---|---|---|
| Stream overlay at home | On | On | Full lock-step copy |
| Wide context view | Off | Off | Independent viewport, same product |
| Same position, different product | On | On | Product sync off |

**Acceptance criteria:**
- Mirror opens and tracks source pane product/tilt changes in real time
- Toggling zoom sync off allows independent zoom without affecting source
- Toggling pan sync off allows independent pan without affecting source
- Source disconnected state appears when source pane is closed
- Mirror does not close automatically when source is closed

**Implementation note:**
Use a `PaneStateController` signal/slot architecture. Source pane emits
`ProductChanged`, `TiltChanged`, `ZoomChanged`, `PanChanged` signals.
Mirror subscribes selectively based on toggle state. Connect/disconnect slots
at runtime when toggles change — do not use conditionals in the render loop.

### Window management

- **View → Detached Windows** — lists all open duplicates and mirrors by title
- **View → Reopen Closed Window** — history of recently closed windows with last-known
  settings restored
- Session restore opt-in, off by default, togglable in preferences

---

### Product name labels

Every product code in the pane header, dropdowns, and title bar gets a short label
from a static lookup map. Same map feeds the tooltip system.

**Full product label reference**

*Reflectivity*

| Code | Label |
|---|---|
| N0B / N1B / N2B / N3B | Base Refl. (Hi-res) |
| N0Q / N1Q / N2Q / N3Q | Base Reflectivity |
| NCR | Composite Refl. |
| NET | Echo Tops |
| EET | Enhanced Echo Tops |
| NVL / DVL | Vert. Integrated Liquid |
| NLA | Low Layer Composite Refl. |
| NML | Mid Layer Composite Refl. |
| NHL | High Layer Composite Refl. |
| DHR | Digital Hybrid Scan Refl. |

*Velocity*

| Code | Label |
|---|---|
| N0G / N1G / NAG | Base Velocity (Hi-res) |
| N0U / N1U / N2U / N3U | Base Velocity |
| N0S / N1S / N2S / N3S | Storm-Relative Vel. |
| NSW | Spectrum Width |

*Dual-pol*

| Code | Label |
|---|---|
| N0X / N1X / N2X / N3X | Differential Refl. (ZDR) |
| N0C / N1C / N2C / N3C | Correlation Coeff. (CC) |
| N0K / N1K / N2K / N3K | Spec. Diff. Phase (KDP) |
| N0H / N1H / N2H / N3H | Hydrometeor Class |

*Derived / alerts*

| Code | Label |
|---|---|
| NMD | Mesocyclone Detection |
| NTV | Tornado Vortex Sig. |
| NHI | Hail Index |
| NST | Storm Tracking |
| DPA / DHR | Precip. Array |
| N0F | Power Removed Control |

Missing codes fall back to the raw code — no crash, no blank label.

**Implementation note:**
`static const QMap<QString, QString> kProductLabels` defined once in a shared
utility header (e.g. `scwx/qt/util/product_labels.h`). Accessed by pane header,
title bar formatter, dropdown delegate, and tooltip builder. Never duplicated.

**Acceptance criteria:**
- Every product code in the dropdown shows the label appended
- Pane header shows label next to product code
- Title bar updates label when product changes
- An unrecognized product code shows the raw code, does not crash

---

### Interactive color scale

**Behavior:**
- Left-click expands to labeled scale with tick marks and plain-English categories
- Right-click opens context menu: Always show labels / Show intensity guide /
  Edit color palette
- Expanded state persists per product in user prefs

**Expanded scale layout (reflectivity):**
```
  5   10   15   20   25   30   35   40   45   50   55   60   65  75+
  |    |    |    |    |    |    |    |    |    |    |    |    |    |
[       light        ] [    moderate    ] [    heavy    ] [ extreme ]
```

**dBZ plain-English guide:**

| Range | Description |
|---|---|
| < 20 dBZ | Light precipitation or ground clutter |
| 20–30 dBZ | Light rain |
| 30–40 dBZ | Moderate rain |
| 40–50 dBZ | Heavy rain, possible small hail |
| 50–60 dBZ | Very heavy rain, likely hail |
| 60–65 dBZ | Extreme precipitation, large hail likely |
| > 65 dBZ | Destructive hail core |

**Per-product units:**

| Product | Units | Notes |
|---|---|---|
| Reflectivity | dBZ | Categories above |
| Velocity | kts | Bipolar — inbound green, outbound red, zero line marked |
| ZDR | dB | Positive = large oblate drops, negative = ice crystals |
| CC | 0–1 | High = uniform precip, low = mixed/debris |
| KDP | °/km | High = heavy rain rate |
| Hydrometeor | categories | Discrete labels per band |

**Implementation note:**
Promote the color bar from a passive painted widget to a `QWidget` subclass.
`mousePressEvent` handles left/right click. `paintEvent` renders compact or expanded
state. Product type detected via a `ProductType` enum switch — same enum used by
the label map. Expanded height: compact ~16px, expanded ~52px.

**Acceptance criteria:**
- Left-clicking the color bar expands it with visible tick marks and category labels
- Left-clicking again collapses it
- "Always show labels" persists across app restarts for that product
- Units shown match the active product (dBZ for reflectivity, kts for velocity, etc.)
- Right-click menu appears at cursor position with all three options functional

---

### Contextual hover tooltips

- Most necessary(advanced products, names, etc) product button, tilt selector, color scale, alert badge, and toolbar icon has
  a plain-language tooltip
- Tooltip on product name: full name, units, what to look for, "Learn more" link
- Text sourced from product label map and user guide content

**Acceptance criteria:**
- Hovering any product button shows tooltip within Qt's default delay
- Tooltip text is accurate for the product
- "Learn more" link opens the correct help panel section

### Integrated help panel

- Collapsible sidebar with user guide embedded and searchable
- "What am I looking at?" button on each pane — context-aware to active product
- Tooltips cross-link into the panel

**Acceptance criteria:**
- Help panel opens and closes without affecting radar rendering
- "What am I looking at?" opens the correct article for the active product
- Search returns relevant results

---

### Beginner mode toggle

- Hides advanced dual-pol products from dropdown by default
- Adds "Recommended products" section at top of dropdown
- Contextual prompt: "What should I be looking at?" based on active alert type
- TOR active → suggests N0G for rotation, N0C for debris signature

**Acceptance criteria:**
- Beginner mode hides dual-pol products from the dropdown
- Recommended products section appears at top and works correctly
- Contextual prompt appears when a warning is active in the current view
- Toggling beginner mode off immediately restores full product list

### First-launch setup wizard improvements

- Guided tour with annotated callouts on first launch
- Quick start auto-selects nearest radar site via IP geolocation
- Skip option for experienced users

### Plain-English alert summaries

- Plain-English summary at top of warning click panel
- Radar interpretation tip linking to relevant product

### Keyboard shortcut reference overlay

- `?` key shows full shortcut reference as non-modal overlay
- Organized by category, editable

### Loop playback improvements

- Visible persistent loop speed slider
- Frame counter: `Frame 8 / 12`
- Arrow key frame stepping
- Rock mode (forward then reverse)
- Per-frame timestamp in local time and UTC

**Acceptance criteria:**
- Loop speed slider visible on pane, persists between sessions
- Arrow keys step frames forward and backward correctly
- Rock mode reverses direction at loop boundaries
- Timestamps on frames match actual scan times

### Auto-refresh and staleness indicator

- Time-since-refresh indicator, color-coded: green < 5 min, yellow 5–10 min, red > 10 min
- One-click manual refresh on pane header

### Radar site info panel

- Click site ID label → info card with name, location, status, elevation, distance
  from GPS, current VCP with plain-English label

---

## Phase 2 — Analysis overlays and intermediate QOL {#phase-2}

### Vertical cross-section / RHI slice

See detailed spec below.

### Lightning strike layer

- Real-time overlay via Blitzortung or ENTLN
- Age-faded markers — configurable decay window (5 / 10 / 30 min)
- Toggle CG and IC independently
- Strike density heatmap mode for high-activity periods

**Implementation note:**
Blitzortung provides a free WebSocket feed at `wss://ws.blitzortung.org`. Connect
on layer enable, disconnect on disable or app background. Strikes stored in a
`QList<LightningStrike>` with a timestamp. A `QTimer` fires every 30 seconds to
prune strikes older than the decay window and trigger a repaint. Age-fade computed
as `opacity = 1.0 - (age_seconds / decay_window_seconds)`.

**Acceptance criteria:**
- Lightning strikes appear on the map within 5 seconds of occurrence
- Strikes fade correctly according to the configured decay window
- CG and IC toggles independently show/hide respective strike types
- Disconnecting and reconnecting the layer does not cause duplicate strikes
- No strikes rendered outside the current map viewport (culling)

### Warning polygon enhancements

- Click polygon → full warning text, hazard details (hail size, wind, tornado confirmed)
- PDS warnings visually distinct
- Tornado Emergency, Confirmed TOR, PDS Watch with increasing visual weight
- Optional flashing outline on new warnings (configurable, 60 sec default)
- Severity filter — hide SVR while keeping TOR, etc.

**Acceptance criteria:**
- Clicking a warning polygon opens detail panel with correct NWS text
- PDS warnings render with distinct visual style from standard warnings
- Tornado Emergency is visually distinct from standard Tornado Warning
- Severity filter correctly hides/shows warning types
- Flashing outline appears on newly issued warnings and stops after configured time

### Ground clutter filter control

- Adjustable reflectivity gate filter for near-range clutter suppression
- AP clutter suppression toggle

**Acceptance criteria:**
- Filter slider visibly reduces near-range clutter on reflectivity products
- AP toggle correctly enables/disables anomalous propagation suppression
- Filter state persists per radar site between sessions

### Storm-relative motion overlay

- Storm motion vector arrow on cell centroid from SCIT data
- Ghost outlines at +15, +30, +60 min projected positions
- ETA ring to dropped pin or waypoint

**Implementation note:**
SCIT data comes from the NST (Storm Tracking Information) Level 3 product.
Parse `StormId`, `CurrentPosition` (lat/lon), `Speed` (kts), and `Direction` (°)
fields. Projected position computed as:
```
lat2 = lat1 + (speed_kts * cos(dir_rad) * time_hrs) / 60.0
lon2 = lon1 + (speed_kts * sin(dir_rad) * time_hrs) / (60.0 * cos(lat1_rad))
```
Ghost outlines rendered as semi-transparent copies of the current reflectivity
footprint translated to projected positions. ETA to a pin:
```
distance_nm = haversine(cell_lat, cell_lon, pin_lat, pin_lon)
eta_minutes = (distance_nm / speed_kts) * 60.0
```

**Acceptance criteria:**
- Storm motion arrows appear on active cells when SCIT data is available
- Projected ghost positions update with each new SCIT scan
- ETA ring updates when cell motion changes
- No crash or blank state when SCIT data is unavailable

### SPC convective outlook overlay

- Day 1, Day 2, Day 3 categorical outlook polygons (TSTM through HIGH)
- Toggled independently per day
- Color-coded per SPC standard: TSTM green → MRGL dark green → SLGT yellow →
  ENH orange → MDT red → HIGH magenta
- Click any outlook polygon → category name, probability, and plain-English
  description of what the outlook means
- Auto-refreshes on the standard SPC issuance schedule (Day 1: 0600z, 1300z, 1630z,
  2000z, 0100z; Day 2: 0600z, 1730z; Day 3: 0730z)

**Implementation note:**
SPC publishes outlook polygons as GeoJSON at:
`https://www.spc.noaa.gov/products/outlook/day{N}otlk_{YYYYMMDD}_{HHmm}_cat.lyr.geojson`
Fetch on enable and on schedule. Parse `LABEL` field for category. Render as
semi-transparent filled polygons with border. Opacity configurable (default 35%).

**Acceptance criteria:**
- Day 1/2/3 outlooks render correctly with SPC color coding
- Click on a polygon shows the correct category and description
- Outlooks update automatically on issuance schedule
- Transparency is adjustable without disappearing entirely
- Overlays do not obstruct radar data readability

### Road condition overlay

Live road conditions from state DOT feeds, layered below radar data so storm
coverage naturally masks the overlay where it matters least.

- Default data source: NOAA 511 / state DOT RWIS (Road Weather Information System)
  feeds — pre-configured and working out of the box
- Conditions displayed: dry, wet, snow/ice, flooded, closed
- Color-coded road segments; configurable which condition types are visible
- Automatically suppressed under high reflectivity areas (> 35 dBZ threshold,
  configurable) so it does not compete visually with active storm data
- Data source is configurable — users can substitute a different state DOT feed URL
  for their region if the default does not cover it
- Refresh interval: 10 minutes (configurable)
- Only visible at zoom levels where individual road segments are distinguishable
  (auto-hides at zoomed-out national view)

**Implementation note:**
NOAA's 511 feeds vary by state. The most reliable cross-state source for road
conditions is the NOAA RWIS network via the Operational Road Weather API at
`https://api.weather.gov/points/{lat},{lon}` combined with state DOT GeoJSON feeds
where available. Build a `RoadConditionProvider` abstract base class with a default
`NOAAProvider` implementation. Users can configure a custom URL that returns
GeoJSON with a `condition` property on each LineString feature. The dBZ suppression
mask is computed by checking each road segment's midpoint against the current
reflectivity grid at the lowest tilt — if > threshold, set segment opacity to 0.

**Acceptance criteria:**
- Road conditions visible on the map at street zoom level
- Conditions color-coded correctly by type
- Segments under active storm radar (> 35 dBZ) are suppressed / not visible
- Overlay auto-hides at national zoom level
- Custom data source URL accepted and working
- Refresh happens on schedule without user intervention

---

## Phase 3 — Chase operations {#phase-3}

### GPS speed and heading HUD improvements

GPS support exists upstream. This extends it:

- Speed (mph / kts / kph, configurable)
- Heading in degrees and cardinal direction
- Bearing and distance to current target waypoint
- Bearing to nearest active warning polygon centroid
- Configurable HUD position (corner snap) and scale
- Compact mode for smaller windows

**Implementation note:**
Upstream GPS feeds position via `GpsManager`. Subscribe to its `PositionUpdated`
signal. Bearing to waypoint: `atan2(sin(dLon)*cos(lat2), cos(lat1)*sin(lat2) -
sin(lat1)*cos(lat2)*cos(dLon))` converted to degrees 0–360. Nearest warning
centroid computed by iterating active `AlertManager` polygons each position update.

**Acceptance criteria:**
- Speed readout updates smoothly with GPS position changes
- Bearing to waypoint updates as GPS position changes
- Bearing to nearest warning updates when new warnings are issued
- HUD remains readable at all configured positions and scales
- Compact mode fits in a duplicate window without overlap

### Waypoint planner with escape route logic

See detailed spec below.

### Offline tile caching and disconnected mode

See detailed spec below.

### SpotterNetwork native integration

See detailed spec below.

### Multi-site radar transition assist

- Detects when GPS or current target is closer to an adjacent site
- Non-intrusive notification: "KLZK is now closer — switch?"
- One-click accept transitions at same product and nearest matching tilt
- Optional auto-switch mode

**Implementation note:**
On each GPS position update, compute distance to all NEXRAD sites using the
haversine formula against a bundled site coordinate table. If a different site
is closer than the current site by more than a configurable hysteresis margin
(default 15 nm — prevents toggling at the boundary), emit a `NearerSiteDetected`
signal. The hysteresis prevents rapid switching when near a radar boundary.

**Acceptance criteria:**
- Notification appears when a nearer site is detected
- One-click transition switches site without changing product or tilt
- Hysteresis prevents notification from flickering near a site boundary
- Auto-switch mode switches without requiring user interaction
- Notification does not appear if the user has manually dismissed it for this site

### SPC mesoscale discussion overlay

- Active mesoscale discussions (MDs) displayed as polygons on the map
- Click polygon → full MD text, associated outlook, issuance time
- Color-coded by associated probability (e.g. 40% tornado probability = orange)
- Auto-refreshes every 15 minutes

**Implementation note:**
SPC MDs available at `https://www.spc.noaa.gov/products/md/` as HTML with embedded
polygon coordinates, and as JSON at the SPC API. Parse the JSON feed and render
each active MD as a dashed polygon border (filled at low opacity) distinct from
the solid convective outlook polygons.

**Acceptance criteria:**
- Active MDs render as dashed polygons visually distinct from outlook polygons
- Click opens full MD text
- MDs expire and are removed from the map after their valid time
- Color coding matches associated probability

### Quick-switch site hotkeys

- Assign keyboard shortcuts to up to 8 radar sites
- Press assigned key → instantly switches active pane to that site at current product
- Configured in preferences with a site picker

**Acceptance criteria:**
- Hotkey switches site in < 500ms
- Product and tilt preserved on switch
- Hotkeys configurable per user, saved between sessions
- Conflict detection warns if assigned key is already in use

### Manual storm cell pin

For when SCIT data is unavailable or the storm of interest is not being tracked.

- Right-click anywhere on the map → "Pin storm cell here"
- Dropped pin acts as a manual cell centroid for HUD bearing/distance calculations
- Optionally enter a manual motion vector (speed in kts, direction in degrees)
- If manual vector entered, the pin moves automatically on the configured interval
- Visual indicator distinguishes manual pin from SCIT-tracked cells

**Acceptance criteria:**
- Right-click context menu includes "Pin storm cell here" option
- HUD bearing/distance updates relative to the pin position
- Pin moves correctly when a manual motion vector is configured
- Motion vector input validates that speed is > 0 and direction is 0–360

---

## Phase 4 — Radar guide and signature reference {#phase-4}

### Overview

The radar guide is a standalone floating window accessible from the Help menu and
linkable from tooltips, alert summaries, and the help panel. Split into two modes
sharing a common image viewer and annotation system.

Images sourced externally (NWS training materials, COMET MetEd, College of DuPage
NEXLAB) and linked — not bundled.

### Two modes

**Basic Signatures** — reflectivity and velocity products, no prior knowledge assumed.
**Dual-Pol Signatures** — N0X, N0C, N0K, N0H, with product explanation before signatures.

Modes share the image viewer and any signatures appearing in both (hook echo in Basic
is revisited in Dual-Pol with CC and ZDR alongside).

---

### Basic signatures

**Hook echo** — N0B/N0Q
- Precipitation wrapping around a rotating updraft
- Tight, well-defined hook = strong rotation, possible tornado
- Hotspots: hook tip (tornado), hook body (RFD), inflow notch, forward flank
- Common mistake: curved precip band ≠ true hook

**Bow echo** — N0B/N0Q
- Forward-bulging arc with rear-inflow notch and book-end vortices
- Damaging straight-line winds at apex
- Hotspots: apex, rear-inflow notch, book-end vortices

**Three-body scatter spike (TBSS)** — N0B/N0Q
- False reflectivity spike 30–60° downrange from hail core
- Indicates large hail (> 2") — entirely an artifact, nothing is there
- Common mistake: mistaken for precipitation

**Gate-to-gate shear couplet** — N0G/N0U/N0S
- Adjacent strong inbound and outbound gates
- Tight high-magnitude couplet = mesocyclone or tornado
- Hotspots: inbound max, outbound max, couplet center
- Common mistake: velocity aliasing creates false couplets

**Inflow notch** — N0B/N0Q
- V-shaped indentation on the inflow (SE/E) side
- Strong organized updraft ingesting warm moist air

**Storm top divergence** — upper-tilt velocity
- Outbound returns on all sides of storm top at upper tilts
- Strong updraft reaching tropopause and spreading out

**BWER / vault** — vertical cross-section (RHI slice tool)
- Low-reflectivity region extending upward into high-dBZ core
- Only visible on cross-section — powerful updraft lofting precip before fallout

---

### Dual-pol signatures

**ZDR column** — N0X
- ZDR measures horizontal vs vertical return ratio
- High positive ZDR column above freezing level = strong sustained updraft
- Hotspots: column top (updraft strength), ZDR arc (wind shear sorting)

**Hail signature** — N0X + N0B
- High reflectivity (> 55 dBZ) + near-zero ZDR = large hail
- Tumbling hailstones return equal H/V power, driving ZDR to zero
- Common mistake: expecting high dBZ to mean high ZDR — hail breaks this

**Tornadic debris signature (TDS)** — N0C
- CC measures return similarity across pulse volume
- Low CC (0.60–0.80) collocated with velocity couplet at low altitude = lofted debris
- Confirms tornado in contact with ground
- Always cross-reference with velocity — isolated low CC may be biological

**Melting layer / bright band** — N0C
- Ring of reduced CC at uniform range during stratiform precip
- Artifact of beam geometry meeting the melting layer — not a storm feature
- Common mistake: mistaken for a precipitation feature

**KDP foot / column** — N0K
- KDP measures phase shift rate — high KDP = heavy rain, immune to hail contamination
- High KDP (> 2–3 °/km) = extreme liquid water content
- KDP near zero with high dBZ = hail core not rain core

**ZDR arc** — N0X
- High positive ZDR band along right-forward flank
- Wind shear size-sorting drops — sign of strong low-level shear, tornadogenesis contributor

**Hydrometeor classification** — N0H
- Algorithmic output from combined dual-pol inputs
- Categories: Rain, Heavy Rain, Big Drops, Graupel, Small/Large Hail, Ice Crystals, etc.
- Not ground truth — cross-reference with underlying products
- Common mistake: treating hail category as definitive

---

### Image viewer spec

- External linked images, not bundled
- Static callout arrows identifying major features
- Clickable hotspot regions — hover = tooltip, click = full entry in text panel
- Side-by-side multi-product views where needed (hook on reflectivity + couplet on
  velocity shown simultaneously)
- "View in archive" button (Phase 6 — links to known archive event)

**Acceptance criteria:**
- Guide window opens from Help menu without affecting radar rendering
- All images load from external links; graceful fallback if offline
- Hotspot tooltips appear on hover
- Clicking a hotspot opens the correct entry in the text panel
- Basic and Dual-Pol modes toggle without reloading the window

---

## Phase 5 — Broadcast and annotation tools {#phase-5}

### Operational annotation mode

Toggled from **View → Annotations → Enable Annotation Mode** or the **"Annotation Mode"**
button in the menu bar (right of last menu item).

- Single toggle applies to all panes simultaneously
- "Annotation Mode" button always visible when feature is available
- Click to disable: empty canvas = immediate; annotations present = confirmation dialog
  "Clear and exit annotation mode?" → Clear and Exit / Cancel

**Mini floating toolbar** (draggable, snaps to pane edge):
Tools: freehand pen, arrow, text label, circle, rectangle, color picker
(red, yellow, white, cyan, orange), stroke width dropdown (Thin/Medium/Thick/Extra Thick),
clear all (with confirmation if not empty)

**Persistence dropdown:**

| Setting | Behavior |
|---|---|
| Clear on product change | Wipes on product switch |
| Clear on site change | Wipes on site switch |
| Clear on both (default) | Wipes on either |
| Never | Manual clear only |

Annotations stored in map coordinates (lat/lon) — stay anchored on pan/zoom.
Loop behavior: persistent across all frames, not frame-stamped.
Save: `Ctrl+Shift+S` bakes current frame + canvas to PNG.

**Acceptance criteria:**
- Annotation mode button visible in menu bar when enabled
- Drawing on one pane does not affect any other pane's data
- Annotations remain anchored when map is panned or zoomed
- Annotations persist across loop frames
- Clear confirmation dialog appears only when canvas has content
- Saved screenshot includes both radar frame and annotations correctly composited

### Broadcast window

Opened from **View → New Broadcast Window**.
Fourth window type. Source pane untouched. OBS window capture workflow — no built-in
RTMP or chat integration.

- Configurable aspect ratio lock (16:9 / 4:3)
- Toolbox auto-hides on hover — never in OBS capture frame
- Independent zoom/pan from source pane
- Safe area grid (toggleable, not in OBS capture)
- Title bar: `Supercell Wx — {SITE} — {PRODUCT} {TILT}° [Broadcast]`

**Toggleable overlays:**
- Now-showing banner: `KDFW — N0G — Base Velocity (Hi-res) — 0.5° — Live`
- Timestamp (local and/or UTC)
- Geographic scale bar
- Product legend lower-third (plain-English product explanation for viewers)
- Branding / watermark (PNG with transparency, configurable position/opacity)
- Spotlight tool (soft-edged dimming circle, draggable in real time)
- Storm stats ticker (active warnings, storm motion, nearest cell distance)

**Broadcast drawing tools:**
Same tool architecture as operational annotations. Differences:
- Default stroke weight: Thick
- Default color: White
- Signature label stamps: "Hook echo," "Rotation," "RFD," "Inflow," "Hail core,"
  "Debris signature," "Bow echo," "TBSS," "Gate-to-gate shear"

**Stroke smoothing (both modes):**
Mouse input. Ramer-Douglas-Peucker simplification → cubic Bézier interpolation →
`QPainter::drawPath`. Clean curves, no input lag.

**Acceptance criteria:**
- Broadcast window opens without affecting source pane
- All overlays toggle independently
- Now-showing banner updates when product/site/tilt changes
- Spotlight circle draggable in real time while overlay is active
- Branding PNG loads from configured path and renders with correct opacity
- OBS can capture the broadcast window as a window source cleanly
- Drawing tools produce smooth Bézier curves, not jagged raw mouse input

---

## Phase 6 — Documentation, extensibility, and advanced features {#phase-6}

### Event log and annotation system

- Timestamped notes tied to lat/lon and radar frame
- Tags: tornado, wall cloud, RFD, funnel, hook, rotation, etc.
- Map pins with hover preview
- Export as GeoJSON for NWS storm reports
- Import from previous sessions

**Acceptance criteria:**
- Notes can be dropped on the map at any time
- Tags filter correctly in the log view
- GeoJSON export opens correctly in QGIS and geojson.io
- Imported annotations from a previous session display correctly

### Settings import / export

- Export: saves all user preferences to an encrypted `.scwxprefs` file
- API keys and sensitive values AES-256 encrypted with a user-supplied passphrase
- Non-sensitive settings (layout, color, shortcuts) stored in plain JSON within
  the same file for human readability of non-sensitive fields
- Import: validates file integrity before applying, rolls back on failure
- **Auto-save:** settings saved automatically on every change — no manual export
  needed between sessions. The export function is for migrating to a new machine
  or sharing a layout with another user.
- Settings file location displayed in preferences so users know where it lives

**Implementation note:**
Use Qt's `QSettings` for the live session store (already used upstream).
The export format wraps `QSettings` output in a JSON envelope with a version
field and an encrypted blob for sensitive keys. Use Qt's `QCryptographicHash`
for integrity checking and OpenSSL (already a dependency via AWS SDK) for
AES-256 encryption. Never store API keys in plain text anywhere on disk.

**Acceptance criteria:**
- Settings persist between app restarts without any user action
- Export produces a valid `.scwxprefs` file
- API keys not readable as plain text in the exported file
- Import from a `.scwxprefs` file restores all settings correctly
- Import rolls back cleanly if the file is corrupt or passphrase is wrong
- Settings file path visible in preferences

### Plugin / extension API

- Stable interface for community plugins: data sources, overlays, UI panels
- Plugin manifest (JSON): name, version, author, entry point, permissions requested
- Plugin manager UI: install, enable/disable, uninstall
- Sandboxed — plugins cannot access GPS data, API keys, or file system outside
  a designated plugin data directory without explicit permission
- Plugin API versioned — breaking changes increment the major version and old
  plugins display a compatibility warning

**Acceptance criteria:**
- A minimal example plugin (e.g. a static text overlay) installs and runs correctly
- Disabling a plugin removes its UI and data contributions without restarting
- Uninstalling a plugin removes all its files from the plugin directory
- A plugin requesting an undeclared permission is rejected at load time

### Multi-site rotation / broadcast mode

Auto-cycle through a list of radar sites on a configurable interval.

- Per-site dwell time
- Per-site product override
- Pause on active warning in current view

**Acceptance criteria:**
- Sites cycle on configured interval
- Product override applies correctly per site
- Rotation pauses when an active warning is present
- Manual pause/resume button works

### MRMS mosaic support

Multi-Radar Multi-Sensor national composite as an additional data source.

**Acceptance criteria:**
- MRMS composite visible as a selectable product in a pane
- Updates on MRMS refresh schedule (~2 min)
- Visually distinct from single-site NEXRAD products

### Debug / diagnostics panel

A developer and power-user panel accessible from **Help → Diagnostics**.

- Active data connections and their status (connected, retrying, latency)
- Last scan time and data age per pane
- Active product fetch queue
- GPS connection status and raw NMEA sentence display
- Log level control (Info / Debug / Verbose)
- "Copy diagnostics to clipboard" button for bug reports

**Acceptance criteria:**
- Panel shows accurate connection status for all active data sources
- Log level changes take effect immediately without restart
- Copied diagnostics include app version, OS, and all connection states

### Changelog and update notifier

- On launch, silently check GitHub releases API for a newer version
- If newer version available, show a non-blocking banner: "Version X.X is available
  — see what's new"
- Clicking "see what's new" opens the in-app changelog panel
- Changelog panel renders the release notes from GitHub releases
- Check frequency: once per launch, with a 24-hour cooldown

**Acceptance criteria:**
- Update check does not delay app launch
- Banner appears only when a newer version is available
- Banner can be dismissed and does not reappear until next launch
- Changelog renders correctly with no internet → shows cached last-known changelog

### Automated signature detection (high-confidence only)

> **Note:** Requires extensive testing against known archive events. A false positive
> mid-chase is worse than no detection. Confidence thresholds must be validated before
> any release. Late Phase 6 or post-1.0.

- Passive analysis on active pane's current scan
- Only notifies when confidence exceeds validated threshold
- Initial targets: TBSS (geometric, rule-based), TDS (low CC + velocity couplet
  cross-check significantly raises confidence)
- Does not attempt hook or gate-to-gate shear detection in early versions
- Non-intrusive indicator on pane border — not a popup or sound
- Click indicator → opens relevant signature guide entry

**Acceptance criteria:**
- Detection runs without affecting radar render performance
- No false positives on 20 tested clear-air or non-tornadic scans
- TDS detection correctly identifies known events in archive testing
- TBSS detection correctly identifies known events in archive testing
- Notification is dismissable and does not reappear for the same scan

### Archive event library

- Curated list of significant events linking into the archive radar viewer
- Each signature guide entry links to one or more archive cases
- Events include: 2011 Joplin, 2013 El Reno, 2019 Memorial Day outbreak, others

---

## Detailed spec: duplicate and mirror windows {#spec-windows}

### Duplicate window

| Property | Behavior |
|---|---|
| Trigger | Right-click grid pane → "Duplicate in new window" |
| Source pane | Stays in grid, untouched |
| Initial state | Full copy of source at moment of duplication |
| Post-open state | Fully independent, no shared state |
| Controls | Full toolbox — L2/L3 dropdowns, map settings, tilt, timeline |
| Toolbox | Docked by default, draggable to floating panel |
| Title bar | `Supercell Wx — {SITE} — {PRODUCT} {TILT}°`, updates live |
| Multiple instances | Unlimited |
| On close | Window closes, nothing else affected |
| Session restore | Opt-in via preferences, off by default |

### Mirror window

| Property | Behavior |
|---|---|
| Trigger | Right-click grid pane → "Mirror in new window" |
| Source pane | Stays in grid, untouched |
| Sync: product / tilt | On by default, toggleable |
| Sync: loop playback | On by default, toggleable |
| Sync: zoom | Off by default, toggleable |
| Sync: pan | Off by default, toggleable |
| Controls | Active only for unsynced axes |
| Mirror settings access | Gear icon or toolbox panel |
| Source disconnected | "Source disconnected" state, does not close |
| Title bar | `Supercell Wx — {SITE} — {PRODUCT} {TILT}° [Mirror]` |
| Multiple instances | Unlimited |

---

## Detailed spec: product name labels {#spec-labels}

`static const QMap<QString, QString> kProductLabels` in
`scwx/qt/util/product_labels.h`. Accessed by pane header, title bar formatter,
dropdown delegate, and tooltip builder. Never duplicated.

```
"N0B" → "Base Refl. (Hi-res)"       "N0G" → "Base Velocity (Hi-res)"
"N0C" → "Correlation Coeff. (CC)"   "N0X" → "Differential Refl. (ZDR)"
"N0K" → "Spec. Diff. Phase (KDP)"   "N0H" → "Hydrometeor Class"
"NTV" → "Tornado Vortex Sig."       "NMD" → "Mesocyclone Detection"
"NCR" → "Composite Refl."           "EET" → "Enhanced Echo Tops"
"N0S" → "Storm-Relative Vel."       "NHI" → "Hail Index"
"NST" → "Storm Tracking"            "NET" → "Echo Tops"
"NVL" → "Vert. Integrated Liquid"   "N0U" → "Base Velocity"
"N0Q" → "Base Reflectivity"         "NSW" → "Spectrum Width"
```

---

## Detailed spec: interactive color scale {#spec-colorscale}

### Component structure

`ColorScaleWidget` subclasses `QWidget`:
- `mousePressEvent` — left click toggle, right click `QMenu`
- `paintEvent` — compact (~16px) or expanded (~52px) render
- `ProductType` enum drives unit system and category labels
- Expanded state stored per product code in `QSettings`

### Product type detection

```
Reflectivity (N0B, N0Q, NCR, EET, NVL, DHR...)  →  dBZ scale
Velocity (N0G, N0U, N0S, NSW...)                 →  kts, bipolar
ZDR (N0X...)                                     →  dB scale
CC (N0C...)                                      →  0.00–1.05
KDP (N0K...)                                     →  °/km
Hydrometeor (N0H...)                             →  discrete labels
```

---

## Detailed spec: vertical cross-section / RHI slice {#spec-crosssection}

### Overview

The cross-section tool renders a vertical slice through the atmosphere along a
user-drawn line, compositing all available tilt angles into a single height-vs-range
display. This reveals vertical storm structure invisible from any single tilt.

### User interaction

1. Activate from toolbar or `Tools → Cross-Section`
2. Click two points on the map to define the slice line
3. Cross-section panel opens (dockable, default bottom of window)
4. Drag either endpoint to update the slice in real time
5. Deactivate returns to normal radar interaction

### Rendering

The cross-section panel renders a 2D grid:
- X axis: distance along the slice line (nm)
- Y axis: altitude (kft AGL), range 0–70 kft
- Color: reflectivity value at each (range, height) cell using the active color table

**Data compositing algorithm:**
For each tilt angle θ available:
```
beam_height(range_km) = range_km * sin(θ_rad) + (range_km² / (2 * Re_eff))
```
where `Re_eff = 8500 km` (4/3 Earth radius effective for standard refraction).

For each pixel cell at (range r, height h):
- Find which tilt angle's beam passes closest to (r, h)
- Sample the reflectivity value at that range gate
- If the cell is below the lowest beam or above the highest, mark as no data

**Overlays on the cross-section panel:**
- Freezing level line (0°C isotherm) — dashed white line
- Tropopause estimate — dotted line at climatological height for the season/latitude
- BWER annotation if detected (low-reflectivity notch extending upward)
- Hail core annotation if high-dBZ region extends above freezing level

### Data source

Level 2 data only — Level 3 products do not provide sufficient tilt coverage for
a meaningful cross-section. The cross-section tool is disabled when only Level 3
data is available, with a tooltip explaining why.

### Implementation note

The compositing grid is a `QImage` rendered in a background thread on each scan
update or slice line change. Post to the UI thread via `QMetaObject::invokeMethod`
when rendering is complete. Never block the radar rendering thread.

Grid resolution: 0.5 nm × 500 ft cells is sufficient for operational use without
being computationally expensive. Total grid for a 150nm slice = 300 × 140 = 42,000
cells — trivial to compute.

### Acceptance criteria

- Cross-section panel opens when tool is activated
- Slice line visible on the radar map
- Dragging either endpoint updates the cross-section in real time (< 500ms)
- Freezing level line visible and approximately correct
- Cross-section tool is disabled with explanation when only L3 data is available
- Rendering does not block or slow the main radar update loop
- Panel is dockable and its size is resizable

---

## Detailed spec: waypoint planner and escape route logic {#spec-waypoints}

### Waypoint types

| Type | Color | Description |
|---|---|---|
| Intercept | Red | Target storm intercept location |
| Bail-out | Orange | Escape destination if intercept goes wrong |
| Fuel | Yellow | Fuel stop |
| Staging | Blue | Pre-chase staging area |
| Custom | White | User-labeled, user-colored |

Waypoints dropped via right-click on map → "Add waypoint here" → type picker.
Drag to reposition. Click to edit label. Delete key removes selected waypoint.
Waypoints persist between sessions. Export/import as JSON.

### Escape routes

Escape routes are directional road segments drawn on the map.

**Adding an escape route:**
1. Activate escape route tool from toolbar
2. Click to place nodes along the road
3. Arrow indicators show travel direction
4. Right-click any segment → direction options: one-way north/south/east/west,
   or bidirectional
5. Right-click segment → "Mark as escape route" to tag an existing road

**Visual distinction:**
- Marked escape route: solid green line with directional arrows
- Suggested fallback route (system-generated): dashed yellow line with directional
  arrows and "SUGGESTED" label

### Storm motion cutoff detection algorithm

The escape route system continuously evaluates whether a storm's projected motion
will intersect any marked escape route within a configurable lookahead time window
(default 30 minutes, configurable 10–60 min).

**Data source:** SCIT product (NST Level 3). Fields used:
`CurrentPosition` (lat/lon), `Speed` (kts), `Direction` (°meteorological).

**Meteorological to mathematical direction conversion:**
```
math_angle_rad = (270 - met_direction_deg) * π / 180
velocity_x = speed_kts * cos(math_angle_rad)   // eastward component
velocity_y = speed_kts * sin(math_angle_rad)   // northward component
```

**Projected storm polygon at time T:**
The storm is approximated as a circle of radius R (default 15 nm, user configurable).
At time T minutes ahead:
```
proj_lat = cell_lat + (velocity_y * T / 60) / 60.0
proj_lon = cell_lon + (velocity_x * T / 60) / (60.0 * cos(cell_lat * π/180))
```
Projected storm footprint = circle of radius R centered at (proj_lat, proj_lon).

**Intersection test:**
For each escape route segment (defined as a series of lat/lon nodes):
For each segment AB between consecutive nodes:
1. Convert segment endpoints to local Cartesian (nm) relative to projected storm center
2. Compute minimum distance from circle center to line segment AB
3. If distance < R → intersection detected

**Cutoff condition:**
An escape route is considered "cut off" if the projected storm footprint intersects
it at any point within the lookahead window AND the intersection point is between
your current GPS position and the route's destination (i.e. the storm is cutting
off your path, not just near a section you've already passed).

**Rerouting behavior:**

Step 1 — Check other marked escape routes:
Evaluate all other marked escape routes. If any remain clear for the full lookahead
window, highlight the clearest one and show a non-intrusive banner:
"Primary escape route cut off in ~{T} min. Alternative route highlighted."

Step 2 — If no marked routes are clear, suggest a road from the map:
Using the OpenStreetMap road network (already available via the map tile layer),
run a simple bearing-based road selection:
- From current GPS position, identify all roads within 5 nm heading away from the storm
- Score roads by: (1) heading away from storm, (2) road class (highway > arterial >
  local), (3) distance from projected storm path
- Highlight the highest-scoring road as a suggested fallback in dashed yellow
- Banner: "All escape routes cut off. Suggested fallback highlighted — verify conditions."

Step 3 — If no viable road found:
Banner: "No clear escape route detected. Seek shelter immediately."

**SCIT unavailable fallback:**

If SCIT data is not available (radar not in precip mode, product not decoded):
- Option A: User can run a second pane in the background on the radar site's N0B
  product. The escape route layer checks if SCIT is available from any active pane,
  not just the primary.
- Option B: User can manually enter a storm motion vector (speed in kts, direction
  in degrees) via the escape route settings panel. This manual vector is used in
  place of SCIT data. A warning indicator shows that manual mode is active.
- The escape route layer is a toggleable layer in the layer settings panel, so it
  can be shown in a duplicate/mirror window independently of the main pane.

### Implementation note

The cutoff detection runs in a background `QThread` on each new SCIT scan or GPS
position update — whichever occurs first. Post results to UI thread via signal.
Do not run geometry intersection on the UI thread.

The OpenStreetMap road network for fallback routing is queried via the Overpass API
(already used in your other projects) with a bounding box around the current GPS
position. Cache the response for 10 minutes to avoid repeated queries.

### Acceptance criteria

- Waypoints drop correctly at right-click location
- Escape routes render with correct directional arrows
- Cutoff detection correctly identifies when a projected storm path intersects a route
- Rerouting first checks other marked routes before suggesting map roads
- Suggested fallback road visually distinct from marked escape routes
- SCIT unavailable: manual motion vector input works correctly
- Manual mode indicator visible when manual vector is active
- "No clear route" banner appears when all options are exhausted
- All geometry runs in background thread without affecting radar frame rate
- Cutoff lookahead time configurable in settings

---

## Detailed spec: chase log {#spec-chaselog}

### Overview

The chase log automatically records everything that happens during a session without
requiring any user action. It is always running when the app is open. The user can
review, annotate, and export the log after a chase.

### Recorded events (automatic)

| Event type | Data captured |
|---|---|
| GPS position | Lat, lon, speed, heading — sampled every 5 seconds |
| Radar site change | Timestamp, from site, to site |
| Product change | Timestamp, pane index, from product, to product |
| Tilt change | Timestamp, pane index, from tilt, to tilt |
| Warning issued | Timestamp, event type, affected area, polygon WKT |
| Warning expired | Timestamp, event type |
| Manual annotation | Timestamp, lat/lon, tag, note text |
| App start / stop | Timestamp |

### Session file format

Sessions stored as newline-delimited JSON (NDJSON) in the app data directory:
`sessions/{YYYY-MM-DD}_{HHMM}.scwxlog`

Each line is a JSON object:
```json
{"t": 1234567890, "type": "gps", "lat": 35.123, "lon": -97.456, "spd": 65.2, "hdg": 195}
{"t": 1234567891, "type": "product_change", "pane": 0, "from": "N0B", "to": "N0G"}
{"t": 1234567892, "type": "warning", "event": "TOR", "area": "Dallas County TX"}
```

### Session replay

After a chase, sessions can be replayed in the app:
- **File → Open Session** → pick a `.scwxlog` file
- Replay panel opens at the bottom of the window
- Timeline scrubber shows the full session duration
- Play/pause/speed control (1x, 2x, 4x, 8x)
- During replay: GPS track animates on the map, product changes happen automatically,
  warnings appear and expire on schedule
- Radar data pulled from the archive for the session's time range (requires archive
  data availability)
- Manual annotations visible as pins at their recorded time

### Export formats

| Format | Contents | Use case |
|---|---|---|
| GPX | GPS track only | Import into mapping apps, Garmin devices |
| GeoJSON | GPS track + waypoints + annotations | QGIS, storm report mapping |
| CSV | All events in tabular form | Spreadsheet analysis |
| KML | GPS track + waypoints | Google Earth |

### Implementation note

Log writer runs in a dedicated background thread. Events are posted to the thread
via a lock-free `QQueue` from any producing thread (GPS, product manager, alert
manager). Writer flushes to disk every 10 events or every 30 seconds, whichever
comes first. File handle kept open for the session duration — do not open/close
on every write.

Session files are capped at 50MB. If a session exceeds this (very long day),
a new file is started automatically with `_part2` suffix.

### Acceptance criteria

- Log file created automatically on app start
- All event types recorded with correct timestamps
- GPS track accurate to within 5 seconds of actual position
- Session replay correctly reconstructs product changes in order
- GPX export imports correctly into Google Maps and Garmin BaseCamp
- GeoJSON export valid per spec (verifiable at geojson.io)
- Log writer does not cause frame drops or UI hitches
- 50MB cap handled gracefully with correct part file naming

---

## Detailed spec: offline tile caching {#spec-offline}

### Overview

Pre-downloads map tiles and recent radar frames for a geographic bounding box
so the app remains functional in low/no signal areas during a chase.

### Bounding box definition

- User draws a rectangle on the map by dragging
- Or enters a center point + radius (e.g. "50 nm around Amarillo, TX")
- Or selects from saved named regions (configurable in preferences)
- Zoom levels to cache: configurable range (default zoom 4–14, covering national
  overview through street level)

### Storage estimate

Before downloading, the app shows:
"Estimated download: ~{N} MB for map tiles + ~{M} MB for {X} hours of radar frames"
User confirms before download begins.

Tile count estimate: `tiles = Σ(4^z)` for each zoom level z in range,
clipped to the bounding box. At default zoom 4–14 for a 200×200 nm box ≈ ~800MB.
This is shown clearly so the user can narrow the box or reduce zoom range.

### Radar frame caching

- Caches the most recent N frames (configurable, default 12) for the selected
  radar site(s) within the bounding box
- Also caches the SCIT, NTV, and NHI products for the same time window
- Cache invalidation: cached frames older than 24 hours are purged on next launch

### Disconnected mode behavior

When internet connection is lost:
- App detects loss via a periodic connectivity check (every 60 seconds)
- Banner appears: "Offline — using cached data. Last updated {time}."
- Map tiles served from cache — no blank tiles
- Radar updates stop (no new data available) — the loop continues with cached frames
- Alert manager freezes at last known state with a staleness indicator
- GPS continues to function (device-local)

### Cache management UI

In Preferences → Offline Cache:
- List of saved cache regions with size, last updated, delete button
- Total cache size with a global clear button
- Download new region button
- Auto-cache toggle: "Automatically cache my current map view on launch"

### Implementation note

Map tile caching: intercept tile requests in the MapLibre tile provider layer.
On cache hit, serve from disk. On cache miss in offline mode, return a placeholder
tile. Tiles stored as files: `cache/tiles/{z}/{x}/{y}.png`.

Radar frame caching: serialize the Level 2/3 product data to disk after decode.
Store in `cache/radar/{site}/{product}/{timestamp}.bin`. On playback from cache,
deserialize and pass to the existing render pipeline — no special case needed
in the renderer.

### Acceptance criteria

- Bounding box can be drawn on map or entered as center + radius
- Storage estimate shown before download begins
- Download progress shown with cancel option
- In offline mode, map tiles load from cache without blank tiles
- Radar loop plays cached frames correctly
- Offline banner appears within 60 seconds of connectivity loss
- Cache region management UI correctly shows sizes and allows deletion
- Auto-cache on launch option works correctly

---

## Detailed spec: SpotterNetwork native integration {#spec-spotter}

### Overview

SpotterNetwork spotter and chaser dots displayed natively without manual placefile
URL entry. Upstream already supports SpotterNetwork via placefiles — this wraps
it in a dedicated UI with filtering and click interaction.

### Data source

SpotterNetwork provides a publicly accessible data feed. The upstream placefile
at `https://www.spotternetwork.org/feeds/gr.txt` is the existing integration point.
Native integration parses this feed directly into typed structs rather than treating
it as a generic placefile.

Feed refresh interval: 60 seconds (SpotterNetwork standard).

### Feature types

| Type | Description | Default visible |
|---|---|---|
| Chaser | Mobile spotters with GPS | Yes |
| Spotter | Stationary spotters | Yes |
| Storm report | Tornado/hail/wind reports | Yes |
| Emergency manager | EM personnel | Yes |
| Trained spotter | SKYWARN trained | Yes |

Each toggleable independently in the SpotterNetwork settings panel.

### Dot interaction

Click any spotter/chaser dot → popup with:
- Callsign
- Report type / status
- Last report text and timestamp
- Distance and bearing from your GPS position
- Link to SpotterNetwork profile (opens in browser)

### Proximity filter

- Filter slider: "Show only spotters within X miles of my position"
- Default: off (show all)
- When enabled, dots outside the radius are hidden (not just dimmed)

### Implementation note

Parse the GR placefile format from the SpotterNetwork feed. The `ICON` lines
contain lat/lon and icon type. The `TEXT` lines contain callsign and report text.
Map icon types to the feature type enum above. Store as `QList<SpotterEntry>`.
Re-fetch every 60 seconds via `QNetworkAccessManager`. Parse on a background thread,
post results to UI thread.

The SpotterNetwork terms of service permit display of spotter data in weather
applications. Verify current ToS before release.

### Acceptance criteria

- Spotter dots appear on the map and update every 60 seconds
- Each feature type toggles independently
- Clicking a dot shows correct callsign, report, and distance/bearing
- Proximity filter correctly hides dots outside the configured radius
- GPS bearing to each dot correct within 5°
- Feed parse errors logged but do not crash the app
- ToS compliance verified before release

---

## Detailed spec: broadcast window and annotation system {#spec-broadcast}

### Window type summary

| Type | Trigger | Source | Independent | Toolbox |
|---|---|---|---|---|
| Duplicate | Right-click → Duplicate | Any pane | Fully | Docked, draggable |
| Mirror | Right-click → Mirror | Any pane | Partial (synced) | Docked, draggable |
| Broadcast | View → New Broadcast Window | Any pane | Fully + canvas | Auto-hide |
| Operational | View → Annotations / menu bar | All panes | Shared canvas | Floating mini-toolbar |

### AnnotationCanvas layer

Transparent `QWidget` overlay above the radar map layer, below all UI chrome.

**Stroke pipeline:**
1. `QMouseEvent` points collected on `mouseMoveEvent`
2. Ramer-Douglas-Peucker simplification removes noise
3. Cubic Bézier interpolation produces smooth curves
4. `QPainter::drawPath` renders to canvas
5. Final stroke committed on `mouseReleaseEvent`

**Coordinate system:** Map coordinates (lat/lon) — stays anchored on pan/zoom,
scales correctly at any window size or OBS capture resolution.

### Stroke width options

| Option | Width | Best for |
|---|---|---|
| Thin | ~1.5px | Operational notes |
| Medium | ~2.5px | General use |
| Thick | ~4px | Broadcast default |
| Extra Thick | ~6px | Large displays |

### "Annotation Mode" button behavior

| Canvas state | Click result |
|---|---|
| Empty | Disables immediately |
| Has annotations | "Clear and exit annotation mode?" → Clear and Exit / Cancel |

### Persistence dropdown

| Option | Default for |
|---|---|
| Clear on product change | — |
| Clear on site change | — |
| Clear on both | Operational mode |
| Never | Broadcast mode |

---

*End of document*

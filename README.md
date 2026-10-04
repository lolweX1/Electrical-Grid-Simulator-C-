# Electrical Grid Simulator

Build with Qt 6 Widgets and a C++17 compiler using `make`; run the result with
`make run` from the repository root so the icons in `icons/` can be loaded.

## Using the simulator

- The palette starts on **None**, which lets you inspect components without
  placing more. Select a component tool to preview a snapped, semi-transparent
  placement ghost; its outline turns red where it would overlap another object.
- Apartments are separate from houses and start with a 600 V rating and 20 A
  current requirement; both values can be adjusted in their properties.
- Press **R** to rotate the placement preview before placing it, or click an
  existing component and press **R** to rotate that component. Press **Delete**
  to remove the selected component and its attached wires. Re-click the active
  palette tool to resume placing after selecting an existing component.
- Choose a house, apartment, utility pole, wire separator, or power generator
  in the left palette and click an empty grid cell to place it. Components
  occupy footprints derived from their icon dimensions: 64 source pixels equal
  one grid cell, rounded up, with a one-cell minimum.
- Choose **Connect wires**, then connect the colored terminal ports. Wire both
  sides of the source/load to make a complete circuit; poles are junctions.
  Green and blue ports are positive/return (or switch input/output); orange
  ports are pole junctions. Power-pole terminals are both at the center and
  electrically joined. Bridging a generator's two ports creates a short
  circuit. A dashed ghost wire follows the cursor from the selected start
  terminal; right-click cancels a pending connection. Each terminal accepts at
  most one wire. Power generators provide four positive output ports and four
  return input ports that share their internal source buses.
- Select a house to set its required current and rated voltage. Select a
  generator to set its output voltage and current limit. Generators start at
  480 V and 40 A; houses start at 480 V and 10 A.
- Select a wire to see its voltage drop, current, resistance, temperature and
  temperature change, sag profile, length, mass, and thermal expansion. Choose
  copper, aluminum, or steel and adjust its resistivity in the Properties panel.
  Every object has an editable wire connection height above ground (default:
  house 10 m, apartment 25 m, utility pole 10 m, separator 8 m, generator 8 m).
  The sag graph plots actual wire height against the highest attachment height;
  wires whose lowest point is under 5 m above ground are red.
  Press **Delete** to remove a selected wire. A house or apartment receiving
  less than 99% of its required current is
  outlined in yellow, allowing a small tolerance for ordinary feeder voltage
  drop.
  Wires turn red when shorted, overloaded, too hot, or sagging beyond the safe
  threshold.
- Wire separators conduct by default. Each has one input on its bottom side
  and three outputs; connect any outputs needed and leave unused ones open.
  Select a separator and toggle **Conducting** in the properties panel to
  disconnect/reconnect all three outputs. Rotate it with **R** to reorient its
  ports.
- Use **Run simulation** and **Stop simulation** in the toolbar to control
  generation and thermal updates. While running, generators supply power,
  sunlight heats wires, and wind cools them; stopping freezes wire temperatures
  while retaining a live electrical demand estimate. The stopped estimate uses
  the configured generator ratings and current limit, actual circuit topology,
  and wire material, resistivity, and length, with standard conditions of 20 C
  wire/ambient temperature and 0 mph wind. Dynamic wire heating, sunlight, and
  convection are paused while stopped. While running, Joule (I^2R) and solar
  heating are integrated alongside natural and wind-driven thermal convection;
  defaults are 800 W/m^2 solar irradiance and 0 mph wind. The button stays
  checked and reads **Stop simulation** while active; a solver error stops the
  run and reports the error. Simulation time follows elapsed real time, scaled
  by the toolbar speed multiplier, which starts at 10x.
- Choose **Reset simulation** to stop the simulation and restore wire
  temperatures to the 20 C base condition. The grid layout and component/wire
  settings remain in place.
- Choose **Display all information** to open a read-only table of objects and
  wires, including their eight-character IDs and electrical/thermal values.
  Click an object row to highlight and center it on the grid; Refresh updates
  the displayed values.
- Use **File > Save** and **File > Load** for JSON snapshots of the layout,
  object and wire settings, unique IDs, and speed multiplier. Saves use the
  base grid: wire lengths are the unstressed layout lengths and temperatures
  are saved as 20 C, regardless of the live simulation state. The included
  [stress test](<./save/stress test.json>) is a New York City-inspired layout
  with 15 dense districts across all five boroughs. It contains 60 nearby
  generators, 60 closed distribution separators, 120 houses, and 60 apartments,
  connected by 420 wires. The model uses 208 V residential
  service, 250 V local sources, copper feeders, and diversified loads of about
  3.2 kW per house and 5.1 kW per apartment.
- Right-click and release to switch back to **None**. Right-click-drag or
  middle-click-drag to pan the grid; use **WASD** or the arrow keys for keyboard
  panning. Zoom with the mouse wheel.
- The grid's top-right readout shows the cell under the cursor as `(row, col)`;
  row and column numbers follow the same grid coordinates used for placement.
  Zoom out further with the mouse wheel to view the full city; grid lines hide
  automatically below 12% zoom to keep dense layouts readable.

## Electrical and thermal model

The network is solved as a resistive DC nodal circuit with explicit component
terminals. With simulation stopped, demand status is an estimate at a standard
wire temperature of 20 C and wind speed of 0 mph using the configured source
and load ratings. While running, the electrical solve instead uses each wire's
current temperature.
Each generator is a Thevenin source with nonzero internal resistance
and a current limit; houses are resistive loads sized from their rated voltage
and required current. Poles join all connected wires, while separators are
four-terminal switches with one input and three switched outputs. This supports
closed series and parallel circuits without assuming an implicit return wire.

Wires use aluminum resistivity by default, their endpoint distance (10 m per
grid cell), and a 25 mm^2 cross-section. Copper, aluminum, and steel use
different resistivity, density, specific heat, thermal expansion, and
temperature coefficients. Resistance changes with temperature and length.
The displayed wire length is the thermally expanded length, alongside its
unstressed base length. Joule (I^2R) and solar heating are balanced against
natural and wind-driven convection while the simulation runs; with 0 mph wind,
natural convection still cools hot wire toward the 20 C ambient. Solar and
electrical heating can keep the wire above ambient, but the model approaches a
heat-balance temperature rather than holding it at 20 C. Expansion increases
displayed sag.
Directly bridging a
generator's positive and return terminals produces a red, current-limited fault.
Wire elevation follows a parabolic sag profile between the connection heights of
its endpoint objects. Clearance is measured at the lowest point of that profile.
Series loads share current and divide source voltage according to their
resistances; parallel branches share bus voltage and their currents add at the
source. Each generator has four internally bussed output terminals and four
internally bussed return terminals.

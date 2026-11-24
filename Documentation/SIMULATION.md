# Simulation model, version 0.1

All physiological and decompression values are unvalidated gameplay outputs. The model must not be used as a real dive computer, training aid or medical prediction.

## Units and integration

The independent header `DiveModel.h` uses metres, seconds, newtons, kilograms, litres and bar. Unreal converts distances by 100 cm/m. The game applies physics in 20 ms steps. Pressure uses freshwater density 1000 kg/m³ and gravitational acceleration 9.80665 m/s². The scenario surface pressure is configurable, initially 0.965 bar; it is not a live weather measurement.

`P = P_surface + rho * g * depth / 100000`.

The body has a nominal 100 kg inertial mass. Thrust, quadratic drag relative to the current, suit compression, remaining gas mass, ballast and BCD gas contribute to movement. Suit, body and lung displacements are deliberately approximate. There is no full hydrodynamic body/fin model yet.

## Gas and buoyancy

Surface respiratory minute volume starts at 16 L/min and rises deterministically with effort, stress and fatigue. Ambient pressure scales tank consumption. BCD inflation draws gas from the same tank. BCD gas is stored as equivalent litres at one bar; its current volume is this quantity divided by ambient pressure. An 18 L relief limit vents excess gas. Empty cylinders cannot supply additional BCD gas.

Breath phase drives lung displacement, exhalation sound and bubble emission. Ascending bubbles use the cube root of the pressure ratio to change radius. The player must distinguish swimming upward (Space) from inflating the BCD (Q).

## Physiology

Heart rate and breathing rate approach targets derived from exertion, stress and fatigue. Stress responds to low gas, plan exceedance and rapid ascent. Cold exposure depends on depth temperature and suit choice. Simulated SpO2 declines after gas exhaustion underwater, with a terminal gameplay state. These are qualitative couplings, not clinical relationships.

The thermocline is a smooth transition centred on 10 m, between configurable 18 °C and 6 °C defaults. These values are scenario parameters, not Walensee measurements.

## Nitrogen approximation

Sixteen nitrogen compartments use exponential uptake/release under constant pressure within each integration step. Public Bühlmann nitrogen half-times and coefficients are used with a fixed gradient factor of 0.8. The computed ceiling is the maximum compartment ambient pressure requirement converted to freshwater depth. Displayed NDL is time at the current depth until the surface limit is reached; it does not simulate an ascent, gas switching, repetitive dives, GF-low/high interpolation or a complete stop schedule.

Reference for the published coefficient family and conventions: [Subsurface's decompression model](https://github.com/subsurface/subsurface/blob/master/core/deco.cpp). No Subsurface implementation code is included. Coefficients are numerical model data; integration and game logic here are independently implemented. The first compartment uses the 4-minute variant (a=1.2599, b=0.5050).

A separate gameplay safety-stop timer counts 180 seconds at 3–6 m after exceeding 10 m. It is not a substitute for the compartment ceiling. Model validation against independent reference profiles is still required before claiming faithful decompression simulation.

## Modes and persistence

Exploration uses unlimited tank gas and automatic neutral trim, with breathing-induced movement retained. Simulation uses finite gas and manual BCD. Hardcore currently exposes the same simulation rules; equipment faults and reduced assistance are pending and explicitly labelled in the menu.

Samples are recorded once per second. Versioned JSON includes position, depth, gas, vital signs, warnings and temperatures. Every 30 samples, a temporary file is written and renamed to the session log. Completion, interruption and gas-exhaustion states are distinct. Repeated dives currently reset tissues; persistent repetitive-dive physiology is not implemented.

## Tests

`Tools/TestCore.cmd` builds a standalone C++ test executable without Unreal. It checks hydrostatic pressure, depth scaling of gas use, exertion coupling, BCD consumption, expansion/relief, step partition invariance, exploration, tank exhaustion, tissue uptake/release, ceilings, stop timing and input sanitization. A passing result establishes internal consistency, not medical validity.

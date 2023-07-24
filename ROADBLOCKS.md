Roadblocks are a type of HeavySupportStrategy used by GroundSupportRequest, the latter of which creates a VehicleVector (list of vehicles) that gets passed to the AICopManager, creating the roadblock using the aforementioned vehicles. Roadblock setups are defined by their minimum width, required vehicle count, and 6 elements - each with their own X and Z offset, rotation angle and their type (`none`, `car`, `barrier` or `spikestrip`).

# Roadblock creation
First, the width of the street gets calculated **250.0** units away from the player using WRoadNav calculations (the navigation, or in layman terms "just driving forward like freeroam AI racers do"). You may change this distance using BulbToys's "Distance" slider in the "Roadblock setups" menu. You may also visualize the street width using the "Overlay" option under the same menu, either calculating from the player's position or the camera's position depending on whether "Try to use camera position/direction" is ticked. Please note that having this option ticked does NOT impact the location of the roadblock and is purely visual.

The width calculation does NOT start from the very beginning of the street, instead it's usually offset by a certain small (mostly constant) amount (on normal streets for example, usually from one end of the sidewalk to another, ie. only the driveway). The navigation also checks for two road segment flags, IsDecision (like intersections) and TrafficNotAllowed (don't think it's used anywhere?), and if either of the two flags are true, it will invalidate the calculation. If for any other reason the calculation is invalid (if you're in the void or something lol), BulbToys displays "N/A" as the width, the street width does not render, and the roadblock does not get created.

If the width calculation is valid, it gets incremented by 1 and `PickRoadBlockSetup` gets called. It returns a roadblock setup and takes three parameters:
- `float`, the width of the street
- `int`, the number of vehicles in the VehicleVector
- `bool`, whether to use spikes or not (depends on the `roadblockspikechance` VLT field)

Unless the "Use custom setups" option is ticked, which overrides BOTH of the roadblock setup arrays with "My RBs", the function picks either the "Normal RBs" should the bool be set to false, or "Spiked RBs" if it's set to true. For each setup in the array which meets the vehicle count criteria (ie. if the VehicleVector has an equal or greater amount of vehicles than the required amount), it finds the one which fits the best (ie. leaves the smallest gap behind, or `min_width = street_width - roadblock_width <=> min_width > 0`) and returns it. If not a single roadblock fulfills the vehicle count and/or width criteria, it returns null and the roadblock does not get created.

It will iterate through the array until it finds a setup with the required roadblock width under than 0.1 (the "last" setup). The last setup (ie. the first element index excluded from the loop) is shown in BulbToys next to the arrow iteration buttons as "Last: N", where N is the index (the last setup in "My RBs" (index 99) already fulfills this criteria and isn't accessible to prevent misuse crashes).

Finally, once the setup has been chosen, the game iterates through its elements until it finds an element with the `none` type, which **ends the iteration** (it doesn't just skip the element). The X, Z and angle offsets get mirrored to the other side, depending on whether you're closer to the left or to the right side of the street - roadblocks will spawn on the left side if you're closer to it and vice versa for the right side.

# Notes
- You can save and load roadblock setups, which stores the entire struct into a 104-byte `.rbs` (**R**oad**b**lock **S**etup) file.
- "Normal RBs" and "Spiked RBs" are the hardcoded roadblock setups in vanilla, and are designed to be read only (you can still save them though). If you wish to use your own roadblocks and edit them, use the "My RBs" tab instead.
- The roadblock you currently have selected will show in the overlay, but that doesn't guarantee that it will spawn - it is purely visual to aid in the modification/creation of roadblock setups.
- The middle lines are drawn from the center to the front of the object, indicating its forward vector. (ie. where it's pointing at)
  - `none` elements aren't drawn, but they are colored white.
  - `car` elements use "copmidsize" dimensions, as it is the largest vehicle by width/length, and are colored blue. The forward vector is its headlights.
  - `barrier` elements are the sawhorses, colored red. The forward vector is where the STOP sign is pointing.
  - `spikestrip` elements are colored yellow.
- You are allowed to set the number of required vehicles lower or higher than the amount of `car` elements, but it is not recommended. Needs further testing.
- Roadblocks may fail to create (or cause other issues?) if the setup's minimum width is lower than the roadblock's actual width. Needs further testing.
- Currently, certain values are clamped to values which i deem "make sense". You may override these by typing them out (ctrl-clicking on the sliders), but loading these invalid setups will be rejected should you choose to save them.
- It is helpful to know the street widths before creating your own setups, usually by driving around with the overlay enabled and distance lowered. For example, most shortcuts are from around 1.97 units to around 5 units, four-lane streets are usually around 16 units, etc... This unfortunately means a good portion of roadblocks will go unused, which will require a rewrite of `PickRoadBlockSetup` to for example use random chance and a roadblock minimum width tolerance range. (WIP?)
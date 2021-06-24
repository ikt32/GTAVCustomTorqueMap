# Custom Gear Ratios

Script to change car gear ratios in GTA V.

## Downloads

[GTA5-Mods.com](https://www.gta5-mods.com/scripts/custom-gear-ratios)

## XML files
In the folder `CustomGearRatios/Configs`, you can put XML files with gearbox descriptions. One XML file is one configuration. If multiple are defined, only the first one is used.

Layout:
```xml
<?xml version="1.0"?>
<Vehicle>
	<Description>5-gear AE86</Description>
	<ModelName>Futo</ModelName>
	<PlateText>undefined</PlateText>
	<TopGear>5</TopGear>
	<DriveMaxVel>48.07</DriveMaxVel>
	<Gear0>-3.484</Gear0>
	<Gear1>3.587</Gear1>
	<Gear2>2.022</Gear2>
	<Gear3>1.384</Gear3>
	<Gear4>1.000</Gear4>
	<Gear5>0.861</Gear5>
</Vehicle>
```

The file name is not used, so you can put whatever you want there.

The description is what's used to display the configuration in-game in the menu.

The `ModelName` is used to match the vehicle model for auto-loading.

The following applies for `PlateText`:

* `autoload_model`: Loads the config for all vehicles with this model.
* `undefined`: Doesn't automatically load the config.
* Any other string: Matches the plate text to only load for specific model + plate combinations. Will override definitions for the model with `autoload_model`.

Unit for `DriveMaxVel` is in m/s (like the game internally), so take care of that fact when editing manually. This is basically the final drive thing.

When not enough `GearX` entries are provided for the `TopGear`, the file is not loaded.

## Notes

Gear ratios are changed by the gearbox tuning and other scripts that call `MODIFY_VEHICLE_TOP_SPEED`. The script tries to revert back to the gearbox settings before this, but it's recommended to disable all functionalities in scripts that modify the top speed using the mentioned native.

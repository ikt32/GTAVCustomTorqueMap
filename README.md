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

The ModelName is used to match the vehicle model for auto-loading. The same with PlateText.

Unit for `DriveMaxVel` is in m/s (like the game internally), so take care of that fact when editing manually. This is basically the final drive thing.

When not enough `GearX` entries are provided for the `TopGear`, the file is not loaded.

## Building

### Requirements
* [ScriptHookV SDK by Alexander Blade](http://www.dev-c.com/gtav/scripthookv/)
* [Manual Transmission](https://github.com/E66666666/GTAVManualTransmission)
* [GTAVMenuBase](https://github.com/E66666666/GTAVMenuBase)

Download the [ScriptHookV SDK](http://www.dev-c.com/gtav/scripthookv/) and extract its contents to ScriptHookV_SDK.

Clone this repository to the same folder ScriptHookV_SDK was extracted so you have ScriptHookV_SDK and GTAVManualTransmission in the same folder. If you get build errors about missing functions, update your [natives.h](hhttps://raw.githubusercontent.com/E66666666/GTAVMenuBase/master/thirdparty/scripthookv-sdk-updates/natives.h).

Clone my [GTAVMenuBase](https://github.com/E66666666/GTAVMenuBase) to the same folder you're gonna clone this to.

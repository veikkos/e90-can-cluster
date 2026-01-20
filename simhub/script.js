return "S"
	+ ",RPM="  + Math.round($prop('Rpms') || 0)
	+ ",SPD="  + Math.round($prop('SpeedKmh') || 0)
	+ ",GEAR=" + Math.round($prop('Gear') || 0)
	+ ",FUEL=" + Math.round($prop('FuelPercent' || 100) * 10)
	+ ",OIL="  + Math.round($prop('OilTemperature') || 0)
	+ ",IGN="  + ($prop('EngineIgnitionOn') ? 1 : 0)
	+ "\n";

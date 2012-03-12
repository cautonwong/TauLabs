/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{
 * @addtogroup Sensors
 * @brief Acquires sensor data 
 * Specifically updates the the @ref Gyros, @ref Accels, and @ref Magnetometer objects
 * @{
 *
 * @file       sensors.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      Module to handle all comms to the AHRS on a periodic basis.
 *
 * @see        The GNU Public License (GPL) Version 3
 *
 ******************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/**
 * Input objects: None, takes sensor data via pios
 * Output objects: @ref Gyros @ref Accels @ref Magnetometer
 *
 * The module executes in its own thread.
 *
 * UAVObjects are automatically generated by the UAVObjectGenerator from
 * the object definition XML file.
 *
 * Modules have no API, all communication to other modules is done through UAVObjects.
 * However modules may use the API exposed by shared libraries.
 * See the OpenPilot wiki for more details.
 * http://www.openpilot.org/OpenPilot_Application_Architecture
 *
 */

#include "pios.h"
#include "attitude.h"

#include "accels.h"
#include "attitudeactual.h"
#include "attitudesettings.h"
#include "baroaltitude.h"
#include "gyros.h"
#include "gyrosbias.h"
#include "flightstatus.h"
#include "gpsposition.h"
#include "homelocation.h"
#include "magnetometer.h"
#include "revocalibration.h"

#include "CoordinateConversions.h"

// Private constants
#define STACK_SIZE_BYTES 1540
#define TASK_PRIORITY (tskIDLE_PRIORITY+3)
#define SENSOR_PERIOD 2

#define F_PI 3.14159265358979323846f
#define PI_MOD(x) (fmod(x + F_PI, F_PI * 2) - F_PI)
// Private types

// Private variables
static xTaskHandle sensorsTaskHandle;

// Private functions
static void SensorsTask(void *parameters);

/**
 * Initialise the module.  Called before the start function
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t SensorsInitialize(void)
{
	AccelsInitialize();
	BaroAltitudeInitialize();
	GyrosInitialize();
	GyrosBiasInitialize();
	MagnetometerInitialize();
	RevoCalibrationInitialize();

	return 0;
}

/**
 * Start the task.  Expects all objects to be initialized by this point.
 * \returns 0 on success or -1 if initialisation failed
 */
int32_t SensorsStart(void)
{
	// Start main task
	xTaskCreate(SensorsTask, (signed char *)"Sensors", STACK_SIZE_BYTES/4, NULL, TASK_PRIORITY, &sensorsTaskHandle);
	TaskMonitorAdd(TASKINFO_RUNNING_SENSORS, sensorsTaskHandle);
	PIOS_WDG_RegisterFlag(PIOS_WDG_SENSORS);

	return 0;
}

MODULE_INITCALL(SensorsInitialize, SensorsStart)

/**
 * Simulated sensor task.  Run a model of the airframe and produce sensor values
 */
static void SensorsTask(void *parameters)
{
	portTickType lastSysTime;

	AlarmsClear(SYSTEMALARMS_ALARM_SENSORS);
	
	HomeLocationData homeLocation;
	HomeLocationGet(&homeLocation);
	homeLocation.Latitude = 0;
	homeLocation.Longitude = 0;
	homeLocation.Altitude = 0;
	homeLocation.Be[0] = 26000;
	homeLocation.Be[1] = 400;
	homeLocation.Be[2] = 40000;
	homeLocation.Set = HOMELOCATION_SET_TRUE;
	HomeLocationSet(&homeLocation);

	// Main task loop
	lastSysTime = xTaskGetTickCount();
	while (1) {


		AccelsData accelsData; // Skip get as we set all the fields
		accelsData.x = 0;
		accelsData.y = -1;
		accelsData.z = -8;
		accelsData.temperature = 0;
		AccelsSet(&accelsData);


		GyrosData gyrosData; // Skip get as we set all the fields
		gyrosData.x = 2;
		gyrosData.y = 0;
		gyrosData.z = 1;

		// Apply bias correction to the gyros
		GyrosBiasData gyrosBias;
		GyrosBiasGet(&gyrosBias);
		gyrosData.x += gyrosBias.x;
		gyrosData.y += gyrosBias.y;
		gyrosData.z += gyrosBias.z;

		GyrosSet(&gyrosData);
		
		BaroAltitudeData baroAltitude;
		BaroAltitudeGet(&baroAltitude);
		baroAltitude.Altitude = 1;
		BaroAltitudeSet(&baroAltitude);

		GPSPositionData gpsPosition;
		GPSPositionGet(&gpsPosition);
		gpsPosition.Latitude = 0;
		gpsPosition.Longitude = 0;
		gpsPosition.Altitude = 0;		
		GPSPositionSet(&gpsPosition);
		
		// Because most crafts wont get enough information from gravity to zero yaw gyro, we try
		// and make it average zero (weakly)
		MagnetometerData mag;
		mag.x = 400;
		mag.y = 0;
		mag.z = 800;
		MagnetometerSet(&mag);
		
		PIOS_WDG_UpdateFlag(PIOS_WDG_SENSORS);

		vTaskDelay(20 / portTICK_RATE_MS);
	}
}

/**
  * @}
  * @}
  */
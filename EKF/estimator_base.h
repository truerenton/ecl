/****************************************************************************
 *
 *   Copyright (c) 2015 Estimation and Control Library (ECL). All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name ECL nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/**
 * @file estimator_base.h
 * Definition of base class for attitude estimators
 *
 * @author Roman Bast <bapstroman@gmail.com>
 *
 */

#include <stdint.h>
#include <matrix/matrix/math.hpp>
#include <lib/geo/geo.h>
#include "RingBuffer.h"

struct gps_message {
	uint64_t time_usec;
    int32_t lat;                // Latitude in 1E-7 degrees
    int32_t lon;                // Longitude in 1E-7 degrees
    int32_t alt;                // Altitude in 1E-3 meters (millimeters) above MSL
    uint8_t fix_type;           // 0-1: no fix, 2: 2D fix, 3: 3D fix, 4: RTCM code differential, 5: Real-Time
    float eph;                  // GPS horizontal position accuracy in m
    float epv;                  // GPS vertical position accuracy in m
    float sacc;                 // GPS speed accuracy in m/s
    uint64_t time_usec_vel; 	// Timestamp for velocity informations
    float vel_m_s;              // GPS ground speed (m/s)
    float vel_ned[3];           // GPS ground speed NED
    bool vel_ned_valid;         // GPS ground speed is valid
    uint8_t nsats;              // number of satellites used
    float gdop;                 // geometric dilution of precision
};

struct parameters {
	float mag_delay_ms = 0.0f;
	float baro_delay_ms = 0.0f;
	float gps_delay_ms = 200.0f;
	float airspeed_delay_ms = 200.0f;

	// input noise
	float gyro_noise = 0.001f;
	float accel_noise = 0.1f;

	// process noise
	float gyro_bias_p_noise = 1e-5f;
	float accel_bias_p_noise = 1e-3f;
	float gyro_scale_p_noise = 1e-4f;
	float mag_p_noise = 1e-2f;
	float wind_vel_p_noise = 0.05f;

	float gps_vel_noise = 0.05f;
	float gps_pos_noise = 1.0f;
	float baro_noise = 0.1f;
    float baro_innov_gate = 5.0f;       // barometer fusion innovation consistency gate size in standard deviations
    float velD_innov_gate = 5.0f;       // Vertical velocity fusion innovation consistency gate size in standard deviations
    float posNE_innov_gate = 5.0f;      // Horizontal position innovation consistency gate size in standard deviations
    float velNE_innov_gate = 5.0f;      // Horizontal velocity fusion innovation consistency gate size in standard deviations

	float mag_heading_noise = 3e-2f;	// measurement noise used for simple heading fusion
	float mag_declination_deg = 0.0f;	// magnetic declination in degrees
    float heading_innov_gate = 3.0f;	// heading fusion innovation consistency gate size in standard deviations
    float mag_innov_gate = 3.0f;        // magnetometer fusion innovation consistency gate size in standard deviations

    // these parameters control the strictness of GPS quality checks used to determine uf the GPS is
    // good enough to set a local origin and commence aiding
    int gps_check_mask = 21;    // bitmask used to control which GPS quality checks are used
    float req_hacc = 5.0f;      // maximum acceptable horizontal position error
    float req_vacc = 8.0f;      // maximum acceptable vertical position error
    float req_sacc = 1.0f;      // maximum acceptable speed error
    int req_nsats = 6;          // minimum acceptable satellite count
    float req_gdop = 2.0f;      // maximum acceptable geometric dilution of precision
    float req_hdrift = 0.3f;    // maximum acceptable horizontal drift speed
    float req_vdrift = 0.5f;    // maximum acceptable vertical drift speed
};

class EstimatorBase
{
public:
	EstimatorBase();
	~EstimatorBase();

	virtual bool update() = 0;

	// gets the innovations of velocity and position measurements
	// 0-2 vel, 3-5 pos
	virtual void get_vel_pos_innov(float vel_pos_innov[6]) = 0;

	// gets the innovations of the earth magnetic field measurements
	virtual void get_mag_innov(float mag_innov[3]) = 0;

	// gets the innovations of the heading measurement
	virtual void get_heading_innov(float *heading_innov) = 0;

	// gets the innovation variances of velocity and position measurements
	// 0-2 vel, 3-5 pos
	virtual void get_vel_pos_innov_var(float vel_pos_innov_var[6]) = 0;

	// gets the innovation variances of the earth magnetic field measurements
	virtual void get_mag_innov_var(float mag_innov_var[3]) = 0;

	// gets the innovation variance of the heading measurement
	virtual void get_heading_innov_var(float *heading_innov_var) = 0;

	virtual void get_state_delayed(float *state) = 0;

	virtual void get_covariances(float *covariances) = 0;

	// set delta angle imu data
	void setIMUData(uint64_t time_usec, uint64_t delta_ang_dt, uint64_t delta_vel_dt, float *delta_ang, float *delta_vel);

	// set magnetometer data
	void setMagData(uint64_t time_usec, float *data);

	// set gps data
	void setGpsData(uint64_t time_usec, struct gps_message *gps);

	// set baro data
	void setBaroData(uint64_t time_usec, float *data);

	// set airspeed data
	void setAirspeedData(uint64_t time_usec, float *data);

	// set range data
	void setRangeData(uint64_t time_usec, float *data);

	// set optical flow data
	void setOpticalFlowData(uint64_t time_usec, float *data);

	// return a address to the parameters struct
	// in order to give access to the application
	parameters *getParamHandle() {return &_params;}
protected:

	typedef matrix::Vector<float, 2> Vector2f;
	typedef matrix::Vector<float, 3> Vector3f;
	typedef matrix::Quaternion<float> Quaternion;
	typedef matrix::Matrix<float, 3, 3> Matrix3f;

	struct stateSample {
		Vector3f    ang_error;
		Vector3f    vel;
		Vector3f    pos;
		Vector3f    gyro_bias;
		Vector3f    gyro_scale;
		float       accel_z_bias;
		Vector3f    mag_I;
		Vector3f    mag_B;
		Vector2f    wind_vel;
		Quaternion  quat_nominal;
	} _state;

	struct outputSample {
		Quaternion  quat_nominal;
		Vector3f    vel;
		Vector3f    pos;
		uint64_t 	time_us;
	};

	struct imuSample {
		Vector3f    delta_ang;
		Vector3f    delta_vel;
		float       delta_ang_dt;
		float       delta_vel_dt;
		uint64_t    time_us;
	};

	struct gpsSample {
		Vector2f    pos;
		float       hgt;
		Vector3f    vel;
		uint64_t    time_us;
	};

	struct magSample {
		Vector3f    mag;
		uint64_t    time_us;
	};

	struct baroSample {
		float       hgt;
		uint64_t    time_us;
	};

	struct rangeSample {
		float       rng;
		uint64_t    time_us;
	};

	struct airspeedSample {
		float       airspeed;
		uint64_t    time_us;
	};

	struct flowSample {
		Vector2f    flowRadXY;
		Vector2f    flowRadXYcomp;
		uint64_t    time_us;
	};

	parameters _params;		// filter parameters

	static const uint8_t OBS_BUFFER_LENGTH = 10;
	static const uint8_t IMU_BUFFER_LENGTH = 30;
	static const unsigned FILTER_UPDATE_PERRIOD_MS = 10;

	float _dt_imu_avg;
	uint64_t _imu_time_last;

	imuSample _imu_sample_delayed;
	imuSample _imu_down_sampled;
	Quaternion
	_q_down_sampled;

	magSample _mag_sample_delayed;
	baroSample _baro_sample_delayed;
	gpsSample _gps_sample_delayed;
	rangeSample _range_sample_delayed;
	airspeedSample _airspeed_sample_delayed;
	flowSample _flow_sample_delayed;

	outputSample _output_sample_delayed;
	outputSample _output_new;
	imuSample _imu_sample_new;


	uint64_t _imu_ticks;

	bool _imu_updated = false;
	bool _start_predict_enabled = false;
	bool _initialised = false;
	bool _gps_initialised = false;
	bool _gps_speed_valid = false;

	bool _mag_healthy = false;		// computed by mag innovation test
    float _yaw_test_ratio;          // yaw innovation consistency check ratio
    float _mag_test_ratio[3];       // magnetometer XYZ innovation consistency check ratios

    float _vel_pos_test_ratio[6];   // velocity and position innovation consistency check ratios

    bool _in_air = true;			// indicates if the vehicle is in the air

	RingBuffer<imuSample> _imu_buffer;
	RingBuffer<gpsSample> _gps_buffer;
	RingBuffer<magSample> _mag_buffer;
	RingBuffer<baroSample> _baro_buffer;
	RingBuffer<rangeSample> _range_buffer;
	RingBuffer<airspeedSample> _airspeed_buffer;
	RingBuffer<flowSample> 	_flow_buffer;
	RingBuffer<outputSample> _output_buffer;

	uint64_t _time_last_imu;
	uint64_t _time_last_gps;
	uint64_t _time_last_mag;
	uint64_t _time_last_baro;
	uint64_t _time_last_range;
	uint64_t _time_last_airspeed;

	// flags capturing information about severe nummerical problems for various fusions
	struct {
		bool bad_mag_x: 1;
		bool bad_mag_y: 1;
		bool bad_mag_z: 1;
		bool bad_airspeed: 1;
		bool bad_sideslip: 1;
	} _fault_status;

	void initialiseVariables(uint64_t timestamp);

	void initialiseGPS(struct gps_message *gps);

	bool gps_is_good(struct gps_message *gps);

    // variables used for the GPS quality checks
    float _gpsDriftVelN = 0.0f;     // GPS north position derivative (m/s)
    float _gpsDriftVelE = 0.0f;     // GPS east position derivative (m/s)
    float _gps_drift_velD = 0.0f;     // GPS down position derivative (m/s)
    float _gps_velD_diff_filt = 0.0f;   // GPS filtered Down velocity (m/s)
    float _gps_velN_filt = 0.0f;  // GPS filtered North velocity (m/s)
    float _gps_velE_filt = 0.0f;   // GPS filtered East velocity (m/s)
    uint64_t _last_gps_fail_us = 0;   // last system time in usec that the GPS failed it's checks

public:
	void printIMU(struct imuSample *data);
	void printStoredIMU();
	void printQuaternion(Quaternion &q);
	void print_imu_avg_time();
	void printMag(struct magSample *data);
	void printStoredMag();
	void printBaro(struct baroSample *data);
	void printStoredBaro();
	void printGps(struct gpsSample *data);
	void printStoredGps();

	bool position_is_valid();

	void copy_quaternion(float *quat)
	{
		for (unsigned i = 0; i < 4; i++) {
			quat[i] = _output_new.quat_nominal(i);
		}
	}
	void copy_velocity(float *vel)
	{
		for (unsigned i = 0; i < 3; i++) {
			vel[i] = _output_new.vel(i);
		}
	}
	void copy_position(float *pos)
	{
		for (unsigned i = 0; i < 3; i++) {
			pos[i] = _output_new.pos(i);
		}
	}
	void copy_timestamp(uint64_t *time_us)
	{
		*time_us = _imu_time_last;
	}

    // Variables used to publish the WGS-84 location of the EKF local NED origin
    uint64_t _last_gps_origin_time_us = 0;              // time the origin was last set (uSec)
    struct map_projection_reference_s _pos_ref = {};     // Contains WGS-84 position latitude and longitude (radians)
    float _gps_alt_ref = 0.0f;                          // WGS-84 height (m)

    bool _vehicle_armed = false;     // vehicle arm status used to turn off funtionality used on the ground

    // publish the status of various GPS quality checks
    union gps_check_fail_status_u {
        struct {
            uint16_t nsats  : 1; // 0 - true if number of satellites used is insufficient
            uint16_t gdop   : 1; // 1 - true if geometric dilution of precision is insufficient
            uint16_t hacc   : 1; // 2 - true if reported horizontal accuracy is insufficient
            uint16_t vacc   : 1; // 3 - true if reported vertical accuracy is insufficient
            uint16_t sacc   : 1; // 4 - true if reported speed accuracy is insufficient
            uint16_t hdrift : 1; // 5 - true if horizontal drift is excessive (can only be used when stationary on ground)
            uint16_t vdrift : 1; // 6 - true if vertical drift is excessive (can only be used when stationary on ground)
            uint16_t hspeed : 1; // 7 - true if horizontal speed is excessive (can only be used when stationary on ground)
            uint16_t vspeed : 1; // 8 - true if vertical speed error is excessive
        } flags;
        uint16_t value;
    };
    gps_check_fail_status_u _gps_check_fail_status;

};

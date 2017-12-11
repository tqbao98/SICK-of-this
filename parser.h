#ifndef _SCAN_DATA_FRAME_H_
#define _SCAN_DATA_FRAME_H_

#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <math.h>
#include <chrono>
#include <ctime>
#include <utility>
//#include <errno.h>
//#include <fcntl.h>

#define MAXSIZE 100000
#define DEBUG

#ifdef DEBUG
#define DEBUG_PRINT(...) \
            do { fprintf(stderr, ##__VA_ARGS__); fprintf(stderr, "\n"); } while (0)
#else
#define DEBUG_PRINT(...) ;
#endif


struct data_point_t {
	uint16_t range;
	uint8_t intensity;
};

typedef data_point_t DataPoint;

class ScanDataFrame
{
private:
	std::chrono::high_resolution_clock::time_point m_time_stamp;
	float m_start_angle;
	float m_end_angle;
	float m_angular_step;
	int32_t m_num_points;
	DataPoint* m_points;
	bool trigger = false;
	time_t start, end;


public:
	ScanDataFrame();
	ScanDataFrame(const ScanDataFrame& new_data);
	~ScanDataFrame();

	void ParseDatagram(char* buffer, int buffer_length);
	bool ReadFromFile(const char* filename);
	bool SaveToFile(const char* filename);
	bool Section(bool trigger);
	std::pair <bool, bool> Timer(bool, bool);

	void SetTimeStampToCurrentTime() {
		m_time_stamp = std::chrono::high_resolution_clock::now();
	}

	std::chrono::high_resolution_clock::time_point GetTimeStamp() {
		return m_time_stamp;
	}

	float GetStartAngle() { return m_start_angle; }
	float GetEndAngle() { return m_end_angle; }
	float GetAngularStep() { return m_angular_step; }
	int32_t GetNumPoints() { return m_num_points; }
	data_point_t* GetPointList() { return m_points; }
};

#endif // _SCAN_DATA_FRAME_H_


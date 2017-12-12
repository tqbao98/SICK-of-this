#include "parser.h"
#include <iostream>

using namespace std;

static int ReadNextField(char* source_buffer, int source_length, int index, char* store_buffer)
{
	int c = index;
	while (c < source_length && (source_buffer[c] != '_') && !isalnum(source_buffer[c])) {
		++c;
	}
	int len = 0;
	while ((isalnum(source_buffer[c]) || source_buffer[c] == '_') && c < source_length && source_buffer[c] != 0x03 && source_buffer[c] != 0x00) {
		store_buffer[len++] = source_buffer[c++];
	}
	store_buffer[len] = '\0';
	if (source_buffer[c] == 0x03) {
		return -1;
	}
	/*DEBUG_PRINT(store_buffer);*/
	return c;
}

ScanDataFrame::ScanDataFrame()
	: m_points(NULL)
{
}

ScanDataFrame::ScanDataFrame(const ScanDataFrame& new_data)
	: m_time_stamp(new_data.m_time_stamp),
	m_start_angle(new_data.m_start_angle),
	m_end_angle(new_data.m_end_angle),
	m_angular_step(new_data.m_angular_step),
	m_num_points(new_data.m_num_points)
{
	m_points = new data_point_t[new_data.m_num_points];
	for (int i = 0; i < new_data.m_num_points; ++i) {
		m_points[i] = new_data.m_points[i];
	}
}

ScanDataFrame::~ScanDataFrame()
{
	delete[] m_points;
}


void ScanDataFrame::ParseDatagram(char* buffer, int buffer_length)
{
	std::vector<std::string> fields(0);
	/*cout << buffer << endl;
	cout << buffer_length << endl;*/
	int k = 0;
	int num_fields = 0;
	while (k < buffer_length && k != -1) {
		char* s = new char[MAXSIZE + 1];
		k = ReadNextField(buffer, buffer_length, k, s);
		if (k >= buffer_length) {
			cout << "break" << endl;
			break;
		}
		fields.push_back(std::string(s));
		++num_fields;
		delete[] s;
	}

	int32_t temp;

	/*char* s = new char[1000];*/
	//  Utils::GetTimeString(&m_time_stamp, s);
	//  DEBUG_PRINT(" + Time: %s", s);

	sscanf(fields[23].c_str(), "%x", &temp);
	m_start_angle = (float)temp / 10000;
	cout << " + Start angle: " << m_start_angle << endl;

	sscanf(fields[24].c_str(), "%x", &temp);
	m_angular_step = (float)temp / 10000;
	cout << " + Angular step: " << m_angular_step << endl;

	sscanf(fields[25].c_str(), "%x", &m_num_points);
	cout << " + Number of data points: " << m_num_points << endl;

	m_end_angle = m_start_angle + (m_num_points - 1)*m_angular_step;
	cout << " + End angle: " << m_end_angle << endl;

	m_points = new data_point_t[m_num_points];
	int offset = 26;
	for (int i = 0; i < m_num_points; ++i) {
		sscanf(fields[offset + i].c_str(), "%x", &temp);
		m_points[i].range = (uint16_t)temp;
		DEBUG_PRINT("range[%d] = %d", i, m_points[i].range);
		m_points[i].intensity = 0;
	}
	for (; offset < (int)fields.size(); ++offset) {
		if (strcmp("RSSI1", fields[offset].c_str()) == 0) {
			break;
		}
	}
	if (offset < (int)fields.size()) { // RSSI1 included in the message
		for (int i = 0; i < m_num_points; ++i) {
			sscanf(fields[offset + i].c_str(), "%x", &temp);
			m_points[i].intensity = (uint8_t)temp;
			DEBUG_PRINT("intensity[%d] = %d", i, m_points[i].intensity);
		}
	}

	/*delete[] s;*/
}

bool ScanDataFrame::ReadFromFile(const char* filename)
{
	std::ifstream in_file;
	in_file.open(filename, std::ios_base::binary);
	if (!in_file.is_open()) {
		DEBUG_PRINT("Failed to open file %s!", filename);
		return false;
	}
	in_file.read((char*)this, sizeof(ScanDataFrame) - sizeof(data_point_t*));
	m_points = new data_point_t[m_num_points];
	for (int i = 0; i < m_num_points; ++i) {
		in_file.read((char*)&m_points[i], sizeof(data_point_t));
	}
	in_file.close();
	return true;
}

bool ScanDataFrame::SaveToFile(const char* filename)
{
	std::ofstream out_file;
	out_file.open(filename, std::ios_base::binary);
	if (!out_file.is_open()) {
		return false;
	}
	out_file.write((const char*)this, sizeof(ScanDataFrame) - sizeof(data_point_t*));
	out_file.write((const char*)m_points, sizeof(data_point_t)*m_num_points);
	out_file.close();
	return true;
}

bool ScanDataFrame::Section(bool trigger) {
		for (int i = 350; i <= 450; i++) {
			if (m_points[i].range <= 150 && m_points[i].range >= 30) {
				trigger = true; //global var
				break;
				}
				else trigger = false;
			}
		return trigger;
}

std::pair <bool, bool> ScanDataFrame::Timer(bool memory, bool turnoff) {
	pair <bool, bool> tuple;
	tuple.first = memory;
	tuple.second = turnoff;
	time(&start);
	while ((double)(clock() - start) / CLOCKS_PER_SEC <= 3)
	{
		if (turnoff == true)
		{
			start = 4;
			turnoff = false;
			memory = false;
			break;
		}
	}
	system("xset dpms force on");
	memory = false;
	return tuple;
}


#include "buffer.h"
#include "parser.h"
#include <iostream>
#include <thread>

int main() {
	
	
		/*unsigned char* receiveBuffer = new char[65536];*/
	bool maintrigger = false, mainmemory = false, mainturnoff = true;
	system("xset dpms force off");
	


	for (int loop = 0; loop == 1500; loop++) {
		int actual_length = 0;
		char* receiveBuffer = new char[65536];
		ScanDataFrame data;
		SickTimCommonTcp myTcp(5000);

			myTcp.init_device();
			myTcp.sendSOPASCommand("\x02sEN  LMDscandata 1\x03\0");
			//myTcp.sendSOPASCommand("\x02sWN LMPoutputRange 1 2710 FFF92230 225510\x03\0");

			myTcp.get_datagram(receiveBuffer, 65536, &actual_length);

			cout << receiveBuffer << endl;

			data.ParseDatagram(receiveBuffer, actual_length);
			data.Section(maintrigger);
			if (maintrigger == true) {
				if (mainmemory == false) {
					thread ThrObj(&ScanDataFrame::Timer, data, mainmemory, mainturnoff);
					ThrObj.detach();
				}
			} else {mainturnoff = true; system("xset dpms force off");}


			delete[] receiveBuffer;

			if (actual_length <= 0) return main();

			myTcp.close_device();
		}
		return 0;
}

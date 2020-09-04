#include <cctype>
#include <stdlib.h>
#include <iostream>

bool state = 0; //Producer/Consumer
unsigned int delay = 0; //ms
unsigned int memSize = 0; //KB
unsigned int numMsg = 0;
unsigned int msgSize = 0; //0 for random

void inputParser(const int* argc, char** argv) {
	unsigned short argChecked = 0;
	switch (std::tolower(argv[1][0])) {
		case 'c':
			//Consumer
			break;
		case 'p':
			//Producer
			break;
	}
	unsigned int* varPtr[4] = {&delay, &memSize, &numMsg, &msgSize };
	for (int i = 0; i < 4; i++) {
		*varPtr[i] = atoi(argv[i+2]);
	}
}


int main(int argc, char** argv) {
	if(argc > 1){
		inputParser(&argc, argv);
	}
	if (!(memSize*numMsg > 0)) {
		//memory size and number of messages must both be nonzero
		std::cout << "Invalid argument\r\nExpected: Producer|Consumer, delay, memory size, number of messages, message size\r\n";
		return 0;
	}

	getchar();
	return 0;
}
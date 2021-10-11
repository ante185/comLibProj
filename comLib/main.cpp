#define WIN32_LEAN_AND_MEAN
#pragma comment (lib, "user32")
#include <windows.h>
#include <winnt.h>
#include <WinBase.h>
#include <cctype>
#include <stdlib.h>
#include <iostream>

#define headerStep 16

bool state				= 0; //Producer/Consumer
bool random				= 1;
UINT64 delay			= 1; //ms
UINT64 memSize			= 64; //KB
UINT64 numMsg			= 150000;
unsigned int msgSize	= 1024; //0 for random
UINT64 RWHeadPos		= 0;
UINT64 lastpos			= 0;

// 1) define variables (global or in a struct/class)
HANDLE hFileMap;

unsigned char* mData;
bool exists = false;


unsigned int counter = 0;

void inputParser(const int* argc, char** argv) {
	unsigned short argChecked = 0;
	switch (std::tolower(argv[1][0])) {
		case 'c':
			//Consumer
			state = true;
			break;
		case 'p':
			//Producer
			state = false;
			break;
	}
	UINT64* varPtr[3] = {&delay, &memSize, &numMsg};
	for (int i = 0; i < 3; i++) {
		*varPtr[i] = atoi(argv[i+2]);
	}
	if (std::strcmp( argv[5],"random") == 0) {
		random = true;
	}
	else {
		msgSize = atoi(argv[5]);
	}
}

struct message {
	unsigned int size = 64;
	unsigned char flag = 6;
	unsigned char* data = nullptr;
	~message() {
		if (data) {
			delete data;
		}
	}
};

void write(message* data) {

	if (!(data->size > 0)) {
		std::cout << "Zero size message ";
	}
	while (true) {
		Sleep(delay);
		if (RWHeadPos + sizeof(message::size) + sizeof(message::flag) >= memSize) {
			RWHeadPos = 0;
		}
		if (RWHeadPos + data->size + sizeof(message::size) + sizeof(message::flag) >= memSize || RWHeadPos + 64 >= memSize) {
			mData[RWHeadPos + sizeof(message::size)] = (unsigned char)15;
			//std::cout << "CR" << std::endl;
			RWHeadPos = 0;
		}

		if ((mData[RWHeadPos + data->size] | mData[RWHeadPos + data->size + 1] | mData[RWHeadPos + data->size + sizeof(message::size)]) == 0) {
			//std::memset(data->data + msgSize - 2u, 0u, 1u);
			std::memcpy(mData + RWHeadPos + sizeof(message::size) + sizeof(message::flag), data->data, data->size);
			std::memcpy(mData + RWHeadPos + sizeof(message::size), &data->flag, sizeof(message::flag));
			std::memcpy(mData + RWHeadPos, &data->size, sizeof(message::size));
			RWHeadPos += (UINT64)data->size + sizeof(message::size) + sizeof(message::flag);
			if (RWHeadPos + sizeof(message::size) + sizeof(message::flag) >= memSize) {
				RWHeadPos = 0;
			}
			counter++;
			//std::cout << counter << ':' << data->data << ' ' << data->size << ' ' << lastpos << ' ' << RWHeadPos << std::endl;
			
			break;
		}
		else {
			//Msg loops arounds
		}
	}
}

unsigned char* read() {
	unsigned char* msgData;

	if (RWHeadPos + sizeof(message::size) + sizeof(message::flag) >= memSize) {
		RWHeadPos = 0;
	}
	//Wait for size to be written if it hasn't
	while ((mData[RWHeadPos] | mData[RWHeadPos + 1]) == 0 || mData[RWHeadPos + sizeof(message::size)] == 15)
	{
		Sleep(delay);

		if (mData[RWHeadPos + sizeof(message::size)] == 15u) { //'15' == Carrage return, interpret as return to buffer start
			//std::cout << "CR" << std::endl;
			std::memset(mData + RWHeadPos, 0, sizeof(message::size) + sizeof(message::flag));
			RWHeadPos = 0;
		}
	}
	unsigned int mgSize;
	std::memcpy(&mgSize, mData + RWHeadPos, sizeof(message::size));
	msgData = new unsigned char[((UINT64)mgSize + 3u)];
	unsigned int sizeIndex = (mgSize + RWHeadPos);
	while ((mData[sizeIndex - 1] | mData[sizeIndex - 2] ) == 0) {
		Sleep(delay);
		//Wait for last bit to be written if it hasn't
	}
	if (RWHeadPos + mgSize + sizeof(message::size) + sizeof(message::flag) < memSize) {
		std::memcpy(msgData, (mData + RWHeadPos + sizeof(message::size) + sizeof(message::flag)), mgSize);
		std::memset(mData + RWHeadPos + sizeof(message::size) + sizeof(message::flag), 0, (UINT64)mgSize);
		std::memset(mData + RWHeadPos, 0, sizeof(message::size));
		RWHeadPos += (UINT64)mgSize + sizeof(message::size) + sizeof(message::flag);
	}
	else {
		//Msg loops around
		std::cout << "Message end outside of buffer" << std::endl;
	}
	counter++;
	return msgData;
}

static uint32_t nLehmer = 0;
//A better LCG than plain rand()
int LehmerInt() {
	nLehmer += 0xe120fc15;
	uint64_t temp;
	temp = (uint64_t)nLehmer * 0x4a39b70d;
	uint32_t m1 = (temp >> 32) ^ temp;
	temp = (uint64_t)m1 * 0x12fad5c9;
	uint32_t m2 = (temp >> 32) ^ temp;
	return m2;
}
// random character generation of "len" bytes.
// the pointer s must point to a big enough buffer to hold "len" bytes.
void gen_random(char *s, const int len) {
	static const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		;

	for (auto i = 0; i < len; ++i) {
		//s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
		s[i] = alphanum[LehmerInt() % (sizeof(alphanum) - 1)];
	}
	s[len - 1] = 0;
}

#undef min
#undef max

bool producer(){

	lastpos = RWHeadPos;

	message msg;

	if (random) {
		msgSize = (unsigned)LehmerInt();
		msgSize %= (memSize/2u);
		msgSize -= 5u;
		msgSize = std::max(msgSize, 64u);
		msgSize = std::min((UINT64)msgSize, (memSize / 2u) - 5u);
	}
	msg.size = msgSize;
	msg.data = new unsigned char[msg.size]{ 0 };

	if (numMsg > 0) {
		if ((mData[RWHeadPos] | mData[RWHeadPos + 1]) == 0) {
			numMsg--;
			gen_random((char*)msg.data, msg.size - 1);
			write(&msg);
			std::cout << counter << ' ' << msg.data << std::endl;
		}
	}
	else {
		if (*(unsigned int*)(mData + RWHeadPos) == 0) {
			msg.size = 16;
			msg.flag = 4;
			gen_random((char*)msg.data, msg.size - 1);
			write(&msg);
			return false;
		}
	}

	return true;
}

bool consumer() {

	unsigned char* dataPtr;

	lastpos = RWHeadPos;

	dataPtr = read();

	if (lastpos + sizeof(message::size) < memSize && mData[lastpos + sizeof(message::size)] == (unsigned char)4) {
		return false;
	}
	else if (lastpos + sizeof(message::size) + sizeof(message::flag) < memSize) {
		//std::cout << (unsigned int)mData[lastpos] << ' ';
		//std::cout << counter << ':' << dataPtr << ' ' << lastpos << ' ' << RWHeadPos << std::endl;
		std::cout << counter << ' ' << dataPtr << std::endl;
		mData[lastpos + sizeof(message::size)] = (unsigned char)0;
	}

	delete dataPtr;


	return true;
}



int main(int argc, char** argv) {
	std::srand(std::time(nullptr));
	nLehmer = std::time(nullptr);
	nLehmer += std::rand();
	if (argc > 1) {
		inputParser(&argc, argv);
	}
	memSize *= 1024u;
	// 2) API call to create FileMap (use pagefile as a backup)
	hFileMap = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		(DWORD)0,
		(UINT)memSize,
		(LPCSTR) "myFileMap");
	// 3) check if hFileMap is NULL -> FATAL ERROR
	if(!hFileMap)
		return false;
	// 4) check 
	//          if (GetLastError() == ERROR_ALREADY_EXISTS)
	//    This means that the file map already existed!, but we
	//    still get a handle to it, we share it!
	//    THIS COULD BE USEFUL INFORMATION FOR OUR PROTOCOL.
	// 5) Create one VIEW of the map with a void pointer
	//    This API Call will create a view to ALL the memory
	//    of the File map.
	

	
	mData = (unsigned char*)MapViewOfFile(hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);


	if (!(memSize*numMsg > 0)) {
		//memory size and number of messages must both be nonzero and larger than zero
		std::cout << "Invalid argument\r\nExpected: Producer|Consumer, delay, memory size, number of messages, message size\r\n";
		return 0;
	}


	if(state) {
		//Consumer
		//std::cout << "Consumer" << std::endl;
		while(consumer()){
			Sleep(delay);
		};
	}
	else{
		//Producer
		//std::cout << "Producer" << std::endl;
		//std::memset(mData, (char)0, memSize);
		msgSize = std::max(msgSize, 64u);
		//msgSize -= 3u;
		Sleep(delay * 50);
		while(producer()){
			Sleep(delay);
		};
	}
	//std::cout << std::endl << "EOT";
	//getchar();
	// LAST BUT NOT LEAST
	// Always, close the view, and close the handle, before quiting
	UnmapViewOfFile((LPCVOID)mData);
	CloseHandle(hFileMap);
	return 0;
}
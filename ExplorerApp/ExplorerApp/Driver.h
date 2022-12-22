#pragma once // что бы не подключить второй раза windows.h в основном файле
#include <windows.h>
#include <string>

class Driver
{
public:
	int DriverCount = 0;
	std::wstring* Name;
	Driver();
	~Driver();
};


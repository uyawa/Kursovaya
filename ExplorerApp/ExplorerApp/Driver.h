#pragma once // ��� �� �� ���������� ������ ���� windows.h � �������� �����
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


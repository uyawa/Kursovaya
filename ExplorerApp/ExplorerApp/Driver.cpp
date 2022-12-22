#include "Driver.h"

Driver::Driver(){
	const int BUFFER_SIZE = 200;
	WCHAR str[BUFFER_SIZE];
	int StringSize = GetLogicalDriveStrings(BUFFER_SIZE, str);
	// ������ str ����� ��� C://\0\0. \0 ����� ����� ������� �������� � � ����� ������
	// ���������� ���-�� ���������
	for (int i = 0; i < StringSize; i++)
	{
		if (str[i] == '\0')
			DriverCount++;
	}

	Name = new std::wstring[DriverCount];
	// �������� �������� ����� �� ����� ������
	for (int i = 0, j = 0; i < StringSize; i++)
	{
		if (str[i] != '\0')
			Name[j].push_back(str[i]);
		else // ���� str[i] = '\0' - �������� ��������� ������ ������
			j++; 
	}
}

Driver::~Driver()
{
	delete[] Name;
}

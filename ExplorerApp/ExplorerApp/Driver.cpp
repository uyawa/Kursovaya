#include "Driver.h"

Driver::Driver(){
	const int BUFFER_SIZE = 200;
	WCHAR str[BUFFER_SIZE];
	int StringSize = GetLogicalDriveStrings(BUFFER_SIZE, str);
	// строка str имеет вид C://\0\0. \0 стоит после каждого драйвера и в конце строки
	// записываем кол-во драйверов
	for (int i = 0; i < StringSize; i++)
	{
		if (str[i] == '\0')
			DriverCount++;
	}

	Name = new std::wstring[DriverCount];
	// Отделяем названия томов из общей строки
	for (int i = 0, j = 0; i < StringSize; i++)
	{
		if (str[i] != '\0')
			Name[j].push_back(str[i]);
		else // если str[i] = '\0' - начинаем заполнять другой массив
			j++; 
	}
}

Driver::~Driver()
{
	delete[] Name;
}

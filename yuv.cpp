// yuv.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include <iostream>
using namespace std;

struct Frame 
{
	unsigned char *y, *u, *v;
};

/*---MAD mean absolute difference ---*/
float costFuncMAD(Frame cur, int i, int j, Frame prev, int ii, int jj, int n, int width, int height)
{
	float err = 0;
	for (int k = 0; k < n; k++)		// n - размер стороны блока
	{
		for (int l = 0; l < n; l++)
		{
			err += abs(cur.y[i * width + j] - prev.y[ii * width + jj]);
			err += abs(cur.u[i / 2 * width + j / 2] - prev.u[ii / 2 * width + jj / 2]);
			err += abs(cur.v[i / 2 * width + j / 2] - prev.v[ii / 2 * width + jj / 2]);
			j++; jj++;
		}
		i++; ii++;
	}
	return (err / (n*n));
}

void minCost(float **costs, int p, int *dx, int *dy)
{
	int row = 2 * p + 1, col = 2 * p + 1;
	float min = 65537;
	for (int i = 0; i < row; i++)
		for (int j = 0; j < row; j++)
			if (costs[i][j] < min)
			{
				min = costs[i][j];
				*dx = j; *dy = i;
			}
}

int main()
{
	// формат 4.2.0
	FILE *yuvFile;
	int width = 352, height = 288;
	int mbsize = 16, p = 7;		// mbsize размер блока, p = 7 пикселей - параметр для области поиска 
	int **vectors;
	Frame current, previous;
	current.y = new unsigned char[width*height];
	current.u = new unsigned char[width*height / 4];
	current.v = new unsigned char[width*height / 4];
	
	previous.y = new unsigned char[width*height];
	previous.u = new unsigned char[width*height / 4];
	previous.v = new unsigned char[width*height / 4];

	if ((yuvFile = fopen("bus_cif.y4m", "rb")) == NULL) // "rb" Открывает двоичный файл для чтения
		cout << "Cannot open file.\n";
	else
	{
		// считывание двух кадров
		fread(previous.y, 1, width * height, yuvFile);
		fread(previous.u, 1, width * height / 4, yuvFile);
		fread(previous.v, 1, width * height / 4, yuvFile);

		int l = fread(current.y, 1, width * height, yuvFile);
		fread(current.u, 1, width * height / 4, yuvFile);
		fread(current.v, 1, width * height / 4, yuvFile);

		while (l == width * height)
		{
			
			vectors = new int *[2];		// координаты векторов движения
			for (int i = 0; i < 2; i++)
			{
				vectors[i] = new int[height*width / mbsize*mbsize];
				for (int j = 0; j < height*width / mbsize*mbsize; j++)
					vectors[i][j] = 0;
			}
			float **costs;		// матрица вычислений разности
			costs = new float*[2 * p + 1];
			for (int i = 0; i < 2 * p + 1; i++)
			{
				costs[i] = new float[2 * p + 1];
				for (int j = 0; j < 2 * p + 1; j++)
					costs[i][j] = 1 * 65537;
			}
			int mbCount = 0;	// кол-во проверенных блоков кадра
			for (int i = 0; i <= width - mbsize; i += mbsize)
				for (int j = 0; j <= height - mbsize; j += mbsize)
				{
					for (int m = -p; m <= p; m++)
						for (int n = -p; n <= p; n++)
						{
							int refBlkVer = i + m;	// координаты по строке ссылочного блока
							int refBlkHor = j + n;  // координаты по столбцу ссылочного блока
							if (refBlkVer < 0 || refBlkVer + mbsize - 1 > width || refBlkHor < 0 || refBlkHor + mbsize - 1 > height)
								continue;
							costs[m + p][n + p] = costFuncMAD(current, i, j, previous, refBlkVer, refBlkHor, mbsize, width, height);
						}
					int dx, dy;
					minCost(costs, p, &dx, &dy);
					vectors[0][mbCount] = dy - p;
					vectors[1][mbCount] = dx - p;
					mbCount++;

					for (int i = 0; i < 2 * p + 1; i++)
					{
						costs[i] = new float[2 * p + 1];
						for (int j = 0; j < 2 * p + 1; j++)
							costs[i][j] = 1 * 65537;
					}
				}
			for (int i = 0; i < height / mbsize; i++)
			{
				for (int j = 0; j < width / mbsize; j++)
					cout << vectors[0][i*width/mbsize + j] << ", " << vectors[1][i*width/mbsize + j] << " | ";
				cout << endl;
			}
			cout << endl;
			for (int i = 0; i < height * width; i++)
				previous.y[i] = current.y[i];
			for (int i = 0; i < height * width / 4; i++)
			{
				previous.u[i] = current.u[i];
				previous.v[i] = current.v[i];
			}
			l = fread(current.y, 1, width * height, yuvFile);
			fread(current.u, 1, width * height / 4, yuvFile);
			fread(current.v, 1, width * height / 4, yuvFile);
		}
	}
    return 0;
}
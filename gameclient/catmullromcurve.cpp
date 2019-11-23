#include "catmullromcurve.h"
#include <math.h>

static const float alpha = 0.5f;

float GetT(float t, Point p0, Point p1)
{
	float deltaX = p1.X - p0.X;
	float deltaY = p1.Y - p0.Y;
	float a = deltaX * deltaX + deltaY * deltaY;
	float b = sqrt(a);
	//float c = Mathf.Pow(b, alpha);
	float c = sqrt(b);


	return (c + t);
}

void CatmulRom(int amountOfPoints, Point* pointArray, const Point& p0, const Point& p1, const Point& p2, const Point& p3)
{
	float t0 = 0.0f;
	float t1 = GetT(t0, p0, p1);
	float t2 = GetT(t1, p1, p2);
	float t3 = GetT(t2, p2, p3);

	int i = 0;
	float step = (t2 - t1) / amountOfPoints;
	for (float t = t1; t < t2; t += step)
	{
        float tmp1, tmp2, X, Y;
		tmp1 = (t1 - t) / (t1 - t0);
		tmp2 = (t - t0) / (t1 - t0);
		X = tmp1 * p0.X + tmp2 * p1.X;
		Y = tmp1 * p0.Y + tmp2 * p1.Y;
		Point A1 = { X, Y };


		tmp1 = (t2 - t) / (t2 - t1);
		tmp2 = (t - t1) / (t2 - t1);
		X = tmp1 * p1.X + tmp2 * p2.X;
		Y = tmp1 * p1.Y + tmp2 * p2.Y;
		Point A2 = { X, Y };

		tmp1 = (t3 - t) / (t3 - t2);
		tmp2 = (t - t2) / (t3 - t2);
		X = tmp1 * p2.X + tmp2 * p3.X;
		Y = tmp1 * p2.Y + tmp2 * p3.Y;
		Point A3 = { X, Y };

		tmp1 = (t2 - t) / (t2 - t0);
		tmp2 = (t - t0) / (t2 - t0);
		X = tmp1 * A1.X + tmp2 * A2.X;
		Y = tmp1 * A1.Y + tmp2 * A2.Y;
		Point B1 = { X, Y };

		tmp1 = (t3 - t) / (t3 - t1);
		tmp2 = (t - t1) / (t3 - t1);
		X = tmp1 * A2.X + tmp2 * A3.X;
		Y = tmp1 * A2.Y + tmp2 * A3.Y;
		Point B2 = { X, Y };

		tmp1 = (t2 - t) / (t2 - t1);
		tmp2 = (t - t1) / (t2 - t1);
		X = tmp1 * B1.X + tmp2 * B2.X;
		Y = tmp1 * B1.Y + tmp2 * B2.Y;
		Point C = { X, Y };

		pointArray[i++] = C;
	}
}
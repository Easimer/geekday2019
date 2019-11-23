#pragma once

struct Point {
    float X;
    float Y;
};

void CatmulRom(int amountOfPoints, Point* pointArray, const Point& p0, const Point& p1, const Point& p2, const Point& p3);
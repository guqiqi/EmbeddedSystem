//
// Created by kiki G on 2018/10/31.
//

#include <cstdio>
#include <cmath>
#include <iostream>
#include "GPIOlib.h"

using namespace std;
using namespace GPIO;

int readingLeft = 0, readingRight = 0;

// speed of the car
const int speed = 10;
// default angle with the wheel
const int wheel = 2;
// default solpe of the left line in camera
const double defaultSlope = 1.0;
// params of PID
const double kp = 1;
const double ki = 0.002;
const double kd = 0.003;

double oldAngle = 0.0;

int getAngleByPoint(double x, double y){
    double angle = atan(x / y) / M_PI * 180.0;

    cout << "tempAngle: " << angle << endl;
    int turnAngle = kp * angle + ki * (angle + oldAngle) * speed + kd * (angle - oldAngle);
    oldAngle = angle;

    return turnAngle;
}

int getAngleBySlope(double slope){
    cout << "slope: " << slope << endl;
    double angle = 0.0;
    if(slope > 0 && slope < defaultSlope){
        angle = 90.0 - atan(slope) / M_PI * 180.0;
    }
    else if(slope < 0 && slope > (-1.0 * defaultSlope)){
        angle = 90.0 + atan(slope) / M_PI * 180.0;
    }
    else {
        angle = oldAngle;
    }

    cout << "tempAngle: " << angle << endl;
    int turnAngle = kp * angle + ki * (angle + oldAngle) * speed + kd * (angle - oldAngle);
    oldAngle = angle;

    return turnAngle;
}

// 常量，默认左右两直接的角度
const float left = 60;
const float right = 120;

int main() {

    init();

    controlLeft(FORWARD, 10);
    controlRight(FORWARD, 10);

    // distance is in cm
    double totalLeft = 0;
    double totalRight = 0;

    // stop until run to 1m
    while (totalLeft < 100) {
        resetCounter();
        delay(1000);
        getCounter(&readingLeft, &readingRight);
        if (readingLeft == -1 || readingRight == -1) {
            printf("Error!\n");
            continue;
        }
        // Distance is in mm.
        // M_PI is default number of PI 3.1415……
        double distanceLeft = readingLeft * 63.4 * M_PI / 390;
        double distanceRight = readingRight * 63.4 * M_PI / 390;

        totalLeft += distanceLeft / 10;
        totalRight += distanceRight / 10;
        printf("Left wheel moved %.2lf cm, right wheel moved %.2lf cm in last second.\n", distanceLeft / 10,
               distanceRight / 10);

        int angle = 60;
        int turnAngle = 0;
        if (angle > 90) { // 右边直线
            turnAngle = (int) (angle - right) / 2;
        } else { // 左边直线
            turnAngle = (int) (angle - left) / 2;
        }

        turnTo(turnAngle);
    }
    // 停止
    stopLeft();
    stopRight();
    return 0;
}

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

int main() {
    init();

    controlLeft(FORWARD, 8);
    controlRight(FORWARD, 8);

    // distance is in cm
    double totalLeft = 0;
    double totalRight = 0;

    // stop until run to 1m
    while (totalLeft < 1400) {
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
    }
    // 停止
    stopLeft();
    stopRight();
    return 0;
}

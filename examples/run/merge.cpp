#include <cstdlib>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdio>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>

#include "GPIOlib.h"

#define PI 3.1415926

//Uncomment this line at run-time to skip GUI rendering
//#define _DEBUG

using namespace cv;
using namespace std;
using namespace GPIO;

const string CAM_PATH = "/dev/video0";
const string MAIN_WINDOW_NAME = "Processed Image";
const string CANNY_WINDOW_NAME = "Canny";

const int CANNY_LOWER_BOUND = 50;
const int CANNY_UPPER_BOUND = 250;
const int HOUGH_THRESHOLD = 150;


#define true 1
#define false 0

int readingLeft = 0, readingRight = 0;

int IMAGE_WIDTH = 0;
int IMAGE_HEIGHT = 0;
const double DIVIDE = 9 / 4;

void drawDetectLines(Mat &image, const vector<Vec4i> &lines, Scalar &color) {
    // 将检测到的直线在图上画出来
    vector<Vec4i>::const_iterator it = lines.begin();
    while (it != lines.end()) {
        Point pt1((*it)[0], (*it)[1]);
        Point pt2((*it)[2], (*it)[3]);
        line(image, pt1, pt2, color, 3); //  线条宽度设置为2
        ++it;
    }
}

void drawDetectLines2(Mat &image, const vector<Vec4i> &lines, Scalar &color) {
    for (int i = 0; i < 2; i++) {
        Point pt1(lines[i][0], lines[i][1]);
        Point pt2(lines[i][2], lines[i][3]);
        line(image, pt1, pt2, color, 3);
    }
}

int detectLineNumberGOne(const vector<Vec4i>& lines){
    if (lines.size() <= 1){
        return false;
    }
    
    double slope = (lines[0][3] - lines[0][1]) * 1.0 / (lines[0][2] - lines[0][0]);
    int flag1 = slope > 0 ? 1 : -1;

    for (int i = 1; i < lines.size(); i++){
        slope = (lines[i][3] - lines[i][1]) * 1.0 / (lines[i][2] - lines[i][0]);
        int flag2 = slope > 0 ? 1: -1;

        if (flag1 != flag2){
            return true;
        }
    }
    
    return false;
}


vector<double> getCrossPoint(const vector<Vec4i> &lines) {
    Point p1(lines[0][0], lines[0][1]);
    Point p2(lines[0][2], lines[0][3]);
    Point p3(lines[1][0], lines[1][1]);
    Point p4(lines[1][2], lines[1][3]);

    vector<double> result;

    cout << p1.x << p1.y << p2.x << p2.y;
    double k1 = (p1.y - p2.y) * 1.0 / (p1.x - p2.x);
    double k3 = (p3.y - p4.y) * 1.0 / (p3.x - p4.x);

    double x0 = (p1.y - p3.y - k1 * p1.x + k3 * p3.x) / (k3 - k1);
    double y0 = k1 * (x0 - p1.x) + p1.y;
    double y1 = k3 * (x0 - p3.x) + p3.y;

    cout << "x0:" << x0 << "y1:" << y1 << endl;
    result.push_back(x0 - 320);
    result.push_back(480 - y0);

    return result;
}


vector<Vec4i> caluculateMaxTwoLines(const vector<Vec4i> &lines) {
    double max1, max2 = 0;
    int dot1x, dot1y, dot2x, dot2y;
    vector<Vec4i> result;
    double slope1, slope2;
    int flag1, flag2;

    for (vector<Vec4i>::const_iterator it = lines.begin(); it != lines.end(); it++) {
        double edge1 = pow(((*it)[0] - (*it)[2]), 2);
        double edge2 = pow(((*it)[1] - (*it)[3]), 2);
        double length = sqrt(edge1 + edge2);

        if (max1 < length) {
            dot1x = (*it)[0];
            dot1y = (*it)[1];
            dot2x = (*it)[2];
            dot2y = (*it)[3];
            max1 = length;
        }
    }

    result.push_back(Vec4i(dot1x, dot1y + IMAGE_HEIGHT / DIVIDE, dot2x, dot2y + IMAGE_HEIGHT / DIVIDE));

    slope1 = (dot1x - dot2x) * 1.0 / (dot1y - dot2y);
    flag1 = slope1 > 0 ? 1 : -1;

    for (vector<Vec4i>::const_iterator it = lines.begin(); it != lines.end(); it++) {
        double edge1 = pow(((*it)[0] - (*it)[2]), 2);
        double edge2 = pow(((*it)[1] - (*it)[3]), 2);
        double length = sqrt(edge1 + edge2);

        slope2 = ((*it)[0] - (*it)[2]) * 1.0 / ((*it)[1] - (*it)[3]);
        flag2 = slope2 > 0 ? 1 : -1;

        //cout << endl<<"max1" << max1 << "max2"  << max2 << "length"<<length<< endl << "flag1" << flag1 << "flag2"<<flag2 << "slope2" << slope2 <<endl;

        if (max2 < length && length < max1 && flag1 != flag2) {
            dot1x = (*it)[0];
            dot1y = (*it)[1];
            dot2x = (*it)[2];
            dot2y = (*it)[3];
            max2 = length;
        }
    }

    result.push_back(Vec4i(dot1x, dot1y + IMAGE_HEIGHT / DIVIDE, dot2x, dot2y + IMAGE_HEIGHT / DIVIDE));

    // cout << "Fucking length" <<result.size();
    // for (vector<Vec4i>::iterator it = result.begin(); it != result.end(); it++)
    //     cout << (*it);
    // cout << "Fucking length" <<result.size();
    return result;
}

int main() {
    // init the car
    init();
    controlLeft(FORWARD, 10);
    controlRight(FORWARD, 10);

    // distance is in cm
    double totalLeft = 0;
    double totalRight = 0;

    //	VideoCapture capture(CAM_PATH);
    VideoCapture capture(0);
    //If this fails, try to open as a video camera, through the use of an integer param
    if (!capture.isOpened()) {
        cout << "-1";
        capture.open(atoi(CAM_PATH.c_str()));
    }

    double dWidth = capture.get(CV_CAP_PROP_FRAME_WIDTH);            //the width of frames of the video
    double dHeight = capture.get(CV_CAP_PROP_FRAME_HEIGHT);        //the height of frames of the video
    clog << "Frame Size: " << dWidth << "x" << dHeight << endl;

    Mat image;
    while (totalLeft < 100){
        // run control
        capture >> image;
        if (image.empty())
            break;

        resetCounter();
        delay(1000);
        getCounter(&readingLeft, &readingRight);
        if (readingLeft == -1 || readingRight == -1)
        {
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

        // image resolve
        GaussianBlur(image, image, Size(5, 5), 0, 0);
        Mat element = getStructuringElement(MORPH_ELLIPSE, Size(4, 5));
        dilate(image, image, element);
        erode(image, image, element);
        // imshow("Blur", image);
        // imshow("erode", image);

        Rect roi(0, image.rows / DIVIDE, image.cols, image.rows / 3);
        Mat imgROI = image(roi);

        // Mat grayImage;
        // cvtColor(imgROI, grayImage, CV_BGR2GRAY);
        Mat contours;
        Canny(imgROI, contours, 80,
              250);   // void cvCanny(const CvArr* image, CvArr* edges, double threshold1, double threshold2, int aperture_size=3)
        // imshow("Canny", contours);
        threshold(contours, contours, 100, 255, THRESH_BINARY);

        // imshow("imgR", contours);

        // 检测直线，最小投票为90，线条不短于50，间隙不小于10
        vector<Vec4i> lines, result;
        HoughLinesP(contours, lines, 1, CV_PI / 180, 80, 30, 10);
        Scalar sc(0, 255, 0);
        Mat canvas(image.size(), CV_8UC3, Scalar(255));


        int twoLineChecker = detectLineNumberGOne(lines);
        if (twoLineChecker == false) {
			if (lines.size() > 0){
				Point p0(lines[0][0], lines[0][1]);
				Point p1(lines[0][2], lines[0][3]);
				double k0 = (p0.y - p1.y) * 1.0 / (p0.x - p1.x);

				// calculate the angle to turn
				int angle = (int)(atan(k0) / M_PI * 180.0);
				turnTo(angle);
			}
            drawDetectLines2(canvas, result, sc);
            imshow("canvas", canvas);
        } else {
            result = caluculateMaxTwoLines(lines);
            vector<double> pointX = getCrossPoint(result);

            double angle = atan(pointX[0]*1.0/pointX[1]) / M_PI * 180.0;
            turnTo((int)(-1.0 * angle));

            cout << "begin" << endl;
            for (vector<Vec4i>::iterator it = lines.begin(); it != lines.end(); it++)
                cout << (*it);
            cout << "end" << endl;

            drawDetectLines(canvas, lines, sc);
            imshow("canvas", canvas);
        }


        drawDetectLines(imgROI, lines, sc);
        imshow("image", image);

        lines.clear();
        result.clear();

        waitKey(5);

    }

    // stop
    stopLeft();
    stopRight();

    return 0;
}

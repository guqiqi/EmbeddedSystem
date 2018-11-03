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

// speed of the car
const int speed = 6;
// default angle with the wheel
const int wheel = 2;
// default solpe of the left line in camera
const double defaultSlope = 0.9;
// params of PID
const double kp = 1.2;
const double ki = 0.002;
const double kd = 0.003;

const double fault = 3.6;

double oldAngle = 0.0;

int IMAGE_WIDTH = 0;
int IMAGE_HEIGHT = 0;
const double DIVIDE = 9 / 4;
int getAngleByPoint(double x, double y)
{
    double angle = atan(x / y) / PI * 180.0;
    int turnAngle = kp * angle + ki * (angle + oldAngle) * speed + kd * (angle - oldAngle);
    
    cout << "angle" << turnAngle << endl;    

    if(turnAngle > 44)
	turnAngle = 44;
    if(turnAngle < -44)
	turnAngle = -44;
    
    oldAngle = angle;

    return turnAngle;
}

int getAngleBySlope(double slope)
{
    cout << "slope: " << slope << endl;
    double angle = 0.0;
    if (slope > 0) // <default
    {
        angle = (atan(slope) / M_PI * 180.0) - (atan(defaultSlope) / M_PI * 180.0);
        angle = -1.0 * angle;
    }
    else if (slope < 0) // >default
    {
        angle = (atan(-1.0 * defaultSlope) / M_PI * 180.0) -(atan(slope) / M_PI * 180.0);
    }
    else
    {
        angle = oldAngle;
    }

    int turnAngle = kp * angle + ki * (angle + oldAngle) * speed + kd * (angle - oldAngle);
    //turnAngle = kp * angle;
    cout << endl;
    oldAngle = angle;

    cout << "angle" << turnAngle << endl;

    if(turnAngle > 44)
	turnAngle = 44;
    if(turnAngle < -44)
	turnAngle = -44;
    return turnAngle;
}

void drawDetectLines(Mat &image, const vector<Vec4i> &lines, Scalar &color) {
    // 将检测到的直线在图上画出来
    vector<Vec4i>::const_iterator it = lines.begin();
    while (it != lines.end()) {
        Point pt1((*it)[0], (*it)[1]+IMAGE_HEIGHT/DIVIDE);
        Point pt2((*it)[2], (*it)[3]+IMAGE_HEIGHT/DIVIDE);
        line(image, pt1, pt2, color, 3); //  线条宽度设置为2
        ++it;
    }
}

void drawDetectLines2(Mat &image, const vector<Vec4i> &lines, Scalar &color, int index) {
    for (int i = 0; i < index; i++) {
        Point pt1(lines[i][0], lines[i][1]+IMAGE_HEIGHT/DIVIDE);
        Point pt2(lines[i][2], lines[i][3]+IMAGE_HEIGHT/DIVIDE);
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

    slope1 = (dot1y - dot2y) * 1.0 / (dot1x - dot2x);
    flag1 = slope1 > 0 ? 1 : -1;

    for (vector<Vec4i>::const_iterator it = lines.begin(); it != lines.end(); it++) {
        double edge1 = pow(((*it)[0] - (*it)[2]), 2);
        double edge2 = pow(((*it)[1] - (*it)[3]), 2);
        double length = sqrt(edge1 + edge2);

        slope2 = ((*it)[1] - (*it)[3]) * 1.0 / ((*it)[0] - (*it)[2]);
        flag2 = slope2 > 0 ? 1 : -1;

        //cout << endl<<"max1" << max1 << "max2"  << max2 << "length"<<length<< endl << "flag1" << flag1 << "flag2"<<flag2 << "slope2" << slope2 <<endl;

        if (max2 < length && length < max1 && flag1 != flag2 && abs(slope2) > 0.2) {
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

    int counter = 0;
    Mat image;
    while (true){
        // run control
        capture >> image;
	IMAGE_HEIGHT = image.rows;
        if (image.empty())
            break;

        resetCounter();
        delay(10);

        // image resolve
        // GaussianBlur(image, image, Size(5, 5), 0, 0);

        Rect roi(0, image.rows / DIVIDE, image.cols, image.rows / 3);
        Mat imgROI = image(roi);

        // Mat grayImage;
        // cvtColor(imgROI, grayImage, CV_BGR2GRAY);
        Mat contours;
        inRange(imgROI, Scalar(20,20,30), Scalar(110,110,110), imgROI);
        Mat element = getStructuringElement(MORPH_ELLIPSE, Size(4, 5));
        dilate(imgROI, imgROI, element);
        erode(imgROI, imgROI, element);

        Canny(imgROI, contours, 60, 250);
        threshold(contours, contours, 100, 255, THRESH_BINARY);

        // imshow("imgR", contours);

        // 检测直线，最小投票为90，线条不短于50，间隙不小于10
        vector<Vec4i> lines, result;
        HoughLinesP(contours, lines, 1, CV_PI / 180, 80, 30, 10);
        Scalar sc(0, 255, 0);
        Mat canvas(image.size(), CV_8UC3, Scalar(255));

        int twoLineChecker = detectLineNumberGOne(lines);

        int angle = 0;
        if (twoLineChecker == false) {
		cout << endl << "linesNum" << lines.size() << endl;
			if (lines.size() > 0){
				Point p0(lines[0][0], lines[0][1]);
				Point p1(lines[0][2], lines[0][3]);
				double k0 = (p1.y - p0.y) * 1.0 / (p0.x - p1.x);
				
				
				cout << endl << "P0:" << p0 << "P1:" << p1 << "k0:" << k0<< endl;
                double temp1 = oldAngle;
				// calculate the angle to turn
                angle = getAngleBySlope(k0);
				turnTo(angle-temp1+fault);
                //cout << "turn angle: " << (angle - temp1) << endl;

                drawDetectLines2(canvas, lines, sc, 1);
            }
            
            imshow("canvas", canvas);
        } else {
            result = caluculateMaxTwoLines(lines);
            vector<double> pointX = getCrossPoint(result);

            double angle = atan(pointX[0]*1.0/pointX[1]) / M_PI * 180.0;

            double temp2 = oldAngle;
            // calculate the angle to turn
            angle = getAngleByPoint(pointX[0], pointX[1]);
             turnTo(angle - temp2 + fault);

            cout << "begin" << endl;
            for (vector<Vec4i>::iterator it = lines.begin(); it != lines.end(); it++)
                cout << (*it);
            cout << "end" << endl;

            drawDetectLines2(canvas, result, sc, 2);
            imshow("canvas", canvas);
        }


        drawDetectLines(image, lines, sc);
        imshow("image", image);

        lines.clear();
        result.clear();

        waitKey(1);
        cout << ++counter;
    }

    return 0;
}

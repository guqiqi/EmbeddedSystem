extern void func0();
extern void func1();
extern void func2();
extern void funcE();
extern void func3();
extern void func4();
extern void func5();

#include<opencv2/opencv.hpp>
#include<vector>
using namespace cv;
using namespace std;

#define true 1
#define false 0

int IMAGE_WIDTH = 0;
int IMAGE_HEIGHT = 0;
const double DIVIDE = 9/4;

void drawDetectLines(Mat& image, const vector<Vec4i>& lines, Scalar& color)
{
    // 将检测到的直线在图上画出来
    vector<Vec4i>::const_iterator it=lines.begin();
    while(it!=lines.end())
    {
        Point pt1((*it)[0],(*it)[1]);
        Point pt2((*it)[2],(*it)[3]);
        line(image, pt1, pt2,color, 3); //  线条宽度设置为2
        ++it;
    }
}

void drawDetectLines2(Mat& image, const vector<Vec4i>& lines, Scalar& color)
{
    for (int i = 0; i < 2; i++){
        Point pt1(lines[i][0],lines[i][1]);
        Point pt2(lines[i][2],lines[i][3]);
        line(image, pt1, pt2,color, 3); 
    }
}

int detectLineNumberGOne(const vector<Vec4i>& lines){
    if (lines.size() <= 1){
        return 0;
    }
    else return 1;
}


vector<double> getCrossPoint(const vector<Vec4i>& lines){
    Point p1(lines[0][0], lines[0][1]);
    Point p2(lines[0][2], lines[0][3]);
    Point p3(lines[1][0], lines[1][1]);
    Point p4(lines[1][2], lines[1][3]);

    vector<double> result;

    cout << p1.x << p1.y << p2.x<< p2.y;
    double k1 = (p1.y - p2.y) * 1.0/(p1.x - p2.x);
    double k3 = (p3.y - p4.y) * 1.0/(p3.x - p4.x);

    double x0 = (p1.y - p3.y - k1*p1.x + k3*p3.x) / (k3-k1);
    double y0 = k1*(x0-p1.x)+p1.y;
    double y1 = k3*(x0-p3.x)+p3.y;

    cout << "x0:" << x0 << "y1:" << y1 << endl;
    result.push_back(x0-320);
    result.push_back(y0);

    return result;
}



vector<Vec4i> caluculateMaxTwoLines(const vector<Vec4i>& lines){
    double max1, max2 = 0;
    int dot1x, dot1y, dot2x, dot2y;
    vector<Vec4i> result;
    double slope1, slope2;
    int flag1, flag2;

    for (vector<Vec4i>::const_iterator it = lines.begin(); it != lines.end(); it++){
        double edge1 = pow(((*it)[0] - (*it)[2]), 2);
        double edge2 = pow(((*it)[1] - (*it)[3]), 2);
        double length = sqrt(edge1 + edge2);
        
        if(max1 < length){
            dot1x = (*it)[0]; dot1y = (*it)[1]; dot2x = (*it)[2]; dot2y = (*it)[3];
            max1 = length;
        }
    }

    result.push_back(Vec4i(dot1x, dot1y+IMAGE_HEIGHT/DIVIDE, dot2x, dot2y+IMAGE_HEIGHT/DIVIDE));
    
    slope1 = (dot1x-dot2x) * 1.0/(dot1y-dot2y);
    flag1 = slope1 > 0 ? 1 : -1;

    for (vector<Vec4i>::const_iterator it = lines.begin(); it != lines.end(); it++){
        double edge1 = pow(((*it)[0] - (*it)[2]), 2);
        double edge2 = pow(((*it)[1] - (*it)[3]), 2);
        double length = sqrt(edge1 + edge2);
        
        slope2 = ((*it)[0]-(*it)[2]) *1.0/ ((*it)[1]-(*it)[3]);
        flag2 = slope2 > 0 ? 1 : -1;

        //cout << endl<<"max1" << max1 << "max2"  << max2 << "length"<<length<< endl << "flag1" << flag1 << "flag2"<<flag2 << "slope2" << slope2 <<endl;

        if(max2 < length && length < max1 && flag1 != flag2){
            dot1x = (*it)[0]; dot1y = (*it)[1]; dot2x = (*it)[2]; dot2y = (*it)[3];
            max2 = length;
        }
    }

    result.push_back(Vec4i(dot1x, dot1y+IMAGE_HEIGHT/DIVIDE, dot2x, dot2y+IMAGE_HEIGHT/DIVIDE));

    // cout << "Fucking length" <<result.size();
    // for (vector<Vec4i>::iterator it = result.begin(); it != result.end(); it++)
    //     cout << (*it);
    // cout << "Fucking length" <<result.size();
    return result;
}

int main(){
    // cols = 640, rows = 480
    Mat image = imread("image/b.png");
    cout << image.rows;
    IMAGE_HEIGHT = image.rows;
    IMAGE_WIDTH = image.cols;
    //Mat image, image2;
    
    Mat imageSingle;
    // cvtColor(image, image, CV_RGB2GRAY);
    // inRange(image, Scalar(45, 55, 45), Scalar(150, 150, 150), image);
    GaussianBlur(image, image, Size(5, 5), 0, 0);
    Mat element = getStructuringElement(MORPH_ELLIPSE, Size(4, 5));
    dilate(image, image, element);
    erode(image, image, element);
    // imshow("Blur", image);
    // imshow("erode", image);

    Rect roi(0, image.rows/DIVIDE, image.cols, image.rows/3);
	Mat imgROI = image(roi);
    
    // Mat grayImage;
    // cvtColor(imgROI, grayImage, CV_BGR2GRAY);     
    Mat contours;
    Canny(imgROI, contours, 80, 250);   // void cvCanny(const CvArr* image, CvArr* edges, double threshold1, double threshold2, int aperture_size=3)
    // imshow("Canny", contours);
    threshold(contours, contours, 100, 255, THRESH_BINARY);

    // imshow("imgR", contours);

    // 检测直线，最小投票为90，线条不短于50，间隙不小于10
    vector<Vec4i> lines, result; 
    HoughLinesP(contours,lines,1,CV_PI/180,80,30,10); 
    Scalar sc(0, 255, 0);
    Mat canvas(image.size(), CV_8UC3, Scalar(255));
    

    int twoLineChecker = detectLineNumberGOne(lines);
    if (twoLineChecker == false){
        Point p0(lines[0][0], lines[0][1]);
        Point p1(lines[0][2], lines[0][3]);
        double k0 = (p0.y-p1.y) *1.0 / (p0.x-p1.x);

        drawDetectLines2(canvas, result, sc);
        imshow("canvas",canvas);
    }
    else{
        result = caluculateMaxTwoLines(lines);
        vector<double> pointX = getCrossPoint(result);
        cout << "begin" << endl;
        for (vector<Vec4i>::iterator it = lines.begin(); it != lines.end(); it++)
            cout << (*it);
        cout << "end" << endl;

        drawDetectLines(canvas, lines, sc);
        imshow("canvas",canvas);
    }

    
    drawDetectLines(imgROI, lines, sc);     
    imshow("image",image);
    
    lines.clear();
    result.clear();
    waitKey(5);
}
   

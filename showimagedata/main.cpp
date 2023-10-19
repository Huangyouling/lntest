#include <opencv2/opencv.hpp>
#include <iostream>
#include <ctime>

using namespace cv;
using namespace std;


void addTextToImage(Mat& image, const string& text, const Point& position, const Scalar& textColor, const Scalar& backgroundColor)
{
    int fontFace = FONT_HERSHEY_SIMPLEX;
    double fontScale = 1.0;
    int thickness = 2;
    int baseline = 0;

    Size textSize = getTextSize(text, fontFace, fontScale, thickness, &baseline);
    Point textOrg(position.x, position.y + textSize.height);

    putText(image, text, textOrg, fontFace, fontScale, textColor, thickness, LINE_AA);
}

int main()
{
    Mat image = imread("/home/hyl/datav/test/showimagedata/dog.png");

    if (image.empty())
    {
        cout << "Failed to read the image." << endl;
        return -1;
    }

    time_t now = time(0);
    tm* currentTime = localtime(&now);

    char dateTimeStr[100];
    strftime(dateTimeStr, sizeof(dateTimeStr), "%m/%d/%Y %A %H:%M:%S", currentTime);
    string deviceInfo = "Device Name: Your Device";

    Point dateTimePosition(10, 30);
    Point deviceInfoPosition(image.cols - deviceInfo.length() * 10 - 10, image.rows - 10);
    Scalar textColor(0, 0, 0);
    Scalar backgroundColor(255, 255, 255);

    addTextToImage(image, dateTimeStr, dateTimePosition, textColor, backgroundColor);
    addTextToImage(image, deviceInfo, deviceInfoPosition, textColor, backgroundColor);

    imwrite("output_image.jpg", image);

    cout << "Image with text saved as output_image.jpg." << endl;

    return 0;
}



#include "MouseEvents.h"

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>

#include <iostream>
#include <string>

int main()
{
    auto inFilename = 0;

    mouseevents::CMouseEvents MEvents("Draw", "C:/Users/ahkad/Desktop/Config.xml", "C:/Users/ahkad/Desktop/Config.jpg", false);

    cv::VideoCapture inVid;
    inVid.open(inFilename);
    if (!inVid.isOpened())
    {
        std::cout<<"Video capture could not be initialized for file: "<<inFilename<<std::endl;
        return -1;
    }

    cv::Mat_<cv::Vec3b> Frame;
    inVid >> Frame;

    while(1)
    {
        MEvents.Show(Frame);
        inVid >> Frame;
        if(Frame.empty())
        {
            inVid.set(cv::CAP_PROP_POS_MSEC, 1);
            inVid >> Frame;
        }
    }

    return 0;
}

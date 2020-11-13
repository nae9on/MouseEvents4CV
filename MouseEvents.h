#pragma once

#include <opencv2/highgui.hpp>
#include "opencv2/imgproc.hpp"

#include "TinyXml/tinyxml.h"

#include <iostream>
#include <vector>
#include <map>
#include <fstream>

namespace MouseEvents
{

std::ostream& operator<<(std::ostream& OS, const cv::Point& P);

bool operator==(const cv::Point& P1, const cv::Point& P2);

bool operator!=(const cv::Point& P1, const cv::Point& P2);

void MyFilledCircle(cv::Mat& Img, const cv::Point& Center, cv::Scalar Color = cv::Scalar(255, 255, 255));

void MyLine(cv::Mat& Img, const cv::Point& Start, const cv::Point& End, cv::Scalar Color = cv::Scalar(0, 255, 0));

template<typename T>
void DrawText(cv::Mat& Img, const T& Data, const cv::Point& Location, cv::Scalar Color = cv::Scalar(0, 0, 0));

class CMouseEvents
{
public:
    using VectorOfLinesType = std::vector<std::pair<cv::Point, cv::Point>>;

    CMouseEvents() = default;

    CMouseEvents(const std::string& WinName, const std::string& ConfigPath, const std::string& SnapPath)
        : m_WinName{WinName}
        , m_WinNameZoom{m_WinName+"Zoom"}
        , m_ConfigPath{ConfigPath}
        , m_SnapPath{SnapPath}
    {
        cv::namedWindow(m_WinName, cv::WINDOW_AUTOSIZE);
        cv::setMouseCallback(m_WinName, OnMouse);
        if(m_DrawROI)
        {
            cv::namedWindow(m_WinNameZoom, cv::WINDOW_AUTOSIZE);
        }
        //m_Ofs = std::ofstream(m_ConfigPath, std::ofstream::out | std::ofstream::trunc);
    }

    // Show the current frame
    void Show(const cv::Mat& Frame)
    {
        ++m_FrameNum;
        cv::resize(Frame, ScaledImage, cv::Size(Frame.cols*m_Scale, Frame.rows*m_Scale));
        m_CurrentFramePtr = &ScaledImage;
        AddLines();
        Draw();
        if(m_DrawROI)
        {
            DrawROI();
        }
        cv::imshow(m_WinName, *m_CurrentFramePtr);
        cv::waitKey(m_Delay);
    }

private:
    // Add lines to the vector of lines
    void AddLines();

    // Draw lines on the current frame
    void Draw();

    // Zoom the image around the points in another window
    void DrawROI();

    // Mouse events related (static members used in the Callback function for mouse events)
    static void OnMouse(int Event, int X, int Y, int Flag, void* Param);

    // Write configuration file
    template<typename T>
    void WriteConfigXML(T& Ofs, const VectorOfLinesType& CurrentLines);

    // Write configuration file in a nice format (same as Notepad++->Plugins->XML Tools->Pretty print)
    void PrettyPrint(const std::string&);

    static cv::Point m_P1, m_P2, m_ScaledP1, m_ScaledP2;
    static bool m_LeftClicked;
    static bool m_RightClicked;
    static bool m_LeftDoubleClicked;
    static cv::MouseEventFlags m_Flag;
    bool m_LastLeftClicked{false};
    bool m_LastRightClicked{false};

    // Display related
    static const int m_Scale{1};
    const std::string m_WinName{};
    const std::string m_WinNameZoom{};
    const std::string m_ConfigPath{};
    const std::string m_SnapPath{};
    cv::Mat ScaledImage;
    cv::Mat* m_CurrentFramePtr;
    int m_FrameNum{-1}; // Frame counter
    int m_Delay{50}; // Corresponds to 30 FPS
    const bool m_DrawROI{false};

    // Domain lines related
    int m_DomainId{1};
    VectorOfLinesType m_CurrentLines;
    std::map<int, VectorOfLinesType> m_AllLines;
    std::ofstream m_Ofs;
    TiXmlDocument m_Doc{};
};

}

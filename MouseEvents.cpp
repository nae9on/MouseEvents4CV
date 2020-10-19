#include "MouseEvents.h"

#include <iostream>
#include <stdlib.h>

namespace MouseEvents
{

// Pixels within range [0 10] are considered identical
constexpr int Int_Pixel_Precision{10};

cv::Point CMouseEvents::m_P1{};
cv::Point CMouseEvents::m_P2{};
bool CMouseEvents::m_LeftClicked = false;
bool CMouseEvents::m_RightClicked = false;
bool CMouseEvents::m_LeftDoubleClicked = false;
cv::MouseEventFlags CMouseEvents::m_Flag = cv::MouseEventFlags::EVENT_FLAG_LBUTTON;

std::ostream& operator<<(std::ostream& OS, const cv::Point& Pixel)
{
    return OS << Pixel.x << " " << Pixel.y;
}

// Compare in integer arithmetic!
bool operator==(const cv::Point& P1, const cv::Point& P2)
{
    return (abs(P1.x-P2.x)<Int_Pixel_Precision &&
            abs(P1.y-P2.y)<Int_Pixel_Precision);
}

bool operator!=(const cv::Point& P1, const cv::Point& P2)
{
    return !(P1==P2);
}

void MyFilledCircle(cv::Mat& Img, const cv::Point& Center, cv::Scalar Color)
{
    int Radius{4};

    cv::circle(Img,
               Center,
               Radius,
               Color,
               cv::FILLED,
               cv::LINE_8);
}

void MyLine(cv::Mat& Img, const cv::Point& Start, const cv::Point& End, cv::Scalar Color)
{
    int Thickness = 2;
    int LineType = cv::LINE_8;

    cv::line(Img,
             Start,
             End,
             Color,
             Thickness,
             LineType);
}

void DrawText(cv::Mat& Img, const cv::Point& Pixel, cv::Scalar Color)
{
    std::stringstream ss;
    ss << Pixel;
    cv::putText(Img, ss.str().c_str(), Pixel, cv::FONT_HERSHEY_SIMPLEX, 0.5, Color);
}

void CMouseEvents::AddLines()
{
    // Left click drag and drop to add lines to the current zone
    if(!m_LeftClicked && m_LastLeftClicked)
    {
        // Add only those lines whose start and end points are not close enough
        if(m_P1!=m_P2)
        {
            if(m_CurrentLines.size()>0)
            {
                // Close the loop if the end point of previous line and the start point of new line are close enough
                if(m_P1==m_CurrentLines.back().second)
                {
                    m_CurrentLines.emplace_back(m_CurrentLines.back().second, m_P2);
                }
                else
                {
                    m_CurrentLines.emplace_back(m_P1, m_P2);
                }
            }
            else
            {
                m_CurrentLines.emplace_back(m_P1, m_P2);
            }

        }
    }
    m_LastLeftClicked = m_LeftClicked;

    // Right click to end adding lines to the current zone
    if(!m_RightClicked && m_LastRightClicked)
    {
        // Close the loop if the end point of last line and the first point of first line are close enough
        if(m_CurrentLines.size()>0)
        {
            if(m_CurrentLines[0].first==m_CurrentLines.back().second)
            {
                m_CurrentLines.back().second = m_CurrentLines[0].first;
            }
        }

        // Print all lines in the current zone
        WriteConfigXML(std::cout, m_ZoneId, m_CurrentLines);

        // Add lines in the current zone to all lines
        m_AllLines.emplace(m_ZoneId++, m_CurrentLines);

        // Clear current lines
        m_CurrentLines.clear();
    }
    m_LastRightClicked = m_RightClicked;

    // Left double click to write all zones to the configuration file
    if(m_LeftDoubleClicked)
    {
        for(const auto& KeyValue : m_AllLines)
        {
            WriteConfigXML(m_Ofs, KeyValue.first, KeyValue.second);
        }
    }
    m_LeftDoubleClicked = false;
}

void CMouseEvents::Draw()
{
    if(m_LeftClicked)
    {
        MyFilledCircle(*m_CurrentFramePtr, m_P1);
        MyFilledCircle(*m_CurrentFramePtr, m_P2);
        MyLine(*m_CurrentFramePtr, m_P1, m_P2);
        DrawText(*m_CurrentFramePtr, m_P1);
        DrawText(*m_CurrentFramePtr, m_P2);
    }

    for(const auto& Line : m_CurrentLines)
    {
        MyLine(*m_CurrentFramePtr, Line.first, Line.second);
        DrawText(*m_CurrentFramePtr, Line.first);
        DrawText(*m_CurrentFramePtr, Line.second);
    }

    for(const auto& KeyValue : m_AllLines)
    {
        for(const auto& Line : KeyValue.second)
        {
            MyLine(*m_CurrentFramePtr, Line.first, Line.second, cv::Scalar(255, 0, 0));
        }
    }
}

void CMouseEvents::DrawROI()
{
    int Radius = 100;
    int Scale = 4;
    cv::Mat& Src = *m_CurrentFramePtr;
    cv::Rect ROI1(m_P1.x-Radius, m_P1.y-Radius, 2*Radius, 2*Radius);
    cv::Rect ROI2(m_P2.x-Radius, m_P2.y-Radius, 2*Radius, 2*Radius);
    cv::Rect ROI = ((ROI1 | ROI2) & cv::Rect(0, 0, Src.cols, Src.rows));
    const cv::Mat& CroppedImage = Src(ROI);
    cv::Mat ScaledImage;
    cv::resize(CroppedImage, ScaledImage, cv::Size(CroppedImage.cols*Scale, CroppedImage.rows*Scale));
    cv::imshow(m_WinNameZoom, ScaledImage);
}

void CMouseEvents::OnMouse(int Event, int X, int Y, int Flag, void* /*Param*/)
{
    m_Flag = static_cast<cv::MouseEventFlags>(Flag);

    switch(Event){

    case  cv::EVENT_LBUTTONDOWN:
        m_LeftClicked = true;
        m_P1.x=X;
        m_P1.y=Y;
        m_P2.x=X;
        m_P2.y=Y;
        break;

    case cv::EVENT_RBUTTONDOWN:
        m_RightClicked = true;
        break;

    case  cv::EVENT_LBUTTONUP:
        m_LeftClicked = false;
        m_P2.x=X;
        m_P2.y=Y;
        break;

    case cv::EVENT_RBUTTONUP:
        m_RightClicked  = false;
        break;

    case cv::EVENT_LBUTTONDBLCLK:
        m_LeftDoubleClicked = true;
        break;

    case cv::EVENT_MOUSEMOVE:
        if(m_LeftClicked)
        {
            m_P2.x=X;
            m_P2.y=Y;
        }
        break;

    default:
        break;
    }
}

template<typename T>
void CMouseEvents::WriteConfigXML(T& Ofs, int ZoneId, const VectorOfLinesType& CurrentLines)
{
    for(const auto& Line : CurrentLines)
    {
        Ofs << Line.first << " " << Line.second << std::endl;
    }
}

}

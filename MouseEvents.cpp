#include "MouseEvents.h"

#include <iostream>
#include <stdlib.h>
#include <stdio.h> // for FILE*

namespace MouseEvents
{

const std::string Pre{"<Data>"};
const std::string Post{"</Data>"};

// Pixels within range [0 10] are considered identical
constexpr int Int_Pixel_Precision{10};

cv::Point CMouseEvents::m_P1{};
cv::Point CMouseEvents::m_P2{};
cv::Point CMouseEvents::m_ScaledP1{};
cv::Point CMouseEvents::m_ScaledP2{};
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

template<typename T>
void DrawText(cv::Mat& Img, const T& Data, const cv::Point& Location, cv::Scalar Color)
{
    std::stringstream ss;
    ss << Data;
    cv::putText(Img, ss.str().c_str(), Location, cv::FONT_HERSHEY_SIMPLEX, 0.5, Color);
}

void CMouseEvents::AddLines()
{
    // Left click drag and drop to add lines to the current Domain
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

    // Right click to end adding lines to the current Domain
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

        // Print all lines in the current Domain
        WriteConfigXML(std::cout, m_CurrentLines);

        // Add lines in the current Domain to all lines
        m_AllLines.emplace(m_DomainId++, m_CurrentLines);

        // Clear current lines
        m_CurrentLines.clear();
    }
    m_LastRightClicked = m_RightClicked;

    // Left double click to write all Domains to the configuration file
    if(m_LeftDoubleClicked)
    {
        m_Ofs = std::ofstream(m_ConfigPath, std::ofstream::out | std::ofstream::trunc);

        m_Ofs << Pre << std::endl;
        m_Ofs << "<Domains>" << std::endl;
        for(const auto& KeyValue : m_AllLines)
        {
            WriteConfigXML(m_Ofs, KeyValue.second);
        }
        m_Ofs << "</Domains>" << std::endl;
        m_Ofs << Post << std::endl;
        m_Ofs << std::flush;

        PrettyPrint(m_ConfigPath);
    }
}

void CMouseEvents::Draw()
{
    if(m_LeftClicked)
    {
        MyFilledCircle(*m_CurrentFramePtr, m_ScaledP1);
        MyFilledCircle(*m_CurrentFramePtr, m_ScaledP2);
        MyLine(*m_CurrentFramePtr, m_ScaledP1, m_ScaledP2);
        DrawText(*m_CurrentFramePtr, m_P1, m_ScaledP1);
        DrawText(*m_CurrentFramePtr, m_P2, m_ScaledP2);
    }

    for(const auto& Line : m_CurrentLines)
    {
        MyLine(*m_CurrentFramePtr, Line.first*m_Scale, Line.second*m_Scale);
        DrawText(*m_CurrentFramePtr, Line.first, Line.first*m_Scale);
        DrawText(*m_CurrentFramePtr, Line.second, Line.second*m_Scale);
    }

    for(const auto& KeyValue : m_AllLines)
    {
        for(const auto& Line : KeyValue.second)
        {
            MyLine(*m_CurrentFramePtr, Line.first*m_Scale, Line.second*m_Scale, cv::Scalar(255, 0, 0));
        }
    }

    if(m_LeftDoubleClicked)
    {
        cv::imwrite(m_SnapPath, *m_CurrentFramePtr); // write image

        // Reset
        m_LeftClicked = false;
        m_RightClicked = false;
        m_LeftDoubleClicked = false;
        m_LastLeftClicked = false;
        m_LastRightClicked = false;
        m_DomainId = 1;
        m_CurrentLines.clear();
        m_AllLines.clear();
        m_Ofs.close();
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
        m_P1.x=X/m_Scale;
        m_P1.y=Y/m_Scale;
        m_P2.x=X/m_Scale;
        m_P2.y=Y/m_Scale;
        m_ScaledP1.x = X;
        m_ScaledP1.y = Y;
        m_ScaledP2.x = X;
        m_ScaledP2.y = Y;
        break;

    case cv::EVENT_RBUTTONDOWN:
        m_RightClicked = true;
        break;

    case  cv::EVENT_LBUTTONUP:
        m_LeftClicked = false;
        m_P2.x=X/m_Scale;
        m_P2.y=Y/m_Scale;
        m_ScaledP2.x = X;
        m_ScaledP2.y = Y;
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
            m_P2.x=X/m_Scale;
            m_P2.y=Y/m_Scale;
            m_ScaledP2.x = X;
            m_ScaledP2.y = Y;
        }
        break;

    default:
        break;
    }
}

template<typename T>
void CMouseEvents::WriteConfigXML(T& Ofs, const VectorOfLinesType& CurrentLines)
{
    Ofs << "<Domain>" << std::endl;
    for(const auto& Line : CurrentLines)
    {
        Ofs << "\t<Point X=\"" << Line.first.x << "\" Y=\"" << Line.first.y << "\"/>" << std::endl;
    }
    Ofs << "</Domain>" << std::endl;
}

void CMouseEvents::PrettyPrint(const std::string& FileName = std::string{})
{
    m_Doc.LoadFile(FileName.c_str(), TiXmlEncoding::TIXML_ENCODING_UTF8);
    if(!FileName.empty())
    {
        FILE *Fp = fopen(FileName.c_str(), "w+");
        m_Doc.Print(Fp);
        fclose(Fp);
    }

    m_Doc.Print();
    std::cout << std::flush;
}

}

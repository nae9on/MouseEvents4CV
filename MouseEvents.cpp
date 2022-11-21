#include "MouseEvents.h"

#include <iostream>
#include <numeric>
#include <stdio.h> // for FILE*
#include <stdlib.h>

namespace mouseevents
{

namespace
{

// The absolute length should not matter for direction. Can be adjusted for better visualization.
constexpr int ArrowLength{100};

// Write configuration file
template<typename T>
void WriteConfigXML(T& Ofs, const CMouseEvents::SZone& Zone)
{
    Ofs << "<Zone ZoneId=\"" << Zone.s_ZoneId << "\" ZoneName=\"" << Zone.s_ZoneName << "\">" << std::endl;
    Ofs << "\t<Shape Type=\"POLYGON\">" << std::endl;
    for(auto It = Zone.s_Lines.cbegin(); It != Zone.s_Lines.cend(); ++It)
    {
        auto Line = *It;

        // Print the 1st point of line
        Ofs << "\t\t<Point X=\"" << Line.first.x << "\" Y=\"" << Line.first.y << "\"/>" << std::endl;

        // Print the 2nd point of line if it does not match with the first point of next line
        auto NextLine = std::next(It) != Zone.s_Lines.cend() ? *std::next(It) : Zone.s_Lines.front();
        if(Line.second != NextLine.first)
        {
            Ofs << "\t\t<Point X=\"" << Line.second.x << "\" Y=\"" << Line.second.y << "\"/>" << std::endl;
        }
    }
    Ofs << "\t</Shape>" << std::endl;
    Ofs << "\t<Characteristics/>" << std::endl;
    Ofs << "\t\t<Direction>" << std::endl;
    Ofs << "\t\t<Point X=\"" << Zone.GetCenter().x << "\" Y=\"" << Zone.GetCenter().y << "\"/>" << std::endl;
    Ofs << "\t\t<Point X=\"" << Zone.GetArrowHead().x << "\" Y=\"" << Zone.GetArrowHead().y << "\"/>" << std::endl;
    Ofs << "\t\t</Direction>" << std::endl;
    Ofs << "</Zone>" << std::endl;
}

// Write configuration file in a nice format (same as Notepad++->Plugins->XML Tools->Pretty print)
void PrettyPrint(TiXmlDocument& Doc, const std::string& FileName = std::string{})
{
    Doc.LoadFile(FileName.c_str(), TiXmlEncoding::TIXML_ENCODING_UTF8);
    if(!FileName.empty())
    {
        FILE *Fp = fopen(FileName.c_str(), "w+");
        Doc.Print(Fp);
        fclose(Fp);
    }

    Doc.Print();
}

}

const std::string PreConfigElement{""};
const std::string PostConfigElement{""};

// Pixels within range [0 10] are considered identical
constexpr int Int_Pixel_Precision{10};

CMouseEvents::PointType CMouseEvents::m_P1{};
CMouseEvents::PointType CMouseEvents::m_P2{};
CMouseEvents::PointType CMouseEvents::m_PMousePointer{};
CMouseEvents::PointType CMouseEvents::m_ScaledP1{};
CMouseEvents::PointType CMouseEvents::m_ScaledP2{};
CMouseEvents::PointType CMouseEvents::m_ScaledPMousePointer{};
int CMouseEvents::m_ClosestZoneId{-1};
int CMouseEvents::m_Rotation{0};
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

CMouseEvents::PointType CMouseEvents::SZone::GetCenter() const
{
    if(!s_Center)
    {
        std::vector<cv::Point2d> Points;
        for(const auto& Line : s_Lines)
        {
            Points.push_back(Line.first);
            Points.push_back(Line.second);
        }
        cv::Point2d Center = std::accumulate(Points.cbegin(), Points.cend(), cv::Point2d{});
        Center /= static_cast<int>(Points.size());
        s_Center = Center;
    }

    return s_Center.value();
}

CMouseEvents::PointType CMouseEvents::SZone::GetArrowHead() const
{
    if(!s_ArrowHead)
    {
        s_ArrowHead = GetCenter() + PointType(ArrowLength, 0); // Default arrow head to start with
    }
    return s_ArrowHead.value();
}

double CMouseEvents::SZone::GetDistance(PointType Point) const
{
    // Eucledian distance
    return cv::norm(GetCenter()-Point);
}

void CMouseEvents::SZone::Rotate(int Degree)
{
    s_Angle += Degree;
    s_Angle = s_Angle > 359 ? s_Angle - 360 : s_Angle;
    auto AngleInRadians = -s_Angle*CV_PI/180;
    s_ArrowHead = GetCenter() + PointType(GetDistance(GetArrowHead()) * cv::Point2d(std::cos(AngleInRadians), std::sin(AngleInRadians)));
}

CMouseEvents::CMouseEvents()
    : CMouseEvents("Zones", "/tmp/Config.xml", "/tmp/Zones.jpg", false)
{}

CMouseEvents::CMouseEvents(const std::string& WinName, const std::string& ConfigPath, const std::string& SnapPath, bool DrawROI)
    : m_WinName{WinName}
    , m_WinNameZoom{m_WinName + "Zoom"}
    , m_ConfigPath{ConfigPath}
    , m_SnapPath{SnapPath}
    , m_DrawROI{DrawROI}
{
    cv::namedWindow(m_WinName, cv::WINDOW_AUTOSIZE);
    cv::setMouseCallback(m_WinName, OnMouse);
    if(m_DrawROI)
    {
        cv::namedWindow(m_WinNameZoom, cv::WINDOW_AUTOSIZE);
    }
}

void CMouseEvents::SetConfigZones(const std::map<int, SZone>& Zones)
{
    m_Zones = Zones;
    m_ZoneId = m_Zones.rbegin()->first + 1; // New id starts from max + 1
}

void CMouseEvents::Show(const cv::Mat& Frame)
{
    cv::resize(Frame, m_CurrentScaledFrame, cv::Size(Frame.cols*m_Scale, Frame.rows*m_Scale));
    AddLines();
    Update();
    Draw();
    if(m_DrawROI)
    {
        DrawROI();
    }
    cv::imshow(m_WinName, m_CurrentScaledFrame);
    cv::waitKey(m_Delay);
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

        SZone Zone;
        Zone.s_ZoneId = m_ZoneId++;
        Zone.s_Lines = m_CurrentLines;

        // Print all lines in the current zone
        WriteConfigXML(std::cout, Zone);

        // Add lines in the current zone to all lines
        m_Zones.emplace(Zone.s_ZoneId, Zone);

        // Clear current lines
        m_CurrentLines.clear();
    }
    m_LastRightClicked = m_RightClicked;

    // Left double click to write all zones to the configuration file
    if(m_LeftDoubleClicked)
    {
        m_Ofs = std::ofstream(m_ConfigPath, std::ofstream::out | std::ofstream::trunc);
        m_Ofs << PreConfigElement << std::endl;
        m_Ofs << "<Zones>" << std::endl;
        for(const auto& [ZoneId, Zone] : m_Zones)
        {
            WriteConfigXML(m_Ofs, Zone);
        }
        m_Ofs << "</Zones>" << std::endl;
        m_Ofs << PostConfigElement << std::endl;
        m_Ofs << std::flush;

        PrettyPrint(m_Doc, m_ConfigPath);
    }
}

void CMouseEvents::Update()
{
    auto ClosestZoneId = m_Zones.empty() ? -1 : m_Zones.cbegin()->first;
    auto Distance = m_Zones.empty() ? -1 : m_Zones[ClosestZoneId].GetDistance(m_PMousePointer);
    for(const auto& [ZoneId, Zone] : m_Zones)
    {
        auto NewDistance = Zone.GetDistance(m_PMousePointer);
        if(NewDistance < Distance)
        {
            Distance = NewDistance;
            ClosestZoneId = ZoneId;
        }
    }

    if(ClosestZoneId != m_ClosestZoneId)
    {
        m_Rotation = 0;
        m_LastRotation = 0;
    }
    m_ClosestZoneId = ClosestZoneId;

    // Update Arrow head of the closest zone
    if(m_Rotation != m_LastRotation)
    {
        m_Zones[m_ClosestZoneId].Rotate(m_Rotation-m_LastRotation);
    }
    m_LastRotation = m_Rotation;
}

void CMouseEvents::Draw()
{
    // Draw mouse pointer
    MyFilledCircle(m_CurrentScaledFrame, m_ScaledPMousePointer);

    // Draw point (in green) when holding and moving mouse over the image using left click
    if(m_LeftClicked)
    {
        MyFilledCircle(m_CurrentScaledFrame, m_ScaledP1);
        MyFilledCircle(m_CurrentScaledFrame, m_ScaledP2);
        MyLine(m_CurrentScaledFrame, m_ScaledP1, m_ScaledP2);
        DrawText(m_CurrentScaledFrame, m_P1, m_ScaledP1);
        DrawText(m_CurrentScaledFrame, m_P2, m_ScaledP2);
    }

    // Draw current zone (in green) before right click
    for(const auto& Line : m_CurrentLines)
    {
        MyLine(m_CurrentScaledFrame, Line.first*m_Scale, Line.second*m_Scale);
        DrawText(m_CurrentScaledFrame, Line.first, Line.first*m_Scale);
        DrawText(m_CurrentScaledFrame, Line.second, Line.second*m_Scale);
    }

    // Draw all saved zones (in blue) after right click.
    for(const auto& [ZoneId, Zone] : m_Zones)
    {
        // Draw all lines/zones
        for(const auto& Line : Zone.s_Lines)
        {
            MyLine(m_CurrentScaledFrame, Line.first*m_Scale, Line.second*m_Scale, cv::Scalar(255, 0, 0));
            DrawText(m_CurrentScaledFrame, Line.first, Line.first*m_Scale);
            DrawText(m_CurrentScaledFrame, Line.second, Line.second*m_Scale);
        }

        // Draw all centers
        auto Center = Zone.GetCenter();
        MyFilledCircle(m_CurrentScaledFrame, Center*m_Scale);
        DrawText(m_CurrentScaledFrame, ZoneId, Center*m_Scale);
        DrawText(m_CurrentScaledFrame, Center, Center*m_Scale + PointType(5, 10));

        // Draw all Arrow Head
        auto ArrowHead = Zone.GetArrowHead();
        MyFilledCircle(m_CurrentScaledFrame, ArrowHead*m_Scale);
        MyLine(m_CurrentScaledFrame, Center*m_Scale, ArrowHead*m_Scale, cv::Scalar(255, 0, 0));
        DrawText(m_CurrentScaledFrame, Zone.s_Angle, ArrowHead*m_Scale);
    }

    // Highligh center closest to mouse pointer
    if(!m_Zones.empty())
    {
        auto Center = m_Zones[m_ClosestZoneId].GetCenter();
        auto ArrowHead = m_Zones[m_ClosestZoneId].GetArrowHead();
        MyFilledCircle(m_CurrentScaledFrame, Center*m_Scale, cv::Scalar(0, 0, 255));
        MyLine(m_CurrentScaledFrame, Center*m_Scale, ArrowHead*m_Scale, cv::Scalar(0, 0, 255));
    }

    if(m_LeftDoubleClicked)
    {
        cv::Mat m_Snapshot;
        cv::resize(m_CurrentScaledFrame, m_Snapshot, cv::Size(m_CurrentScaledFrame.cols/m_Scale, m_CurrentScaledFrame.rows/m_Scale));
        cv::imwrite(m_SnapPath, m_Snapshot); // write image
    }
    m_LeftDoubleClicked = false;
}

void CMouseEvents::DrawROI()
{
    int Radius = 100;
    int Scale = 2;
    cv::Mat Src{m_CurrentScaledFrame};
    cv::Rect ROI1(m_ScaledP1.x-Radius, m_ScaledP1.y-Radius, 2*Radius, 2*Radius);
    cv::Rect ROI2(m_ScaledP2.x-Radius, m_ScaledP2.y-Radius, 2*Radius, 2*Radius);
    cv::Rect ROI = ((ROI1 | ROI2) & cv::Rect(0, 0, Src.cols, Src.rows));
    cv::Mat CroppedImage = Src(ROI);
    cv::Mat ScaledImage;
    cv::resize(CroppedImage, ScaledImage, cv::Size(CroppedImage.cols*Scale, CroppedImage.rows*Scale));
    cv::imshow(m_WinNameZoom, ScaledImage);
}

void CMouseEvents::OnMouse(int Event, int X, int Y, int Flag, void* /*Param*/)
{
    m_Flag = static_cast<cv::MouseEventFlags>(Flag);

    switch(Event){

    case cv::EVENT_LBUTTONDOWN:
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

    case cv::EVENT_LBUTTONUP:
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
        m_PMousePointer.x=X/m_Scale;
        m_PMousePointer.y=Y/m_Scale;
        m_ScaledPMousePointer.x = X;
        m_ScaledPMousePointer.y = Y;
        if(m_LeftClicked)
        {
            m_P2 = m_PMousePointer;
            m_ScaledP2 = m_ScaledPMousePointer;
        }
        break;

    case cv::EVENT_MOUSEWHEEL:
    case cv::EVENT_MOUSEHWHEEL:
        if(cv::getMouseWheelDelta(Flag) > 0)
        {
            // Scroll down
            if(m_ClosestZoneId != -1)
            {
                ++m_Rotation;
            }
        }
        else
        {
            // Scroll up
            if(m_ClosestZoneId != -1)
            {
                --m_Rotation;
            }
        }
        break;

    default:
        break;
    }
}

}

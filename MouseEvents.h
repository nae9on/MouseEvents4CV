#pragma once

#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <vector>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "TinyXml/tinyxml.h"

namespace mouseevents
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
    using PointType = cv::Point;
    using LineType = std::pair<PointType, PointType>;
    using LinesType = std::vector<LineType>;

    struct SZone
    {
        PointType GetCenter() const;
        PointType GetArrowHead() const;
        double GetDistance(PointType Point) const;
        void Rotate(int Degree);

        int s_ZoneId{-1};
        std::string s_ZoneName{"Default"};
        LinesType s_Lines;
        mutable std::optional<PointType> s_Center{};
        mutable std::optional<PointType> s_ArrowHead{};
        int s_Angle{0};
    };

    CMouseEvents();

    CMouseEvents(const std::string& WinName, const std::string& ConfigPath, const std::string& SnapPath, bool DrawRoI);

    void SetConfigZones(const std::map<int, SZone>& Zones);

    // Show the current frame
    void Show(const cv::Mat& Frame);

private:
    // Add lines to the vector of lines
    void AddLines();

    // Update zones
    void Update();

    // Draw lines on the current frame
    void Draw();

    // Zoom the image around the points in another window
    void DrawROI();

    // Mouse events related (static members used in the Callback function for mouse events)
    static void OnMouse(int Event, int X, int Y, int Flag, void* Param);

    // Store mouse actions between OnMouse callbacks
    static PointType m_P1, m_P2, m_PMousePointer, m_ScaledP1, m_ScaledP2, m_ScaledPMousePointer;
    static int m_ClosestZoneId, m_Rotation;
    static bool m_LeftClicked;
    static bool m_RightClicked;
    static bool m_LeftDoubleClicked;
    static cv::MouseEventFlags m_Flag;
    int m_LastRotation{};
    bool m_LastLeftClicked{false};
    bool m_LastRightClicked{false};

    // Display related
    static const int m_Scale{1};
    const std::string m_WinName{};
    const std::string m_WinNameZoom{};
    const std::string m_ConfigPath{};
    const std::string m_SnapPath{};
    cv::Mat m_CurrentScaledFrame;
    int m_Delay{33}; // delay in ms, corresponds to 30 FPS
    const bool m_DrawROI{false};

    // Zone lines related
    int m_ZoneId{1};
    LinesType m_CurrentLines;
    std::map<int /*Zone Id*/, SZone> m_Zones;
    std::ofstream m_Ofs;
    TiXmlDocument m_Doc{};
};

}

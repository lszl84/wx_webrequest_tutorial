#pragma once

#include <wx/wx.h>
#include <wx/graphics.h>
#include <wx/dcbuffer.h>

#include <vector>

#include "animator.h"
#include "animatedvalue.h"

enum class BitmapScaling : int
{
    Center = 0,
    Fit,
    FillWidth,
    FillHeight
};

class BitmapGallery : public wxWindow
{
public:
    BitmapGallery(wxWindow *parent, wxWindowID id = wxID_ANY, const wxPoint &pos = wxDefaultPosition, const wxSize &size = wxDefaultSize, long style = 0)
        : wxWindow(parent, id, pos, size, wxFULL_REPAINT_ON_RESIZE | style)
    {
        this->SetBackgroundStyle(wxBG_STYLE_PAINT); // needed for windows

        this->Bind(wxEVT_PAINT, &BitmapGallery::OnPaint, this);

        this->Bind(wxEVT_KEY_DOWN, &BitmapGallery::OnKeyDown, this);
        this->Bind(wxEVT_LEFT_DOWN, &BitmapGallery::OnLeftDown, this);
        this->Bind(wxEVT_LEFT_DCLICK, &BitmapGallery::OnLeftDown, this);
        this->Bind(wxEVT_MOTION, &BitmapGallery::OnMouseMove, this);
        this->Bind(wxEVT_LEAVE_WINDOW, &BitmapGallery::OnMouseLeave, this);
    }

    void OnPaint(wxPaintEvent &evt)
    {
        wxAutoBufferedPaintDC dc(this);
        dc.Clear();

        wxGraphicsContext *gc = wxGraphicsContext::Create(dc);

        if (gc && bitmaps.size() > 0)
        {
            const wxSize drawSize = GetClientSize();

            DrawBitmaps(gc, drawSize);

            double arrowLineLength = NavigationRectSize().GetWidth() * 2 / 3;
            double arrowLineWidth = FromDIP(5);

            if (shouldShowLeftArrow)
            {
                DrawNavigationRect(gc, NavigationRectLeft());
                DrawArrow(gc, NavigationRectLeft(), arrowLineLength, arrowLineWidth, 0);
            }

            if (shouldShowRightArrow)
            {
                DrawNavigationRect(gc, NavigationRectRight());
                DrawArrow(gc, NavigationRectRight(), arrowLineLength, arrowLineWidth, M_PI);
            }

            const int dotRadius = FromDIP(4);
            const int dotSpacing = FromDIP(6);

            const int dotCount = bitmaps.size();

            if (dotCount > 1)
            {
                DrawDots(gc, drawSize, dotCount, dotRadius, dotSpacing);
            }
        }
        if (gc)
        {
            delete gc;
        }
    }

    void DrawBitmaps(wxGraphicsContext *gc, const wxSize &drawSize)
    {
        const auto currentTransform = gc->GetTransform();
        const wxSize dipDrawSize = ToDIP(drawSize);

        gc->Translate(-FromDIP(dipDrawSize.GetWidth()) * selectedIndex, 0);

        if (animator.IsRunning())
        {
            gc->Translate(-FromDIP(dipDrawSize.GetWidth()) * animationOffsetNormalized, 0);
        }

        for (const auto &bitmap : bitmaps)
        {
            const wxSize bmpSize = bitmap.GetSize();

            // treating image size as DIP
            double imageW = bmpSize.GetWidth();
            double imageH = bmpSize.GetHeight();

            if (scaling == BitmapScaling::Fit)
            {
                double scaleX = dipDrawSize.GetWidth() / imageW;
                double scaleY = dipDrawSize.GetHeight() / imageH;

                double scale = std::min(scaleX, scaleY);

                imageW *= scale;
                imageH *= scale;
            }
            else if (scaling == BitmapScaling::FillWidth)
            {
                double scaleX = dipDrawSize.GetWidth() / imageW;

                imageW *= scaleX;
                imageH *= scaleX;
            }
            else if (scaling == BitmapScaling::FillHeight)
            {
                double scaleY = dipDrawSize.GetHeight() / imageH;

                imageW *= scaleY;
                imageH *= scaleY;
            }

            double cellCenterX = dipDrawSize.GetWidth() / 2;
            double imageCenterX = imageW / 2;

            double cellCenterY = dipDrawSize.GetHeight() / 2;
            double imageCenterY = imageH / 2;

            double bitmapX = cellCenterX - imageCenterX;
            double bitmapY = cellCenterY - imageCenterY;

            gc->Clip(0, 0, FromDIP(dipDrawSize.GetWidth()), FromDIP(dipDrawSize.GetHeight()));
            gc->DrawBitmap(bitmap, FromDIP(bitmapX), FromDIP(bitmapY), FromDIP(imageW), FromDIP(imageH));

            gc->ResetClip();

            gc->Translate(FromDIP(dipDrawSize.GetWidth()), 0);
        }

        gc->SetTransform(currentTransform);
    }

    void DrawNavigationRect(wxGraphicsContext *gc, const wxRect &rect)
    {
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush(wxColor(255, 255, 255, 64)));

        gc->DrawRectangle(rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight());
    }

    void DrawArrow(wxGraphicsContext *gc, const wxRect &rectToCenterIn, double lineLength, double lineWidth, double rotationAngle)
    {
        const auto currentTransform = gc->GetTransform();
        const auto rectCenter = rectToCenterIn.GetPosition() + rectToCenterIn.GetSize() / 2;

        gc->SetPen(wxPen(wxColor(255, 255, 255, 255), lineWidth));

        gc->Translate(rectCenter.x, rectCenter.y);
        gc->Rotate(-M_PI / 4);
        gc->Rotate(rotationAngle);
        gc->Translate(-lineLength / 4, -lineLength / 4);

        gc->StrokeLine(0, 0, lineLength, 0);
        gc->StrokeLine(0, 0, 0, lineLength);

        gc->SetTransform(currentTransform);
    }

    void DrawDots(wxGraphicsContext *gc, const wxSize &drawSize, int dotCount, int dotRadius, int dotSpacing)
    {
        const auto currentTransform = gc->GetTransform();

        const int dotsWidth = dotCount * dotRadius * 2 + (dotCount - 1) * dotSpacing;

        gc->Translate(-dotsWidth / 2, -dotRadius);
        gc->Translate(drawSize.GetWidth() / 2, drawSize.GetHeight() - dotRadius * 4);

        for (int i = 0; i < dotCount; i++)
        {
            gc->SetPen(*wxTRANSPARENT_PEN);

            if (i == selectedIndex)
            {
                gc->SetBrush(wxBrush(wxColor(255, 255, 255, 255)));
            }
            else
            {
                gc->SetBrush(wxBrush(wxColor(255, 255, 255, 64)));
            }

            gc->DrawEllipse(0, 0, dotRadius * 2, dotRadius * 2);

            gc->Translate(dotRadius * 2 + dotSpacing, 0);
        }

        gc->SetTransform(currentTransform);
    }

    void OnKeyDown(wxKeyEvent &evt)
    {
        if (evt.GetKeyCode() == WXK_LEFT)
        {
            AnimateToPrevious();
        }
        else if (evt.GetKeyCode() == WXK_RIGHT)
        {
            AnimateToNext();
        }
        else
        {
            evt.Skip();
        }
    }

    void OnLeftDown(wxMouseEvent &evt)
    {
        if (shouldShowLeftArrow && NavigationRectLeft().Contains(evt.GetPosition()))
        {
            AnimateToPrevious();
        }
        else if (shouldShowRightArrow && NavigationRectRight().Contains(evt.GetPosition()))
        {
            AnimateToNext();
        }
        else
        {
            evt.Skip();
        }
    }

    void OnMouseMove(wxMouseEvent &evt)
    {
        if (NavigationRectLeft().Contains(evt.GetPosition()))
        {
            shouldShowLeftArrow = true;
            Refresh();
        }
        else if (NavigationRectRight().Contains(evt.GetPosition()))
        {
            shouldShowRightArrow = true;
            Refresh();
        }
        else
        {
            shouldShowLeftArrow = false;
            shouldShowRightArrow = false;
            Refresh();
        }
    }

    void OnMouseLeave(wxMouseEvent &evt)
    {
        shouldShowLeftArrow = false;
        shouldShowRightArrow = false;
        Refresh();
    }

    void AnimateToPrevious()
    {
        if (animator.IsRunning())
        {
            return;
        }

        if (selectedIndex <= 0)
        {
            return;
        }

        StartAnimation(0.0, -1.0, selectedIndex - 1);
    }

    void AnimateToNext()
    {
        if (animator.IsRunning())
        {
            return;
        }

        if (selectedIndex >= bitmaps.size() - 1)
        {
            return;
        }

        StartAnimation(0.0, 1.0, selectedIndex + 1);
    }

    void StartAnimation(double offsetStart, double offsetTarget, int indexTarget)
    {
        AnimatedValue xOffset = {
            offsetStart,
            offsetTarget,
            [this](AnimatedValue *sender, double tNorm, double value)
            {
                animationOffsetNormalized = value;
            },
            "xOffset",
            AnimatedValue::EaseInOutCubic};

        animator.SetAnimatedValues({xOffset});
        animator.SetOnIteration([this]()
                                { Refresh(); });

        animator.SetOnStop([this, indexTarget]()
                           {
                               selectedIndex = indexTarget;
                               animationOffsetNormalized = 0;
                               Refresh(); });

        animator.Start(200);
    }

    std::vector<wxBitmap> bitmaps;
    BitmapScaling scaling = BitmapScaling::Center;

    void ResetBitmaps()
    {
        auto reset = [this]()
        {
            bitmaps.clear();
            selectedIndex = 0;
            animationOffsetNormalized = 0;
            Refresh();
        };

        if (animator.IsRunning())
        {
            animator.SetOnStop(reset);
            animator.Stop();
        }
        else
        {
            reset();
        }
    }

private:
    bool shouldShowLeftArrow = false, shouldShowRightArrow = false;
    int selectedIndex = 0;

    Animator animator;
    double animationOffsetNormalized = 0;

    wxSize NavigationRectSize()
    {
        return {FromDIP(30), GetClientSize().GetHeight()};
    }

    wxRect NavigationRectLeft()
    {
        return {0, 0, NavigationRectSize().GetWidth(), NavigationRectSize().GetHeight()};
    }

    wxRect NavigationRectRight()
    {
        return {GetClientSize().GetWidth() - NavigationRectSize().GetWidth(), 0, NavigationRectSize().GetWidth(), NavigationRectSize().GetHeight()};
    }
};
#pragma once

#include <wx/wx.h>
#include <wx/webrequest.h>

#include <queue>

#include "bitmapgallery.h"

class BitmapLoader : public wxEvtHandler
{
public:
    BitmapLoader(BitmapGallery *gallery) : bitmapView(gallery)
    {
        this->Bind(wxEVT_WEBREQUEST_STATE, &BitmapLoader::OnWebRequestState, this);
    }

    void LoadBitmaps(const std::vector<std::string> &urls)
    {
        wxLogDebug("Loading %zu bitmaps", urls.size());

        wxLogDebug("    Replacing URLs and starting popping.");
        urlsToLoad = std::queue<std::string>(std::deque<std::string>(urls.begin(), urls.end()));

        LoadNextBitmap();
    }

private:
    void LoadNextBitmap()
    {
        auto url = urlsToLoad.front();
        urlsToLoad.pop();

        currentRequest = wxWebSession::GetDefault().CreateRequest(this, url);

        currentRequest.Start();
    }

    void OnWebRequestState(wxWebRequestEvent &event)
    {
        if (event.GetState() == wxWebRequest::State_Completed)
        {
            wxLogDebug(" -- Request finished. Adding bitmap: %s", event.GetResponse().GetURL());

            wxImage image = wxImage(*event.GetResponse().GetStream());

            if (image.IsOk())
            {
                auto bitmap = wxBitmap(image);
                bitmapView->bitmaps.push_back(bitmap);
                bitmapView->Refresh();
            }
        }

        if (event.GetState() != wxWebRequest::State_Active)
        {
            if (urlsToLoad.empty())
            {
                wxLogDebug(" -- Request state <%s> and no more URLs to load. Finishing.", state(event.GetState()));
            }
            else
            {
                wxLogDebug(" -- Request state <%s> and more URLs to load. Loading next.", state(event.GetState()));
                LoadNextBitmap();
            }
        }
    }

    static std::string state(wxWebRequest::State state)
    {
        switch (state)
        {
        case wxWebRequest::State_Idle:
            return "Idle";
        case wxWebRequest::State_Active:
            return "Active";
        case wxWebRequest::State_Completed:
            return "Completed";
        case wxWebRequest::State_Unauthorized:
            return "Unauthorized";
        case wxWebRequest::State_Failed:
            return "Failed";
        default:
            return "Unknown";
        }
    }

    std::queue<std::string> urlsToLoad;

    BitmapGallery *bitmapView;
    wxWebRequest currentRequest;
};
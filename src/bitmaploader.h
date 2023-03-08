#pragma once

#include <wx/wx.h>
#include <wx/webrequest.h>

#include <queue>
#include <functional>

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

        if (isIdle)
        {
            wxLogDebug("    Idle. Switching to busy.");
            isIdle = false;

            wxLogDebug("    Resetting bitmaps.");
            bitmapView->ResetBitmaps();

            wxLogDebug("    Replacing URLs and starting popping.");
            urlsToLoad = std::queue<std::string>(std::deque<std::string>(urls.begin(), urls.end()));

            LoadNextBitmap();
        }
        else
        {
            wxLogDebug("    Busy. Setting next batch.");
            nextBatch = urls;

            if (currentRequest.IsOk() && currentRequest.GetState() == wxWebRequest::State_Active)
            {
                wxLogDebug("    Cancelling current request.");
                currentRequest.Cancel();
            }
        }
    }

    bool IsIdle()
    {
        return isIdle;
    }

    void CancelAll(const std::function<void()> &done)
    {
        if (currentRequest.IsOk() && currentRequest.GetState() == wxWebRequest::State_Active)
        {
            urlsToLoad = {};
            nextBatch = {};
            finishCallback = done;
            currentRequest.Cancel();
        }
        else
        {
            done();
        }
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
        bool shouldEnqueueNextBatch = nextBatch.size() > 0;

        if (!shouldEnqueueNextBatch && event.GetState() == wxWebRequest::State_Completed)
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
            if (shouldEnqueueNextBatch)
            {
                wxLogDebug(" -- Request state <%s> and next batch ready. Starting again.", state(event.GetState()));

                isIdle = true;

                LoadBitmaps(nextBatch);
                nextBatch.clear();
            }
            else
            {
                wxLogDebug(" -- Request state <%s> and no next batch ready. Checking if more URLs to load.", state(event.GetState()));

                if (urlsToLoad.empty())
                {
                    wxLogDebug(" -- Request state <%s> and no more URLs to load. Finishing && setting to Idle", state(event.GetState()));
                    isIdle = true;

                    if (finishCallback)
                    {
                        finishCallback();
                    }
                }
                else
                {
                    wxLogDebug(" -- Request state <%s> and more URLs to load. Loading next.", state(event.GetState()));
                    LoadNextBitmap();
                }
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
        case wxWebRequest::State_Cancelled:
            return "Cancelled";
        default:
            return "Unknown";
        }
    }

    std::queue<std::string> urlsToLoad;

    BitmapGallery *bitmapView;
    wxWebRequest currentRequest;

    std::vector<std::string> nextBatch;
    bool isIdle = true;

    std::function<void()> finishCallback;
};
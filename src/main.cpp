#include <wx/wx.h>
#include <wx/settings.h>

#include <wx/webrequest.h>

#include <nlohmann/json.hpp>

#include <vector>
#include <memory>
#include "product.h"

#include "bitmapgallery.h"
#include "bitmaploader.h"

class MyApp : public wxApp
{
public:
    virtual bool OnInit();
};
class MyFrame : public wxFrame
{
public:
    MyFrame(const wxString &title, const wxPoint &pos, const wxSize &size);

private:
    void BuildUI();
    void DownloadProducts();

    void RefreshCurrentProduct();

    void OnClose(wxCloseEvent &event);

    BitmapGallery *bitmapView;

    wxStaticText *titleText;

    wxStaticText *priceLabel;
    wxStaticText *brandLabel;
    wxStaticText *categoryLabel;
    wxStaticText *ratingLabel;

    wxStaticText *priceText;
    wxStaticText *brandText;
    wxStaticText *categoryText;
    wxStaticText *ratingText;

    wxTextCtrl *descriptionField;

    wxWebRequest request;

    std::vector<Product> products;
    int currentProductIndex = 0;

    std::unique_ptr<BitmapLoader> bitmapLoader;

    bool quitRequested = false;
};

wxIMPLEMENT_APP(MyApp);

bool MyApp::OnInit()
{
    wxInitAllImageHandlers(); // to read PNG

    MyFrame *frame = new MyFrame("Hello World", wxDefaultPosition, wxDefaultSize);
    frame->Show(true);
    return true;
}

MyFrame::MyFrame(const wxString &title, const wxPoint &pos, const wxSize &size)
    : wxFrame(NULL, wxID_ANY, title, pos, size)
{
    this->Bind(wxEVT_CLOSE_WINDOW, &MyFrame::OnClose, this);

    BuildUI();
    DownloadProducts();
}

void MyFrame::BuildUI()
{
    auto mainSizer = new wxBoxSizer(wxVERTICAL);

    wxPanel *panel = new wxPanel(this, wxID_ANY);
    auto sizer = new wxBoxSizer(wxVERTICAL);

    bitmapView = new BitmapGallery(panel);
    bitmapView->scaling = BitmapScaling::FillWidth;

    auto gridSizer = new wxGridSizer(2, FromDIP(10), FromDIP(10));

    auto titleFont = wxFont(wxNORMAL_FONT->GetPointSize() * 2, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    titleText = new wxStaticText(panel, wxID_ANY, "Item Title");
    titleText->SetFont(titleFont);

    priceLabel = new wxStaticText(panel, wxID_ANY, "Price");
    priceText = new wxStaticText(panel, wxID_ANY, "0.00");

    brandLabel = new wxStaticText(panel, wxID_ANY, "Brand");
    categoryLabel = new wxStaticText(panel, wxID_ANY, "Category");
    ratingLabel = new wxStaticText(panel, wxID_ANY, "Rating");

    brandText = new wxStaticText(panel, wxID_ANY, "...");
    categoryText = new wxStaticText(panel, wxID_ANY, "...");
    ratingText = new wxStaticText(panel, wxID_ANY, "...");

    descriptionField = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);

    wxFont fieldsFont = wxFont(wxNORMAL_FONT->GetPointSize() * 1.5, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    auto align = wxALIGN_RIGHT;

    for (auto item : {priceLabel, priceText, brandLabel, brandText, categoryLabel, categoryText, ratingLabel, ratingText})
    {
        item->SetFont(fieldsFont);
        gridSizer->Add(item, 1, align);
        align = (align == wxALIGN_RIGHT ? wxALIGN_LEFT : wxALIGN_RIGHT);
    }

    auto navigationSizer = new wxBoxSizer(wxHORIZONTAL);
    auto prevButton = new wxButton(panel, wxID_ANY, "< Prev");
    auto nextButton = new wxButton(panel, wxID_ANY, "Next >");

    navigationSizer->Add(prevButton, 0, wxALL, FromDIP(5));
    navigationSizer->Add(nextButton, 0, wxALL, FromDIP(5));

    sizer->Add(bitmapView, 2, wxEXPAND | wxBOTTOM, FromDIP(10));
    sizer->Add(titleText, 0, wxALIGN_CENTER | wxALL, FromDIP(10));
    sizer->Add(gridSizer, 0, wxEXPAND | wxALL, FromDIP(10));

    sizer->Add(descriptionField, 1, wxEXPAND | wxALL, FromDIP(10));

    sizer->Add(navigationSizer, 0, wxALIGN_CENTER | wxALL, FromDIP(20));

    panel->SetSizer(sizer);

    mainSizer->Add(panel, 1, wxEXPAND);
    mainSizer->SetMinSize(FromDIP(wxSize(400, 400)));
    this->SetSizerAndFit(mainSizer);

    this->SetBackgroundColour(wxSystemSettings::GetAppearance().IsDark() ? *wxBLACK : *wxWHITE);
    this->descriptionField->SetBackgroundColour(this->GetBackgroundColour());

    prevButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent &evt)
                     {
                         if (this->currentProductIndex > 0)
                         {
                             this->currentProductIndex--;
                             this->RefreshCurrentProduct();
                         } });

    nextButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent &evt)
                     {
                         if (this->currentProductIndex < (int)this->products.size() - 1)
                         {
                             this->currentProductIndex++;
                             this->RefreshCurrentProduct();
                         } });

    bitmapLoader = std::make_unique<BitmapLoader>(bitmapView);
}

void MyFrame::RefreshCurrentProduct()
{
    auto product = this->products[this->currentProductIndex];

    this->titleText->SetLabel(product.title);
    this->priceText->SetLabel(wxString::Format("$%.2Lf", product.price));
    this->brandText->SetLabel(product.brand);
    this->categoryText->SetLabel(product.category);
    this->ratingText->SetLabel(wxString::Format("%.1f", product.rating));
    this->descriptionField->SetValue(product.description);

    bitmapLoader->LoadBitmaps(product.imageUrls);

    Layout();
}

void MyFrame::DownloadProducts()
{
    static constexpr auto Url = "https://dummyjson.com/products/";

    request = wxWebSession::GetDefault().CreateRequest(this, Url);

    if (!request.IsOk())
    {
        wxLogError("Failed to create request");
        return;
    }

    this->Bind(wxEVT_WEBREQUEST_STATE, [this](wxWebRequestEvent &evt)
               {
                   if (evt.GetState() == wxWebRequest::State_Completed)
                   {
                       auto response = evt.GetResponse();
                       if (response.GetStatus() == 200)
                       {
                           auto productsJson = nlohmann::json::parse(response.AsString());
                           for (auto &object : productsJson["products"])
                           {
                               Product p{
                                   object["title"],
                                   object["price"],
                                   object["brand"],
                                   object["category"],
                                   object["rating"],
                                   object["description"],
                                   object["images"]};

                               this->products.push_back(p);
                           }

                           this->currentProductIndex = 0;
                           this->RefreshCurrentProduct();
                       }
                       else
                       {
                           wxLogError("Failed to download products");
                       }
                   }

                   if (quitRequested && evt.GetState() == wxWebRequest::State_Cancelled)
                   {
                       this->Close();
                   } });

    request.Start();
}

void MyFrame::OnClose(wxCloseEvent &evt)
{
    if (request.IsOk() && request.GetState() == wxWebRequest::State_Active)
    {
        quitRequested = true;
        this->Hide();
        this->CallAfter([this]()
                        { request.Cancel(); });

        evt.Veto();
    }
    else if (bitmapLoader && !bitmapLoader->IsIdle())
    {
        this->Hide();
        this->CallAfter([this]()
                        { bitmapLoader->CancelAll([this]()
                                                  { this->Close(); }); });
        evt.Veto();
    }
    else
    {
        evt.Skip();
    }
}
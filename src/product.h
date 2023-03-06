#pragma once

#include <wx/image.h>
#include <string>
#include <vector>

struct Product
{
    std::string title;
    long double price;
    std::string brand;
    std::string category;
    double rating;
    std::string description;

    std::vector<std::string> imageUrls;
};
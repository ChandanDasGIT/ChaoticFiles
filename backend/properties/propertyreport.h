#pragma once
#include <string>
#include <vector>

struct PropertyRow {
    std::string label;
    std::string value;
};

struct PropertySection {
    std::string title;
    std::vector<PropertyRow> rows;
};

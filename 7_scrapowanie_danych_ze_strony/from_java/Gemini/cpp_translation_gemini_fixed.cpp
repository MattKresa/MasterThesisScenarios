#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <random>
#include <chrono>
#include <thread>
#include <regex>

#include <gumbo.h>
#include <cpr/cpr.h>

const std::string BASE_URL = "https://quotes.toscrape.com";
const std::string START_URL = BASE_URL + "/page/1/";
const std::string OUTPUT_CSV = "quotes.csv";

// Helper function to find a specific Gumbo attribute by name.
GumboAttribute* findAttribute(const GumboVector* attributes, const std::string& name) {
    for (unsigned int i = 0; i < attributes->length; ++i) {
        GumboAttribute* attr = static_cast<GumboAttribute*>(attributes->data[i]);
        if (std::string(attr->name) == name) {
            return attr;
        }
    }
    return nullptr;
}

// Recursive helper to get text from a Gumbo node.
std::string getText(GumboNode* node) {
    if (!node) return "";
    if (node->type == GUMBO_NODE_TEXT) {
        return std::string(node->v.text.text);
    }
    if (node->type == GUMBO_NODE_ELEMENT) {
        std::string text;
        const GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            text += getText(static_cast<GumboNode*>(children->data[i]));
        }
        return text;
    }
    return "";
}

// Recursive helper to find elements with a specific tag and class.
std::vector<GumboNode*> findElements(GumboNode* parent, GumboTag tag, const std::string& className) {
    std::vector<GumboNode*> foundNodes;
    if (!parent || parent->type != GUMBO_NODE_ELEMENT) return foundNodes;

    const GumboVector* children = &parent->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
        GumboNode* child = static_cast<GumboNode*>(children->data[i]);
        if (child->type == GUMBO_NODE_ELEMENT) {
            if (child->v.element.tag == tag) {
                GumboAttribute* classAttr = findAttribute(&child->v.element.attributes, "class");
                if (classAttr && std::string(classAttr->value).find(className) != std::string::npos) {
                    foundNodes.push_back(child);
                }
            }
            // Recurse into children
            std::vector<GumboNode*> subChildren = findElements(child, tag, className);
            foundNodes.insert(foundNodes.end(), subChildren.begin(), subChildren.end());
        }
    }
    return foundNodes;
}

// Function to find the first element with a given tag and class.
GumboNode* findFirstElement(GumboNode* parent, GumboTag tag, const std::string& className) {
    if (!parent || parent->type != GUMBO_NODE_ELEMENT) return nullptr;

    const GumboVector* children = &parent->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
        GumboNode* child = static_cast<GumboNode*>(children->data[i]);
        if (child->type == GUMBO_NODE_ELEMENT) {
            if (child->v.element.tag == tag) {
                GumboAttribute* classAttr = findAttribute(&child->v.element.attributes, "class");
                if (classAttr && std::string(classAttr->value).find(className) != std::string::npos) {
                    return child;
                }
            }
            // Recurse into children
            GumboNode* result = findFirstElement(child, tag, className);
            if (result) return result;
        }
    }
    return nullptr;
}

// Forward declarations
GumboNode* getPage(const std::string& url);
std::vector<std::map<std::string, std::string>> parseQuotes(GumboNode* doc);
std::string getNextPage(GumboNode* doc);
void saveToCsv(const std::vector<std::map<std::string, std::string>>& quotes, const std::string& filename);
std::string escapeCsv(std::string value);

int main() {
    std::vector<std::map<std::string, std::string>> allQuotes;
    std::string url = START_URL;

    while (!url.empty()) {
        std::cout << "Fetching: " << url << std::endl;
        GumboNode* doc = getPage(url);
        if (!doc) break;

        auto parsedQuotes = parseQuotes(doc);
        allQuotes.insert(allQuotes.end(), parsedQuotes.begin(), parsedQuotes.end());

        url = getNextPage(doc);
        //gumbo_destroy_output(&kGumboDefaultOptions, gumbo_get_output(doc));

        if (!url.empty()) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> distrib(1000, 3000);
            std::this_thread::sleep_for(std::chrono::milliseconds(distrib(gen)));
        }
    }

    if (!allQuotes.empty()) {
        saveToCsv(allQuotes, OUTPUT_CSV);
        std::cout << "Saved " << allQuotes.size() << " quotes to file: " << OUTPUT_CSV << std::endl;
    }
    else {
        std::cerr << "No quotes found." << std::endl;
    }

    return 0;
}

GumboNode* getPage(const std::string& url) {
    try {
        cpr::Response r = cpr::Get(cpr::Url{ url }, cpr::Timeout{ 10000 });
        if (r.status_code == 200) {
            GumboOutput* output = gumbo_parse(r.text.c_str());
            return output->root;
        }
        else {
            std::cerr << "Error fetching page " << url << ": " << r.status_code << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error fetching page " << url << ": " << e.what() << std::endl;
    }
    return nullptr;
}

std::vector<std::map<std::string, std::string>> parseQuotes(GumboNode* doc) {
    std::vector<std::map<std::string, std::string>> quotes;
    std::vector<GumboNode*> quoteBlocks = findElements(doc, GUMBO_TAG_DIV, "quote");

    for (const auto& quote : quoteBlocks) {
        GumboNode* textElement = findFirstElement(quote, GUMBO_TAG_SPAN, "text");
        GumboNode* authorElement = findFirstElement(quote, GUMBO_TAG_SMALL, "author");
        std::vector<GumboNode*> tagElements = findElements(quote, GUMBO_TAG_A, "tag");

        if (textElement && authorElement) {
            std::vector<std::string> tags;
            for (const auto& tag : tagElements) {
                tags.push_back(getText(tag));
            }

            std::map<std::string, std::string> quoteData;
            quoteData["text"] = getText(textElement);
            quoteData["author"] = getText(authorElement);

            std::string tags_string;
            for (size_t i = 0; i < tags.size(); ++i) {
                tags_string += tags[i];
                if (i < tags.size() - 1) {
                    tags_string += ", ";
                }
            }
            quoteData["tags"] = tags_string;
            quotes.push_back(quoteData);
        }
    }
    return quotes;
}

std::string getNextPage(GumboNode* doc) {
    GumboNode* nextButton = findFirstElement(doc, GUMBO_TAG_LI, "next");
    if (nextButton) {
        const GumboVector* children = &nextButton->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            GumboNode* child = static_cast<GumboNode*>(children->data[i]);
            if (child->type == GUMBO_NODE_ELEMENT && child->v.element.tag == GUMBO_TAG_A) {
                GumboAttribute* hrefAttr = findAttribute(&child->v.element.attributes, "href");
                if (hrefAttr) {
                    return BASE_URL + std::string(hrefAttr->value);
                }
            }
        }
    }
    return "";
}

void saveToCsv(const std::vector<std::map<std::string, std::string>>& quotes, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error writing to CSV file: could not open " << filename << std::endl;
        return;
    }

    file << "text,author,tags\n";

    for (const auto& quote : quotes) {
        file << "\"" << escapeCsv(quote.at("text")) << "\",";
        file << "\"" << escapeCsv(quote.at("author")) << "\",";
        file << "\"" << escapeCsv(quote.at("tags")) << "\"\n";
    }
    file.close();
}

std::string escapeCsv(std::string value) {
    std::string::size_type pos = 0;
    while ((pos = value.find("\"", pos)) != std::string::npos) {
        value.replace(pos, 1, "\"\"");
        pos += 2;
    }
    return value;
}
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <chrono>
#include <fstream>
#include <sstream>
#include <random>
#include <curl/curl.h>
#include <gumbo.h>

const std::string BASE_URL = "https://quotes.toscrape.com";
const std::string START_URL = BASE_URL + "/page/1/";
const std::string OUTPUT_CSV = "quotes.csv";

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t totalSize = size * nmemb;
    userp->append((char*)contents, totalSize);
    return totalSize;
}

std::string getPageHtml(const std::string& url) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Curl init failed!" << std::endl;
        return "";
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "Error fetching page: " << curl_easy_strerror(res) << std::endl;
    }
    curl_easy_cleanup(curl);

    return readBuffer;
}

// Helper: get attribute value from node
std::string getAttr(GumboNode* node, const char* attrName) {
    if (node->type != GUMBO_NODE_ELEMENT) return "";
    GumboAttribute* attr = gumbo_get_attribute(&node->v.element.attributes, attrName);
    if (attr) return attr->value;
    return "";
}

// Extract quotes
void searchForQuotes(GumboNode* node, std::vector<std::map<std::string, std::string>>& quotes) {
    if (node->type != GUMBO_NODE_ELEMENT) return;

    if (node->v.element.tag == GUMBO_TAG_DIV) {
        std::string classAttr = getAttr(node, "class");
        if (classAttr == "quote") {
            std::map<std::string, std::string> quoteData;
            quoteData["text"] = "";
            quoteData["author"] = "";
            quoteData["tags"] = "";

            GumboVector* children = &node->v.element.children;
            for (unsigned int i = 0; i < children->length; ++i) {
                GumboNode* child = static_cast<GumboNode*>(children->data[i]);
                if (child->type == GUMBO_NODE_ELEMENT) {
                    std::string childClass = getAttr(child, "class");

                    if (childClass == "text") {
                        if (child->v.element.children.length > 0) {
                            GumboNode* textNode = static_cast<GumboNode*>(child->v.element.children.data[0]);
                            if (textNode->type == GUMBO_NODE_TEXT)
                                quoteData["text"] = textNode->v.text.text;
                        }
                    }

                    if (childClass == "author") {
                        if (child->v.element.children.length > 0) {
                            GumboNode* textNode = static_cast<GumboNode*>(child->v.element.children.data[0]);
                            if (textNode->type == GUMBO_NODE_TEXT)
                                quoteData["author"] = textNode->v.text.text;
                        }
                    }

                    // tags
                    if (child->v.element.tag == GUMBO_TAG_DIV && getAttr(child, "class") == "tags") {
                        std::vector<std::string> tagsList;
                        GumboVector* tagChildren = &child->v.element.children;
                        for (unsigned int j = 0; j < tagChildren->length; ++j) {
                            GumboNode* tagNode = static_cast<GumboNode*>(tagChildren->data[j]);
                            if (tagNode->type == GUMBO_NODE_ELEMENT && tagNode->v.element.tag == GUMBO_TAG_A) {
                                if (getAttr(tagNode, "class") == "tag") {
                                    if (tagNode->v.element.children.length > 0) {
                                        GumboNode* textNode = static_cast<GumboNode*>(tagNode->v.element.children.data[0]);
                                        if (textNode->type == GUMBO_NODE_TEXT)
                                            tagsList.push_back(textNode->v.text.text);
                                    }
                                }
                            }
                        }
                        std::ostringstream oss;
                        for (size_t t = 0; t < tagsList.size(); ++t) {
                            if (t > 0) oss << ", ";
                            oss << tagsList[t];
                        }
                        quoteData["tags"] = oss.str();
                    }
                }
            }
            quotes.push_back(quoteData);
        }
    }

    GumboVector* children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
        searchForQuotes(static_cast<GumboNode*>(children->data[i]), quotes);
    }
}

// Extract next page URL
std::string getNextPage(GumboNode* node) {
    if (node->type != GUMBO_NODE_ELEMENT) return "";

    if (node->v.element.tag == GUMBO_TAG_LI && getAttr(node, "class") == "next") {
        GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            GumboNode* aNode = static_cast<GumboNode*>(children->data[i]);
            if (aNode->type == GUMBO_NODE_ELEMENT && aNode->v.element.tag == GUMBO_TAG_A) {
                std::string href = getAttr(aNode, "href");
                if (!href.empty()) {
                    return BASE_URL + href;
                }
            }
        }
    }

    GumboVector* children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
        std::string url = getNextPage(static_cast<GumboNode*>(children->data[i]));
        if (!url.empty()) return url;
    }
    return "";
}

void saveToCsv(const std::vector<std::map<std::string, std::string>>& quotes, const std::string& filename) {
    std::ofstream file(filename, std::ios::trunc);
    if (!file) {
        std::cerr << "Error opening file for writing." << std::endl;
        return;
    }
    file << "text,author,tags\n";
    for (const auto& quote : quotes) {
        file << "\"" << quote.at("text") << "\","
            << "\"" << quote.at("author") << "\","
            << "\"" << quote.at("tags") << "\"\n";
    }
    file.close();
}

int main() {
    std::vector<std::map<std::string, std::string>> allQuotes;
    std::string url = START_URL;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(1000, 3000);

    curl_global_init(CURL_GLOBAL_ALL);

    while (!url.empty()) {
        std::cout << "Fetching: " << url << std::endl;
        std::string html = getPageHtml(url);
        if (html.empty()) break;

        GumboOutput* output = gumbo_parse(html.c_str());
        searchForQuotes(output->root, allQuotes);
        url = getNextPage(output->root);
        gumbo_destroy_output(&kGumboDefaultOptions, output);

        std::this_thread::sleep_for(std::chrono::milliseconds(dist(gen)));
    }

    if (!allQuotes.empty()) {
        saveToCsv(allQuotes, OUTPUT_CSV);
        std::cout << "Saved " << allQuotes.size() << " quotes to file: " << OUTPUT_CSV << std::endl;
    }
    else {
        std::cerr << "No quotes found." << std::endl;
    }

    curl_global_cleanup();
    return 0;
}

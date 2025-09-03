#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <random>
#include <chrono>
#include <curl/curl.h>
#include <gumbo.h>

struct Quote {
    std::string text;
    std::string author;
    std::string tags;
};

const std::string BASE_URL = "https://quotes.toscrape.com";
const std::string START_URL = BASE_URL + "/page/1/";
const std::string OUTPUT_CSV = "quotes.csv";

// For libcurl to write response into a string
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

std::string get_page_html(const std::string& url) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "[ERROR] Failed to fetch " << url << ": " 
                      << curl_easy_strerror(res) << "\n";
            readBuffer.clear();
        }
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return readBuffer;
}

// Recursively search Gumbo nodes by class
void search_for_quotes(GumboNode* node, std::vector<Quote>& quotes) {
    if (node->type != GUMBO_NODE_ELEMENT) return;

    GumboAttribute* classAttr;
    if ((classAttr = gumbo_get_attribute(&node->v.element.attributes, "class"))) {
        std::string className = classAttr->value;
        if (className == "quote") {
            Quote q;
            GumboVector* children = &node->v.element.children;
            for (unsigned int i = 0; i < children->length; ++i) {
                GumboNode* child = static_cast<GumboNode*>(children->data[i]);
                if (child->type == GUMBO_NODE_ELEMENT) {
                    GumboAttribute* childClass = gumbo_get_attribute(&child->v.element.attributes, "class");
                    if (childClass) {
                        std::string cName = childClass->value;
                        if (cName == "text") {
                            q.text = static_cast<GumboNode*>(child->v.element.children.data[0])->v.text.text;
                        } else if (cName == "author") {
                            q.author = static_cast<GumboNode*>(child->v.element.children.data[0])->v.text.text;
                        } else if (cName == "tags") {
                            GumboVector* tagChildren = &child->v.element.children;
                            std::string tagsCombined;
                            for (unsigned int j = 0; j < tagChildren->length; ++j) {
                                GumboNode* tagNode = static_cast<GumboNode*>(tagChildren->data[j]);
                                if (tagNode->type == GUMBO_NODE_ELEMENT) {
                                    GumboAttribute* tagClass = gumbo_get_attribute(&tagNode->v.element.attributes, "class");
                                    if (tagClass && std::string(tagClass->value) == "tag") {
                                        if (!tagsCombined.empty()) tagsCombined += ", ";
                                        tagsCombined += static_cast<GumboNode*>(tagNode->v.element.children.data[0])->v.text.text;
                                    }
                                }
                            }
                            q.tags = tagsCombined;
                        }
                    }
                }
            }
            quotes.push_back(q);
        }
    }

    GumboVector* children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
        search_for_quotes(static_cast<GumboNode*>(children->data[i]), quotes);
    }
}

std::string get_next_page_url(GumboNode* node) {
    if (node->type != GUMBO_NODE_ELEMENT) return "";

    GumboAttribute* classAttr;
    if ((classAttr = gumbo_get_attribute(&node->v.element.attributes, "class"))) {
        if (std::string(classAttr->value) == "next") {
            GumboVector* children = &node->v.element.children;
            for (unsigned int i = 0; i < children->length; ++i) {
                GumboNode* linkNode = static_cast<GumboNode*>(children->data[i]);
                if (linkNode->type == GUMBO_NODE_ELEMENT && 
                    linkNode->v.element.tag == GUMBO_TAG_A) {
                    GumboAttribute* href = gumbo_get_attribute(&linkNode->v.element.attributes, "href");
                    if (href) {
                        return BASE_URL + href->value;
                    }
                }
            }
        }
    }

    GumboVector* children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
        std::string url = get_next_page_url(static_cast<GumboNode*>(children->data[i]));
        if (!url.empty()) return url;
    }
    return "";
}

void save_to_csv(const std::vector<Quote>& quotes, const std::string& filename) {
    std::ofstream file(filename);
    if (!file) {
        std::cerr << "[ERROR] Unable to open file: " << filename << "\n";
        return;
    }
    file << "text,author,tags\n";
    for (const auto& q : quotes) {
        file << "\"" << q.text << "\","
             << "\"" << q.author << "\","
             << "\"" << q.tags << "\"\n";
    }
    file.close();
}

int main() {
    std::string url = START_URL;
    std::vector<Quote> all_quotes;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(1.0, 3.0);

    while (!url.empty()) {
        std::cout << "[INFO] Fetching: " << url << "\n";
        std::string html = get_page_html(url);
        if (html.empty()) break;

        GumboOutput* output = gumbo_parse(html.c_str());

        std::vector<Quote> quotes;
        search_for_quotes(output->root, quotes);
        all_quotes.insert(all_quotes.end(), quotes.begin(), quotes.end());

        url = get_next_page_url(output->root);
        gumbo_destroy_output(&kGumboDefaultOptions, output);

        std::this_thread::sleep_for(std::chrono::milliseconds(
            static_cast<int>(dis(gen) * 1000)
        ));
    }

    if (!all_quotes.empty()) {
        save_to_csv(all_quotes, OUTPUT_CSV);
        std::cout << "[INFO] Saved " << all_quotes.size() 
                  << " quotes to file: " << OUTPUT_CSV << "\n";
    } else {
        std::cout << "[WARNING] No quotes found.\n";
    }

    return 0;
}

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <thread>
#include <chrono>
#include <random>
#include <algorithm>
#include <sstream>
#include <curl/curl.h>
#include <gumbo.h>

class QuoteScraper {
private:
    static const std::string BASE_URL;
    static const std::string START_URL;
    static const std::string OUTPUT_CSV;

    struct WriteCallback {
        std::string data;
    };

    static size_t WriteCallbackFunc(void* contents, size_t size, size_t nmemb, WriteCallback* userp) {
        userp->data.append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    static std::string getPage(const std::string& url) {
        CURL* curl;
        CURLcode res;
        WriteCallback writeData;

        curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallbackFunc);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &writeData);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (compatible; QuoteScraper/1.0)");

            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);

            if (res != CURLE_OK) {
                std::cerr << "Error fetching page " << url << ": " << curl_easy_strerror(res) << std::endl;
                return "";
            }
        }

        return writeData.data;
    }

    static std::string getTextContent(GumboNode* node) {
        if (node->type == GUMBO_NODE_TEXT) {
            return std::string(node->v.text.text);
        }
        else if (node->type == GUMBO_NODE_ELEMENT &&
            node->v.element.tag != GUMBO_TAG_SCRIPT &&
            node->v.element.tag != GUMBO_TAG_STYLE) {
            std::string contents;
            GumboVector* children = &node->v.element.children;
            for (unsigned int i = 0; i < children->length; ++i) {
                std::string text = getTextContent((GumboNode*)children->data[i]);
                contents += text;
            }
            return contents;
        }
        return "";
    }

    static std::vector<GumboNode*> getElementsByClass(GumboNode* node, const std::string& className) {
        std::vector<GumboNode*> results;

        if (node->type != GUMBO_NODE_ELEMENT) {
            return results;
        }

        GumboAttribute* classAttr;
        if ((classAttr = gumbo_get_attribute(&node->v.element.attributes, "class"))) {
            std::string classValue = classAttr->value;
            if (classValue.find(className) != std::string::npos) {
                results.push_back(node);
            }
        }

        GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            std::vector<GumboNode*> childResults = getElementsByClass((GumboNode*)children->data[i], className);
            results.insert(results.end(), childResults.begin(), childResults.end());
        }

        return results;
    }

    static GumboNode* getFirstElementByClass(GumboNode* node, const std::string& className) {
        if (node->type != GUMBO_NODE_ELEMENT) {
            return nullptr;
        }

        GumboAttribute* classAttr;
        if ((classAttr = gumbo_get_attribute(&node->v.element.attributes, "class"))) {
            std::string classValue = classAttr->value;
            if (classValue.find(className) != std::string::npos) {
                return node;
            }
        }

        GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            GumboNode* result = getFirstElementByClass((GumboNode*)children->data[i], className);
            if (result) return result;
        }

        return nullptr;
    }

    static std::vector<GumboNode*> getElementsByTag(GumboNode* node, GumboTag tag) {
        std::vector<GumboNode*> results;

        if (node->type != GUMBO_NODE_ELEMENT) {
            return results;
        }

        if (node->v.element.tag == tag) {
            results.push_back(node);
        }

        GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            std::vector<GumboNode*> childResults = getElementsByTag((GumboNode*)children->data[i], tag);
            results.insert(results.end(), childResults.begin(), childResults.end());
        }

        return results;
    }

    static std::vector<std::map<std::string, std::string>> parseQuotes(const std::string& html) {
        std::vector<std::map<std::string, std::string>> quotes;

        GumboOutput* output = gumbo_parse(html.c_str());
        std::vector<GumboNode*> quoteBlocks = getElementsByClass(output->root, "quote");

        for (GumboNode* quoteBlock : quoteBlocks) {
            std::map<std::string, std::string> quoteData;

            // Get quote text
            GumboNode* textElement = getFirstElementByClass(quoteBlock, "text");
            if (textElement) {
                std::string text = getTextContent(textElement);
                // Remove surrounding quotes if present
                if (text.length() >= 2 && text[0] == '"' && text.back() == '"') {
                    text = text.substr(1, text.length() - 2);
                }
                quoteData["text"] = text;
            }

            // Get author
            GumboNode* authorElement = getFirstElementByClass(quoteBlock, "author");
            if (authorElement) {
                quoteData["author"] = getTextContent(authorElement);
            }

            // Get tags
            std::vector<GumboNode*> tagElements = getElementsByTag(quoteBlock, GUMBO_TAG_A);
            std::vector<std::string> tags;
            for (GumboNode* tagElement : tagElements) {
                GumboAttribute* classAttr;
                if ((classAttr = gumbo_get_attribute(&tagElement->v.element.attributes, "class"))) {
                    std::string classValue = classAttr->value;
                    if (classValue == "tag") {
                        tags.push_back(getTextContent(tagElement));
                    }
                }
            }

            std::string tagsStr;
            for (size_t i = 0; i < tags.size(); ++i) {
                if (i > 0) tagsStr += ", ";
                tagsStr += tags[i];
            }
            quoteData["tags"] = tagsStr;

            quotes.push_back(quoteData);
        }

        gumbo_destroy_output(&kGumboDefaultOptions, output);
        return quotes;
    }

    static std::string getNextPageUrl(const std::string& html) {
        GumboOutput* output = gumbo_parse(html.c_str());

        // Look for li.next > a
        std::vector<GumboNode*> liElements = getElementsByTag(output->root, GUMBO_TAG_LI);
        for (GumboNode* liElement : liElements) {
            GumboAttribute* classAttr;
            if ((classAttr = gumbo_get_attribute(&liElement->v.element.attributes, "class"))) {
                std::string classValue = classAttr->value;
                if (classValue == "next") {
                    std::vector<GumboNode*> aElements = getElementsByTag(liElement, GUMBO_TAG_A);
                    if (!aElements.empty()) {
                        GumboAttribute* hrefAttr;
                        if ((hrefAttr = gumbo_get_attribute(&aElements[0]->v.element.attributes, "href"))) {
                            std::string href = hrefAttr->value;
                            gumbo_destroy_output(&kGumboDefaultOptions, output);
                            return BASE_URL + href;
                        }
                    }
                }
            }
        }

        gumbo_destroy_output(&kGumboDefaultOptions, output);
        return "";
    }

    static std::string escapeCsv(const std::string& value) {
        std::string escaped = value;

        // Replace " with ""
        size_t pos = 0;
        while ((pos = escaped.find("\"", pos)) != std::string::npos) {
            escaped.replace(pos, 1, "\"\"");
            pos += 2;
        }

        // If contains quotes, commas, or newlines, wrap in quotes
        if (value.find("\"") != std::string::npos ||
            value.find(",") != std::string::npos ||
            value.find("\n") != std::string::npos) {
            escaped = "\"" + escaped + "\"";
        }

        return escaped;
    }

    static void saveToCsv(const std::vector<std::map<std::string, std::string>>& quotes, const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error opening file for writing: " << filename << std::endl;
            return;
        }

        // Write header
        file << "text,author,tags\n";

        // Write data
        for (const auto& quote : quotes) {
            file << escapeCsv(quote.at("text")) << ","
                << escapeCsv(quote.at("author")) << ","
                << escapeCsv(quote.at("tags")) << "\n";
        }

        file.close();
        std::cout << "Successfully wrote " << quotes.size() << " quotes to " << filename << std::endl;
    }

public:
    static void run() {
        // Initialize curl globally
        curl_global_init(CURL_GLOBAL_DEFAULT);

        std::vector<std::map<std::string, std::string>> allQuotes;
        std::string url = START_URL;

        // Random number generator for sleep
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1000, 3000); // 1-3 seconds

        while (!url.empty()) {
            std::cout << "Fetching: " << url << std::endl;

            std::string html = getPage(url);
            if (html.empty()) {
                std::cerr << "Failed to fetch page, stopping." << std::endl;
                break;
            }

            std::vector<std::map<std::string, std::string>> pageQuotes = parseQuotes(html);
            allQuotes.insert(allQuotes.end(), pageQuotes.begin(), pageQuotes.end());

            url = getNextPageUrl(html);

            // Sleep for 1-3 seconds
            if (!url.empty()) {
                int sleepTime = dis(gen);
                std::cout << "Sleeping for " << sleepTime << "ms..." << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
            }
        }

        if (!allQuotes.empty()) {
            saveToCsv(allQuotes, OUTPUT_CSV);
            std::cout << "Saved " << allQuotes.size() << " quotes to file: " << OUTPUT_CSV << std::endl;
        }
        else {
            std::cout << "No quotes found." << std::endl;
        }

        // Cleanup curl
        curl_global_cleanup();
    }
};

// Static member definitions
const std::string QuoteScraper::BASE_URL = "https://quotes.toscrape.com";
const std::string QuoteScraper::START_URL = BASE_URL + "/page/1/";
const std::string QuoteScraper::OUTPUT_CSV = "quotes.csv";

int main() {
    QuoteScraper::run();
    return 0;
}
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <thread>
#include <random>
#include <curl/curl.h>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>

// Configuration
const std::string BASE_URL = "https://quotes.toscrape.com";
const std::string START_URL = BASE_URL + "/page/1/";
const std::string OUTPUT_CSV = "quotes.csv";

// Logging macros
#define LOG_INFO(message) std::cout << "[INFO] " << message << std::endl
#define LOG_WARNING(message) std::cout << "[WARNING] " << message << std::endl
#define LOG_ERROR(message) std::cerr << "[ERROR] " << message << std::endl

// Callback function for libcurl to write response data
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t total_size = size * nmemb;
    output->append((char*)contents, total_size);
    return total_size;
}

// Fetch a web page and return its HTML content
std::string fetchPage(const std::string& url) {
    CURL* curl = curl_easy_init();
    std::string response;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            LOG_ERROR("Failed to fetch " << url << ": " << curl_easy_strerror(res));
            response.clear();
        }

        curl_easy_cleanup(curl);
    }
    else {
        LOG_ERROR("Failed to initialize CURL");
    }

    return response;
}

// Parse quotes from HTML content
std::vector<std::map<std::string, std::string>> parseQuotes(const std::string& html) {
    std::vector<std::map<std::string, std::string>> quotes;
    htmlDocPtr doc = htmlReadDoc((const xmlChar*)html.c_str(), NULL, NULL, HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);

    if (!doc) {
        LOG_ERROR("Failed to parse HTML");
        return quotes;
    }

    xmlXPathContextPtr context = xmlXPathNewContext(doc);
    if (!context) {
        LOG_ERROR("Failed to create XPath context");
        xmlFreeDoc(doc);
        return quotes;
    }

    // Register namespaces if needed (not necessary for this simple HTML)
    // Find all quote blocks
    xmlXPathObjectPtr quoteBlocks = xmlXPathEvalExpression((const xmlChar*)"//div[@class='quote']", context);
    if (!quoteBlocks || xmlXPathNodeSetIsEmpty(quoteBlocks->nodesetval)) {
        LOG_WARNING("No quote blocks found");
        xmlXPathFreeObject(quoteBlocks);
        xmlXPathFreeContext(context);
        xmlFreeDoc(doc);
        return quotes;
    }

    // Process each quote block
    for (int i = 0; i < quoteBlocks->nodesetval->nodeNr; i++) {
        xmlNodePtr quoteNode = quoteBlocks->nodesetval->nodeTab[i];
        std::map<std::string, std::string> quoteData;

        // Get quote text
        xmlXPathSetContextNode(quoteNode, context);
        xmlXPathObjectPtr textObj = xmlXPathEvalExpression((const xmlChar*)".//span[@class='text']", context);
        if (textObj && !xmlXPathNodeSetIsEmpty(textObj->nodesetval)) {
            xmlChar* text = xmlNodeGetContent(textObj->nodesetval->nodeTab[0]);
            quoteData["text"] = std::string((char*)text);
            xmlFree(text);
        }
        xmlXPathFreeObject(textObj);

        // Get author
        xmlXPathObjectPtr authorObj = xmlXPathEvalExpression((const xmlChar*)".//small[@class='author']", context);
        if (authorObj && !xmlXPathNodeSetIsEmpty(authorObj->nodesetval)) {
            xmlChar* author = xmlNodeGetContent(authorObj->nodesetval->nodeTab[0]);
            quoteData["author"] = std::string((char*)author);
            xmlFree(author);
        }
        xmlXPathFreeObject(authorObj);

        // Get tags
        xmlXPathObjectPtr tagsObj = xmlXPathEvalExpression((const xmlChar*)".//a[@class='tag']", context);
        if (tagsObj && !xmlXPathNodeSetIsEmpty(tagsObj->nodesetval)) {
            std::string tags;
            for (int j = 0; j < tagsObj->nodesetval->nodeNr; j++) {
                xmlChar* tag = xmlNodeGetContent(tagsObj->nodesetval->nodeTab[j]);
                if (j > 0) tags += ", ";
                tags += (char*)tag;
                xmlFree(tag);
            }
            quoteData["tags"] = tags;
        }
        xmlXPathFreeObject(tagsObj);

        quotes.push_back(quoteData);
    }

    xmlXPathFreeObject(quoteBlocks);
    xmlXPathFreeContext(context);
    xmlFreeDoc(doc);

    return quotes;
}

// Get the next page URL
std::string getNextPage(const std::string& html) {
    htmlDocPtr doc = htmlReadDoc((const xmlChar*)html.c_str(), NULL, NULL, HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
    if (!doc) {
        LOG_ERROR("Failed to parse HTML for next page");
        return "";
    }

    xmlXPathContextPtr context = xmlXPathNewContext(doc);
    if (!context) {
        LOG_ERROR("Failed to create XPath context for next page");
        xmlFreeDoc(doc);
        return "";
    }

    std::string nextUrl;
    xmlXPathObjectPtr nextBtn = xmlXPathEvalExpression((const xmlChar*)"//li[@class='next']/a/@href", context);
    if (nextBtn && !xmlXPathNodeSetIsEmpty(nextBtn->nodesetval)) {
        xmlChar* href = xmlNodeGetContent(nextBtn->nodesetval->nodeTab[0]);
        nextUrl = BASE_URL + std::string((char*)href);
        xmlFree(href);
    }

    xmlXPathFreeObject(nextBtn);
    xmlXPathFreeContext(context);
    xmlFreeDoc(doc);

    return nextUrl;
}

// Save quotes to CSV file
void saveToCSV(const std::vector<std::map<std::string, std::string>>& quotes, const std::string& filename) {
    if (quotes.empty()) {
        LOG_WARNING("No quotes to save");
        return;
    }

    std::ofstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open file " << filename);
        return;
    }

    // Write header
    const auto& firstQuote = quotes[0];
    bool firstField = true;
    for (const auto& field : firstQuote) {
        if (!firstField) file << ",";
        file << "\"" << field.first << "\"";
        firstField = false;
    }
    file << "\n";

    // Write data
    for (const auto& quote : quotes) {
        firstField = true;
        for (const auto& field : quote) {
            if (!firstField) file << ",";
            file << "\"" << field.second << "\"";
            firstField = false;
        }
        file << "\n";
    }

    file.close();
    LOG_INFO("Saved " << quotes.size() << " quotes to file: " << filename);
}

// Random delay between 1-3 seconds
void randomDelay() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(1.0, 3.0);
    double delay = dis(gen);
    std::this_thread::sleep_for(std::chrono::duration<double>(delay));
}

int main() {
    // Initialize libxml2 and libcurl
    xmlInitParser();
    curl_global_init(CURL_GLOBAL_DEFAULT);

    std::string url = START_URL;
    std::vector<std::map<std::string, std::string>> allQuotes;

    while (!url.empty()) {
        LOG_INFO("Fetching: " << url);
        std::string html = fetchPage(url);
        if (html.empty()) {
            break;
        }

        std::vector<std::map<std::string, std::string>> quotes = parseQuotes(html);
        allQuotes.insert(allQuotes.end(), quotes.begin(), quotes.end());

        url = getNextPage(html);
        if (!url.empty()) {
            randomDelay();
        }
    }

    if (!allQuotes.empty()) {
        saveToCSV(allQuotes, OUTPUT_CSV);
    }
    else {
        LOG_WARNING("No quotes found.");
    }

    // Cleanup
    curl_global_cleanup();
    xmlCleanupParser();

    return 0;
}
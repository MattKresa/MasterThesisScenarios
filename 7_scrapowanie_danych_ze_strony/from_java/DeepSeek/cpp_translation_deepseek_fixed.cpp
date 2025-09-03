#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <chrono>
#include <thread>
#include <random>
#include <curl/curl.h>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>

using namespace std;

class QuoteScraper {
private:
    static const string BASE_URL;
    static const string START_URL;
    static const string OUTPUT_CSV;

    // Callback function for libcurl to write data
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* output) {
        size_t total_size = size * nmemb;
        output->append((char*)contents, total_size);
        return total_size;
    }

    // Helper function to escape CSV fields
    static string escapeCsv(const string& value) {
        if (value.find('"') != string::npos || value.find(',') != string::npos || value.find('\n') != string::npos) {
            string escaped;
            escaped.reserve(value.length() + 2);
            escaped += '"';
            for (char c : value) {
                if (c == '"') escaped += '"';
                escaped += c;
            }
            escaped += '"';
            return escaped;
        }
        return value;
    }

public:
    static void main() {
        vector<map<string, string>> allQuotes;
        string url = START_URL;

        while (!url.empty()) {
            cout << "Fetching: " << url << endl;
            string html = getPage(url);
            if (html.empty()) break;

            xmlDocPtr doc = htmlReadDoc((const xmlChar*)html.c_str(), NULL, NULL, HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
            if (doc) {
                vector<map<string, string>> quotes = parseQuotes(doc);
                allQuotes.insert(allQuotes.end(), quotes.begin(), quotes.end());
                url = getNextPage(doc);
                xmlFreeDoc(doc);
            }
            else {
                break;
            }

            // Sleep for 1-3 seconds
            random_device rd;
            mt19937 gen(rd());
            uniform_int_distribution<> dis(1000, 3000);
            this_thread::sleep_for(chrono::milliseconds(dis(gen)));
        }

        if (!allQuotes.empty()) {
            saveToCsv(allQuotes, OUTPUT_CSV);
            cout << "Saved " << allQuotes.size() << " quotes to file: " << OUTPUT_CSV << endl;
        }
        else {
            cerr << "Warning: No quotes found." << endl;
        }
    }

private:
    static string getPage(const string& url) {
        CURL* curl = curl_easy_init();
        if (!curl) {
            cerr << "Error initializing cURL" << endl;
            return "";
        }

        string response;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            cerr << "Error fetching page " << url << ": " << curl_easy_strerror(res) << endl;
            response.clear();
        }

        curl_easy_cleanup(curl);
        return response;
    }

    static vector<map<string, string>> parseQuotes(xmlDocPtr doc) {
        vector<map<string, string>> quotes;
        xmlXPathContextPtr context = xmlXPathNewContext(doc);
        if (!context) return quotes;

        // Find all quote blocks
        xmlXPathObjectPtr result = xmlXPathEvalExpression((const xmlChar*)"//div[@class='quote']", context);
        if (result && result->nodesetval) {
            for (int i = 0; i < result->nodesetval->nodeNr; ++i) {
                xmlNodePtr quoteNode = result->nodesetval->nodeTab[i];
                map<string, string> quoteData;

                // Get text
                xmlXPathSetContextNode(quoteNode, context);
                xmlXPathObjectPtr textResult = xmlXPathEvalExpression((const xmlChar*)"./span[@class='text']", context);
                if (textResult && textResult->nodesetval && textResult->nodesetval->nodeNr > 0) {
                    xmlChar* text = xmlNodeGetContent(textResult->nodesetval->nodeTab[0]);
                    quoteData["text"] = string((char*)text);
                    xmlFree(text);
                }
                xmlXPathFreeObject(textResult);

                // Get author
                xmlXPathObjectPtr authorResult = xmlXPathEvalExpression((const xmlChar*)".//small[@class='author']", context);
                if (authorResult && authorResult->nodesetval && authorResult->nodesetval->nodeNr > 0) {
                    xmlChar* author = xmlNodeGetContent(authorResult->nodesetval->nodeTab[0]);
                    quoteData["author"] = string((char*)author);
                    xmlFree(author);
                }
                xmlXPathFreeObject(authorResult);

                // Get tags
                xmlXPathObjectPtr tagsResult = xmlXPathEvalExpression((const xmlChar*)"./div[@class='tags']/a[@class='tag']", context);
                if (tagsResult && tagsResult->nodesetval) {
                    string tags;
                    for (int j = 0; j < tagsResult->nodesetval->nodeNr; ++j) {
                        xmlChar* tag = xmlNodeGetContent(tagsResult->nodesetval->nodeTab[j]);
                        if (!tags.empty()) tags += ", ";
                        tags += string((char*)tag);
                        xmlFree(tag);
                    }
                    quoteData["tags"] = tags;
                }
                xmlXPathFreeObject(tagsResult);

                quotes.push_back(quoteData);
            }
        }
        xmlXPathFreeObject(result);
        xmlXPathFreeContext(context);
        return quotes;
    }

    static string getNextPage(xmlDocPtr doc) {
        xmlXPathContextPtr context = xmlXPathNewContext(doc);
        if (!context) return "";

        string nextUrl;
        xmlXPathObjectPtr result = xmlXPathEvalExpression((const xmlChar*)"//li[@class='next']/a/@href", context);
        if (result && result->nodesetval && result->nodesetval->nodeNr > 0) {
            xmlChar* href = xmlNodeGetContent(result->nodesetval->nodeTab[0]);
            nextUrl = BASE_URL + string((char*)href);
            xmlFree(href);
        }

        xmlXPathFreeObject(result);
        xmlXPathFreeContext(context);
        return nextUrl;
    }

    static void saveToCsv(const vector<map<string, string>>& quotes, const string& filename) {
        ofstream outfile(filename);
        if (!outfile.is_open()) {
            cerr << "Error opening file for writing: " << filename << endl;
            return;
        }

        // Write header
        outfile << "text,author,tags" << endl;

        for (const auto& quote : quotes) {
            string text = (quote.find("text") != quote.end()) ? quote.at("text") : "";
            string author = (quote.find("author") != quote.end()) ? quote.at("author") : "";
            string tags = (quote.find("tags") != quote.end()) ? quote.at("tags") : "";

            outfile << escapeCsv(text) << ","
                << escapeCsv(author) << ","
                << escapeCsv(tags) << endl;
        }

        outfile.close();
    }
};

const string QuoteScraper::BASE_URL = "https://quotes.toscrape.com";
const string QuoteScraper::START_URL = QuoteScraper::BASE_URL + "/page/1/";
const string QuoteScraper::OUTPUT_CSV = "quotes.csv";

int main() {
    // Initialize libxml2 and libcurl
    xmlInitParser();
    curl_global_init(CURL_GLOBAL_DEFAULT);

    QuoteScraper::main();

    // Cleanup
    curl_global_cleanup();
    xmlCleanupParser();

    return 0;
}
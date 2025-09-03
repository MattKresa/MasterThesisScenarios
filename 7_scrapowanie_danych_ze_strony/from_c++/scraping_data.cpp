#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <random>
#include <curl/curl.h>
#include <gumbo.h>

const std::string BASE_URL = "https://quotes.toscrape.com";
const std::string START_URL = BASE_URL + "/page/1/";
const std::string OUTPUT_CSV = "quotes.csv";

// Utility to fetch page content
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

std::string get_page(const std::string& url) {
    CURL* curl = curl_easy_init();
    std::string readBuffer;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "[ERROR] Failed to fetch: " << url << " - " << curl_easy_strerror(res) << std::endl;
            readBuffer.clear();
        }
        curl_easy_cleanup(curl);
    }
    return readBuffer;
}

// Helper to extract text from a GumboNode
std::string get_text(GumboNode* node) {
    if (node->type == GUMBO_NODE_TEXT) {
        return std::string(node->v.text.text);
    }
    else if (node->type == GUMBO_NODE_ELEMENT || node->type == GUMBO_NODE_DOCUMENT) {
        std::string contents;
        GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            contents += get_text(static_cast<GumboNode*>(children->data[i]));
        }
        return contents;
    }
    return "";
}

bool node_has_class(GumboNode* node, const std::string& class_name) {
    if (node->type != GUMBO_NODE_ELEMENT) return false;
    GumboAttribute* attr = gumbo_get_attribute(&node->v.element.attributes, "class");
    return attr && std::string(attr->value).find(class_name) != std::string::npos;
}

// Recursively search for quote blocks
void parse_quotes(GumboNode* node, std::vector<std::tuple<std::string, std::string, std::string>>& quotes) {
    if (node->type != GUMBO_NODE_ELEMENT) return;

    if (node_has_class(node, "quote")) {
        std::string text, author;
        std::vector<std::string> tags;

        GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            GumboNode* child = static_cast<GumboNode*>(children->data[i]);
            if (child->type != GUMBO_NODE_ELEMENT) continue;

            if (node_has_class(child, "text")) {
                text = get_text(child);
            }
            else if (node_has_class(child, "tags")) {
                GumboVector* tag_children = &child->v.element.children;
                for (unsigned int j = 0; j < tag_children->length; ++j) {
                    GumboNode* tag_node = static_cast<GumboNode*>(tag_children->data[j]);
                    if (tag_node->type == GUMBO_NODE_ELEMENT &&
                        tag_node->v.element.tag == GUMBO_TAG_A &&
                        node_has_class(tag_node, "tag")) {
                        tags.push_back(get_text(tag_node));
                    }
                }
            }
            else {
                GumboVector* sub_children = &child->v.element.children;
                for (unsigned int j = 0; j < sub_children->length; ++j) {
                    GumboNode* sub_child = static_cast<GumboNode*>(sub_children->data[j]);
                    if (sub_child->type == GUMBO_NODE_ELEMENT &&
                        sub_child->v.element.tag == GUMBO_TAG_SMALL &&
                        node_has_class(sub_child, "author")) {
                        author = get_text(sub_child);
                    }
                }
            }
        }

        std::ostringstream tagStream;
        for (size_t i = 0; i < tags.size(); ++i) {
            tagStream << tags[i];
            if (i + 1 < tags.size()) tagStream << ", ";
        }

        quotes.emplace_back(text, author, tagStream.str());
    }

    // Recurse into children
    GumboVector* children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
        parse_quotes(static_cast<GumboNode*>(children->data[i]), quotes);
    }
}

std::string get_next_page(GumboNode* node) {
    if (node->type != GUMBO_NODE_ELEMENT) return "";

    if (node_has_class(node, "next")) {
        GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            GumboNode* child = static_cast<GumboNode*>(children->data[i]);
            if (child->type == GUMBO_NODE_ELEMENT &&
                child->v.element.tag == GUMBO_TAG_A) {
                GumboAttribute* href = gumbo_get_attribute(&child->v.element.attributes, "href");
                if (href) return BASE_URL + std::string(href->value);
            }
        }
    }

    GumboVector* children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
        std::string next = get_next_page(static_cast<GumboNode*>(children->data[i]));
        if (!next.empty()) return next;
    }

    return "";
}

void save_to_csv(const std::vector<std::tuple<std::string, std::string, std::string>>& quotes, const std::string& filename) {
    std::ofstream file(filename);
    file << "text,author,tags\n";
    for (const auto& [text, author, tags] : quotes) {
        file << "\"" << text << "\",\"" << author << "\",\"" << tags << "\"\n";
    }
    file.close();
}

int main() {
    std::string url = START_URL;
    std::vector<std::tuple<std::string, std::string, std::string>> all_quotes;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(1000, 3000);

    while (!url.empty()) {
        std::cout << "[INFO] Fetching: " << url << std::endl;

        std::string html = get_page(url);
        if (html.empty()) break;

        GumboOutput* output = gumbo_parse(html.c_str());

        std::vector<std::tuple<std::string, std::string, std::string>> quotes;
        parse_quotes(output->root, quotes);
        all_quotes.insert(all_quotes.end(), quotes.begin(), quotes.end());

        url = get_next_page(output->root);
        gumbo_destroy_output(&kGumboDefaultOptions, output);

        std::this_thread::sleep_for(std::chrono::milliseconds(dist(gen)));  // Random delay
    }

    if (!all_quotes.empty()) {
        save_to_csv(all_quotes, OUTPUT_CSV);
        std::cout << "[INFO] Saved " << all_quotes.size() << " quotes to file: " << OUTPUT_CSV << std::endl;
    }
    else {
        std::cerr << "[WARNING] No quotes found." << std::endl;
    }

    return 0;
}

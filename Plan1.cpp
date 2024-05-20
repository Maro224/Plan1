#define CURL_STATICLIB
#include <iostream>
#include <string>
#include <list>
#include <vector>
#include <algorithm>
#include <map>
#include <set>

#include <nlohmann/json.hpp>
// for convenience
using json = nlohmann::json;

#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include "./curl/curl.h"
#pragma comment (lib, "libxml/libxml2.lib")
#ifdef _DEBUG
#pragma comment (lib, "./curl/libcurl_a_debug.lib")
#else
#pragma comment (lib, "./curl/libcurl_a.lib")
#endif
#pragma comment (lib, "Normaliz.lib")
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Wldap32.lib")
#pragma comment (lib, "advapi32.lib")
#pragma comment (lib, "Crypt32.lib")

struct Lesson {
    std::string content;
    std::string ID;
    std::string classID;
    int rowNum;
    int colNum;
    Lesson(std::string content, std::string ID, std::string classID, int rowNum, int colNum)
        : content(content), ID(ID), classID(classID), rowNum(rowNum), colNum(colNum) {}
};

// Define how to convert Lesson to JSON
void to_json(nlohmann::json& j, const Lesson& lesson) {
    j = nlohmann::json{ {"content", lesson.content},
                        {"ID", lesson.ID},
                        {"classID", lesson.classID} };
}

// Callback function to handle libcurl response
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* data) {
    data->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::vector<std::vector<std::vector<Lesson>>> groupLessonsByColAndRow(const std::vector<Lesson>& lessons) {
    // Create a map to group lessons by colNum and rowNum
    std::map<int, std::map<int, std::vector<Lesson>>> colRowGroupedLessons;

    // Populate the map
    for (const Lesson& lesson : lessons) {
        colRowGroupedLessons[lesson.rowNum][lesson.colNum].push_back(lesson);
    }

    // Convert the map to the required nested vector structure
    std::vector<std::vector<std::vector<Lesson>>> groupedLessons;

    for (const auto& colPair : colRowGroupedLessons) {
        std::vector<std::vector<Lesson>> colGroup;
        for (const auto& rowPair : colPair.second) {
            colGroup.push_back(rowPair.second);
        }
        groupedLessons.push_back(colGroup);
    }

    return groupedLessons;
}

std::vector<std::vector<std::vector<Lesson>>> filterAndGroupLessonsByID(const std::vector<Lesson>& lessons, const std::string& id) {
    // Filter lessons by ID
    std::vector<Lesson> filteredLessons;
    for (const Lesson& lesson : lessons) {
        if (lesson.ID == id) {
            filteredLessons.push_back(lesson);
        }
    }

    // Group filtered lessons by colNum and rowNum
    return groupLessonsByColAndRow(filteredLessons);
}

int main() {
    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_ALL);
    CURL* curl = curl_easy_init();
    if (curl) {
        std::list<std::string> links;
        std::vector<std::vector<std::vector<std::vector<Lesson>>>> idLessons;
        std::set<std::string> iDs;
        std::vector<Lesson> lessons;

        // Specify URL to fetch
        std::string url = "http://slowacki.kielce.eu/plan/lista.html";
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // Response data
        std::string htmlContent;

        // Set callback function to write received data into a string
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &htmlContent);

        // Perform the request
        CURLcode res = curl_easy_perform(curl);

        // Check for errors
        if (res != CURLE_OK) {
            std::cerr << "Failed to fetch URL: " << curl_easy_strerror(res) << std::endl;
        }
        else {
            // Initialize libxml2 HTML parser context
            htmlParserCtxtPtr parserContext = htmlCreatePushParserCtxt(NULL, NULL, NULL, 0, NULL, XML_CHAR_ENCODING_UTF8);

            // Parse the HTML content
            htmlParseChunk(parserContext, htmlContent.c_str(), htmlContent.length(), 0);

            // Get the document from the parser context
            htmlDocPtr document = parserContext->myDoc;

            // Iterate through <a> tags to extract links
            xmlNodePtr currentNode = xmlDocGetRootElement(document);
            for (xmlNodePtr node = currentNode->children; node; node = node->next) {
                if (node->type == XML_ELEMENT_NODE && xmlStrEqual(node->name, (const xmlChar*)"body")) {
                    for (xmlNodePtr liNode = node->children; liNode; liNode = liNode->next) {
                        if (liNode->type == XML_ELEMENT_NODE && xmlStrEqual(liNode->name, (const xmlChar*)"ul")) {
                            for (xmlNodePtr aNode = liNode->children; aNode; aNode = aNode->next) {
                                if (aNode->type == XML_ELEMENT_NODE && xmlStrEqual(aNode->name, (const xmlChar*)"li")) {
                                    xmlNodePtr linkNode = aNode->children;
                                    while (linkNode && linkNode->type != XML_ELEMENT_NODE)
                                        linkNode = linkNode->next;
                                    if (linkNode && xmlStrEqual(linkNode->name, (const xmlChar*)"a")) {
                                        xmlChar* href = xmlGetProp(linkNode, (const xmlChar*)"href");
                                        if (href)
                                            links.push_back(std::string((const char*)href));
                                        xmlFree(href);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            // Cleanup libxml2 resources
            xmlFreeDoc(document);
            xmlCleanupParser();
        }

        // Cleanup libcurl
        curl_easy_cleanup(curl);

        for (const std::string& link : links) {
            CURL* curl = curl_easy_init();
            if (curl) {
                // Specify URL to fetch
                std::string url = "http://slowacki.kielce.eu/plan/" + link;
                curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

                // Response data
                std::string htmlContent;

                // Set callback function to write received data into a string
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &htmlContent);

                // Perform the request
                CURLcode res = curl_easy_perform(curl);

                // Check for errors
                if (res != CURLE_OK) {
                    std::cerr << "Failed to fetch URL: " << curl_easy_strerror(res) << std::endl;
                }
                else {
                    // Initialize libxml2 HTML parser context
                    htmlParserCtxtPtr parserContext = htmlCreatePushParserCtxt(NULL, NULL, NULL, 0, NULL, XML_CHAR_ENCODING_UTF8);

                    // Parse the HTML content
                    htmlParseChunk(parserContext, htmlContent.c_str(), htmlContent.length(), 0);

                    // Get the document from the parser context
                    htmlDocPtr document = parserContext->myDoc;

                    // Parse the HTML document
                    xmlDocPtr doc = document;
                    if (doc == NULL) {
                        std::cerr << "Error: Unable to parse HTML document." << std::endl;
                        return 1;
                    }

                    // Create XPath context
                    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
                    if (xpathCtx == NULL) {
                        std::cerr << "Error: Unable to create XPath context." << std::endl;
                        xmlFreeDoc(doc);
                        return 1;
                    }

                    // Compile XPath expression to select all tr elements
                    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(BAD_CAST("//tr"), xpathCtx);
                    if (xpathObj == NULL) {
                        std::cerr << "Error: Unable to evaluate XPath expression." << std::endl;
                        xmlXPathFreeContext(xpathCtx);
                        xmlFreeDoc(doc);
                        return 1;
                    }

                    // Iterate through the results (tr elements)
                    xmlNodeSetPtr nodes = xpathObj->nodesetval;
                    if (nodes) {
                        for (int i = 0; i < nodes->nodeNr; ++i) {
                            xmlNodePtr trNode = nodes->nodeTab[i];
                            int rowNum = 0;
                            int colNum = 0;
                            // Iterate through the td elements within the tr
                            for (xmlNodePtr tdNode = trNode->children; tdNode; tdNode = tdNode->next) {
                                if (tdNode->type == XML_ELEMENT_NODE && xmlStrcmp(tdNode->name, BAD_CAST("td")) == 0) {
                                    xmlAttrPtr classAttr = xmlHasProp(tdNode, BAD_CAST("class"));
                                    if (classAttr && xmlStrcmp(classAttr->children->content, BAD_CAST("nr")) == 0) {
                                        const char* rowNumChar = (const char*)xmlNodeGetContent(tdNode);
                                        rowNum = atoi(rowNumChar);
                                        colNum = 0;
                                    }
                                    if (classAttr && xmlStrcmp(classAttr->children->content, BAD_CAST("l")) == 0) {
                                        // Extract the content of td
                                        std::string content;
                                        // Found the td element with class l
                                        colNum++;

                                        // Iterate through child nodes of tdNode
                                        xmlNodePtr childNodes = tdNode->children;
                                        xmlAttrPtr childClassAttr = xmlHasProp(childNodes, BAD_CAST("class"));
                                        if (childClassAttr) {
                                            xmlChar* childContent = xmlNodeGetContent(tdNode);
                                            content = (const char*)childContent;
                                            xmlFree(childContent);
                                            if (content != "") {
                                                std::string sub1 = content.substr(0, content.find(" "));
                                                std::string sub2 = content.substr(content.find(" ") + 1);
                                                std::string sub3 = sub2.substr(0, sub2.find(" "));
                                                std::string sub4 = sub2.substr(sub2.find(" ") + 1);

                                                Lesson lesson = Lesson(sub1, sub3, sub4, rowNum, colNum);
                                                lessons.push_back(lesson);
                                            }
                                        }
                                        else {
                                            for (xmlNodePtr childNode = tdNode->children; childNode; childNode = childNode->next) {
                                                if (childNode->type == XML_ELEMENT_NODE) {
                                                    // Child node does not have a class attribute
                                                    xmlChar* childContent = xmlNodeGetContent(childNode);
                                                    content = (const char*)childContent;
                                                    xmlFree(childContent);
                                                    // Add the extracted content to the lessons vector
                                                    if (content != "") {
                                                        std::string sub1 = content.substr(0, content.find(" "));
                                                        std::string sub2 = content.substr(content.find(" ") + 1);
                                                        std::string sub3 = sub2.substr(0, sub2.find(" "));
                                                        std::string sub4 = sub2.substr(sub2.find(" ") + 1);

                                                        Lesson lesson = Lesson(sub1, sub3, sub4, rowNum, colNum);
                                                        lessons.push_back(lesson);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Clean up
                    xmlXPathFreeObject(xpathObj);
                    xmlXPathFreeContext(xpathCtx);
                    xmlFreeDoc(document);
                    xmlCleanupParser();
                }
                curl_easy_cleanup(curl);
            }
        }
        
        for (const Lesson& lesson : lessons) {
                iDs.insert(lesson.ID);
        }
        for (const std::string& id : iDs) {
            std::vector<std::vector<std::vector<Lesson>>> groupedLessons = filterAndGroupLessonsByID(lessons, id);
            idLessons.push_back(groupedLessons);
        }

        // Convert the list of lists of lessons to JSON
        nlohmann::json json_lessons = idLessons;

        // Convert JSON to string
        std::string json_string = json_lessons.dump();

        // Initialize CURL for POST request
        curl = curl_easy_init();
        if (curl) {
            // Set the URL for the POST request
            curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:5000/lessons");

            // Set the Content-Type to application/json
            struct curl_slist* headers = NULL;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            // Set the POST data
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_string.c_str());

            // Perform the request, res will get the return code
            res = curl_easy_perform(curl);

            // Check for errors
            if (res != CURLE_OK) {
                std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            }

            // Cleanup
            curl_easy_cleanup(curl);
        }
    }

    // Cleanup libcurl global resources
    curl_global_cleanup();

    return 0;
}

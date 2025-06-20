#include "document.h"
#include <iostream>
#include <string>

std::ostream &operator<<(std::ostream &output, Document document)
{
    using namespace std;
    output << "{ "s
           << "document_id = "s << document.id << ", "s
           << "relevance = "s << document.relevance << ", "s
           << "rating = "s << document.rating << " }"s;
    return output;
}
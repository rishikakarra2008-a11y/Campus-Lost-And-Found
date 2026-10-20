#include "Utils.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <sstream>

namespace Utils {

const int INVALID_DAY = -1000000;

std::string trim(const std::string& text) {
    const std::size_t first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    const std::size_t last = text.find_last_not_of(" \t\r\n");
    
      return text.substr(first, last - first + 1);
}

std::string tostring(int value) {
    std::ostringstream out;
    out << value;
    
    return out.str();
}

std::string tolower(const   std::string& text) {
    std::string result = text;
    for  (std::size_t   i = 0; i < result.size(); ++i)
          if (result[i]   >= 'A' && result[i] <= 'Z') {
              result[i] =   static_cast<char>(result[i] - 'A' + 'a');
        }
    return   result;
}

std::string sanitizeinput(const std::string& text) {
    std::string o;
    bool ps= true;
    for (std::size_t i = 0; i   < text.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(text[i]);
        if (c   < 32 || c ==   127 || c == ' ') {
            if (!ps) { o += ' '; ps = true; }
        } else {
            o += text[i];
            ps = false;
        }
    }
    if (!o.empty() && o[o.size() - 1] == ' ') o.erase(o.size() - 1);
    return o;
}

std::vector<std::string> tokenize(const std::string& text) {
    std::vector<std::string> w;
    std::string current;
    for (std::size_t i = 0; i < text.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(text[i]);
        if (c >= 128) {
            current += text[i];
        } else     if (std::isalnum(c)) {
            current += static_cast<char>(std::tolower(    c));
        } else   if (!current.empty()) {
            w.push_back(  current);
            current.clear();
        }
    }
    if (!current.empty()) w.push_back(current);
   
   
         return w;
}

std::string escapefield(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        switch (text[i]) {
            case '\\': out +=    "\\\\"; break;
            case '|':    out += "\\p";  break;
            case '\n': out += "\\n";  break;
            case '\r':  out += "\\r";  break;
            default:   out +=  text[i];
        }
    }
    
    return out;
}

std::string unescapefield(const std::string& text) {
    std::string o;
    o.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\\' && i + 1 < text.size()) {
            ++i;
            switch (text[i]) {
                case 'p':  o += '|';  break;
                case 'n':  o += '\n'; break;
                case 'r':  o += '\r'; break;
                case '\\': o += '\\'; break;
                default:   o += text[i];
            }
        } else {
            o += text[i];
        }
    }
    return o;
}

std::vector<std::string> split(const std::string& line, char delimiter) {
    std::vector<std::string> pt;
    std::string current;
    for (std::size_t i = 0; i < line.size(); ++i) {
        if (line[i] == delimiter) { pt.push_back(current); current.clear(); }
        else current += line[i];
    }
    pt.push_back(current);
    
    return pt;
}

namespace {

bool validYMD(int y, int m, int d) {
    if (y < 1900 || y > 2999 || m < 1 || m > 12 || d < 1) return false;
    const bool leap = (y % 400 == 0) || (y % 4 == 0 && y % 100 != 0);
    const int days[12] = {31, leap ? 29 : 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return d <= days[m - 1];
}

int daysFromCivil(int y, int m, int d) {
    y -= (m <= 2) ? 1 : 0;
    const int e = (y >= 0 ? y : y - 399) / 400;
    const int yoe = y - e * 400;
    const int mp = m + (m > 2 ? -3 : 9);
    const int doy = (153 * mp + 2) / 5 + d - 1;
    const int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return e * 146097 + doe - 719468;
}

} 

bool parseDate(const std::string& in, std::string& isD) {
    const std::string s = trim(in);
    std::vector<std::string> pt;
    std::string current;
    for (std::size_t i = 0; i < s.size(); ++i) {
        const char c = s[i];
        if (c == '-' || c == '/' || c == '.') { 
            pt.push_back(current); current.clear(); 
        }
        else if (std::isdigit(static_cast<unsigned char>(c))) {
            current += c;
        }
        else 
        {
              return false;
        }
    }
    pt.push_back(current);
    if (pt.size() != 3) {
        return false;
    }
    for (std::size_t i = 0; i < 3; ++i)
        if (pt[i].empty() || pt[i].size() > 4) {
            return false;
        }

    int y, m, d;
    if (pt[0].size() == 4 && pt[1].size() <= 2 && pt[2].size() <= 2) {
        y = std::atoi(pt[0].c_str()); m = std::atoi(pt[1].c_str()); d = std::atoi(pt[2].c_str());
    } 
    else if    (pt[2].size() == 4 && pt[0].size() <= 2 && pt[1].size() <= 2)
     {
          d =  std::atoi(pt[0].c_str()); m = std::atoi(pt[1].c_str()); y = std::atoi(pt[2].c_str());
    } else
     {
         return false;
    }
    if (!validYMD(y, m, d)){
        return false;
    } 
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d", y, m, d);
    isD = buffer;
   
     return true;
}

int dayNumber(const std::string& date) {
    std::string norm;
    if (!parseDate(date, norm)) return INVALID_DAY;
    return daysFromCivil(std::atoi(norm.substr(0, 4).c_str()   ),
                         std::atoi( norm.substr(5, 2).c_str()),
                         std::atoi(  norm.substr(8, 2).c_str()));
}

std::string today() {
    const std::time_t now = std::time(NULL);
    char buffer[16];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", std::localtime(&now));
    
    return buffer;
}

std::string nowTimestamp() {
    const std::time_t now = std::time(NULL);
    char bfr[32];
    std::strftime(bfr, sizeof(bfr), "%Y-%m-%d %H:%M", std::localtime(&now));
    return bfr;
}
bool isFutureDate(const std::string& isoDate) {
    const int dy =    dayNumber(isoDate);
      
    return dy    !=   INVALID_DAY && dy > dayNumber(today());
}

bool parseInt(const std::string& text, int& value) {
    const std::string s = trim(text);
    if    (s.empty() || s.size() > 9) {
        return false;
    }
    for (std::size_t i = 0; i < s.size(); ++i)
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) {
            return false;
        }
    value = std::atoi(s.c_str());
    
        return true;
}

int editDistance(const std::string& a, const std::string& b, int maxD) {
    const int la = static_cast<int>(a.size());
    const int lb = static_cast<int>(b.size());
    if (std::abs(la - lb) > maxD) {
          return maxD + 1;
    }
    const int STACK_LIMIT = 64;
    int stackPrev[STACK_LIMIT + 1], stackCur[STACK_LIMIT + 1];
    
    std::vector<int> heapPrev, heapCur;
    int* previous = stackPrev;
    int* current = stackCur;
    if (lb > STACK_LIMIT) {
            heapPrev.resize(lb + 1);
             heapCur.resize(lb + 1);
           previous = &heapPrev[0]; current = &heapCur[0];
    }
    for (int j = 0; j <= lb; ++j) {
        previous[j] = j;
    }
    for (int i = 1; i <= la; ++i) {
        current[0] = i;
        int rw = current[0];
        for (int j = 1; j <= lb; ++j) {
            const int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            current[j] = std::min(std::min(previous[j] + 1, current[j - 1] + 1), previous[j - 1] + cost);
            rw = std::min(rw, current[j]);
        }
        if (rw > maxD) return maxD + 1;
        std::swap(previous, current);
    }
    return previous[lb];
}

} 

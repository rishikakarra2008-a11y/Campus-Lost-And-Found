#ifndef UTILS_H
#define UTILS_H

#include <cstddef>
#include <string>
#include <vector>


namespace utils {

extern const int INVALID_DAY;

std::string trim(const std::string& text);
std::string tostring(int value);          
std::string tolower(const std::string& text);


std::string sanitizeinput(const std::string& text);


std::vector<std::string> tokenize(const std::string& text);


std::string escapefield(const std::string& text);
std::string unescapefield(const std::string& text);
std::vector<std::string> split(const std::string& line, char delimiter);


bool parsedate(const std::string& input, std::string& isoDate);
int daynumber(const std::string& date);   
std::string today();                      
std::string nowtimestamp();               
bool isfuturedate(const std::string& isoDate);

bool parseint(const std::string& text, int& value); 


int editdistance(const std::string& a, const std::string& b, int maxDistance);

} 

#endif

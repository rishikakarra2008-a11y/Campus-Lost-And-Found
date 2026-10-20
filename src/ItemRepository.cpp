#include "ItemRepository.h"
#include "Utils.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace {
const char* const HEADER = "#CampusFind data v2";
const char* const TAG = "R2";
}

itemrepository::itemrepository(const std::string& dataFile) : p(dataFile) {}

const std::string& itemrepository::getPath() const { return p; }

std::string itemrepository::formatRecord(const Item& it) {
    std::ostringstream out;
    out << TAG << '|' << it.getId() << '|'
        << Item::typeToString(it.getType()) << '|'
        <<   Item::statusToString(it.getStatus()) << '|'
        << Utils::escapeField(it.getName()) << '|'
        <<    Utils::escapeField(it.getCategory()) << '|'
        << Utils::escapeField(it.getDescription()) << '|'
        << Utils::escapeField(it.getLocation())   << '|'
        << it.getDate() << '|'
        << Utils::escapeField(it.getReporter()) << '|'
        << Utils::escapeField(it.getCreatedAt())   <<  '|'
        << it.getLinkedId() << '|'
        <<  Utils::escapeField(it.getResolutionNote());
   
        return out.str();
}

bool itemrepository::parseRecord(const std::string& line, Item& item, bool& legacy) const {
      legacy = false;
    const std::vector<std::string> f = Utils::split(line, '|');

    if (f.size() == 13 && f[0] == TAG) {
        int id = 0, linked = 0;
        Itemtype t;
          Itemstatus s;
        std::string date;
        if (!Utils::parseInt(f[1], id) || id <= 0) {
            return false;
        }
        if (!Item::typeFromString(f[2], t) ||  !Item::statusFromString(f[3], s)) {
            return false;
        }
        
        if (!Utils::parseDate(f[8], date)) return false;
        if (!Utils::parseInt(f[11], linked)) return false;
        const std::string name = Utils::unescapeField(f[4]);
        
        if (name.empty()) return false;
        item = Item(id, t, s, name, Utils::unescapeField(f[5]), Utils::unescapeField(f[6]),
                    Utils::unescapeField(f[7]), date, Utils::unescapeField(f[9]),
                    Utils::unescapeField(f[10]), linked, Utils::unescapeField(f[12]));
        
        
          return true;
    }

    
    if (f.size() == 8) {
        int id = 0;
        
        Itemtype t;
          Itemstatus s;
        std::string date;
        if (!Utils::parseInt(f[0], id) || id <= 0){
            return false;
        } 
          if (!Item::typeFromString(f[6], t) || !Item::statusFromString(f[7], s)){
            return false;
        } 
        if (!Utils::parseDate(f[5], date) || f[1].empty()){
             return false;
        }
        item = Item(id, t, s, f[1], f[2], f[3], f[4], date, "(imported)",
                    "(imported)", 0, "");
        legacy = true;
        
        return true;
    }
     
    return false;
}

bool itemrepository::fileIsEmptyOrMissing() const {
    std::ifstream in(p.c_str(), std::ios::binary);
    if (!in.is_open()){
        return true;
    } 
    
    return in.peek() == std::ifstream::traits_type::eof();
}

void itemrepository::logRejected(const std::string& line) const {
    std::ofstream out((p + ".rejected").c_str(), std::ios::app);
    if (out.is_open()) out << line << '\n';
}

std::vector<Item> itemrepository::loadAll(LoadReport& report) const {
    report = LoadReport();
    std::vector<Item> it;
    std::ifstream in(p.c_str());
    if (!in.is_open()){
        return it;
    }  

    std::unordered_map<int, std::size_t> position;
    
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line[line.size() - 1] == '\r'){
            line.erase(line.size() - 1);
        } 
        
        if (line.empty() || line[0] == '#'){
            continue;
        } 

        Item i;
        bool legacy = false;
        if (!parseRecord(line, i, legacy)) {
            ++report.rejectedLines;
            logRejected(line);
            continue;
        }
        ++report.recordLines;
        if (legacy) ++report.legacyLines;

        std::unordered_map<int, std::size_t>::iterator found = position.find(i.getId());
        if (found != position.end()) {
            it[found->second] = i;                 
        } else {
            position[i.getId()] = it.size();
            it.push_back(i);
        }
    }
    report.liveRecords = it.size();
    return it;
}

bool itemrepository::append(const std::vector<Item>& records, std::string& error) const {
    if (records.empty()) return true;
    const bool n = fileIsEmptyOrMissing();

    std::string block;
    if (n) { block += HEADER; block += '\n'; }
    for (std::size_t i = 0; i < records.size(); ++i) {
         block += formatRecord(records[i]); block += '\n';
         }

    std::ofstream out(p.c_str(), std::ios::app | std::ios::binary);
    if (!out.is_open()) { 
        error = "Cannot open data file '" + p + "' for writing.";
         return false; 
        }

    out.write(block.data(), static_cast<std::streamsize>(block.size()));
   
    out.flush();
    
    if (!out.good()) { 
        error = "Writing to '" + p + "' failed (disk full or read-only?)."; 
        return false; 
    }

    return true;
}

bool itemrepository::compact(const std::vector<Item>& items, std::string& error) const {
    const std::string tmp = p + ".tmp";
       const std::string bak = p + ".bak";
    {
        std::ofstream out(tmp.c_str(), std::ios::trunc | std::ios::binary);
        if (!out.is_open()) { error = "Cannot create '" + tmp + "'."; return false; }
        out << HEADER << '\n';
        for (std::size_t i = 0; i < items.size(); ++i) out << formatRecord(items[i]) << '\n';
        
        out.flush();
        
        if (!out.good()) { 
            out.close(); std::remove(tmp.c_str()); error = "Writing '" + tmp + "' failed."; 
            return false;
         }
    }

    std::remove(bak.c_str());
    const bool hadOriginal = !fileIsEmptyOrMissing();
    if (hadOriginal && std::rename(p.c_str(), bak.c_str()) != 0) {
        std::remove(tmp.c_str());
          error = "Could not back up the data file before compaction.";
        return false;
    }
    
    if (std::rename(tmp.c_str(), p.c_str()) != 0) {
        if (hadOriginal) std::rename(bak.c_str(), p.c_str()); // restore original
        std::remove(tmp.c_str());
           error = "Could not replace the data file; the original was restored.";
        
        return false;
    }
    
    return true;
}

bool itemrepository::shouldCompact(const LoadReport& report) {
       return report.recordLines > 2 * report.liveRecords + 1000;
}

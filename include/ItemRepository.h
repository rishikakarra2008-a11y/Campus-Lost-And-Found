#ifndef ITEM_REPOSITORY_H
#define ITEM_REPOSITORY_H

#include "Item.h"

#include <cstddef>
#include <string>
#include <vector>


class itemrepository {
public:
    struct LoadReport {
        std::size_t recordLines;  
        std::size_t liveRecords;   
        std::size_t rejectedLines; 
        std::size_t legacyLines;   
        LoadReport() : recordLines(0), liveRecords(0), rejectedLines(0), legacyLines(0) {}
    };

private:
    std::string p;

    bool parseRecord(const std::string& line, Item& item, bool& legacy) const;
      static std::string formatRecord(const Item& item);
     bool fileIsEmptyOrMissing() const;
    void logRejected(const std::string& line) const;

public:
    explicit itemrepository(const std::string& dataFile);

       const std::string& getPath() const;

    
    std::vector<Item> loadAll(LoadReport& report) const;

      bool append(const std::vector<Item>& records, std::string& error) const;

    bool compact(const std::vector<Item>& items, std::string& error) const;

    static bool shouldCompact(const LoadReport& report);
};

#endif

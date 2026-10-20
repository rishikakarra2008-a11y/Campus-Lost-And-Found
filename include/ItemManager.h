#ifndef ITEM_MANAGER_H
#define ITEM_MANAGER_H

#include "Item.h"
#include "ItemRepository.h"
#include "MatchingService.h"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

struct searchfilter {
    bool filterType;
    Itemtype type;
    bool openOnly;
    searchfilter() : filterType(false), type(Itemtype::Lost), openOnly(false) {}
};

struct statistics {
    std::size_t total, lost, found, openLost, openFound, claimedFound, closedLost, closedOther;
    double recoveryRatePercent; 

    std::vector<std::pair<std::string, std::size_t> > topCategories;
    statistics() : total(0), lost(0), found(0), openLost(0), openFound(0), claimedFound(0),
                   closedLost(0), closedOther(0), recoveryRatePercent(0.0) {}
};

class ItemManager {
public:
    struct startupinfo {
        std::size_t loaded, rejected, imported, repaired;
        bool compacted;
        std::string warning;
        startupinfo() : loaded(0), rejected(0), imported(0), repaired(0), compacted(false) {}
    };

private:
    ItemRepository repository;
    MatchingService matcher;
    std::vector<Item> items;
    std::vector<std::string> haystacks;          
    std::unordered_map<int, std::size_t> indexById;
    int nextId;
    startupinfo startup;

    static std::string makehaystack(const Item& i);
    void indexitem(std::size_t pos);
    std::size_t reconcile();

public:
    explicit ItemManager(const std::string& datafile);

    const startupinfo& getStartupInfo() const;
    std::size_t size() const;

    int report(Itemtype t, const std::string& n, const std::string& cat,
               const std::string& des, const std::string& l,
               const std::string& da, const std::string& r, std::string& e);

    int finduplicate(Itemtype t, const std::string& n, const std::string& cat,
                      const std::string& l, const std::string& da) const;

    const Item* find(int id) const;
    std::vector<const Item*> list(const searchfilter& filter) const;               
    std::vector<const Item*> search(const std::string& query, const searchfilter& filter) const;

    std::vector<MatchResult> matchesFor(int id, double   minScore =  MatchingService::DEFAULT_MIN_SCORE,
                                        std::size_t limit = 10) const;
    std::vector<MatchResult>  allMatches(double minScore = MatchingService::DEFAULT_MIN_SCORE,
                                        std::size_t limit = 20, bool* truncated = NULL) const;
    bool comparePair(int lostId,int foundId, MatchResult& r) const;

    bool claim(int lostId, int foundId, const std::string& claimant,
               const std::string& verification, std::string& error);
    bool close(int id, const std::string& reason, std::string& error);
    bool edit(int id, const std::string& n, const std::string& ca,
              const std::string& des, const std::string& l,
              const std::string& da, const std::string& r, std::string& e);

    struct statistics statistics() const;
    bool exportCsv(const std::string& p, std::string& e) const;
    bool compactNow(std::string& e);
};

#endif

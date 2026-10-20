#include "ItemManager.h"
#include "Utils.h"

#include <algorithm>
#include <fstream>
#include <map>

namespace {

std::string csvQuote(const std::string& t) {
    std::string o = "\"";
    for (std::size_t i = 0; i < t.size(); ++i) {
        if (t[i] == '"') o += "\"\"";
        else o += t[i];
    }
   
    if (o.size() > 1 && (o[1] == '=' || o[1] == '+'   || o[1] == '-' || o[1] == '@')){
         o.insert(1, "'");
    } 
    o += '"';
    return o;
}

} 

ItemManager::ItemManager(const std::string& dataFile)   : repository(dataFile), nextId(1) {
    itemrepository::LoadReport rep;
    items = repository.loadAll(rep);
    
    startup.loaded = items.size();
    
    startup.rejected = rep.rejectedLines;
    startup.imported = rep.legacyLines;

    int high = 0;
    
    for (std::size_t i = 0; i < items.size(); ++i) {
        high = std::max(high, items[i].getId());
    }
    nextId = high + 1;

    haystacks.resize(items.size());
    for (std::size_t i = 0; i < items.size(); ++i) indexitem(i);

    startup.repaired = reconcile();

    std::string error;
    if (rep.legacyLines > 0 || itemrepository::shouldCompact(rep)) {
        if (repository.compact(items, error)) 
        {
            startup.compacted = true;
        } 
        else 
        {
            startup.warning = error;
        }
    }
}

const ItemManager::startupinfo& ItemManager::getStartupInfo() const { 
    return startup; 
 }
std::size_t ItemManager::size() const { 
    return items.size();
    }

std::string ItemManager::makehaystack(const Item& i) {
    return Utils::toLower(i.getName() + " " + i.getCategory() + " " +
                              i.getDescription() + " " + i.getLocation());
}

void ItemManager::indexitem(std::size_t pos) {
    const Item& i = items[pos];
    indexById[i.getId()] = pos;
    
    haystacks[pos] =  makehaystack(i);
    matcher.add(i);
}


std::size_t ItemManager::reconcile() {
    std::vector<Item> f;
    for (std::size_t i = 0; i < items.size(); ++i) {
        const Item& a = items[i];
        if (a.getLinkedId() == 0 || a.isOpen()){
            continue;
        } 
        
        std::unordered_map<int, std::size_t>::const_iterator it = indexById.find(a.getLinkedId());
        
         if (it == indexById.end()) {
            continue;
         }
        
         const Item& p = items[it->second];
        
         if (!p.isOpen() || p.getType() == a.getType()) {
             continue;
        }
        Item fixed = p;
        fixed.setStatus(fixed.isLost() ?  Itemstatus::Closed : Itemstatus::Claimed);
         fixed.setLinkedId(  a.getId());
        fixed.setResolutionNote( a.getResolutionNote());
        f.push_back(fixed);
    }
    
    if (f.empty()) {
        return 0;
    }
      std::string error;
    
      if (!repository.append(f, error)) { 
         startup.warning = error; return 0; 
    }
    
      for (std::size_t i = 0; i < f.size(); ++i) {
          const std::size_t pos = indexById[f[i].getId()];
          items[pos] = f[i];
        indexitem(pos);
    }
   
    return f.size();
}

int ItemManager::report(Itemtype t, const std::string& n, const std::string& cat,
                        const std::string& des, const std::string& l,
                        const std::string& date, const std::string& rep, std::string& error) {
    std::string isoDate;
    
      if (n.empty() || cat.empty() || des.empty() || l.empty() || rep.empty()) {
        error = "Name, category, description, location and reporter contact are all required.";
        return 0;
    }
    if (!Utils::parseDate(date, isoDate)) { 
        error = "Invalid date. Use YYYY-MM-DD or DD-MM-YYYY."; return 0;
     }

       if (Utils::isFutureDate(isoDate)) {
         error = "The date cannot be in the future."; return 0; 
    }

       Item item(nextId, t, n, cat, des, l, isoDate, rep, Utils::nowTimestamp());
    std::vector<Item> batch(1, item);
       if (!repository.append(batch, error)) {
        return 0;     
    }

      items.push_back(item);
    haystacks.push_back(std::string());
    
      indexitem(items.size() - 1);
    
    return nextId++;
}

int ItemManager::finduplicate(Itemtype t, const std::string& n, const std::string& cat,
                               const std::string& l, const std::string& d) const {
    std::string isoDate;
    if (!Utils::parseDate(d, isoDate)){
        return 0;
    } 
    const std::string n = Utils::toLower(n), c = Utils::toLower(cat), l = Utils::toLower(l);
    for (std::size_t i = items.size(); i-- > 0;) {
        
        const Item& it = items[i];
        
        if (it.getType() != t || !it.isOpen() || it.getDate() != isoDate) {
            continue;
        }
           if (Utils::toLower(it.getName()) == n && Utils::toLower(it.getCategory()) == c &&
                Utils::toLower(it.getLocation()) == l){
                    return it.getId();
            } 
    }
    return 0;
}

const Item* ItemManager::find(int ID) const {
    std::unordered_map<int, std::size_t>::const_iterator it = indexById.find(ID);
      
       return it == indexById.end() ? NULL : &items[it->second];
}

std::vector<const Item*> ItemManager::list(const searchfilter& filter) const {
    std::vector<const Item*> result;
    for (std::size_t i = items.size(); i-- > 0;) {
        const Item& it = items[i];
        if (filter.filterType && it.getType() != filter.type) continue;
        if (filter.openOnly && !it.isOpen()) continue;
        result.push_back(&it);
    }
    
    return result;
}

std::vector<const Item*> ItemManager::search(const std::string& query, const searchfilter& filter) const {
    std::vector<std::string>   terms = Utils::tokenize(query);
    
    std::vector<const Item*>   result;
    if (terms.empty()) {
        return result;
    }

    int idQ = 0;
      const bool numeric = Utils::parseInt(query, idQ);

    for (std::size_t i = items.size(); i-- > 0;) {
           const Item& it = items[i];
        if (filter.filterType && it.getType() != filter.type){
            continue;
        } 
        
        if (filter.openOnly && !it.isOpen()) {
            continue;
        }
          bool all = true;
        for (std::size_t t = 0; t < terms.size() && all; ++t)
            if (haystacks[i].find(terms[t]) == std::string::npos) all = false;
        if (all || (numeric && it.getId() == idQ)) result.push_back(&it);
    }
    return result;
}

std::vector<MatchResult> ItemManager::matchesFor(int id, double minScore, std::size_t limit) const {
      const Item* i = find(id);
    if (!i){
         return std::vector<MatchResult>();
    }
    return matcher.findMatchesFor(*i, minScore, limit);
}

std::vector<MatchResult> ItemManager::allMatches(double minScore, std::size_t limit, bool* truncated) const {
      return matcher.findAllMatches(minScore, limit, 1000, truncated);
}

bool ItemManager::comparePair(int lostId, int foundId, MatchResult& result) const {
    const Item* lost = find(lostId);
   
      const Item* found = find(foundId);
    if (!lost || !found || !lost->isLost() || found->isLost()){
         return false;
    }
    result = matcher.compare(*lost, *found);
    return true;
}

bool ItemManager::claim(int lostid, int foundid, const std::string& claimant,
                        const std::string& verification, std::string& error) {
    const Item* found = find(foundid);
    if (!found) { 
        error = "No report exists with ID " + Utils::toString(foundid) + "."; return false;
     }
    
        if (found->isLost()) { 
        error = "Report #" + Utils::toString(foundid) + " is a LOST report; a FOUND report ID is needed."; return false;
     }
    if (!found->isOpen()) {
         error = "Found report #" + Utils::toString(foundid) + " is already " + Item::statusToString(found->getStatus()) + "."; return false; 
    }

    if (claimant.empty() || verification.empty()) { 
        error = "Claimant details and verification notes are required."; return false;
     }

    const Item* lost = NULL;
    if (lostid != 0) {
        lost = find(lostid);
        if (!lost) {
             error = "No report exists with ID " + Utils::toString(lostid) + "."; return false;
             }
        if (!lost->isLost()) {
             error = "Report #" + Utils::toString(lostid) + " is a FOUND report; a LOST report ID is needed."; return false;
            }
        if (!lost->isOpen()) { 
            error = "Lost report #" + Utils::toString(lostid) + " is already " + Item::statusToString(lost->getStatus()) + ".";
              return false;
         }
    }

    const std::string n = "Handed to: " + claimant + " | Verified by: " + verification;
    Item nF = *found;
    nF.setStatus(Itemstatus::Claimed);
    nF.setLinkedId(lostid);
    nF.setResolutionNote(n);

    std::vector<Item> batch(1, nF);
    Item nL;
    if (lost) {
        nL = *lost;
        nL.setStatus(Itemstatus::Closed);
        nL.setLinkedId(foundid);
        nL.setResolutionNote("Recovered via found report #" + Utils::toString(foundid) + ". " + n);
        batch.push_back(nL);
    }
    if (!repository.append(batch, error)) return false;  

    const std::size_t foundPos = indexById[foundid];
    items[foundPos] = nF;
    indexitem(foundPos);
    if (lost) {
        const std::size_t lostPos = indexById[lostid];
        items[lostPos] = nL;
        indexitem(lostPos);
    }
    return true;
}

bool ItemManager::close(int id, const std::string& reason, std::string& error) {
    const Item* item = find(id);
    if (!item) { error = "No report exists with ID " + Utils::toString(id) + "."; return false; }
    if (!item->isOpen()) { 
        error = "Report #" + Utils::toString(id) + " is already " + Item::statusToString(item->getStatus()) + "."; 
        return false;
     }
    if (reason.empty()) { 
        error = "A reason is required to close a report.";
         return false;
         }

    Item up = *item;
    
    up.setStatus(Itemstatus::Closed);
    up.setResolutionNote("Closed without match: " + reason);
    std::vector<Item> batch(1, up);
    if (!repository.append(batch, error)) return false;

    const std::size_t pos = indexById[id];
    items[pos] = up;
    indexitem(pos);
    return true;
}

bool ItemManager::edit(int id, const std::string& n, const std::string& cat,
                       const std::string& des, const std::string& loc,
                       const std::string& da, const std::string& r, std::string& error) {
    const Item* item = find(id);
    if (!item) {
         error = "No report exists with ID " + Utils::toString(id) + "."; 
         return false; 
        }
    if (!item->isOpen()) { 
        error = "Only OPEN reports can be edited."; 
        return false;
     }
    std::string is;
    if (n.empty() || cat.empty() || des.empty() || loc.empty() || r.empty()) {
        error = "No field may be left empty."; return false;
    }
    if (!Utils::parseDate(da, is)) {
         error = "Invalid date. Use YYYY-MM-DD or DD-MM-YYYY.";
            return false;
         }
    if (Utils::isFutureDate(is)) { 
           error = "The date cannot be in the future."; 
        return false;
     }

    Item updated = *item;
    updated.updateDetails(n, cat, des, loc, is, r);
    std::vector<Item> batch(1, updated);
    if (!repository.append(batch, error)){
        return false;
    } 

    const std::size_t pos = indexById[id];
    items[pos] = updated;
    indexitem(pos);

    return true;
}

struct statistics ItemManager::statistics() const {
    struct statistics st;
    std::map<std::string, std::size_t> categories;
    for (std::size_t i = 0; i < items.size(); ++i) {
        const Item& it = items[i];
        ++st.total;
        categories[Utils::toLower(it.getCategory())]++;
        if (it.isLost()) {
            ++st.lost;
            if (it.isOpen()) ++st.openLost;
            else if (it.getStatus() == Itemstatus::Closed) ++st.closedLost;
        } else {
            ++st.found;
            if (it.isOpen()) ++st.openFound;
            else if (it.getStatus() == Itemstatus::Claimed) ++st.claimedFound;
            else ++st.closedOther;
        }
    }
    if (st.found > 0) st.recoveryRatePercent = 100.0 * static_cast<double>(st.claimedFound) / static_cast<double>(st.found);

    std::vector<std::pair<std::string, std::size_t> > ranked(categories.begin(), categories.end());
    struct ByCount {
        bool operator()(const std::pair<std::string, std::size_t>& a,
                        const std::pair<std::string, std::size_t>& b) const {
            if (a.second != b.second) return a.second > b.second;
            return a.first < b.first;
        }
    };
    std::sort(ranked.begin(), ranked.end(), ByCount());
    if (ranked.size() > 5) ranked.resize(5);
    st.topCategories = ranked;
    return st;
}

bool ItemManager::exportCsv(const std::string& path, std::string& error) const {
    std::ofstream out(path.c_str(), std::ios::trunc);
    
    if (!out.is_open()) { 
        error = "Cannot create '" + path + "'."; 
        return false;
     }
    out << "ID,Type,Status,Name,Category,Description,Location,Date,Reporter,Entered,LinkedID,Note\n";
    
    for (std::size_t i = 0; i < items.size(); ++i) {
        const Item& it = items[i];
        out << it.getId() << ',' << Item::typeToString(it.getType()) << ',' << Item::statusToString(it.getStatus())
            << ',' << csvQuote(it.getName()) << ',' << csvQuote(it.getCategory())
            << ',' << csvQuote(it.getDescription()) << ',' << csvQuote(it.getLocation())
            << ',' << it.getDate() << ',' << csvQuote(it.getReporter()) << ',' << it.getCreatedAt()
            << ',' << it.getLinkedId() << ',' << csvQuote(it.getResolutionNote()) << '\n';
    }
    out.flush();
    if (!out.good()) { 
        error = "Writing the export file failed."; 
        return false; 
    }
    return true;
}

bool ItemManager::compactNow(std::string& error) {
       return repository.compact(items, error); 
    }

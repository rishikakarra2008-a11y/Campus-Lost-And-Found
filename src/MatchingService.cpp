#include "MatchingService.h"
#include "Utils.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <set>
#include <utility>

const double MatchingService::DEFAULT_MIN_SCORE = 40.0;

namespace {

const int DATE_WINDOW_BEFORE = 1;   
const int DATE_WINDOW_AFTER = 90;   
const std::size_t MAX_CANDIDATES_SINGLE = 1000;
const std::size_t MAX_CANDIDATES_ALL = 120;
const std::size_t MAX_PER_LOST_IN_ALL = 5;

const std::set<std::string>& stopWords() {
    static std::set<std::string> w;
    if (w.empty()) {
        const char* list[] = {"a", "an", "the", "of", "and", "with", "in", "on", "at", "my", "is",
                              "it", "for", "near", "to", "from", "has", "have", "was", "by", "this",
                              "that", "very", "some", "one", "i", "me"};
        for (std::size_t i = 0; i < sizeof(list) / sizeof(list[0]); ++i) w.insert(list[i]);
    }
    return w;
}


const std::unordered_map<std::string, std::string>& synonyms() {
    static std::unordered_map<std::string, std::string> m;
    if (m.empty()) {
        const char* groups[][8] = {
            {"phone", "mobile", "cellphone", "smartphone", "iphone", "handset", "", ""},
            {"bottle", "flask", "tumbler", "sipper", "thermos", "", "", ""},
            {"bag", "backpack", "rucksack", "satchel", "handbag", "", "", ""},
            {"wallet", "purse", "billfold", "cardholder", "", "", "", ""},
            {"id", "identity", "", "", "", "", "", ""},
            {"earphone", "earbud", "headphone", "headset", "airpod", "", "", ""},
            {"glasses", "spectacles", "spectacle", "eyeglasses", "specs", "sunglasses", "", ""},
            {"key", "keychain", "keyring", "fob", "", "", "", ""},
            {"laptop", "macbook", "chromebook", "", "", "", "", ""},
            {"charger", "adapter", "adaptor", "", "", "", "", ""},
            {"watch", "wristwatch", "smartwatch", "", "", "", "", ""},
            {"umbrella", "brolly", "", "", "", "", "", ""},
            {"calculator", "calc", "", "", "", "", "", ""},
            {"book", "textbook", "notebook", "", "", "", "", ""},
            {"usb", "pendrive", "flashdrive", "thumbdrive", "", "", "", ""},
            {"grey", "gray", "", "", "", "", "", ""},
            {"gold", "golden", "", "", "", "", "", ""},
            {"blue", "navy", "", "", "", "", "", ""}};
        for (std::size_t g = 0; g < sizeof(groups) / sizeof(groups[0]); ++g)
            for (int i = 0; i < 8; ++i)
                if (groups[g][i][0] != '\0') m[groups[g][i]] = groups[g][0];
    }
    return m;
}

const std::set<std::string>& colourWords() {
    static std::set<std::string> w;
    if (w.empty()) {
        const char* list[] = {"black", "white", "red", "blue", "green", "yellow", "orange", "purple",
                              "pink", "grey", "brown", "silver", "gold", "maroon", "beige"};
        for (std::size_t i = 0; i < sizeof(list) / sizeof(list[0]); ++i) w.insert(list[i]);
    }
    return w;
}

std::string canonical(const std::string& word) {
    const std::unordered_map<std::string, std::string>& map = synonyms();
    
    std::unordered_map<std::string, std::string>::const_iterator it = map.find(word);
    if (it != map.end()) {
        return it->second;
    }
    std::string stem = word;
    if (stem.size() > 3 && stem[stem.size() - 1] == 's' && stem[stem.size() - 2] != 's'){
        stem.erase(stem.size() - 1);
    }
        
    it = map.find(stem);
    return it != map.end() ? it->second : stem;
}

void sortUnique(std::vector<std::string>& v) {
    std::sort(v.begin(), v.end());
    v.erase(std::unique(v.begin(), v.end()), v.end());
}

std::string percent(double ratio) {
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%d%%", static_cast<int>(ratio * 100.0 + 0.5));
    return buffer;
}

bool lessResult(const MatchResult& a, const MatchResult& b) {
    if (a.score != b.score) return a.score > b.score;
    if (a.lostId != b.lostId) return a.lostId < b.lostId;
    return a.foundId < b.foundId;
}

} // namespace

std::vector<std::string> MatchingService::canonicalTokens(const std::string& text) {
    const std::vector<std::string> raw = Utils::tokenize(text);
    std::vector<std::string> out;
    for (std::size_t i = 0; i < raw.size(); ++i) {
        if (stopWords().count(raw[i])) continue;
        out.push_back(canonical(raw[i]));
    }
    
      sortUnique(out);
    return out;
}

MatchingService::Profile MatchingService::makeProfile(const Item& item) {
    Profile p;
    
      p.id = item.getId();
     p.type = item.getType();
      p.open = item.isOpen();
     p.day = Utils::dayNumber(item.getDate());
      p.nameTokens = canonicalTokens(item.getName());
     p.categoryTokens = canonicalTokens(item.getCategory());
     p.descriptionTokens = canonicalTokens(item.getDescription());
     p.locationTokens = canonicalTokens(item.getLocation());

    p.indexTokens = p.nameTokens;
    p.indexTokens.insert(p.indexTokens.end(), p.categoryTokens.begin(), p.categoryTokens.end());
    sortUnique(p.indexTokens);

    std::vector<std::string> all = p.nameTokens;
    all.insert(all.end(), p.descriptionTokens.begin(), p.descriptionTokens.end());
    for (std::size_t i = 0; i < all.size(); ++i)
        if (colourWords().count(all[i])) p.colours.push_back(all[i]);
    sortUnique(p.colours);

    for (std::size_t i = 0; i < p.locationTokens.size(); ++i) {
        if (i) p.locationKey += ' ';
        p.locationKey += p.locationTokens[i];
    }
       return p;
}

double MatchingService::bestMatch(const std::string& token, const std::vector<std::string>& pool) {
    if (std::binary_search(pool.begin(), pool.end(), token)){
        return 1.0;
    }
    
    if (token.size() < 4) return 0.0;
    
    for (std::size_t k = 0; k < token.size(); ++k)
        if (token[k] >= '0' && token[k] <= '9'){
            return 0.0;
        }    
    double best = 0.0;
    for (std::size_t i = 0; i < pool.size(); ++i) {
        const std::string& other = pool[i];
        
        if (other.size() < 4) continue;
        const int limit = std::max(token.size(), other.size()) >= 8 ? 2 : 1;
        
        if (limit == 1 && token[0] != other[0] && token[token.size() - 1] != other[other.size() - 1]) {
            continue;
        }
        if (Utils::editDistance(token, other, limit) <= limit) best = std::max(best, 0.8);
    }
    return best;
}


double MatchingService::softSimilarity(const std::vector<std::string>& a, const std::vector<std::string>& b) {
    if (a.empty() || b.empty()) return 0.0;
    double sum = 0.0;
    for (std::size_t i = 0; i < a.size(); ++i) sum += bestMatch(a[i], b);
    for (std::size_t i = 0; i < b.size(); ++i) sum += bestMatch(b[i], a);
    return sum / static_cast<double>(a.size() + b.size());
}

double MatchingService::scorePair(const Profile& lost, const Profile& found, std::vector<std::string>* reasons) {
    
    if (lost.day != Utils::INVALID_DAY && found.day != Utils::INVALID_DAY &&
        found.day - lost.day < -DATE_WINDOW_BEFORE) return 0.0;

    const double nameSim = softSimilarity(lost.nameTokens, found.nameTokens);
    const double categorySim = softSimilarity(lost.categoryTokens, found.categoryTokens);
    const double titleSim = softSimilarity(lost.indexTokens, found.indexTokens);

    
    if (std::max(nameSim, std::max(categorySim, titleSim)) < 0.3) return 0.0;

    double score = 0.0;
    score += 35.0 * nameSim;
    
    score += 20.0 * categorySim;
    const double descSim = softSimilarity(lost.descriptionTokens, found.descriptionTokens);
    score += 20.0 * descSim;

    double locSim = 0.0;
    if (!lost.locationKey.empty() && lost.locationKey == found.locationKey) {
        locSim = 1.0;
    } else {
        locSim = softSimilarity(lost.locationTokens, found.locationTokens) * 0.7;
    }
    score += 10.0 * locSim;

    double dateScore = 0.0;
    int gap = 0;
    const bool datesKnown = lost.day != Utils::INVALID_DAY && found.day != Utils::INVALID_DAY;
    if (datesKnown) {
        gap = found.day - lost.day;   
        if (gap < -DATE_WINDOW_BEFORE) {
            dateScore = 0.0;
        } else if (gap < 0) {
            dateScore = 8.0;
        } else if (gap <= 3) {
            dateScore = 15.0;
        } else if (gap <= 7) {
            dateScore = 12.0;
        } else if (gap <= 14) {
            dateScore = 8.0;
        } else if (gap <= 30) {
            dateScore = 4.0;
        }
    }
    score += dateScore;

    bool colourClash = false;
    if (!lost.colours.empty() && !found.colours.empty()) {
        colourClash = true;
        for (std::size_t i = 0; i < lost.colours.size(); ++i)
            if (std::binary_search(found.colours.begin(), found.colours.end(), lost.colours[i])) colourClash = false;
        if (colourClash) score -= 10.0;
    }

    score = std::max(0.0, std::min(100.0, score));
    score = std::floor(score * 10.0 + 0.5) / 10.0;

    if (reasons) {
        reasons->clear();
        if (nameSim > 0.0) reasons->push_back("Name similarity " + percent(nameSim));
        if (categorySim >= 0.99) reasons->push_back("Same category");
        else if (categorySim > 0.0) reasons->push_back("Related category (" + percent(categorySim) + ")");
        if (descSim > 0.0) reasons->push_back("Description overlap " + percent(descSim));
        if (locSim >= 0.99) reasons->push_back("Same location");
        else if (locSim > 0.0) reasons->push_back("Similar location");
        if (datesKnown && gap >= -DATE_WINDOW_BEFORE && gap <= 30) {
            char text[64];
            std::snprintf(text, sizeof(text), "Found %d day(s) after it was lost", gap < 0 ? 0 : gap);
            reasons->push_back(text);
        } else if (datesKnown && gap < -DATE_WINDOW_BEFORE) {
            reasons->push_back("Found BEFORE the lost date (no date points)");
        }
        if (colourClash) reasons->push_back("Colours differ (-10)");
    }
    return score;
}

void MatchingService::addPostings(const Profile& p) {
    if (!p.open) return;
    std::unordered_map<std::string, std::vector<int> >& postings =
        (p.type == Itemtype::Lost) ? lostPostings : foundPostings;
    for (std::size_t i = 0; i < p.indexTokens.size(); ++i) {
        postings[p.indexTokens[i]].push_back(p.slot);
    }
}

void MatchingService::removePostings(const Profile& p) {
    if (!p.open) return;
    std::unordered_map<std::string, std::vector<int> >& postings =
        (p.type == Itemtype::Lost) ? lostPostings : foundPostings;
    for (std::size_t i = 0; i < p.indexTokens.size(); ++i) {
        postings[p.indexTokens[i]].push_back(p.slot);
    }
}

void MatchingService::removePostings(const Profile& p) {
    if (!p.open) {
        return;
    }
    std::unordered_map<std::string, std::vector<int> >& postings =
        (p.type == Itemtype::Lost) ? lostPostings : foundPostings;

      for (std::size_t i = 0; i < p.indexTokens.size(); ++i) {
           postings[p.indexTokens[i]].push_back(p.slot);
    }
}

void MatchingService::clear() {
      profiles.clear();
     slotById.clear();
      lostPostings.clear();
    foundPostings.clear();
      scratchCount.clear();
    scratchGap.clear();
}

void MatchingService::add(const Item& item) { update(item); }

void MatchingService::update(const Item& item) {
    Profile fresh = makeProfile(item);
    std::unordered_map<int, int>::iterator known = slotById.find(item.getId());
    if (known != slotById.end()) {
         fresh.slot = known->second;
           removePostings(profiles[fresh.slot]);
          profiles[fresh.slot] = fresh;
    } else {
        fresh.slot = static_cast<int>(profiles.size());
         slotById[item.getId()] = fresh.slot;
        profiles.push_back(fresh);
    }
    addPostings(fresh);
}

namespace {
struct Candidate {
    int slot, shared, gap;
};
bool moreRelevant(const Candidate& a, const Candidate& b) {
    if (a.shared != b.shared){
        return a.shared > b.shared;
    } 
    if (a.gap != b.gap) {
        return a.gap < b.gap;
    }
      
      return a.slot > b.slot;                       
}
} 

std::vector<int> MatchingService::candidatesFor(const Profile& p, std::size_t maxCandidates) const {
    const std::unordered_map<std::string, std::vector<int> >& postings =
        (p.type == Itemtype::Lost) ? foundPostings : lostPostings;

    if (scratchCount.size() < profiles.size()) {
        scratchCount.resize(profiles.size(), 0);
        scratchGap.resize(profiles.size(), 0);
    }
    
    std::vector<int> touched;
    
    for (std::size_t t = 0; t < p.indexTokens.size(); ++t) {
        std::unordered_map<std::string, std::vector<int> >::const_iterator it = postings.find(p.indexTokens[t]);
        
        if (it == postings.end()){
            continue;
        } 
        
        const std::vector<int>& slots = it->second;
        
        for (std::size_t k = 0; k < slots.size(); ++k) {
            const int s = slots[k];
            const Profile& other = profiles[static_cast<std::size_t>(s)];
            if (other.id == p.id){
                continue;
            }
            
            if (scratchCount[s] == 0) {
                int gap = 0;
                if (p.day != Utils::INVALID_DAY && other.day != Utils::INVALID_DAY) {
                    const int lostDay = (p.type == Itemtype::Lost) ? p.day : other.day;
                    
                    const int foundDay = (p.type == Itemtype::Lost) ? other.day : p.day;
                    gap = foundDay - lostDay;
                    if (gap < -DATE_WINDOW_BEFORE || gap > DATE_WINDOW_AFTER) {
                         scratchCount[s] = -1; touched.push_back(s); 
                         continue;
                         }
                }
                
                scratchGap[s] = std::abs(gap);
                    touched.push_back(s);
            }
            if (scratchCount[s] >= 0) {
                scratchCount[s] += 1;
            }
        }
    }

    std::vector<Candidate> ranked;
    ranked.reserve(touched.size());
    for (std::size_t i = 0; i < touched.size(); ++i) {
        const int s = touched[i];
        if (scratchCount[s] > 0) { Candidate c; c.slot = s; c.shared = scratchCount[s]; c.gap = scratchGap[s]; ranked.push_back(c); }
        scratchCount[s] = 0;                      
    }
    if (ranked.size() > maxCandidates) {
        std::partial_sort(ranked.begin(), ranked.begin() + static_cast<std::ptrdiff_t>(maxCandidates),
                          ranked.end(), moreRelevant);
        ranked.resize(maxCandidates);
    }
    std::vector<int> result;
    result.reserve(ranked.size());
    for (std::size_t i = 0; i < ranked.size(); ++i) {
          result.push_back(ranked[i].slot);
    }

   
    if (result.empty() && profiles.size() <= 3000) {
        for (std::size_t s =0; s <  profiles.size(); ++s)
            if (profiles[s].open &&  profiles[s].type != p.type && profiles[s].id != p.id){
                  result.push_back(static_cast<int>(s));
            } 
    }
    
      return result;
}

std::vector<MatchResult> MatchingService::matchesForProfile(const Profile& p, double minScore,
                                                           std::size_t limit, std::size_t maxCandidates) const {
    std::vector<MatchResult> res;
    const std::vector<int> candidates =candidatesFor(p, maxCandidates);
    const bool pIsLost = p.type == Itemtype::Lost;
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        const Profile& other =profiles[static_cast<std::size_t>(candidates[i])];
        const Profile& lost = pIsLost ? p : other;
          const Profile& found = pIsLost ? other : p;
        const double score =scorePair(lost, found, NULL);
        if (score < minScore) continue;
        MatchResult r;
        r.lostId =lost.id;
          r.foundId = found.id;
        r.score = score;
        res.push_back(r);
    }
    std::sort(res.begin(), res.end(), lessResult);
    if (res.size() > limit) res.resize(limit);
    
    for (std::size_t i = 0; i < res.size(); ++i) {
        const Profile& lost = profiles[ static_cast<std::size_t>(slotById.find(res[i].lostId)->second )];
          const Profile& found = profiles[  static_cast<std::size_t>(slotById.find(res[i].foundId)->second )];
        scorePair( pIsLost && lost.id == p.id ? p :  lost, !pIsLost   && found.id == p.id ? p : found, &res[i].reasons );
    }
    return res;
}

std::vector<MatchResult> MatchingService::findMatchesFor(const Item& it, double min, std::size_t l) const {
    if (!it.isOpen()) return std::vector<MatchResult>();
    return matchesForProfile(makeProfile(it), min, l, MAX_CANDIDATES_SINGLE);
}

std::vector<MatchResult> MatchingService::findAllMatches(double minScore, std::size_t limit,
                                                         std::size_t maxLostScanned, bool* truncated) const {
    std::vector<MatchResult> all;
    std::size_t scanned = 0;
    if (truncated) *truncated = false;
    for (std::size_t s = profiles.size(); s-- > 0;) {          
        const Profile& p = profiles[s];
        if (p.type != Itemtype::Lost || !p.open) continue;
        if (scanned >= maxLostScanned) { if (truncated) *truncated = true; break; }
        ++scanned;
        std::vector<MatchResult> mine = matchesForProfile(p, minScore, MAX_PER_LOST_IN_ALL, MAX_CANDIDATES_ALL);
        all.insert(all.end(), mine.begin(), mine.end());
    }
    std::sort(all.begin(), all.end(), lessResult);
    if (all.size() > limit) all.resize(limit);
    return all;
}

MatchResult MatchingService::compare(const Item& lost, const Item& found) const {
    MatchResult r;
    r.lostId = lost.getId();
    r.foundId = found.getId();
    r.score = scorePair(makeProfile(lost), makeProfile(found), &r.reasons);
    return r;
}

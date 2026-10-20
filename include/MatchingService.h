#ifndef MATCHING_SERVICE_H
#define MATCHING_SERVICE_H

#include "Item.h"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

struct MatchResult {
    int lostId;
    int foundId;
    double score;                      
    std::vector<std::string> reasons;  
    MatchResult() : lostId(0), foundId(0), score(0.0) {}
};


class MatchingService {
public:
    static const double DEFAULT_MIN_SCORE;

private:
    struct Profile {
        int id;
        int slot;
        Itemtype type;
        bool open;
        int day;
        std::vector<std::string> nameTokens, categoryTokens, descriptionTokens, locationTokens;
        std::vector<std::string> indexTokens; 
        std::vector<std::string> colours;
        std::string locationKey;
        Profile() : id(0), slot(-1), type(Itemtype::Lost), open(true), day(0) {}
    };

    
    std::vector<Profile> profiles;
    std::unordered_map<int, int> slotById;
    std::unordered_map<std::string, std::vector<int> > lostPostings;  
    std::unordered_map<std::string, std::vector<int> > foundPostings;
    mutable std::vector<int> scratchCount;  
    mutable std::vector<int> scratchGap;

    static Profile makeProfile(const Item& item);
    static std::vector<std::string> canonicalTokens(const std::string& text);
    static double softSimilarity(const std::vector<std::string>& a, const std::vector<std::string>& b);
    static double bestMatch(const std::string& token, const std::vector<std::string>& pool);
    static double scorePair(const Profile& lost, const Profile& found, std::vector<std::string>* reasons);

    void addPostings(const Profile& p);
    void removePostings(const Profile& p);
    std::vector<int> candidatesFor(const Profile& p, std::size_t maxCandidates) const; 
    std::vector<MatchResult> matchesForProfile(const Profile& p, double minScore,
                                               std::size_t limit, std::size_t maxCandidates) const;

public:
    void clear();
    void add(const Item& item);      
    void update(const Item& item);   

    
    std::vector<MatchResult> findMatchesFor(const Item& item, double minScore = DEFAULT_MIN_SCORE,
                                            std::size_t limit = 10) const;

    
    std::vector<MatchResult> findAllMatches(double minScore = DEFAULT_MIN_SCORE,
                                            std::size_t limit = 20,
                                            std::size_t maxLostScanned = 1000,
                                            bool* truncated = NULL) const;

   
    MatchResult compare(const Item& lost, const Item& found) const;
};

#endif

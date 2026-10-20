#ifndef ITEM_H
#define ITEM_H

#include <string>

enum class Itemtype { Lost, Found };
enum class Itemstatus { Open, Claimed, Closed };

class Item {
private:
    int ID;
    Itemtype t;
    Itemstatus s;
    std::string n;
     std::string co;
    std::string  des;
    std::string l;
    std::string d;           // ISO date the item was lost / found
     std::string  r;       // name / roll number / phone of the person reporting
    std::string cr;      // when the report was entered
    int  linkedId;               // id of the partner report once resolved (0 = none)
    std::string resolutionNote; // who collected it / why it was closed
public:
    Item();
    Item(int id, Itemtype  type, const std::string&  name, const std::string& category,
         const std::string& description, const  std::string&  location,
         const std::string&  date, const std::string& reporter,
         const std::string& createdAt);

    Item(int id, Itemtype type, Itemstatus status, const std::string& name,
         const std::string& category,  const std::string& description,
         const std::string& location, const std::string& date,
         const std::string&  reporter, const std::string& createdAt,
         int linkedId, const std::string& resolutionNote);

    int getId() const;
    Itemtype getType() const;
    Itemstatus getStatus() const;
    const std::string& getName() const;
    const std::string&  getCategory() const;
    const std::string& getDescription() const;
    const std::string& getLocation() const;
    const std::string&  getDate() const;
    const std::string& getReporter() const;
    const std::string&    getCreatedAt() const;
    int   getLinkedId() const;
    const std::string& getResolutionNote() const;

    bool isOpen() const;
    bool isLost() const;

    void setStatus(Itemstatus newStatus);
    void setLinkedId(int partnerId);
    void   setResolutionNote(const std::string& note);
    void updateDetails(const std::string&   newName,   const std::string& newCategory,
                       const std::string& newDescription, const std::string& newLocation,
                       const std::string& newDate, const std::string&   newReporter);

    void display(bool full   = false) const;

    static std::string typeToString(Itemtype t);
    static std::string statusToString(Itemstatus s);
    static bool typeFromString(const std::string&   text, Itemtype& t);
    static bool   statusFromString(const   std::string& text, Itemstatus& s);
};

#endif

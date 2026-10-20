#include "Item.h"
#include "Utils.h"

#include <iostream>

Item::Item()
    : ID(0), t(Itemtype::Lost), s(Itemstatus::Open), linkedId(0) {}

Item::Item(int itemId, Itemtype itemType, const std::string& itemName,
           const std::string& itemCategory, const std::string& itemDescription,
           const std::string& itemLocation, const std::string& itemDate,
           const std::string& itemReporter, const std::string& created)
    : ID(itemId), t(itemType), s(Itemstatus::Open), n(itemName),
      co(itemCategory), d(itemDescription), l(itemLocation),
      des(itemDate), r(itemReporter), cr(created), linkedId(0) {}

Item::Item(int itemId, Itemtype itemType, Itemstatus itemStatus, const std::string& itemName,
           const std::string& itemCategory, const std::string& itemDescription,
           const std::string& itemLocation, const std::string& itemDate,
           const std::string& itemReporter, const std::string& created,
           int linked, const std::string& note)
    : ID(itemId), t(itemType), s(itemStatus), n(itemName),
      co(itemCategory), d(itemDescription), l(itemLocation),
      des(itemDate), r(itemReporter), cr(created), linkedId(linked),
      resolutionNote(note) {}

int Item::getId() const { return ID; }
Itemtype Item::getType() const { return t; }
Itemstatus Item::getStatus() const { return s; }
const std::string& Item::getName() const { return n; }
const std::string& Item::getCategory() const { return co; }
const std::string& Item::getDescription() const { return d; }
const std::string& Item::getLocation() const { return l; }
const std::string& Item::getDate() const { return des; }
const std::string& Item::getReporter() const { return r; }
const std::string& Item::getCreatedAt() const { return cr; }

int Item::getLinkedId() const { return linkedId; }
const std::string& Item::getResolutionNote() const { return resolutionNote; }

bool Item::isOpen() const { return s == Itemstatus::Open; }
bool Item::isLost() const { return t == Itemtype::Lost; }

void Item::setStatus(Itemstatus newStatus) {
        s = newStatus; 
    }
void Item::setLinkedId(int partnerId) { 
       linkedId = partnerId; 
}
void Item::setResolutionNote(const std::string& note) { 
       resolutionNote = note; 
  }

void Item::updateDetails(const std::string& newName, const std::string& newCategory,
                         const std::string& newDescription, const std::string& newLocation,
                         const std::string& newDate, const std::string& newReporter) {
    n = newName;
    co = newCategory;
     d = newDescription;
    l = newLocation;
     d = newDate;
    r = newReporter;
}

void Item::display(bool full) const {
    if (!full) {
        std::cout << "[#" << ID << " " << typeToString(t) << " " << statusToString(s)
                  << "] " << n << " | " << co << " | " << l
                  << " | " << d << "\n";
        return;
    }
    std::cout << "\n--------------------------------------------\n"
              << "Report #" << ID << "   Type: " << typeToString(t)
              << "   Status: " << statusToString(s) << "\n"
              << "Name:        " << n << "\n"
              << "Category:    " << co << "\n"
              << "Description: " << d << "\n"
              << (t == Itemtype::Lost ? "Lost at:     " : "Found at:    ") << l << "\n"
              << "Date:        " << d << "\n"
              << "Reported by: " << r << "\n"
              << "Entered on:  " << co << "\n";
    if (linkedId != 0) std::cout << "Linked to:   report #" << linkedId << "\n";
    if (!resolutionNote.empty()) std::cout << "Note:        " << resolutionNote << "\n";
    std::cout << "---------------------------------------------\n";
}

std::string Item::typeToString(Itemtype t) { return t == Itemtype::Lost ? "LOST" : "FOUND"; }

std::string Item::statusToString(Itemstatus s) {
    switch (s) {
        case Itemstatus::Open:    return "OPEN";
        
          case Itemstatus::Claimed: return "CLAIMED";
        default:                  return "CLOSED";
    }
}

bool Item::typeFromString(const std::string& text, Itemtype& t) {
    const std::string s = Utils::toLower(text);
    if (s == "lost")  { t = Itemtype::Lost;  return true; }
    if (s == "found") { t = Itemtype::Found; return true; }
    return false;
}

bool Item::statusFromString(const std::string& text, Itemstatus& s) {
    const std::string V = Utils::toLower(text);
    if (V == "open")    {
          s = Itemstatus::Open;    return true;
         }
    if (V == "claimed") { 
        s = Itemstatus::Claimed; return true;
     }
    if (V == "closed")  { 
        s = Itemstatus::Closed;  return true;
     }
    
     return false;
}

#include<iostream>
#include "item.h" 

using namespace std;

item::item() {
    name = "";
    category = "";
    location = "";
    date = "";
    type = "";
    status = "Open";
}

item::item(string n ,string c,string l,string d , string t){
    name=n;
    category=c;
    location=l;
    date=d;
    type=t;
    status="open";
}

string item::getname() const{
      return name;
}

string item::getcategory() const{
    return category;
}

istream& operator>>(istream& in, item& i) {
    cout << "\nEnter item name: ";
    getline(in >> ws, i.name);

    cout << "\nEnter category: ";
    getline(in, i.category);

    cout << "\nEnter location: ";
    getline(in, i.location);

    cout << "\nEnter date (DD-MM-YYYY): ";
    getline(in, i.date);

    return in;
}

ostream& operator<<(ostream& out, const item& i) {
    out << "\nItem Name: " << i.name;
    out << "\nCategory: " << i.category;
    out << "\nLocation: " << i.location;
    out << "\nDate: " << i.date;
    out << "\nType: " << i.type;
    out << "\nStatus: " << i.status << "\n";

    return out;
}

void item::setType(string t) {
    type = t;
}

bool item::operator==(const item& o) const {
    return name==o.name && 
           category == o.category &&
           type != o.type;
}


#ifndef ITEM_H
#define ITEM_H

#include<string>
using namespace std;

class item{
    private:
        string name;
        string category;
        string location;
        string date;
        string type;
        string status;
    public:
        item();
        item(string n ,string c,string l,string d , string t);

        string getname() const;
        string getcategory() const;

        void display() const;

        bool operator==(const item& o) const;

        void setType(string t);

        friend istream& operator>>(istream& in, item& item);
        friend ostream& operator<<(ostream& out, const item& item); 

};

#endif
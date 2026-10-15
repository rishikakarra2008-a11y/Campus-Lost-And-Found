#include<iostream>
#include<string>
#include "item.h"

using namespace std;

const int MAX_ITEMS = 500;

int main(){
    item items[MAX_ITEMS];
    int count=0;

    int choice;

    do{
        cout << "\n----------COLLEGE LOST AND FOUND PORTAL------------\n";
        cout << "1.Repot lost item" << endl;
        cout << "2.Repot found item" << endl;
        cout << "3.Display all items" << endl;
        cout << "4.Search items" << endl;
        cout << "5.Find possible matches" << endl;
        cout << "6.Exit" << endl;
        cout << "Enter your choices: " << endl;

        if(!(cin >> choice)){
            cout << "Invalid input .\nENTER VALID INPUT!" << endl;
            break;
        }

        if(choice==1 || choice==2){
            if(count >= MAX_ITEMS){
                cout << "Items stroage is full " << endl;
                continue;
            }

            string type;

            if(choice == 1){
                type = "lost";
            }else{
                type = "found";
            }

            cin >> items[count];
            items[count].setType(type);
            count++;

            cout << type << "ITEM REPORTED SUCCESSFULLY" << endl;


        }else if(choice == 3){
              if(count == 0){
                cout << "No items reported yet" << endl;
              }

              for(int i=0;i<count; i++){
                cout << "\n ----Item" << i+1 << "----\n";

                cout << items[i];
              }
        }else if(choice == 4){
            string search_name;
            bool found=false;

            cin.ignore();

            cout << "Enter ITEM NAME to search:" << endl;
            getline(cin,search_name);

            for(int i=0;i<count;i++){
                if(items[i].getname() == search_name){
                    cout << items[i];
                    found=true;
                }
            }

            if(!found){
                cout << "No matching ITEM found.\n";
            }
        }else if(choice == 5){
            bool found = false;

            for(int i=0;i<count;i++){
                for(int j=i+1;j<count;j++){
                    if(items[i] == items[j]){
                        cout << "\nPOSSIBLE MATCH FOUND:" << endl;
                        cout << items[i];
                        cout << items[j];
                        found = true;
                    }
                }
            }

            if(!found){
                cout << "NO POSSIBLE MATCHES FOUND.\n";
            }
        }else if(choice == 6){
            cout << "\nTHANK YOU FOR USING THE PORTAL!\n";
        }else{
            cout << "INVALID CHOICE. TRY AGAIN.\n";
        }
    }while(choice != 6);

    return 0;
}
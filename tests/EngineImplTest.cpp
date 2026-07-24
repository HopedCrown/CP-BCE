// Test file. Uses TUI to draw not to the calculator screen, but a regular Computer Screen.
#include <iostream>
#include <vector>
#include <string>

using namespace std;

void print(string text){
  cout << text << endl;
}

int main(){
  cout << "ChemEngine Implementation Test." << endl;
  print(  "-------------------------------");
  cout << "Testing out Cellular Respiration ..." << endl;
  cout << "Formula Being Tested:" << endl;
  print("C6H12O6 + 6O2 --> 6CO2 + 6H2O");
  print("Formula Inputted as argument:");
  print("C6H12O6 + O2 --> CO2 + H20");
  print("Output:");
  vector<string> reagentss = {"",""}; // Stop here for now.
  return 0;
}

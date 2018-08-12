#include <iostream>
#include <string>

using namespace std;

int main()
{
  while ( true )
    {
      // Show prompt.
      cout << "$ " ;
      char command[128];
      cin.getline( command, 128 );
      
      if(strcmp(command, "exit") == 0){
      	      return 0;
      }
   return 0;
}
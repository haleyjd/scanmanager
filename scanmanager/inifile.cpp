/*

   HL7Daemon - Initialization File

   Derived from Prometheus inifile module - "PANDORA"

*/

#include <map>
#include <string>
#include <fstream>
using namespace std;

#include "inifile.h"
#include "util.h"

//load/save a file in the following format
/*
[bob]
rupert=naughty
zoop=nowhere
larry=good

[nugget]
gold=true
laughable=yes
*/
//to/from a map of string to map of string to string.
//this is, as far as i know, the standard format of an
//ini file.

//
// IniFile::loadOptionsFromFile
//
// Load up the options from a disk file
//
bool IniFile::loadOptionsFromFile(const string &filename)
{
   ifstream thefile;
   string  oneline;
   string  section = "undefined";
   string  key;
   string  value;
   string  valid_characters;

   valid_characters = "abcdefghijklmnopqrstuvwxyz"
                      "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890"
                      " `~!@#$%^&*()-_+=[]{}<>|\\/?'\";:,.";

   thefile.open(filename.c_str());

   if(thefile.is_open())
   {
      getline(thefile, oneline, '\n');

      if(oneline.size() > 0 && oneline.find_first_not_of(valid_characters) != string::npos)
         oneline = StripLast(oneline);

      while(!thefile.eof())
      {
         if(IsBracketed(oneline))
            section = StripFirst(StripLast(oneline));
         else if(oneline.length())
         {
            if(SplitKeyValue(oneline, key, value))
               pandora[section][key] = value;
         }
         getline(thefile, oneline, '\n');

         if(oneline.size() > 0 && oneline.find_first_not_of(valid_characters) != string::npos)
            oneline = StripLast (oneline);
      }

      thefile.close();
   }
   else
     return false;

   return true;
}

//
// IniFile::saveOptionsToFile
//
// Save options previously read in back to file.
//
bool IniFile::saveOptionsToFile(const string &filename)
{
   fstream thefile;

   thefile.open(filename.c_str(), ios::out | ios::trunc);

   if(thefile.is_open())
   {
      for(IniMap::iterator outer = pandora.begin(); outer != pandora.end(); outer++)
      {
         thefile << "[" << outer->first << "]\n";
         for(IniValue::iterator inner = outer->second.begin(); inner != outer->second.end(); inner++)
            thefile << inner->first << "=" << inner->second << "\n";
         thefile << "\n";
      }
      thefile.close();
   }
   else
      return false;

   return true;
}

//
// IniFile::GetIniFile
//
// Constructs and/or returns a singleton instance of the initialization file.
//
IniFile &IniFile::GetIniFile()
{
   static IniFile *theFile = NULL;

   if(!theFile)
      theFile = new IniFile;

   return *theFile;
}

// EOF




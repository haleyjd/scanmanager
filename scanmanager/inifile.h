/** @file inifile.h

   HL7Daemon - Initialization File.

   Derived from Prometheus inifile module - "PANDORA"

   @author Paul Tessman
   @author James Haley
*/

#ifndef pandorafunctions__
#define pandorafunctions__

#include <string>
#include <map>

/**
 * Provides a multi-level map of configuration options by
 * section and field names. This class has a global singleton
 * instance that will be constructed the first time that
 * GetIniFile is called.
 */
class IniFile
{
public:
   /** A single token from the ini file is just a string */
   typedef std::string IniToken;

   /** A set of key-value pairs in the ini file, ie one section */
   typedef std::map<IniToken, IniToken> IniValue;

   /** Map of all config sections */
   typedef std::map<IniToken, IniValue> IniMap;

protected:

   /**
    Configuration data, get with getIniOptions.
   */
   IniMap pandora;

   /** Protected constructor for singleton object */
   IniFile() : pandora()
   {
   }

public:
   /**
    * Call to load options from file.
    * @param filename Absolute path to ini file.
    * @return True if successful, false otherwise.
    */
   bool loadOptionsFromFile(const std::string &filename);
   
   /**
    * Call to save options back to a file.
    * @param filename Target absolute file path for output.
    * @return True if successful, false otherwise.
    */
   bool saveOptionsToFile(const std::string &filename);

   /**
    * Call to retrieve the map of ini sections.
    */
   IniMap &getIniOptions() { return pandora; }

   /**
    * Call to retrieve the global singleton instance. If it has
    * not been constructed yet, it will be constructed by making
    * this call.
    */
   static IniFile &GetIniFile();

   /**
    * Call to retrieve the map of ini sections from the global
    * singleton instance.
    */
   static IniMap  &GetIniOptions() { return GetIniFile().getIniOptions(); }
};

#endif

// EOF


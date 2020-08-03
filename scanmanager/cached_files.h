/** @file cached_files.h

   Prometheus Stub Launcher - Cached Files Module.

   This is the same as in Prometheus, but is cut down to the parts that are
   relevant to management of local files and the cache directory tree only.

   @author James Haley
*/

#ifndef CACHED_FILES_H__
#define CACHED_FILES_H__

#include <string>
#include <utility>

using namespace std;

/**
 * Contains functions for reading and writing files to the user's AppData
 * directory in a portable manner, secure and consistent manipulation of
 * file path strings, testing files and directories for existence, and
 * creating directories.
 */
namespace FileCache
{
   // Utilities

   /**
    * Set the application base path
    */
   void SetBasePath(const string &path);

   /**
    * @return Retrieve the path set as the application's base data path.
    */
   string GetBasePath(void);

   /**
    * Use this function to strip trailing slashes from file paths.
    * @param source_path A file path string.
    * @return The same string minus any trailing slash characters.
    */
   string RemoveTrailingSlash(const string &source_path);

   /**
    * Use this function to strip leading slashes from file paths.
    * @param source_path A file path string.
    * @return The same string minus any leading slash characters.
    */
   string RemoveLeadingSlash(const string &source_path);
   
   /**
    * Concatenate two file path components properly whether or not either
    * component begins or ends with slash characters.
    * @param base The initial component of the path.
    * @param newpart New portion of path to append to base.
    * @return Given base and path, returns base\\path without duplicated slashes.
    */
   string PathConcatenate(const string &base, const string &newpart);
   
   /**
    * Test whether or not a directory exists. Can also test whether or not the
    * current user has full read/write/execute permissions on that path.
    * @param path Absolute path to the directory.
    * @param test_if_full_perms If true, the function will return false if the 
    *        directory exists but the user does not have full permissions to use it.
    * @return Returns true or false to indicate existence and access.
    */
   bool   DirectoryExists(const string &path, bool test_if_full_perms);
   
   /**
    * Tests if a file exists with the given name at the given directory path.
    * @param path Directory path to the file.
    * @param file Name of the file.
    * @return True if the file exists and false otherwise.
    */
   bool   FileExists(const string &path, const string &file);
   
   /**
    * Tests if a file exists in the user's AppData file cache directory.
    * @param path Path to the file, relative to the user's AppData dir.
    * @param file Name of the file.
    * @return True if the file exists and false otherwise.
    * @see GetBasePath
    */
   bool   FileExistsInCache(const string &path, const string &file);
   
   /**
    * Create a single new directory.
    * @param dir_name Name of a single new directory to create. All other
    *        components in the absolute path must already exist.
    * @return True if the directory was created, false otherwise.
    * @see CreateDirectoryRecursive
    */
   bool   CreateDirectorySingle(const string &dir_name);
   
   /**
    * Create every component in the given path which does not already
    * exist.
    * @param path A full path. Every component in this path will be
    *        created as a new directory if it does not already exist.
    * @return True if the process was completely successful, false otherwise.
    */
   bool   CreateDirectoryRecursive(const string &path);

   /**
    * Copy a file into the user's AppData file cache directory.
    * @param file Absolute path to the source file to copy.
    * @param relativedir Directory under the AppData folder into which the 
    *        file will be copied.
    * @return True if the file was copied, false otherwise.
    */
   bool   CopyIntoCache(const string &file, const string &relativedir);

   /**
    * Delete a file from the user's AppData file cache directory, if
    * such a file exists.
    * @param path Path relative to the AppData directory.
    * @param file Name of the file to delete.
    * @return True if the file existed and was deleted, false otherwise.
    */
   bool   DropFromCache(const string &path, const string &file);
   
   /**
    * Splits an absolute file path into directory path and file name
    * components.
    * @param full_path Path and file name of target file.
    * @return Directory path, if any, in pair::first; file name in pair::second.
    *         If there is no path, pair::first will be empty and pair::second
    *         will be equal to the value passed in full_path.
    */
   pair<string, string> GetFileSpec(const string &full_path);
}

#endif

// EOF

